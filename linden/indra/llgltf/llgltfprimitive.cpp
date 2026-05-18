/**
 * @file llgltfprimitive.cpp
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

#include "linden_common.h"

#include "meshoptimizer.h"
#include "mikktspace/mikktspace.hh"

#include "llgltfprimitive.h"

#include "llglslshader.h"
#include "llgltfasset.h"
#include "llgltfbufferutil.h"
#include "llgltfglm.h"

using namespace LLGLTF;

// Mesh data useful for Mikktspace tangent generation (and flat normal
// generation)
class MikktMesh
{
public:
	// Initialize from src primitive and make an unrolled triangle list
	// returns false if the Primitive cannot be converted to a triangle list.
	bool copy(const Primitive* prim)
	{
		bool indexed = !prim->mIndexArray.empty();
		size_t vert_count = indexed ? prim->mIndexArray.size()
									: prim->mPositions.size();

		size_t triangle_count = 0;
		if (prim->mMode == Primitive::Mode::TRIANGLE_STRIP ||
			prim->mMode == Primitive::Mode::TRIANGLE_FAN)
		{
			triangle_count = vert_count - 2;
		}
		else if (prim->mMode == Primitive::Mode::TRIANGLES)
		{
			triangle_count = vert_count / 3;
		}
		else
		{
			llwarns << "Unsupported primitive mode for conversion to triangles: "
					<< (S32)prim->mMode << llendl;
			return false;
		}

		vert_count = triangle_count * 3;
		// Triangle_count will also naturally be under the limit
		llassert(vert_count <= size_t(U32_MAX));

		p.resize(vert_count);
		n.resize(vert_count);
		tc0.resize(vert_count);
		c.resize(vert_count);

		bool has_normals = !prim->mNormals.empty();
		if (has_normals)
		{
			n.resize(vert_count);
		}
		bool has_tangents = !prim->mTangents.empty();
		if (has_tangents)
		{
			t.resize(vert_count);
		}

		bool rigged = !prim->mWeights.empty();
		if (rigged)
		{
			w.resize(vert_count);
			j.resize(vert_count);
		}

		bool multi_uv = !prim->mTexCoords1.empty();
		if (multi_uv)
		{
			tc1.resize(vert_count);
		}

		for (U32 tri_idx = 0; tri_idx < U32(triangle_count); ++tri_idx)
		{
			U32 idx[3] = {0, 0, 0};

			if (prim->mMode == Primitive::Mode::TRIANGLES)
			{
				idx[0] = tri_idx * 3;
				idx[1] = tri_idx * 3 + 1;
				idx[2] = tri_idx * 3 + 2;
			}
			else if (prim->mMode == Primitive::Mode::TRIANGLE_STRIP)
			{
				idx[0] = tri_idx;
				idx[1] = tri_idx + 1;
				idx[2] = tri_idx + 2;

				if (tri_idx % 2 != 0)
				{
					std::swap(idx[1], idx[2]);
				}
			}
			else if (prim->mMode == Primitive::Mode::TRIANGLE_FAN)
			{
				idx[0] = 0;
				idx[1] = tri_idx + 1;
				idx[2] = tri_idx + 2;
			}

			if (indexed)
			{
				idx[0] = prim->mIndexArray[idx[0]];
				idx[1] = prim->mIndexArray[idx[1]];
				idx[2] = prim->mIndexArray[idx[2]];
			}

			for (U32 v = 0; v < 3; ++v)
			{
				U32 i = tri_idx * 3 + v;
				p[i].set(prim->mPositions[idx[v]].getF32ptr());
				tc0[i].set(prim->mTexCoords0[idx[v]]);
				c[i] = prim->mColors[idx[v]];

				if (multi_uv)
				{
					tc1[i].set(prim->mTexCoords1[idx[v]]);
				}

				if (has_normals)
				{
					n[i].set(prim->mNormals[idx[v]].getF32ptr());
				}

				if (rigged)
				{
					w[i].set(prim->mWeights[idx[v]].getF32ptr());
					j[i] = (U64)prim->mJoints[idx[v]];
				}
			}
		}

		return true;
	}

	void genNormals()
	{
		size_t tri_count = p.size() / 3;
		for (size_t i = 0; i < tri_count; ++i)
		{
			LLVector3 v0 = p[i * 3];
			LLVector3 v1 = p[i * 3 + 1];
			LLVector3 v2 = p[i * 3 + 2];

			LLVector3 normal = (v1 - v0) % (v2 - v0);
			normal.normalize();

			n[i * 3] = normal;
			n[i * 3 + 1] = normal;
			n[i * 3 + 2] = normal;
		}
	}

	void genTangents()
	{
		t.resize(p.size());
		mikk::Mikktspace ctx(*this);
		ctx.genTangSpace();
	}

	// Write to target primitive as an indexed triangle list.
	// Only modifies runtime data, does not modify the original glTF data.
	void write(Primitive* prim) const
	{
		// Re-weld
		std::vector<meshopt_Stream> mos =
		{
			{ &p[0], sizeof(LLVector3), sizeof(LLVector3) },
			{ &n[0], sizeof(LLVector3), sizeof(LLVector3) },
			{ &t[0], sizeof(LLVector4), sizeof(LLVector4) },
			{ &tc0[0], sizeof(LLVector2), sizeof(LLVector2) },
			{ &c[0], sizeof(LLColor4U), sizeof(LLColor4U) }
		};

		if (!w.empty())
		{
			mos.push_back({ &w[0], sizeof(LLVector4), sizeof(LLVector4) });
			mos.push_back({ &j[0], sizeof(U64), sizeof(U64) });
		}

		if (!tc1.empty())
		{
			mos.push_back({ &tc1[0], sizeof(LLVector2), sizeof(LLVector2) });
		}

		std::vector<U32> remap;
		remap.resize(p.size());

		size_t stream_count = mos.size();

		size_t vert_count = meshopt_generateVertexRemapMulti(&remap[0], NULL,
															 p.size(),
															 p.size(),
															 mos.data(),
															 stream_count);
		prim->mTexCoords0.resize(vert_count);
		prim->mNormals.resize(vert_count);
		prim->mTangents.resize(vert_count);
		prim->mPositions.resize(vert_count);
		prim->mColors.resize(vert_count);
		if (!w.empty())
		{
			prim->mWeights.resize(vert_count);
			prim->mJoints.resize(vert_count);
		}
		if (!tc1.empty())
		{
			prim->mTexCoords1.resize(vert_count);
		}

		prim->mIndexArray.resize(remap.size());

		for (U32 i = 0; i < remap.size(); ++i)
		{
			U32 src_idx = i;
			U32 dst_idx = remap[i];

			prim->mIndexArray[i] = dst_idx;

			prim->mPositions[dst_idx].load3(p[src_idx].mV);
			prim->mNormals[dst_idx].load3(n[src_idx].mV);
			prim->mTexCoords0[dst_idx] = tc0[src_idx];
			prim->mTangents[dst_idx].loadua(t[src_idx].mV);
			prim->mColors[dst_idx] = c[src_idx];

			if (!w.empty())
			{
				prim->mWeights[dst_idx].loadua(w[src_idx].mV);
				prim->mJoints[dst_idx] = j[src_idx];
			}

			if (!tc1.empty())
			{
				prim->mTexCoords1[dst_idx] = tc1[src_idx];
			}
		}

		prim->mGLMode = LLRender::TRIANGLES;
	}

	LL_INLINE U32 GetNumFaces()
	{
		return U32(p.size() / 3);
	}

	LL_INLINE U32 GetNumVerticesOfFace(U32 face_num)
	{
		return 3;
	}

	LL_INLINE mikk::float3 GetPosition(U32 face_num, U32 vert_num)
	{
		F32* v = p[face_num * 3 + vert_num].mV;
		return mikk::float3(v);
	}

	LL_INLINE mikk::float3 GetTexCoord(U32 face_num, U32 vert_num)
	{
		F32* uv = tc0[face_num * 3 + vert_num].mV;
		return mikk::float3(uv[0], 1.f-uv[1], 1.f);
	}

	LL_INLINE mikk::float3 GetNormal(U32 face_num, U32 vert_num)
	{
		F32* normal = n[face_num * 3 + vert_num].mV;
		return mikk::float3(normal);
	}

	LL_INLINE void SetTangentSpace(U32 face_num, U32 vert_num,
								   mikk::float3 T, bool orientation)
	{
		S32 i = face_num * 3 + vert_num;
		t[i].set(T.x, T.y, T.z, orientation ? 1.f : -1.f);
	}

public:
	std::vector<LLVector3> p;	// positions
	std::vector<LLVector3> n;	// Normals
	std::vector<LLVector4> t;	// Tangents
	std::vector<LLVector2> tc0;	// Texcoords 0
	std::vector<LLVector2> tc1;	// Texcoords 1
	std::vector<LLColor4U> c;	// colors
	std::vector<LLVector4> w;	// Weights
	std::vector<U64> j;			// Joints
};

Primitive::Primitive()
:	mMaterial(-1),
	mIndices(-1),
	mVertexOffset(0),
	mIndexOffset(0),
	mAttributeMask(0),
	mGLMode(LLRender::TRIANGLES),
	mMode(Mode::TRIANGLES),
	mShaderVariant(0)
{
}

Primitive::~Primitive()
{
	mOctree = NULL;
}

// Allocates a vertex buffer. We diverge from the intent of the glTF format
// here to work with our existing render pipeline. glTF wants us to copy the
// buffer views into GPU storage as is and build render commands that source
// that data. For our engine, though, it is better to rearrange the buffers at
// load time into a layout which is more consistent. The glTF native approach
// undoubtedly works well if you can count on VAOs, but VAOs perform much worse
// with our scenes.
// Returns true on success, or false on failure. HB
bool Primitive::prep(Asset& asset)
{
	// Load vertex data
	for (auto& it : mAttributes)
	{
		const std::string& attrib_name = it.first;
		Accessor& accessor = asset.mAccessors[it.second];

		if (attrib_name == "POSITION")
		{
			copy(asset, accessor, mPositions);
		}
		else if (attrib_name == "NORMAL")
		{
			copy(asset, accessor, mNormals);
		}
		else if (attrib_name == "TANGENT")
		{
			copy(asset, accessor, mTangents);
		}
		else if (attrib_name == "COLOR_0")
		{
			copy(asset, accessor, mColors);
		}
		else if (attrib_name == "TEXCOORD_0")
		{
			copy(asset, accessor, mTexCoords0);
		}
		else if (attrib_name == "TEXCOORD_1")
		{
			copy(asset, accessor, mTexCoords1);
		}
		else if (attrib_name == "JOINTS_0")
		{
			copy(asset, accessor, mJoints);
		}
		else if (attrib_name == "WEIGHTS_0")
		{
			copy(asset, accessor, mWeights);
		}
	}

	if (mPositions.size() > (size_t)S32_MAX)
	{
		// That many veertices would trigger an llerrs in
		// LLVertexBuffer::allocateBuffer(). HB
		llwarns << "Too many vertices: " << mPositions.size() << ". Aborted."
				<< llendl;
		return false;
	}

	// Copy index buffer
	if (mIndices != INVALID_INDEX)
	{
		Accessor& accessor = asset.mAccessors[mIndices];
		copy(asset, accessor, mIndexArray);
		for (auto& idx : mIndexArray)
		{
			if (idx >= mPositions.size())
			{
				llwarns << "Invalid index array. Aborted." << llendl;
				return false;
			}
		}
	}
	else
	{
		// Everything must be indexed at runtime
		size_t count = mPositions.size();
		mIndexArray.resize(count);
		for (size_t i = 0; i < count; ++i)
		{
			mIndexArray[i] = i;
		}
	}

	if (mIndexArray.size() > (size_t)S32_MAX)
	{
		// That many veertices would trigger an llerrs in
		// LLVertexBuffer::allocateBuffer(). HB
		llwarns << "Too large an index array: " << mIndexArray.size()
				<< ". Aborted." << llendl;
		return false;
	}

	U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 |
			   LLVertexBuffer::MAP_COLOR;
	mShaderVariant = 0;
	
	if (!mWeights.empty())
	{
		mShaderVariant |= LLGLSLShader::RIGGED;
		mask |= LLVertexBuffer::MAP_WEIGHT4;
		mask |= LLVertexBuffer::MAP_JOINT;
	}

	if (mTexCoords0.empty())
	{
		mTexCoords0.resize(mPositions.size());
	}
	if (!mTexCoords1.empty())
	{
		mask |= LLVertexBuffer::MAP_TEXCOORD1;
	}

	if (mColors.empty())
	{
		mColors.resize(mPositions.size(), LLColor4U::white);
	}

	bool unlit = false;
	if (mMaterial != INVALID_INDEX)
	{
		// Bake material basecolor into color array
		const Material& mat = asset.mMaterials[mMaterial];
		LLColor4 base_col(glm::value_ptr(mat.mPbrMetallicRoughness.mBaseColorFactor));
		for (auto& dst : mColors)
		{
			dst = LLColor4U(base_col * LLColor4(dst));
		}

		if (mat.mUnlit.mPresent)
		{
			// Material uses KHR_materials_unlit
			unlit = true;
			mShaderVariant |= LLGLSLShader::UNLIT;
		}

		if (mat.isMultiUV())
		{
			mShaderVariant |= LLGLSLShader::MULTI_UV;
		}
	}
	if (mNormals.empty() && !unlit)
	{
		mTangents.clear();
		if (mMode == Mode::POINTS || mMode == Mode::LINES ||
			mMode == Mode::LINE_LOOP || mMode == Mode::LINE_STRIP)
		{
			// No normals and no surfaces, this primitive is unlit
			unlit = true;
			mTangents.clear();
			mShaderVariant |= LLGLSLShader::UNLIT;			
		}
		else
		{
			// Unroll into non-indexed array of flat shaded triangles
			MikktMesh data;
			if (!data.copy(this))
			{
				return false;
			}
			data.genNormals();
			data.genTangents();
			data.write(this);
		}
	}
	
	if (mTangents.empty() && !unlit)
	{
		// NOTE: must be done last because tangent generation rewrites the
		// other arrays
		if (mMode == Mode::POINTS || mMode == Mode::LINES ||
			mMode == Mode::LINE_LOOP || mMode == Mode::LINE_STRIP)
		{
			// For points and lines, just make sure tangent is perpendicular to
			// normal
			size_t count = mNormals.size();
			mTangents.resize(count);
			LLVector4a up(0.f, 0.f, 1.f, 0.f);
			LLVector4a left(1.f, 0.f, 0.f, 0.f);
			for (size_t i = 0; i < count; ++i)
			{
				if (fabsf(mNormals[i].getF32ptr()[2]) < 0.999f)
				{
					mTangents[i] = up.cross3(mNormals[i]);
				}
				else
				{
					mTangents[i] = left.cross3(mNormals[i]);
				}
				mTangents[i].getF32ptr()[3] = 1.f;
			}
		}
		else
		{
			MikktMesh data;
			if (!data.copy(this))
			{
				return false;
			}
			data.genTangents();
			data.write(this);
		}
	}

	if (!mNormals.empty())
	{
		mask |= LLVertexBuffer::MAP_NORMAL;
	}
	if (!mTangents.empty())
	{
		mask |= LLVertexBuffer::MAP_TANGENT;
	}

	mAttributeMask = mask;

	if (mMaterial != INVALID_INDEX)
	{
		const Material& mat = asset.mMaterials[mMaterial];
		if (mat.mAlphaMode == Material::AlphaMode::BLEND)
		{
			mShaderVariant |= LLGLSLShader::ALPHA_BLEND;
		}
	}
	
	createOctree();
	
	return true;
}

static void vertical_flip(std::vector<LLVector2>& texcoords)
{
	for (size_t i = 0, count = texcoords.size(); i < count; ++i)
	{
		LLVector2& tc = texcoords[i];
		tc[1] = 1.f - tc[1];
	}
}

void Primitive::upload(LLVertexBuffer* bufferp)
{
	if (!bufferp)
	{
		llwarns << "NULL vertex buffer pointer passed." << llendl;
		llassert(false);
		return;
	}
	mVertexBuffer = bufferp;

	U32 offset = mVertexOffset;
	U32 count = getVertexCount();

	if (mVertexBuffer->getNumVerts() < mPositions.size() + offset ||
		mVertexBuffer->getNumIndices() < mIndexArray.size() + mIndexOffset ||
		mVertexBuffer->getTypeMask() != mAttributeMask)
	{
		llwarns << "Invalid vertex buffer." << llendl;
		llassert(false);
		return;
	}

	mVertexBuffer->setPositionData(mPositions.data(), offset, count);
	mVertexBuffer->setColorData(mColors.data(), offset, count);

	if (!mNormals.empty())
	{
		mVertexBuffer->setNormalData(mNormals.data(), offset, count);
	}
	if (!mTangents.empty())
	{
		mVertexBuffer->setTangentData(mTangents.data(), offset, count);
	}

	if (!mWeights.empty())
	{
		mVertexBuffer->setWeight4Data(mWeights.data(), offset, count);
		mVertexBuffer->setJointData(mJoints.data(), offset, count);
	}

	// Flip texcoord y, upload, then flip back (keep the off-spec data in VRAM
	// only).
	vertical_flip(mTexCoords0);
	mVertexBuffer->setTexCoord0Data(mTexCoords0.data(), offset, count);
	vertical_flip(mTexCoords0);
	if (!mTexCoords1.empty())
	{
		vertical_flip(mTexCoords1);
		mVertexBuffer->setTexCoord1Data(mTexCoords1.data(), offset, count);
		vertical_flip(mTexCoords1);
	}

	if (!mIndexArray.empty())
	{
		static std::vector<U32> index_array;
		size_t count = mIndexArray.size();
		index_array.resize(count);
		for (size_t i = 0; i < count; ++i)
		{
			index_array[i] = mIndexArray[i] + mVertexOffset;
		}
		mVertexBuffer->setIndexData(index_array.data(), mIndexOffset,
									getIndexCount());
	}
}

static void init_octree_triangle(LLVolumeTriangle* trianglep, S32 i0, S32 i1,
								 S32 i2, const LLVector4a& v0,
								 const LLVector4a& v1, const LLVector4a& v2)
{
	// Store pointers to vertex data
	trianglep->mV[0] = &v0;
	trianglep->mV[1] = &v1;
	trianglep->mV[2] = &v2;

	// Store indices
	trianglep->mIndex[0] = i0;
	trianglep->mIndex[1] = i1;
	trianglep->mIndex[2] = i2;

	// Get minimum point
	LLVector4a min = v0;
	min.setMin(min, v1);
	min.setMin(min, v2);

	// Get maximum point
	LLVector4a max = v0;
	max.setMax(max, v1);
	max.setMax(max, v2);

	// Compute center
	LLVector4a center;
	center.setAdd(min, max);
	center.mul(0.5f);

	trianglep->mPositionGroup = center;

	// Compute "radius"
	LLVector4a size;
	size.setSub(max, min);
	constexpr F32 scaler = 0.25f;
	trianglep->mRadius = size.getLength3().getF32() * scaler;
}

void Primitive::createOctree()
{
	mOctree = new LLVolumeOctree();

	const U32 num_indices = getIndexCount();
	if (num_indices < 3)
	{
		// Degenerate triangle: no volume !  HB
		llwarns << "Degenerate triangle found" << llendl;
	}
	else if (mMode == Mode::TRIANGLES)
	{
		const U32 num_triangles = num_indices / 3;
		// Initialize all the triangles we need
		mOctreeTriangles.resize(num_triangles);
		for (U32 tri_idx = 0; tri_idx < num_triangles; ++tri_idx)
		{
			const U32 index = tri_idx * 3;
			S32 i0 = mIndexArray[index];
			S32 i1 = mIndexArray[index + 1];
			S32 i2 = mIndexArray[index + 2];

			const LLVector4a& v0 = mPositions[i0];
			const LLVector4a& v1 = mPositions[i1];
			const LLVector4a& v2 = mPositions[i2];
			
			LLVolumeTriangle* trianglep = &mOctreeTriangles[tri_idx];
			init_octree_triangle(trianglep, i0, i1, i2, v0, v1, v2);
			mOctree->insert(trianglep);
		}
	}
	else if (mMode == Mode::TRIANGLE_STRIP)
	{
		const U32 num_triangles = num_indices - 2;
		// Initialize all the triangles we need
		mOctreeTriangles.resize(num_triangles);
		for (U32 tri_idx = 0; tri_idx < num_triangles; ++tri_idx)
		{
			const U32 index = tri_idx + 2;
			S32 i0 = mIndexArray[index];
			S32 i1 = mIndexArray[index - 1];
			S32 i2 = mIndexArray[index - 2];

			const LLVector4a& v0 = mPositions[i0];
			const LLVector4a& v1 = mPositions[i1];
			const LLVector4a& v2 = mPositions[i2];

			LLVolumeTriangle* trianglep = &mOctreeTriangles[tri_idx];
			init_octree_triangle(trianglep, i0, i1, i2, v0, v1, v2);
			mOctree->insert(trianglep);
		}
	}
	else if (mMode == Mode::TRIANGLE_FAN)
	{
		const U32 num_triangles = num_indices - 2;
		// Initialize all the triangles we need
		mOctreeTriangles.resize(num_triangles);
		for (U32 tri_idx = 0; tri_idx < num_triangles; ++tri_idx)
		{
			const U32 index = tri_idx + 2;
			S32 i0 = mIndexArray[0];
			S32 i1 = mIndexArray[index - 1];
			S32 i2 = mIndexArray[index - 2];

			const LLVector4a& v0 = mPositions[i0];
			const LLVector4a& v1 = mPositions[i1];
			const LLVector4a& v2 = mPositions[i2];

			LLVolumeTriangle* trianglep = &mOctreeTriangles[tri_idx];
			init_octree_triangle(trianglep, i0, i1, i2, v0, v1, v2);
			mOctree->insert(trianglep);
		}
	}
	else
	{
		llwarns << "Unsupported primitive mode: " << (S32)mMode << llendl;
	}

	// Remove unneeded octree layers
	while (!mOctree->balance()) ;

	// Calculate AABB for each node
	LLVolumeOctreeRebound rebound;
	rebound.traverse(mOctree);
}

const LLVolumeTriangle* Primitive::lineSegmentIntersect(const LLVector4a& start,
														const LLVector4a& end,
														LLVector4a* interp,
														LLVector2* tcoordp,
														LLVector4a* normp,
														LLVector4a* tgtp)
{
	if (mOctree.isNull())
	{
		return NULL;
	}

	LLVector4a dir;
	dir.setSub(end, start);

	F32 closest_t = 2.f; // Must be larger than 1

	// Create a proxy LLVolumeFace for the raycast
	LLVolumeFace face;
	face.mPositions = mPositions.data();
	face.mTexCoords = mTexCoords0.data();
	face.mNormals = mNormals.data();
	face.mTangents = mTangents.data();
	face.mIndices = NULL; // unreferenced

	face.mNumIndices = mIndexArray.size();
	face.mNumVertices = mPositions.size();

	LLOctreeTriangleRayIntersectNoOwnership intersect(start, dir, &face,
													  &closest_t, interp,
													  tcoordp, normp, tgtp);
	intersect.traverse(mOctree);

	// Null out proxy data so it does not get freed
	face.mPositions = face.mNormals = face.mTangents = NULL;
	face.mIndices = NULL;
	face.mTexCoords = NULL;

	return intersect.mHitTriangle;
}

void Primitive::serialize(lljson& dst) const
{
	write(mMaterial, "material", dst, -1);
	write(mMode, "mode", dst, Primitive::Mode::TRIANGLES);
	write(mIndices, "indices", dst, INVALID_INDEX);
	write(mAttributes, "attributes", dst);
}

const Primitive& Primitive::operator=(const lljson& src)
{
    if (src.is_object())
    {
		copy(src, "material", mMaterial);
		copy(src, "mode", mMode);
		copy(src, "indices", mIndices);
		copy(src, "attributes", mAttributes);

		switch (mMode)
		{
			case Mode::POINTS:
				mGLMode = LLRender::POINTS;
				break;

			case Mode::LINES:
				mGLMode = LLRender::LINES;
				break;

			case Mode::LINE_LOOP:
				mGLMode = LLRender::LINE_LOOP;
				break;

			case Mode::LINE_STRIP:
				mGLMode = LLRender::LINE_STRIP;
				break;

			case Mode::TRIANGLES:
				mGLMode = LLRender::TRIANGLES;
				break;

			case Mode::TRIANGLE_STRIP:
				mGLMode = LLRender::TRIANGLE_STRIP;
				break;

			case Mode::TRIANGLE_FAN:
				mGLMode = LLRender::TRIANGLE_FAN;
				break;

			default:
				mGLMode = LLRender::TRIANGLES;
		}
	}
	return *this;
}
