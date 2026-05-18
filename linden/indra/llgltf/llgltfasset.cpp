/**
 * @file llgltfasset.cpp
 * @brief LLGLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; version 2.1 of the License only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA"
 * $/LicenseInfo$
 */

#include <fstream>

#include "linden_common.h"

#include "llgltfasset.h"

#include "lldir.h"
#include "llframetimer.h"
#include "llgl.h"
#include "llgltexture.h"
#include "llgltfanimation.h"
#include "llgltfbufferutil.h"
#include "llgltfprimitive.h"
#include "lluri.h"
#include "llvolumeoctree.h"

// 0 to match LL's newest code, but then it breaks glTF objects preview. HB
#define LL_PREPARE_VB 1

using namespace LLGLTF;

namespace LLGLTF
{

Material::AlphaMode gltf_alpha_mode_to_enum(const std::string& alpha_mode)
{
	if (alpha_mode == "MASK")
	{
		return Material::AlphaMode::MASK;
	}
	if (alpha_mode == "BLEND")
	{
		return Material::AlphaMode::BLEND;
	}
	return Material::AlphaMode::OPAQUE;
}

const std::string& enum_to_gltf_alpha_mode(Material::AlphaMode alpha_mode)
{
	if (alpha_mode == Material::AlphaMode::MASK)
	{
		static const std::string mask = "MASK";
		return mask;
	}
	if (alpha_mode == Material::AlphaMode::BLEND)
	{
		static const std::string blend = "BLEND";
		return blend;
	}
	static const std::string opaque = "OPAQUE";
	return opaque;
}

}	// namespace LLGLTF

