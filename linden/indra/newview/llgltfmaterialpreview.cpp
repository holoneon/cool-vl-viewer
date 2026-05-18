/**
 * @file llgltfmaterialpreview.cpp
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include <memory>

#include "llgltfmaterialpreview.h"

#include "llavatarappearancedefines.h"
#include "llvolumemgr.h"

#include "llpipeline.h"
#include "llselectmgr.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"

constexpr S32 FULLY_LOADED = 0;
constexpr S32 NOT_LOADED = 99;

// LLGLTFPreviewTexture::MaterialLoadLevels sub-class

bool LLGLTFPreviewTexture::MaterialLoadLevels::isFullyLoaded() const
{
	for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
	{
		if (mLevels[i] != FULLY_LOADED)
		{
			return false;
		}
	}
	return true;
}

LLGLTFPreviewTexture::MaterialLoadLevels::MaterialLoadLevels()
{
	for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
	{
		mLevels[i] = NOT_LOADED;
	}
}

bool LLGLTFPreviewTexture::MaterialLoadLevels::operator<(const MaterialLoadLevels& other) const
{
	bool less = false;
	for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
	{
		if ((*this)[i] > other[i])
		{
			return false;
		}
		less |= (*this)[i] < other[i];
	}
	return less;
}

bool LLGLTFPreviewTexture::MaterialLoadLevels::operator>(const MaterialLoadLevels& other) const
{
	bool great = false;
	for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
	{
		if ((*this)[i] < other[i])
		{
			return false;
		}
		great |= (*this)[i] > other[i];
	}
	return great;
}

// Helper functions

static void fetch_tex_for_ui(LLPointer<LLViewerFetchedTexture>& texp,
							 const LLUUID& id)
{
	if (texp.isNull() && id.notNull())
	{
		if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(id))
		{
			LLViewerObject* objp = gSelectMgr.getSelection()->getFirstObject();
			if (objp)
			{
				LLViewerTexture* imgp = objp->getBakedTextureForMagicId(id);
				texp = imgp ? imgp->asFetched() : NULL;
			}
		}
		else
		{
			texp =
				LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, true,
														  LLGLTexture::BOOST_PREVIEW,
														  LLViewerTexture::LOD_TEXTURE);
		}
	}
	if (texp)
	{
		texp->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
		texp->forceToSaveRawImage(0);
	}
}

// Note: does not use the same conventions as texture discard level. Lower is
// better.
static S32 get_texture_load_level(LLViewerFetchedTexture* texp)
{
	if (!texp)
	{
		return FULLY_LOADED;
	}
	S32 raw_level = texp->getDiscardLevel();
	return raw_level < 0 ? NOT_LOADED : raw_level;
}

static LLGLTFPreviewTexture::MaterialLoadLevels get_load_levels(LLFetchedGLTFMaterial* matp)
{
	llassert(!matp->isFetching());

	using MaterialTextures =
		LLPointer<LLViewerFetchedTexture>*[LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT];

	MaterialTextures textures;
	textures[BASECOLIDX] = &matp->mBaseColorTexture;
	textures[NORMALIDX] = &matp->mNormalTexture;
	textures[MROUGHIDX] = &matp->mMetallicRoughnessTexture;
	textures[EMISSIVEIDX] = &matp->mEmissiveTexture;

	LLGLTFPreviewTexture::MaterialLoadLevels levels;

	for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
	{
		fetch_tex_for_ui(*textures[i], matp->mTextureId[i]);
		levels[i] = get_texture_load_level(*textures[i]);
	}

	return levels;
}

//static
LLPointer<LLViewerTexture> LLGLTFPreviewTexture::getPreview(LLFetchedGLTFMaterial* matp)
{
	if (!matp || matp->isFetching())
	{
		return nullptr;
	}

	LLGLTFPreviewTexture::MaterialLoadLevels levels = get_load_levels(matp);
	for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
	{
		if (levels[i] == NOT_LOADED)
		{
			return nullptr;
		}
	}

	return new LLGLTFPreviewTexture(matp);
}

LLGLTFPreviewTexture::LLGLTFPreviewTexture(LLFetchedGLTFMaterial* matp,
										   S32 width)
:	LLViewerDynamicTexture(width, width, 4, ORDER_FIRST, false),
	mGLTFMaterial(matp),
	mShouldRender(true)
{
}

//virtual
bool LLGLTFPreviewTexture::needsRender()
{
	if (!mShouldRender && mBestLoad.isFullyLoaded())
	{
		return false;
	}

	MaterialLoadLevels current_load = get_load_levels(mGLTFMaterial.get());
	if (current_load < mBestLoad)
	{
		mShouldRender = true;
		mBestLoad = current_load;
		return true;
	}

	return false;
}

//virtual
void LLGLTFPreviewTexture::preRender(bool clear_depth)
{
	if (mShouldRender)
	{
		LLViewerDynamicTexture::preRender(clear_depth);
	}
}

class GLTFPreviewModel
{
public:
	GLTFPreviewModel(LLDrawInfo* infop, const LLMatrix4& mat)
	:	mDrawInfo(infop),
		mModelMatrix(mat)
	{
		mDrawInfo->mModelMatrix = &mModelMatrix;
	}

	GLTFPreviewModel(GLTFPreviewModel&) = delete;

	~GLTFPreviewModel()
	{
		// No model matrix necromancy
		llassert(gGLLastMatrix != &mModelMatrix);
		gGLLastMatrix = NULL;
	}

public:
	LLPointer<LLDrawInfo>	mDrawInfo;
	LLMatrix4				mModelMatrix;	// Referenced by mDrawInfo
};

using PreviewSpherePart = std::unique_ptr<GLTFPreviewModel>;
using PreviewSphere = std::vector<PreviewSpherePart>;

// Like LLVolumeGeometryManager::registerFace but without batching or
// too-many-indices/vertices checking.
static PreviewSphere create_preview_sphere(LLFetchedGLTFMaterial* matp,
										   const LLMatrix4& model_matrix)
{
	const LLColor4U vertex_color(matp->mBaseColor);

	LLPrimitive prim;
	prim.setPCode(LL_PCODE_VOLUME);
	LLVolumeParams params;
	params.setType(LL_PCODE_PROFILE_CIRCLE_HALF, LL_PCODE_PATH_CIRCLE);
	params.setBeginAndEndS(0.f, 1.f);
	params.setBeginAndEndT(0.f, 1.f);
	params.setRatio(1, 1);
	params.setShear(0, 0);
	constexpr auto MAX_LOD = LLVolumeLODGroup::NUM_LODS - 1;
	prim.setVolume(params, MAX_LOD);

	LLVolume* volp = prim.getVolume();
	for (LLVolumeFace& face : volp->getVolumeFaces())
	{
		face.createTangents();
	}

	PreviewSphere sphere;
	sphere.reserve(volp->getNumFaces());

	constexpr U32 mask = LLVertexBuffer::MAP_VERTEX |
						 LLVertexBuffer::MAP_NORMAL |
						 LLVertexBuffer::MAP_TEXCOORD0 |
						 LLVertexBuffer::MAP_COLOR |
						 LLVertexBuffer::MAP_TANGENT;
	LLPointer<LLVertexBuffer> bufp = new LLVertexBuffer(mask);
	U32 nv = 0;
	U32 ni = 0;
	for (LLVolumeFace& face : volp->getVolumeFaces())
	{
		nv += face.mNumVertices;
		ni += face.mNumIndices;
	}
	bufp->allocateBuffer(nv, ni);

	// UV hacks
	// Higher factor helps to see more details on the preview sphere
	const LLVector2 uv_factor(2.f, 2.f);
	// Offset places center of material in center of view
	const LLVector2 uv_offset(-0.5f, -0.5f);

	LLStrider<U16> indices;
	LLStrider<LLVector4a> positions;
	LLStrider<LLVector4a> normals;
	LLStrider<LLVector2> texcoords;
	LLStrider<LLColor4U> colors;
	LLStrider<LLVector4a> tangents;
	bufp->getIndexStrider(indices);
	bufp->getVertexStrider(positions);
	bufp->getNormalStrider(normals);
	bufp->getTexCoord0Strider(texcoords);
	bufp->getColorStrider(colors);
	bufp->getTangentStrider(tangents);
	U32 index_offset = 0;
	U32 vertex_offset = 0;
	for (const LLVolumeFace& face : volp->getVolumeFaces())
	{
		for (S32 i = 0; i < face.mNumIndices; ++i)
		{
			*indices++ = face.mIndices[i] + vertex_offset;
		}
		for (S32 v = 0; v < face.mNumVertices; ++v)
		{
			*positions++ = face.mPositions[v];
			*normals++ = face.mNormals[v];
			LLVector2 uv(face.mTexCoords[v]);
			uv.scaleVec(uv_factor);
			uv += uv_offset;
			*texcoords++ = uv;
			*colors++ = vertex_color;
			*tangents++ = face.mTangents[v];
		}

		LLPointer<LLDrawInfo> infop = new LLDrawInfo(vertex_offset,
													 vertex_offset +
													 face.mNumVertices - 1,
													 face.mNumIndices,
													 index_offset,
													 NULL, bufp.get());
		infop->mGLTFMaterial = matp;
		sphere.emplace_back(std::make_unique<GLTFPreviewModel>(infop,
															   model_matrix));
		index_offset += face.mNumIndices;
		vertex_offset += face.mNumVertices;
	}

	bufp->unmapBuffer();

	return sphere;
}

static void set_preview_sphere_material(PreviewSphere& sphere,
										LLFetchedGLTFMaterial* matp)
{
	if (sphere.empty())
	{
		llassert(false);
		return;
	}

	const LLColor4U vertex_color(matp->mBaseColor);

	// See comments about unmapBuffer in llvertexbuffer.h
	for (PreviewSpherePart& part : sphere)
	{
		LLDrawInfo* infop = part->mDrawInfo.get();
		infop->mGLTFMaterial = matp;
		LLVertexBuffer* bufp = infop->mVertexBuffer.get();
		LLStrider<LLColor4U> colors;
		const S32 count = infop->mEnd - infop->mStart + 1;
		bufp->getColorStrider(colors, infop->mStart, count);
		for (S32 i = 0; i < count; ++i)
		{
			*colors++ = vertex_color;
		}
		bufp->unmapBuffer();
	}
}

static PreviewSphere& get_preview_sphere(LLFetchedGLTFMaterial* matp,
										 const LLMatrix4& model_matrix)
{
	static PreviewSphere sphere;
	if (sphere.empty())
	{
		sphere = create_preview_sphere(matp, model_matrix);
	}
	else
	{
		set_preview_sphere_material(sphere, matp);
	}
	return sphere;
}

//virtual
bool LLGLTFPreviewTexture::render()
{
	if (!mShouldRender || !gUsePBRShaders)
	{
		return false;
	}

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	LLGLDepthTest(GL_FALSE);
	LLGLDisable scissor(GL_SCISSOR_TEST);

	bool old_depth_of_field = LLPipeline::RenderDepthOfField;
	LLPipeline::RenderDepthOfField = false;

	bool old_glow = LLPipeline::RenderGlow;
	LLPipeline::RenderGlow = false;

	bool old_ssr = LLPipeline::sRenderSSR;
	LLPipeline::sRenderSSR = false;

	U32 old_samples = LLPipeline::RenderFSAASamples;
	LLPipeline::RenderFSAASamples = 0;

	LLPipeline::RenderTargetPack* old_pack = gPipeline.mRT;
	gPipeline.mRT = &gPipeline.mAuxillaryRT;

	gPipeline.mReflectionMapManager.forceDefaultProbeAndUpdateUniforms();

	LLViewerCamera camera;
	// Calculate the object distance at which the object of a given radius will
	// span the partial width of the screen given by fill_ratio. Assume the
	// primitive has a scale of 1 (this is the default).
	constexpr F32 fill_ratio = 0.8f;
	constexpr F32 object_radius = 0.5f;
	// Negative coordinate shows the textures on the sphere right-side up, when
	// combined with the UV hacks in create_preview_sphere
	static const LLVector3 obj_pos(0.f,
								   -(object_radius / fill_ratio) *
								   tanf(camera.getDefaultFOV()),
								   0.f);
	LLMatrix4 object_transform;
	object_transform.translate(obj_pos);

	// Set up camera and viewport
	camera.lookAt(LLVector3::zero, obj_pos);
	camera.setAspect(mFullWidth / mFullHeight);
	const LLRect texture_rect(0, mFullHeight, mFullWidth, 0);
	camera.setPerspective(NOT_FOR_SELECTION, texture_rect.mLeft,
						  texture_rect.mBottom, texture_rect.getWidth(),
						  texture_rect.getHeight(), false, camera.getNear(),
						  MAX_FAR_CLIP * 2.f);

	// Generate sphere object on-the-fly. Discard afterwards (vertex buffer is
	// discarded, but the sphere should be cached in LLVolumeMgr).
	PreviewSphere& sphere = get_preview_sphere(mGLTFMaterial.get(),
											   object_transform);

	// Do not let environment settings influence our scene lighting.
	HBPreviewLighting preview_light(true);

	LLRenderTarget& screen = gPipeline.mAuxillaryRT.mScreen;
	LLRenderTarget& depth = gPipeline.mAuxillaryRT.mDeferredScreen;

	// *HACK: Force reset of the model matrix
	gGLLastMatrix = NULL;

#if 0
	if (mGLTFMaterial->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_OPAQUE ||
		mGLTFMaterial->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
	{
		// *TODO: Opaque/alpha mask rendering
	}
	else
#endif
	{
		// Alpha blend rendering

		screen.bindTarget();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		LLGLSLShader& shader = gDeferredPBRAlphaProgram;

		gPipeline.bindDeferredShader(shader);
		shader.uniform1f(LLShaderMgr::DENSITY_MULTIPLIER, 0.f);
		// Ignore shadows (if enabled)
		for (U32 i = 0; i < 6; ++i)
		{
			S32 chan =
				shader.getTextureChannel(LLShaderMgr::DEFERRED_SHADOW0 + i);
			if (chan != -1)
			{
				gGL.getTexUnit(chan)->bind(LLViewerFetchedTexture::sWhiteImagep,
										   true);
			}
		}

		for (PreviewSpherePart& part : sphere)
		{
			LLRenderPass::pushGLTFBatch(*part->mDrawInfo);
		}

		gPipeline.unbindDeferredShader(shader);

		screen.flush();
	}

	gPipeline.generateLuminance(&screen, &gPipeline.mLuminanceMap);
	constexpr bool DO_NOT_USE_HISTORY = false;
	gPipeline.generateExposure(&gPipeline.mLuminanceMap,
							   &gPipeline.mExposureMap, DO_NOT_USE_HISTORY);
	gPipeline.gammaCorrect(&screen, &gPipeline.mPostMap);
	LLVertexBuffer::unbind();
	gPipeline.generateGlow(&gPipeline.mPostMap);
	gPipeline.combineGlow(&gPipeline.mPostMap, &screen);

	// *HACK: restore mExposureMap (it will be consumed by generateExposure()
	// at next frame).
	gPipeline.mExposureMap.swapFBORefs(gPipeline.mLastExposure);

	// Final render

	gDeferredPostNoDoFProgram.bind();

	// From LLPipeline::renderFinalize: "Whatever is last in the above post
	// processing chain should _always_ be rendered directly here. If not,
	// expect problems."
	gDeferredPostNoDoFProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE,
										  &screen);
	gDeferredPostNoDoFProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH,
										  &depth, true);

	{
		LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_ALWAYS);
		gPipeline.mScreenTriangleVB->setBuffer();
		gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
	}

	gDeferredPostNoDoFProgram.unbind();

	// Clean up
	gPipeline.mReflectionMapManager.forceDefaultProbeAndUpdateUniforms(false);
	gPipeline.mRT = old_pack;
	LLPipeline::RenderFSAASamples = old_samples;
	LLPipeline::sRenderSSR = old_ssr;
	LLPipeline::RenderGlow = old_glow;
	LLPipeline::RenderDepthOfField = old_depth_of_field;

	return true;
}

//virtual
void LLGLTFPreviewTexture::postRender(bool success)
{
	if (mShouldRender)
	{
		mShouldRender = false;
		LLViewerDynamicTexture::postRender(success);
	}
}