// Helper function used to upload a matrix pallete to an UBO. HB
static void upload_matrice_palette_to_ubo(const std::vector<mat4>& mat_vec,
										  U32& ubo)
{
	if (!ubo)
	{
		glGenBuffers(1, &ubo);
	}

	U32 count = mat_vec.size();

	// Use a static storage for speed. HB
	static std::vector<F32> glmp;
	glmp.resize(count * 12);
	F32* mp = glmp.data();

	// Same optimization (Kathrine Jansma's) as used to copy rigged
	// matrix palletes in LLVOAvatar::initRiggedMatrixCache().
#ifdef __AVX2__
	// Offsets to copy
	__m256i vindex_low = _mm256_setr_epi32(0, 1, 2, 12, 4, 5, 6, 13);
	__m256i vindex_high = _mm256_setr_epi32(8, 9, 10, 14, 8, 8, 8, 8);
	// We only need 128 bit, so mask out the rest
	__m256i high_mask = _mm256_set_epi32(0, 0, 0, 0, -1, -1, -1, -1);
	for (U32 i = 0; i < count; ++i)
	{
		const F32* m = glm::value_ptr(mat_vec[i]);
		U32 idx = i * 12;
		_mm256_storeu_ps(mp + idx,
						 _mm256_i32gather_ps(m, vindex_low, 4));
		_mm256_maskstore_ps(mp + idx + 8, high_mask,
							_mm256_i32gather_ps(m, vindex_high, 4));
	}
#else
	for (U32 i = 0; i < count; ++i)
	{
		const F32* m = glm::value_ptr(mat_vec[i]);

		U32 idx = i * 12;

		mp[idx] = m[0];
		mp[idx + 1] = m[1];
		mp[idx + 2] = m[2];
		mp[idx + 3] = m[12];

		mp[idx + 4] = m[4];
		mp[idx + 5] = m[5];
		mp[idx + 6] = m[6];
		mp[idx + 7] = m[13];

		mp[idx + 8] = m[8];
		mp[idx + 9] = m[9];
		mp[idx + 10] = m[10];
		mp[idx + 11] = m[14];
	}
#endif

	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, glmp.size() * sizeof(F32), glmp.data(),
				 GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Scene class
///////////////////////////////////////////////////////////////////////////////

void Scene::updateTransforms(Asset& asset)
{
	mat4 identity = glm::identity<mat4>();
	for (auto& idx : mNodes)
	{
		Node& node = asset.mNodes[idx];
		node.updateTransforms(asset, identity);
	}
}

const Scene& Scene::operator=(const lljson& src)
{
	copy(src, "nodes", mNodes);
	copy(src, "name", mName);
	return *this;
}

void Scene::serialize(lljson& dst) const
{
	write(mNodes, "nodes", dst);
	write(mName, "name", dst);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Node class
///////////////////////////////////////////////////////////////////////////////

Node::Node()
:	mMatrix(glm::identity<mat4>()),
	mTranslation(vec3(0.f, 0.f ,0.f)),
	mRotation(glm::identity<quat>()),
	mScale(vec3(1.f, 1.f ,1.f)),
	mParent(INVALID_INDEX),
	mMesh(INVALID_INDEX),
	mSkin(INVALID_INDEX),
	mMatrixValid(false),
	mTRSValid(false),
	mNeedsApplyMatrix(false)
{
}

void Node::updateTransforms(Asset& asset, const mat4& parent_mat)
{
	makeMatrixValid();
	mAssetMatrix = parent_mat * mMatrix;

	mAssetMatrixInv = glm::inverse(mAssetMatrix);

	S32 my_index = S32(this - &asset.mNodes[0]);

	for (auto& idx : mChildren)
	{
		Node& child = asset.mNodes[idx];
		child.mParent = my_index;
		child.updateTransforms(asset, mAssetMatrix);
	}
}

void Node::makeMatrixValid()
{
	if (!mMatrixValid && mTRSValid)
	{
		mMatrix = glm::recompose(mScale, mRotation, mTranslation,
								 vec3(0.f, 0.f, 0.f),
								 vec4(0.f, 0.f, 0.f, 1.f));
		mMatrixValid = true;
	}
}

void Node::makeTRSValid()
{
	if (!mTRSValid && mMatrixValid)
	{
		vec3 skew;
		vec4 perspective;
		glm::decompose(mMatrix, mScale, mRotation, mTranslation, skew,
					   perspective);
		mTRSValid = true;
	}
}

void Node::setRotation(const quat& q)
{
	makeTRSValid();
	mRotation = q;
	mMatrixValid = false;
}

void Node::setTranslation(const vec3& t)
{
	makeTRSValid();
	mTranslation = t;
	mMatrixValid = false;
}

void Node::setScale(const vec3& s)
{
	makeTRSValid();
	mScale = s;
	mMatrixValid = false;
}

const Node& Node::operator=(const lljson& src)
{
	copy(src, "name", mName);
	mMatrixValid = copy(src, "matrix", mMatrix);
	copy(src, "rotation", mRotation);
	copy(src, "translation", mTranslation);
	copy(src, "scale", mScale);
	copy(src, "children", mChildren);
	copy(src, "mesh", mMesh);
	copy(src, "skin", mSkin);

	if (!mMatrixValid)
	{
		mTRSValid = true;
	}

	return *this;
}

void Node::serialize(lljson& dst) const
{
	write(mName, "name", dst);
	write(mMatrix, "matrix", dst, glm::identity<mat4>());
	write(mRotation, "rotation", dst, glm::identity<quat>());
	write(mTranslation, "translation", dst, vec3(0.f, 0.f, 0.f));
	write(mScale, "scale", dst, vec3(1.f, 1.f, 1.f));
	write(mChildren, "children", dst);
	write(mMesh, "mesh", dst, INVALID_INDEX);
	write(mSkin, "skin", dst, INVALID_INDEX);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Asset class
///////////////////////////////////////////////////////////////////////////////

//static
Asset::gltex_from_fetched_fn_t Asset::sTexFromFetchedFn = NULL;
Asset::gltex_from_file_fn_t Asset::sTexFromFileFn = NULL;
Asset::gltex_from_memory_fn_t Asset::sTexFromMemoryFn = NULL;
Asset::localtex_filename_fn_t Asset::sGetLocaTexFilenameFn = NULL;

//static
void Asset::setHooks(gltex_from_fetched_fn_t fn1,
					 gltex_from_file_fn_t fn2,
					 gltex_from_memory_fn_t fn3,
					 localtex_filename_fn_t fn4)
{
	sTexFromFetchedFn = fn1;
	sTexFromFileFn = fn2;
	sTexFromMemoryFn = fn3;
	sGetLocaTexFilenameFn = fn4;
}

Asset::Asset()
:	mScene(INVALID_INDEX),
	mPendingBuffers(0),
	mLastUpdateTime((F32)LLFrameTimer::getElapsedSeconds()),
	mNodesUBO(0),
	mMaterialsUBO(0),
	mLoadIntoVRAM(false)
{
}

Asset::~Asset()
{
	if (mNodesUBO)
	{
		LLGLManager::deleteBuffers(1, &mNodesUBO);
	}
	if (mMaterialsUBO)
	{
		LLGLManager::deleteBuffers(1, &mNodesUBO);
	}
}

Asset::Asset(const lljson& src)
{
	*this = src;
}

void Asset::updateTransforms()
{
	for (auto& scene : mScenes)
	{
		scene.updateTransforms(*this);
	}
	uploadTransforms();
}

void Asset::uploadTransforms()
{
	// This is the maximum number of 3x4 matrices than can fit in a UBO
	const U32 max_nodes = LLGLManager::sMaxUniformBlockSize / 48;

	// Use a static storage for speed. HB
	static bool initialized = false;
	alignas(16) static std::vector<mat4> t_mp;
	if (!initialized)
	{
		initialized = true;
		t_mp.reserve(max_nodes);
	}

	// Prepare matrix palette
	U32 node_count = llmin(max_nodes, mNodes.size());
	t_mp.resize(node_count);
	for (U32 i = 0; i < node_count; ++i)
	{
		Node& node = mNodes[i];
		// Build matrix palette in asset space
		t_mp[i] = node.mAssetMatrix;
	}

	upload_matrice_palette_to_ubo(t_mp, mNodesUBO);
}

void Asset::uploadMaterials()
{
	constexpr U32 material_size = sizeof(vec4) * 12;
	// This is the maximum number of materials than can fit in a UBO
	const U32 max_materials = LLGLManager::sMaxUniformBlockSize /
							  material_size;

	// Use a static storage for speed. HB
	static bool initialized = false;
	alignas(16) static std::vector<vec4> md;
	if (!initialized)
	{
		initialized = true;
		md.reserve(max_materials * 12);
	}

	U32 mat_count = llmin(mMaterials.size(), max_materials);
	md.resize(mat_count * 12);
	for (U32 i = 0, count =  mat_count * 12; i < count; i += 12)
	{
		Material& mat = mMaterials[i / 12];
		// Add texture transforms and UV indices
		mat.mPbrMetallicRoughness.mBaseColorTexture.mTextureTransform.getPacked(&md[i]);
		md[i + 1].g = (F32)mat.mPbrMetallicRoughness.mBaseColorTexture.getTexCoord();
		mat.mNormalTexture.mTextureTransform.getPacked(&md[i + 2]);
		md[i + 3].g = (F32)mat.mNormalTexture.getTexCoord();
		mat.mPbrMetallicRoughness.mMetallicRoughnessTexture.mTextureTransform.getPacked(&md[i + 4]);
		md[i + 5].g = (F32)mat.mPbrMetallicRoughness.mMetallicRoughnessTexture.getTexCoord();
		mat.mEmissiveTexture.mTextureTransform.getPacked(&md[i + 6]);
		md[i + 7].g = (F32)mat.mEmissiveTexture.getTexCoord();
		mat.mOcclusionTexture.mTextureTransform.getPacked(&md[i + 8]);
		md[i + 9].g = (F32)mat.mOcclusionTexture.getTexCoord();
		// Add material properties
		md[i + 10] = vec4(mat.mEmissiveFactor, 1.f);
		F32 min_alpha = -1.f;
		if (mat.mAlphaMode == Material::AlphaMode::MASK)
		{
			min_alpha = mat.mAlphaCutoff;
		}
		md[i + 11] = vec4(0.f, mat.mPbrMetallicRoughness.mRoughnessFactor,
						  mat.mPbrMetallicRoughness.mMetallicFactor,
						  min_alpha);
	}

	if (!mMaterialsUBO)
	{
		glGenBuffers(1, &mMaterialsUBO);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, mMaterialsUBO);
	glBufferData(GL_UNIFORM_BUFFER, md.size() * sizeof(vec4), md.data(),
				 GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

S32 Asset::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
								LLVector4a* intersectp, LLVector2* tcoordp,
								LLVector4a* normalp, LLVector4a* tangentp,
								S32* prim_hitp)
{
	S32 node_hit = -1;
	S32 primitive_hit = -1;

	LLVector4a p, local_start, local_end;
	LLVector4a asset_end = end;
	for (auto& node : mNodes)
	{
		if (node.mMesh != INVALID_INDEX)
		{
			bool new_hit = false;

			LLMatrix4a ami;
			ami.loadu(glm::value_ptr(node.mAssetMatrixInv));
			// Transform start and end to this node's local space
			ami.affineTransform(start, local_start);
			ami.affineTransform(asset_end, local_end);

			Mesh& mesh = mMeshes[node.mMesh];
			for (auto& primitive : mesh.mPrimitives)
			{
				const LLVolumeTriangle* tri =
					primitive.lineSegmentIntersect(local_start, local_end, &p,
												   tcoordp, normalp, tangentp);
				if (tri)
				{
					new_hit = true;
					local_end = p;

					// Pointer math to get the node index
					node_hit = (S32)(&node - &mNodes[0]);
					llassert(&mNodes[node_hit] == &node);

					// Pointer math to get the primitive index
					primitive_hit = (S32)(&primitive - &mesh.mPrimitives[0]);
					llassert(&mesh.mPrimitives[primitive_hit] == &primitive);
				}
			}

			if (new_hit)
			{
				LLMatrix4a am;
				am.loadu(glm::value_ptr(node.mAssetMatrix));
				// Shorten line segment on hit
				am.affineTransform(p, asset_end);

				// Transform results back to asset space
				if (intersectp)
				{
					*intersectp = asset_end;
				}

				if (normalp || tangentp)
				{
					mat4 normalMatrix = glm::transpose(node.mAssetMatrixInv);

					LLMatrix4a norm_mat;
					norm_mat.loadu(glm::value_ptr(normalMatrix));

					if (normalp)
					{
						LLVector4a n = *normalp;
						F32 w = n.getF32ptr()[3];
						n.getF32ptr()[3] = 0.f;

						norm_mat.affineTransform(n, *normalp);
						normalp->getF32ptr()[3] = w;
					}

					if (tangentp)
					{
						LLVector4a t = *tangentp;
						F32 w = t.getF32ptr()[3];
						t.getF32ptr()[3] = 0.f;

						norm_mat.affineTransform(t, *tangentp);
						tangentp->getF32ptr()[3] = w;
					}
				}
			}
		}
	}

	if (node_hit != -1)
	{
		if (prim_hitp)
		{
			*prim_hitp = primitive_hit;
		}
	}

	return node_hit;
}

void Asset::update()
{
	F32 frame_time = (F32)LLFrameTimer::getElapsedSeconds();
	F32 dt = frame_time - mLastUpdateTime;
	if (dt <= 0.f)
	{
		return;
	}
	mLastUpdateTime = frame_time;

	if (mAnimations.size() > 0)
	{
#if 0	// For now, these settings are not even listed in LL's viewer
		// app_settings/settings.xml, so they default to 0 and 1.f
		// respectively. If configuration is ever to be provided, just use
		// static variables in the LLGLTF::Asset class and setter methods to
		// set them on viewer launch from llappviewer.cpp, and on change from
		// llviewercontrol.cpp... HB
		static LLCachedControl<U32> anim_idx(gSavedSettings,
											 "GLTFAnimationIndex", 0);
		static LLCachedControl<F32> anim_speed(gSavedSettings,
											   "GLTFAnimationSpeed", 1.f);
		U32 idx = llclamp(anim_idx(), 0U, mAnimations.size() - 1);
#else
		constexpr U32 idx = 0;
		constexpr F32 anim_speed = 1.f;
#endif
		mAnimations[idx].update(*this, dt * anim_speed);
	}

	updateTransforms();

	for (auto& skin : mSkins)
	{
		skin.uploadMatrixPalette(*this);
	}

	uploadMaterials();

	const F32 max_vsize = gMaxImageSizeDefault * gMaxImageSizeDefault;
	for (auto& image : mImages)
	{
		if (image.mLoadIntoTexturePipe && image.mTexture.notNull())
		{
			// *HACK: force to load textures at full resolution.
			// *TODO: calculate actual vsize.
			image.mTexture->addTextureStats(max_vsize);
			// *HACK: mark as a sculpt texture so that it does not get evicted
			// from VRAM after a while by the texture fetcher, while still in
			// use. HB
			image.mTexture->setBoostLevel(LLGLTexture::BOOST_SCULPTED);
		}
	}
}

bool Asset::prep()
{
	if (mLoadIntoVRAM && !LLGLSLShader::sCurBoundShaderPtr)
	{
		llwarns << "No bound shader !" << llendl;
		llassert(false);
		return false;
	}

	// Check required extensions and fail if not supported
	for (size_t i = 0, count = mExtensionsRequired.size(); i < count; ++i)
	{
		const std::string& extension = mExtensionsRequired[i];
		if (extension != "KHR_materials_unlit" &&
			extension != "KHR_texture_transform")
		{
			mUnsupportedExtensions.emplace_back(extension);
			llwarns << "Unsupported extension: " << extension << llendl;
		}
		else if (extension == "KHR_materials_pbrSpecularGlossiness")
		{
			mIgnoredExtensions.emplace_back(extension);
			llinfos << "Ignored extension: " << extension << llendl;
		}
	}
	if (!mUnsupportedExtensions.empty())
	{
		return false;
	}

	// Do buffers first as other resources depend on them
	for (auto& buffer : mBuffers)
	{
		if (!buffer.prep(*this))
		{
			llwarns << "Failed to prepare buffer: " << buffer.mName << llendl;
			return false;
		}
	}

	for (auto& image : mImages)
	{
		if (!image.prep(*this, mLoadIntoVRAM))
		{
			llwarns << "Failed to prepare image: " << image.mName << llendl;
			return false;
		}
	}

	for (auto& mesh : mMeshes)
	{
		if (!mesh.prep(*this))
		{
			llwarns << "Failed to prepare mesh: " << mesh.mName << llendl;
			return false;
		}
	}

	for (auto& animation : mAnimations)
	{
		if (!animation.prep(*this))
		{
			llwarns << "Failed to prepare animation: " << animation.mName
					<< llendl;
			return false;
		}
	}

	for (auto& skin : mSkins)
	{
		if (!skin.prep(*this))
		{
			llwarns << "Failed to prepare skin: " << skin.mName << llendl;
			return false;
		}
	}

	if (!mLoadIntoVRAM)
	{
		return true;
	}

	// Prepare vertex buffers

	// Material count is number of materials + 1 for default material
	const S32 mat_count = mMaterials.size() + 1;
	LL_DEBUGS("LLGLTF") << "Number of materials: " << mat_count << LL_ENDL;

	for (U32 double_sided = 0; double_sided < 2; ++double_sided)
	{
		RenderData& rd = mRenderData[double_sided];
		for (U8 i = 0; i < LLGLSLShader::NUM_GLTF_VARIANTS; ++i)
		{
			rd.mBatches[i].resize(mat_count);
		}

		// For each material
		for (S32 mat_id = -1; mat_id < mat_count - 1; ++mat_id)
		{
			if (mat_id >= 0 && mMaterials[mat_id].mDoubleSided)
			{
				if (!double_sided)
				{
					continue;
				}
			}
			else if (double_sided)
			{
				continue;
			}

			// For each shader variant
			U32 vertex_count[LLGLSLShader::NUM_GLTF_VARIANTS] = { 0 };
			U32 index_count[LLGLSLShader::NUM_GLTF_VARIANTS] = { 0 };
			for (U8 v = 0; v < LLGLSLShader::NUM_GLTF_VARIANTS; ++v)
			{
				U32 attribute_mask = 0;
				// For each mesh
				for (auto& mesh : mMeshes)
				{
					// For each primitive in this mesh
					for (auto& prim : mesh.mPrimitives)
					{
						if (prim.mMaterial != mat_id ||
							prim.mShaderVariant != v)
						{
							continue;
						}
						prim.mVertexOffset = vertex_count[v];
						prim.mIndexOffset = index_count[v];
						vertex_count[v] += prim.getVertexCount();
						index_count[v] += prim.getIndexCount();
						LL_DEBUGS("LLGLTF") << "Added primitive. vertex_count = "
												<< vertex_count[v]
												<< " - index_count = "
												<< index_count[v] << LL_ENDL;
						if (attribute_mask &&
							attribute_mask != prim.mAttributeMask)
						{
							llwarns_once << "Discrepancy in primitives vertex buffer mask for mesh: "
										 << mesh.mName << llendl;
							llassert(false);
						}
						attribute_mask |= prim.mAttributeMask;
					}

#if LL_PREPARE_VB
					// Allocate vertex buffer and pack it
					if (vertex_count[v])
					{
						U32 prim_uploads = 0;
						U32 mat_idx = mat_id + 1;
						LLVertexBuffer* vb = new LLVertexBuffer(attribute_mask);
						rd.mBatches[v][mat_idx].mVertexBuffer = vb;
						// *HACK: double index count... *TODO: find a better way to
						// indicate 32 bits indices will be used.
						vb->allocateBuffer(vertex_count[v], index_count[v] * 2);
						vb->setBuffer();
						for (auto& mesh : mMeshes)
						{
							for (auto& prim : mesh.mPrimitives)
							{
								if (prim.mMaterial == mat_id &&
									prim.mShaderVariant == v)
								{
									prim.upload(vb);
									++prim_uploads;
								}
							}
						}
						vb->unmapBuffer();
						vb->unbind();
						LL_DEBUGS("LLGLTF") << "Allocated vertex buffer for "
											<< prim_uploads
											<< " primitives in mesh: "
											<< mesh.mName << LL_ENDL;
					}
#endif
				}
			}
		}
	}

#if LL_PREPARE_VB
	// Sanity-check that all primitives have a vertex buffer
	LL_DEBUGS("LLGLTF") << "Checking vertex buffers... ";
	U32 good_buffers = 0;
	U32 bad_buffers = 0;
	for (auto& mesh : mMeshes)
	{
		bool missing_buffer = false;
		for (auto& prim : mesh.mPrimitives)
		{
			if (prim.mVertexBuffer.isNull())
			{
				++bad_buffers;
				missing_buffer = false;
			}
			else
			{
				++good_buffers;
			}
		}
		if (missing_buffer)
		{
			LL_CONT << "Missing vertex buffer in mesh: " << mesh.mName << ". ";
		}
	}
	LL_CONT << good_buffers << " primitives with vertex buffers, "
			<< bad_buffers << " without." << LL_ENDL;

	// Build render batches
	for (S32 i = 0, count = mNodes.size(); i < count; ++i)
	{
		Node& node = mNodes[i];
		if (node.mMesh < 0)
		{
			continue;
		}
		auto& mesh = mMeshes[node.mMesh];
		S32 mat_idx = mesh.mPrimitives[0].mMaterial + 1;
		S32 double_sided = mat_idx == 0 ? 0
										: mMaterials[mat_idx - 1].mDoubleSided;
		for (S32 j = 0, count2 = mesh.mPrimitives.size(); j < count2; ++j)
		{
			auto& primitive = mesh.mPrimitives[j];
			U8 v = primitive.mShaderVariant;
			RenderData& rd = mRenderData[double_sided];
			RenderBatch& rb = rd.mBatches[v][mat_idx];
			rb.mPrimitives.emplace_back(j, i);
		}
	}
#endif

	return true;
}

const Asset& Asset::operator=(const lljson& src)
{
	if (src.is_object())
	{
		const auto it = src.find("asset");
		if (it != src.end())
		{
			const lljson& asset = it.value();
			copy(asset, "version", mVersion);
			copy(asset, "minVersion", mMinVersion);
			copy(asset, "generator", mGenerator);
			copy(asset, "copyright", mCopyright);
#if 0		// Not used anywhere (for now ?). HB
			copy(asset, "extras", mExtras);
#endif
		}

		copy(src, "scene", mScene);
		copy(src, "scenes", mScenes);
		copy(src, "nodes", mNodes);
		copy(src, "meshes", mMeshes);
		copy(src, "materials", mMaterials);
		copy(src, "buffers", mBuffers);
		copy(src, "bufferViews", mBufferViews);
		copy(src, "textures", mTextures);
		copy(src, "samplers", mSamplers);
		copy(src, "images", mImages);
		copy(src, "accessors", mAccessors);
		copy(src, "animations", mAnimations);
		copy(src, "skins", mSkins);
		copy(src, "extensionsUsed", mExtensionsUsed);
		copy(src, "extensionsRequired", mExtensionsRequired);
	}
	return *this;
}

void Asset::serialize(lljson& dst) const
{
	static const std::string sGenerator = "Linden Lab GLTF Prototype v0.1";

	dst["asset"] = lljson{};
	lljson& asset = dst["asset"];

	write(mVersion, "version", asset);
	write(mMinVersion, "minVersion", asset, std::string());
	write(sGenerator, "generator", asset);
	write(mScene, "scene", dst, INVALID_INDEX);
	write(mScenes, "scenes", dst);
	write(mNodes, "nodes", dst);
	write(mMeshes, "meshes", dst);
	write(mMaterials, "materials", dst);
	write(mBuffers, "buffers", dst);
	write(mBufferViews, "bufferViews", dst);
	write(mTextures, "textures", dst);
	write(mSamplers, "samplers", dst);
	write(mImages, "images", dst);
	write(mAccessors, "accessors", dst);
	write(mAnimations, "animations", dst);
	write(mSkins, "skins", dst);
	write(mExtensionsUsed, "extensionsUsed", dst);
	write(mExtensionsRequired, "extensionsRequired", dst);
}

bool Asset::load(std::string_view filename, bool load_into_vram)
{
	mLoadIntoVRAM = load_into_vram;
	mFilename = filename;
	std::string ext = LLDir::getExtension(mFilename);
	if (ext != "gltf" && ext != "glb")
	{
		llwarns << "Unsupported file type: " << ext << ". Aborted." << llendl;
		return false;
	}

	llifstream file(filename.data(), std::ios::binary);
	if (!file.is_open())
	{
		llwarns << "Failed to open file: " << filename << ". Aborted."
				<< llendl;
		return false;
	}

	std::string str((std::istreambuf_iterator<char>(file)),
					 std::istreambuf_iterator<char>());
	file.close();

	if (ext == "gltf")
	{
		lljson val;
		try
		{
			val = lljson::parse(str);
		}
		catch (const nlohmann::json::parse_error& e)
		{
			llwarns << "JSON parsing error: " << e.what() << llendl;
			return false;
		}
		*this = val;
		return prep();
	}

	return loadBinary(str, mLoadIntoVRAM);
}

bool Asset::loadBinary(const std::string& data, bool load_into_vram)
{
	mLoadIntoVRAM = load_into_vram;
	const U8* ptr = (const U8*)data.data();
	const U8* end = ptr + data.size();

	if (end - ptr < 12)
	{
		llwarns << "GLB file too short. Aborted." << llendl;
		return false;
	}

	U32 magic = *(U32*)ptr;
	ptr += 4;
	if (magic != 0x46546C67)
	{
		llwarns << "Invalid GLB file. Aborted." << llendl;
		return false;
	}

	U32 version = *(U32*)ptr;
	ptr += 4;
	if (version != 2)
	{
		llwarns << "Unsupported GLB version. Aborted." << llendl;
		return false;
	}

	U32 len = *(U32*)ptr;
	ptr += 4;
	if (len != data.size())
	{
		llwarns << "GLB length mismatch. Aborted." << llendl;
		return false;
	}

	U32 chunk_len = *(U32*)ptr;
	ptr += 4;
	if (end - ptr < chunk_len + 8)
	{
		llwarns << "GLB chunk too short. Aborted." << llendl;
		return false;
	}

	U32 chunk_type = *(U32*)ptr;
	ptr += 4;
	if (chunk_type != 0x4E4F534A)
	{
		llwarns << "Invalid GLB chunk type. Aborted." << llendl;
		return false;
	}

	lljson val;
	try
	{
		val = lljson::parse(std::string_view((const char*)ptr, chunk_len));
	}
	catch (const nlohmann::json::parse_error& e)
	{
		llwarns << "JSON parsing error: " << e.what() << llendl;
		return false;
	}
	*this = val;

	if (mBuffers.size() > 0 && mBuffers[0].mUri.empty())
	{
		ptr += chunk_len;
		if (end - ptr < 8)
		{
			llwarns << "GLB chunk too short. Aborted." << llendl;
			return false;
		}
		chunk_len = *(U32*)ptr;
		ptr += 4;

		chunk_type = *(U32*)ptr;
		ptr += 4;
		if (chunk_type != 0x004E4942)
		{
			llwarns << "Invalid GLB chunk type. Aborted." << llendl;
			return false;
		}

		auto& buffer = mBuffers[0];
		if (ptr + buffer.mByteLength > end)
		{
			llwarns << "Buffer too short. Aborted." << llendl;
			return false;
		}
		buffer.mData.resize(buffer.mByteLength);
		memcpy((void*)buffer.mData.data(), (void*)ptr, buffer.mByteLength);
	}

	return prep();
}

bool Asset::save(const std::string& filename)
{
	if (filename.empty())
	{
		return false;
	}

	std::string folder = LLDir::getDirName(filename);

	// Save images
	for (auto& image : mImages)
	{
		if (!image.save(*this, folder))
		{
			return false;
		}
	}

	// Save buffers
	// NOTE: we save buffers after saving images as saving images may remove
	// image data from buffers.
	for (auto& buffer : mBuffers)
	{
		if (!buffer.save(*this, folder))
		{
			return false;
		}
	}

	// Save the .gltf file
	llofstream file(filename, std::ios::binary);
	if (!file.is_open())
	{
		llwarns << "Failed to open file: " << filename << ". Aborted."
				<< llendl;
		return false;
	}
	lljson obj;
	serialize(obj);
	std::string buffer = obj.dump();
	file.write(buffer.c_str(), buffer.size());

	return true;
}

void Asset::eraseBufferView(S32 bufferview)
{
	mBufferViews.erase(mBufferViews.begin() + bufferview);

	for (auto& accessor : mAccessors)
	{
		if (accessor.mBufferView > bufferview)
		{
			--accessor.mBufferView;
		}
	}

	for (auto& image : mImages)
	{
		if (image.mBufferView > bufferview)
		{
			--image.mBufferView;
		}
	}
}

void Asset::serializeToString(std::string& buffer)
{
	lljson obj;
	serialize(obj);
	buffer = obj.dump();
}

//static
Asset* Asset::createFromJsonData(const std::string& data)
{
	lljson json;
	try
	{
		json = lljson::parse(data);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		llwarns << "Failed to parse data with error: " << e.what() << llendl;
		return NULL;
	}
	return new Asset(json);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::TextureTransform class
///////////////////////////////////////////////////////////////////////////////

TextureTransform::TextureTransform()
:	mOffset(0.f, 0.f),
	mScale(1.f, 1.f),
	mRotation(0.f),
	mTexCoord(INVALID_INDEX),
	mPresent(false)
{
}

const TextureTransform& TextureTransform::operator=(const lljson& src)
{
	mPresent = true;
	if (src.is_object())
	{
		copy(src, "offset", mOffset);
		copy(src, "rotation", mRotation);
		copy(src, "scale", mScale);
		copy(src, "texCoord", mTexCoord);
	}
	return *this;
}

void TextureTransform::serialize(lljson& dst) const
{
	write(mOffset, "offset", dst, vec2(0.f, 0.f));
	write(mRotation, "rotation", dst, 0.f);
	write(mScale, "scale", dst, vec2(1.f, 1.f));
	write(mTexCoord, "texCoord", dst, -1);
}

void TextureTransform::getPacked(vec4* packedp) const
{
	packedp[0] = vec4(mScale.x, mScale.y, mRotation, mOffset.x);
	packedp[1] = vec4(mOffset.y, 0.f, 0.f, 0.f);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::TextureInfo class and derivates
///////////////////////////////////////////////////////////////////////////////

bool TextureInfo::operator==(const TextureInfo& rhs) const
{
	return mIndex == rhs.mIndex && mTexCoord == rhs.mTexCoord;
}

bool TextureInfo::operator!=(const TextureInfo& rhs) const
{
	return mIndex != rhs.mIndex || mTexCoord != rhs.mTexCoord;
}

S32 TextureInfo::getTexCoord() const
{
	if (mTextureTransform.mPresent &&
		mTextureTransform.mTexCoord != INVALID_INDEX)
	{
		return mTextureTransform.mTexCoord;
	}
	return mTexCoord;
}

const TextureInfo& TextureInfo::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "index", mIndex);
		copy(src, "texCoord", mTexCoord);
		copy_extensions(src, "KHR_texture_transform", &mTextureTransform);
	}
	return *this;
}

void TextureInfo::serialize(lljson& dst) const
{
	write(mIndex, "index", dst, INVALID_INDEX);
	write(mTexCoord, "texCoord", dst, 0);
	write_extensions(dst, &mTextureTransform, "KHR_texture_transform");
}

const OcclusionTextureInfo& OcclusionTextureInfo::operator=(const lljson& src)
{
	if (src.is_object())
	{
		TextureInfo::operator=(src);
		copy(src, "strength", mStrength);
	}
	return *this;
}

void OcclusionTextureInfo::serialize(lljson& dst) const
{
	TextureInfo::serialize(dst);
	write(mStrength, "strength", dst, 1.f);
}

const NormalTextureInfo& NormalTextureInfo::operator=(const lljson& src)
{
	if (src.is_object())
	{
		TextureInfo::operator=(src);
		copy(src, "index", mIndex);
		copy(src, "texCoord", mTexCoord);
		copy(src, "scale", mScale);
	}
	return *this;
}

void NormalTextureInfo::serialize(lljson& dst) const
{
	TextureInfo::serialize(dst);
	write(mScale, "scale", dst, 1.f);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Material class and sub-classes
///////////////////////////////////////////////////////////////////////////////

Material::Material()
:	mEmissiveFactor(0.f, 0.f, 0.f),
	mAlphaMode(AlphaMode::OPAQUE),
	mAlphaCutoff(0.5f),
	mDoubleSided(false)
{
}

const Material& Material::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "name", mName);
		copy(src, "emissiveFactor", mEmissiveFactor);
		copy(src, "pbrMetallicRoughness", mPbrMetallicRoughness);
		copy(src, "normalTexture", mNormalTexture);
		copy(src, "occlusionTexture", mOcclusionTexture);
		copy(src, "emissiveTexture", mEmissiveTexture);
		copy(src, "alphaMode", mAlphaMode);
		copy(src, "alphaCutoff", mAlphaCutoff);
		copy(src, "doubleSided", mDoubleSided);
		copy_extensions(src, "KHR_materials_unlit", &mUnlit);
	}
	return *this;
}

void Material::serialize(lljson& dst) const
{
	write(mName, "name", dst);
	write(mEmissiveFactor, "emissiveFactor", dst, vec3(0.f, 0.f, 0.f));
	write(mPbrMetallicRoughness, "pbrMetallicRoughness", dst);
	write(mNormalTexture, "normalTexture", dst);
	write(mOcclusionTexture, "occlusionTexture", dst);
	write(mEmissiveTexture, "emissiveTexture", dst);
	write(mAlphaMode, "alphaMode", dst, AlphaMode::OPAQUE);
	write(mAlphaCutoff, "alphaCutoff", dst, 0.5f);
	write(mDoubleSided, "doubleSided", dst, false);
	write_extensions(dst, &mUnlit, "KHR_materials_unlit");
}

bool Material::isMultiUV() const
{
	return mPbrMetallicRoughness.mBaseColorTexture.getTexCoord() ||
		   mPbrMetallicRoughness.mMetallicRoughnessTexture.getTexCoord() ||
		   mNormalTexture.getTexCoord() || mOcclusionTexture.getTexCoord() ||
		   mEmissiveTexture.getTexCoord();
}

Material::PbrMetallicRoughness::PbrMetallicRoughness()
:	mBaseColorFactor(1.f, 1.f, 1.f, 1.f),
	mMetallicFactor(1.f),
	mRoughnessFactor(1.f)
{
}

const Material::PbrMetallicRoughness&
Material::PbrMetallicRoughness::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "baseColorFactor", mBaseColorFactor);
		copy(src, "baseColorTexture", mBaseColorTexture);
		copy(src, "metallicFactor", mMetallicFactor);
		copy(src, "roughnessFactor", mRoughnessFactor);
		copy(src, "metallicRoughnessTexture", mMetallicRoughnessTexture);
	}
	return *this;
}

void Material::PbrMetallicRoughness::serialize(lljson& dst) const
{
	write(mBaseColorFactor, "baseColorFactor", dst, vec4(1.f, 1.f, 1.f, 1.f));
	write(mBaseColorTexture, "baseColorTexture", dst);
	write(mMetallicFactor, "metallicFactor", dst, 1.f);
	write(mRoughnessFactor, "roughnessFactor", dst, 1.f);
	write(mMetallicRoughnessTexture, "metallicRoughnessTexture", dst);
}

bool Material::PbrMetallicRoughness::operator==(const Material::PbrMetallicRoughness& rhs) const
{
	return mBaseColorFactor == rhs.mBaseColorFactor &&
		   mBaseColorTexture == rhs.mBaseColorTexture &&
		   mMetallicFactor == rhs.mMetallicFactor &&
		   mRoughnessFactor == rhs.mRoughnessFactor &&
		   mMetallicRoughnessTexture == rhs.mMetallicRoughnessTexture;
}

bool Material::PbrMetallicRoughness::operator!=(const Material::PbrMetallicRoughness& rhs) const
{
	return mBaseColorFactor != rhs.mBaseColorFactor ||
		   mBaseColorTexture != rhs.mBaseColorTexture ||
		   mMetallicFactor != rhs.mMetallicFactor ||
		   mRoughnessFactor != rhs.mRoughnessFactor ||
		   mMetallicRoughnessTexture != rhs.mMetallicRoughnessTexture;
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Mesh class
///////////////////////////////////////////////////////////////////////////////

const Mesh& Mesh::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "primitives", mPrimitives);
		copy(src, "weights", mWeights);
		copy(src, "name", mName);
	}
	return *this;
}

void Mesh::serialize(lljson& dst) const
{
	write(mPrimitives, "primitives", dst);
	write(mWeights, "weights", dst);
	write(mName, "name", dst);
}

bool Mesh::prep(Asset& asset)
{
	for (auto& primitive : mPrimitives)
	{
		if (!primitive.prep(asset))
		{
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Texture class
///////////////////////////////////////////////////////////////////////////////

Texture::Texture()
:	mSampler(INVALID_INDEX),
	mSource(INVALID_INDEX)
{
}

const Texture& Texture::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "sampler", mSampler);
		copy(src, "source", mSource);
		copy(src, "name", mName);
	}
	return *this;
}

void Texture::serialize(lljson& dst) const
{
	write(mSampler, "sampler", dst, INVALID_INDEX);
	write(mSource, "source", dst, INVALID_INDEX);
	write(mName, "name", dst);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Image class
///////////////////////////////////////////////////////////////////////////////

Image::Image()
:	mBufferView(INVALID_INDEX),
	mWidth(-1),
	mHeight(-1),
	mComponent(-1),
	mBits(-1),
	mPixelType(-1),
	mLoadIntoTexturePipe(false)
{
}

const Image& Image::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "uri", mUri);
		copy(src, "mimeType", mMimeType);
		copy(src, "bufferView", mBufferView);
		copy(src, "name", mName);
		copy(src, "width", mWidth);
		copy(src, "height", mHeight);
		copy(src, "component", mComponent);
		copy(src, "bits", mBits);
		copy(src, "pixelType", mPixelType);
	}
	return *this;
}

void Image::serialize(lljson& dst) const
{
	write(mUri, "uri", dst);
	write(mMimeType, "mimeType", dst);
	write(mBufferView, "bufferView", dst, INVALID_INDEX);
	write(mName, "name", dst);
	write(mWidth, "width", dst, -1);
	write(mHeight, "height", dst, -1);
	write(mComponent, "component", dst, -1);
	write(mBits, "bits", dst, -1);
	write(mPixelType, "pixelType", dst, -1);
}

bool Image::prep(Asset& asset, bool load_into_vram)
{
	llassert(Asset::sTexFromFetchedFn && Asset::sTexFromFileFn &&
			 Asset::sTexFromMemoryFn);

	mLoadIntoTexturePipe = load_into_vram;

	if (mUri.find("data:") == 0)
	{
		// Embedded in a data URI, load the texture from the URI.
		llwarns << "Data URIs not yet supported" << llendl;
		return false;
	}

	LLUUID id;
	if (mUri.size() == UUID_STR_SIZE && LLUUID::parseUUID(mUri, &id) &&
		id.notNull())
	{
		mTexture = Asset::sTexFromFetchedFn(id);
	}
	else if (mBufferView != INVALID_INDEX)
	{
		// Embedded in a buffer, load the texture from the buffer
		BufferView& bview = asset.mBufferViews[mBufferView];
		Buffer& buffer = asset.mBuffers[bview.mBuffer];
		if (mLoadIntoTexturePipe)
		{
			U8* datap = buffer.mData.data() + bview.mByteOffset;
			mTexture = Asset::sTexFromMemoryFn(datap, bview.mByteLength,
											   mMimeType);
		}
		else if (mTexture.isNull() && mLoadIntoTexturePipe)
		{
			llwarns << "Failed to load image '" << mName
					<< "' from buffer. MIME type: " << mMimeType << llendl;
			return false;
		}
	}
	else if (!asset.mFilename.empty() && !mUri.empty())
	{
		if (mLoadIntoTexturePipe)
		{
			std::string dir = LLDir::getDirName(asset.mFilename);
			std::string img_file = dir + LL_DIR_DELIM_STR + mUri;
			if (!LLFile::exists(img_file))
			{
				// Characters might be escaped in the URI
				img_file = dir + LL_DIR_DELIM_STR + LLURI::unescape(mUri);
				if (!LLFile::exists(img_file))
				{
					llwarns << "Failed to load image '" << mUri
							<< "' from buffer. MIME type: " << mMimeType
							<< llendl;
					return false;
				}
			}
			mTexture = Asset::sTexFromFileFn(img_file);
			if (mTexture.isNull())
			{
				llwarns << "Failed to load image '" << mName
						<< "' from buffer. MIME type: " << mMimeType << llendl;
				return false;
			}
		}
	}
	else
	{
		llwarns << "Failed to load image: " << mName << llendl;
		return false;
	}

	if (!asset.mFilename.empty() && mLoadIntoTexturePipe)
	{
		// *HACK: mark as a sculpt texture so that it does not get evicted
		// from VRAM after a while by the texture fetcher, while still in use.
		// HB
		mTexture->setBoostLevel(LLGLTexture::BOOST_SCULPTED);
#if 0	// We cannot do this here (in order to keep the llgltf library outside
		// of the main viewer (newview) code, mTexture is a LLGLTexture, even
		// if it does point to a LLViewerTexture, which is the sub-class
		// implementing the forceToSaveRawImage() method). This is instead done
		// in LLModelPreview::loadTextures(), as it should be anyway... HB
		mTexture->forceToSaveRawImage(0, F32_MAX);
#endif
	}

	return true;
}

void Image::clearData(Asset& asset)
{
	if (mBufferView != INVALID_INDEX)
	{
		// Remove data from buffer
		BufferView& bufferview = asset.mBufferViews[mBufferView];
		Buffer& buffer = asset.mBuffers[bufferview.mBuffer];
		buffer.erase(asset, bufferview.mByteOffset, bufferview.mByteLength);
		asset.eraseBufferView(mBufferView);
	}
	mBufferView = INVALID_INDEX;
	mWidth = mHeight = mComponent = mBits = mPixelType = -1;
	mLoadIntoTexturePipe = false;
	mMimeType.clear();
}

// Note: this *MUST* be a lossless save since artists use this to save their
// work repeatedly, so adding any compression artifacts here would degrade
// images over time.
bool Image::save(Asset& asset, const std::string& folder)
{
	llassert(Asset::sGetLocaTexFilenameFn);

	std::string name = mName;
	if (name.empty())
	{
		S32 idx = (S32)(this - asset.mImages.data());
		name = llformat("image_%d", idx);
	}

	if (mBufferView != INVALID_INDEX)
	{
		// We have the bytes of the original image, save that out in its
		// original format
		std::string extension;
		if (mMimeType == "image/jpeg")
		{
			extension = ".jpg";
		}
		else if (mMimeType == "image/png")
		{
			extension = ".png";
		}
		else if (mMimeType == "image/tga")
		{
			extension = ".tga";
		}
		else
		{
			extension = ".bin";
			llwarns << "Unsupported mime image type, saved as .bin" << llendl;
		}
		mUri = name + extension;

		std::string filename = folder + LL_DIR_DELIM_STR + name + extension;
		llofstream file(filename, std::ios::binary);
		if (!file.is_open())
		{
			llwarns << "Failed to open file: " << filename << ". Aborted."
					<< llendl;
			return false;
		}
		BufferView& bview = asset.mBufferViews[mBufferView];
		Buffer& buffer = asset.mBuffers[bview.mBuffer];
		file.write((const char*)buffer.mData.data() + bview.mByteOffset,
				   bview.mByteLength);
		clearData(asset);
		return true;
	}

	// No image ?...
	if (mTexture.isNull())
	{
		clearData(asset);
	    return true;
	}

	const std::string& source =
		Asset::sGetLocaTexFilenameFn(mTexture->getID());
	if (!source.empty())
	{
		if (!LLFile::exists(source))
		{
			llwarns << "File not found: " << source << llendl;
			return false;
		}
		// This is a local texture: just copy it over.
		std::string filename = LLDir::getBaseFileName(source);
		std::string dest = folder + LL_DIR_DELIM_STR + filename;
		LLFile::copy(source, dest);
		mUri = filename;
		clearData(asset);
	    return true;
	}

	if (mUri.empty())
	{
		llwarns << "Image is not a local image and has no URI, cannot save."
				<< llendl;
		return false;
	}

	std::string base_filename = LLDir::getBaseFileName(mUri);
	std::string from_dir = LLDir::getDirName(asset.mFilename);
	std::string filename = from_dir + LL_DIR_DELIM_STR + base_filename;
	if (!LLFile::exists(filename))
	{
		llwarns << "File not found: " << filename << llendl;
		return false;
	}
	std::string dest = folder + LL_DIR_DELIM_STR + base_filename;
	LLFile::copy(filename, dest);
	mUri = base_filename;
	clearData(asset);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Sampler class
///////////////////////////////////////////////////////////////////////////////

const Sampler& Sampler::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "magFilter", mMagFilter);
		copy(src, "minFilter", mMinFilter);
		copy(src, "wrapS", mWrapS);
		copy(src, "wrapT", mWrapT);
		copy(src, "name", mName);
	}
	return *this;
}

void Sampler::serialize(lljson& dst) const
{
	write(mMagFilter, "magFilter", dst, (S32)GL_LINEAR);
	write(mMinFilter, "minFilter", dst, (S32)GL_LINEAR_MIPMAP_LINEAR);
	write(mWrapS, "wrapS", dst, (S32)GL_REPEAT);
	write(mWrapT, "wrapT", dst, (S32)GL_REPEAT);
	write(mName, "name", dst);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Skin class
///////////////////////////////////////////////////////////////////////////////

Skin::Skin()
:	mUBO(0),
	mInverseBindMatrices(INVALID_INDEX),
	mSkeleton(INVALID_INDEX)
{
}

Skin::~Skin()
{
	if (mUBO)
	{
		LLGLManager::deleteBuffers(1, &mUBO);
	}
}

bool Skin::prep(Asset& asset)
{
	if (mInverseBindMatrices != INVALID_INDEX)
	{
		Accessor& accessor = asset.mAccessors[mInverseBindMatrices];
		copy(asset, accessor, mInverseBindMatricesData);
	}
	return true;
}

const Skin& Skin::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "name", mName);
		copy(src, "skeleton", mSkeleton);
		copy(src, "inverseBindMatrices", mInverseBindMatrices);
		copy(src, "joints", mJoints);
	}
	return *this;
}

void Skin::serialize(lljson& obj) const
{
	write(mInverseBindMatrices, "inverseBindMatrices", obj, INVALID_INDEX);
	write(mJoints, "joints", obj);
	write(mName, "name", obj);
	write(mSkeleton, "skeleton", obj, INVALID_INDEX);
}

void Skin::uploadMatrixPalette(Asset& asset)
{
	// This is the maximum number of 3x4 matrices than can fit in a UBO
	const U32 max_joints = LLGLManager::sMaxUniformBlockSize / 48;

	// Use static buffers for speed. HB
	static bool initialized = false;
	alignas(16) static std::vector<mat4> t_mp;
	if (!initialized)
	{
		initialized = true;
		t_mp.reserve(max_joints);
	}

	// Prepare matrix palette
	U32 joint_count = llmin(max_joints, mJoints.size());
	t_mp.resize(joint_count);
	for (U32 i = 0; i < joint_count; ++i)
	{
		Node& joint = asset.mNodes[mJoints[i]];
		// Build matrix palette in asset space
		t_mp[i] = joint.mAssetMatrix * mInverseBindMatricesData[i];
	}

	upload_matrice_palette_to_ubo(t_mp, mUBO);
}
