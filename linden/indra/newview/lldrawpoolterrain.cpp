/**
 * @file lldrawpoolterrain.cpp
 * @brief LLDrawPoolTerrain class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolterrain.h"

#include "imageids.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "lldrawable.h"
#include "llenvironment.h"
#include "llenvsettings.h"
#include "llface.h"
#include "llpipeline.h"
//MK
#include "mkrlinterface.h"
//mk
#include "llsky.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"			// For gRenderParcelOwnership
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"		// To get alpha gradients
#include "llvlcomposition.h"
#include "llvosurfacepatch.h"
#include "llworld.h"

constexpr F32 DETAIL_SCALE = 1.f / 16.f;
int DebugDetailMap = 0;

F32 LLDrawPoolTerrain::sDetailScale = DETAIL_SCALE;
static LLGLSLShader* sShader = NULL;

LLDrawPoolTerrain::LLDrawPoolTerrain(LLViewerTexture* texturep)
:	LLFacePool(POOL_TERRAIN),
	mTexturep(texturep)
{
	static LLCachedControl<F32> terrain_scale(gSavedSettings,
											  "RenderTerrainScale");
	sDetailScale = 1.f / llmax(0.1f, (F32)terrain_scale);

	mAlphaRampImagep =
		LLViewerTextureManager::getFetchedTexture(IMG_ALPHA_GRAD);
	mAlphaRampImagep->setAddressMode(LLTexUnit::TAM_CLAMP);

	m2DAlphaRampImagep =
		LLViewerTextureManager::getFetchedTexture(IMG_ALPHA_GRAD_2D);
	m2DAlphaRampImagep->setAddressMode(LLTexUnit::TAM_CLAMP);

	if (mTexturep)
	{
		mTexturep->setBoostLevel(LLGLTexture::BOOST_TERRAIN);
	}
}

//virtual
U32 LLDrawPoolTerrain::getVertexDataMask()
{
	if (LLPipeline::sShadowRender)
	{
		return LLVertexBuffer::MAP_VERTEX;
	}
	U32 mask = VERTEX_DATA_MASK;
	if (gUsePBRShaders)
	{
		// We only need tangents for PBR terrains.
		mask = (U32)VERTEX_DATA_MASK | (U32)LLVertexBuffer::MAP_TANGENT;
	}
	if (LLGLSLShader::sCurBoundShaderPtr)
	{
		return mask & ~(LLVertexBuffer::MAP_TEXCOORD2 |
						LLVertexBuffer::MAP_TEXCOORD3);
	}
	return mask;
}

//virtual
void LLDrawPoolTerrain::prerender()
{
	mShaderLevel =
		gViewerShaderMgrp->getShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT);
}

// For use by the EE renderer only
//virtual
void LLDrawPoolTerrain::beginRenderPass(S32)
{
	LL_FAST_TIMER(FTM_RENDER_TERRAIN);

	sShader = LLPipeline::sUnderWaterRender ? &gTerrainWaterProgram
											: &gTerrainProgram;
	if (mShaderLevel > 1 && sShader->mShaderLevel > 0)
	{
		sShader->bind();
	}
}

// For use by the EE renderer only
//virtual
void LLDrawPoolTerrain::endRenderPass(S32 pass)
{
	LL_FAST_TIMER(FTM_RENDER_TERRAIN);

	if (mShaderLevel > 1 && sShader->mShaderLevel > 0)
	{
		sShader->unbind();
	}
}

// For use by the EE renderer only
//virtual
void LLDrawPoolTerrain::render(S32 pass)
{
	LL_FAST_TIMER(FTM_RENDER_TERRAIN);

	if (mDrawFace.empty())
	{
		return;
	}

	// HB. Useless to do it at every render pass...
	// FIXME: see to move this to LLVLComposition so to avoid re-doing it at
	// each frame !!!
	if (pass == 0)
	{
		boostTerrainDetailTextures();
	}

	LLOverrideFaceColor color_override(this, 1.f, 1.f, 1.f, 1.f);

	// Render simplified land if video card cannot do sufficient multitexturing
	if (LLGLManager::sNumTextureImageUnits < 2)
	{
		renderSimple(); // Render without multitexture
		return;
	}

	LLGLSPipeline gls;

	if (mShaderLevel > 1 && sShader->mShaderLevel > 0)
	{
		gPipeline.enableLightsDynamic();
		renderFullShader();
	}
	else
	{
		gPipeline.enableLightsStatic();

		if (LLGLManager::sNumTextureImageUnits < 4)
		{
			renderFull2TU();
		}
		else
		{
			renderFull4TU();
		}
	}

	// Special case for land ownership feedback
	static LLCachedControl<bool> show_parcel_owners(gSavedSettings,
													"ShowParcelOwners");
	if (show_parcel_owners)
	{
		hilightParcelOwners();
	}
}

//virtual
void LLDrawPoolTerrain::beginDeferredPass(S32)
{
	LL_FAST_TIMER(FTM_RENDER_TERRAIN);

	if (!gUsePBRShaders)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sShader = &gDeferredTerrainWaterProgram;
		}
		else
		{
			sShader = &gDeferredTerrainProgram;
		}
		sShader->bind();
	}
}

//virtual
void LLDrawPoolTerrain::endDeferredPass(S32 pass)
{
	LL_FAST_TIMER(FTM_RENDER_TERRAIN);
	LLFacePool::endRenderPass(pass);
	sShader->unbind();
}

//virtual
void LLDrawPoolTerrain::renderDeferred(S32 pass)
{
	LL_FAST_TIMER(FTM_RENDER_TERRAIN);

	if (mDrawFace.empty())
	{
		return;
	}

	if (pass == 0 && gUsePBRShaders)
	{
		boostTerrainDetailTextures();
	}

	renderFullShader();

	// Special case for land ownership feedback
	static LLCachedControl<bool> show_parcel_owners(gSavedSettings,
													"ShowParcelOwners");
	if (show_parcel_owners)
	{
		hilightParcelOwners();
	}
}

//virtual
void LLDrawPoolTerrain::beginShadowPass(S32)
{
	LL_FAST_TIMER(FTM_SHADOW_TERRAIN);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gDeferredShadowProgram.bind();
	gDeferredShadowProgram.uniform1i(LLShaderMgr::SUN_UP_FACTOR,
									 gEnvironment.getIsSunUp() ? 1 : 0);
}

//virtual
void LLDrawPoolTerrain::endShadowPass(S32 pass)
{
	LL_FAST_TIMER(FTM_SHADOW_TERRAIN);
	LLFacePool::endRenderPass(pass);
	gDeferredShadowProgram.unbind();
}

//virtual
void LLDrawPoolTerrain::renderShadow(S32 pass)
{
	LL_FAST_TIMER(FTM_SHADOW_TERRAIN);
	if (mDrawFace.empty())
	{
		return;
	}
#if 0
	LLGLEnable offset(GL_POLYGON_OFFSET);
	glCullFace(GL_FRONT);
#endif
	drawLoop();
#if 0
	glCullFace(GL_BACK);
#endif
}

void LLDrawPoolTerrain::drawLoop()
{
	for (U32 i = 0, count = mDrawFace.size(); i < count; ++i)
	{
		LLFace* facep = mDrawFace[i];
		if (!facep) continue;	// Paranoia

		LLDrawable* drawablep = facep->getDrawable();
		if (!drawablep) continue;

		LLViewerRegion* regionp = drawablep->getRegion();
		if (!regionp) continue;

		LLRenderPass::applyModelMatrix(&(regionp->mRenderMatrix));

		facep->renderIndexed();
	}
}

void LLDrawPoolTerrain::renderFullShader()
{
//MK
	if (gRLenabled && gRLInterface.mContainsCamTextures)
	{
		if (!gUsePBRShaders && LLPipeline::sUnderWaterRender)
		{
			sShader = &gDeferredTerrainWaterProgram;
		}
		else
		{
			sShader = &gDeferredTerrainProgram;
		}
		renderSimple();
		return;
	}
//mk
	LLViewerRegion* regionp =
		mDrawFace[0]->getDrawable()->getVObj()->getRegion();
	if (!regionp) return;	// Paranoia

	if (gUsePBRShaders)
	{
		// Late PBR shader setup (used to be set in beginDeferredPass(), now
		// reserved to ALM), to be able to render both PBR and non-PBR terrains
		// during the same render frame in PBR rendering mode. HB
		if (LLPipeline::sRenderPBRTerrain &&
			regionp->getComposition()->isPBR())
		{
			sShader = &gDeferredPBRTerrainProgram;
			sShader->bind();
			renderFullShaderPBR(regionp);
		}
		else
		{
			sShader = &gDeferredTerrainProgram;
			sShader->bind();
			renderFullShaderTextures(regionp);
		}
	}
	else
	{
		renderFullShaderTextures(regionp);
	}
}

void LLDrawPoolTerrain::renderFullShaderTextures(LLViewerRegion* regionp)
{
	static std::vector<LLViewerTexture*> textures;
	LLVLComposition* compp = regionp->getComposition();
	// *HACK: when the terrain is a PBR one, this will return the base color
	// textures, allowing to render something better than grey terrain, when
	// not in PBR rendering mode. HB
	compp->getTextures(textures);

	const F64 divisor = 1.0 / (F64)sDetailScale;
	LLVector3d region_origin_global = regionp->getOriginGlobal();
	F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], divisor) *
				   sDetailScale;
	F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], divisor) *
				   sDetailScale;

	static LLVector4 tp0, tp1;
	tp0.set(sDetailScale, 0.f, 0.f, offset_x);
	tp1.set(0.f, sDetailScale, 0.f, offset_y);

	// Detail texture 0
	S32 detail0 = sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL0);
	LLTexUnit* unitdet0 = gGL.getTexUnit(detail0);
	unitdet0->bind(textures[0]);
	// *BUG: why LL's EEP viewer needs this while it ruins it for us ?
	//unitdet0->setTextureAddressMode(LLTexUnit::TAM_WRAP);
	unitdet0->activate();

	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader);

	shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_S, 1, tp0.mV);
	shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_T, 1, tp1.mV);

	// Detail texture 1
	S32 detail1 = sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL1);
	LLTexUnit* unitdet1 = gGL.getTexUnit(detail1);
	unitdet1->bind(textures[1]);
	// *BUG: why LL's EEP viewer needs this while it ruins it for us ?
	//unitdet1->setTextureAddressMode(LLTexUnit::TAM_WRAP);
	unitdet1->activate();

	S32 detail2 = sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL2);
	LLTexUnit* unitdet2 = gGL.getTexUnit(detail2);
	unitdet2->bind(textures[2]);
	// *BUG: why LL's EEP viewer needs this while it ruins it for us ?
	//unitdet2->setTextureAddressMode(LLTexUnit::TAM_WRAP);
	unitdet2->activate();

	S32 detail3 = sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL3);
	LLTexUnit* unitdet3 = gGL.getTexUnit(detail3);
	unitdet3->bind(textures[3]);
	// *BUG: why LL's EEP viewer needs this while it ruins it for us ?
	//unitdet3->setTextureAddressMode(LLTexUnit::TAM_WRAP);
	unitdet3->activate();

	S32 alpha_ramp = sShader->enableTexture(LLShaderMgr::TERRAIN_ALPHARAMP);
	LLTexUnit* unitalpha = gGL.getTexUnit(alpha_ramp);
	unitalpha->bind(m2DAlphaRampImagep);
	// *BUG: why LL's EEP viewer needs this while it ruins it for us ?
	//unitalpha->setTextureAddressMode(LLTexUnit::TAM_WRAP);
	unitalpha->activate();

	// GL_BLEND disabled by default
	drawLoop();

	// Disable multitexture
	sShader->disableTexture(LLShaderMgr::TERRAIN_ALPHARAMP);
	sShader->disableTexture(LLShaderMgr::TERRAIN_DETAIL0);
	sShader->disableTexture(LLShaderMgr::TERRAIN_DETAIL1);
	sShader->disableTexture(LLShaderMgr::TERRAIN_DETAIL2);
	sShader->disableTexture(LLShaderMgr::TERRAIN_DETAIL3);

	unitalpha->unbind(LLTexUnit::TT_TEXTURE);
	unitalpha->disable();
	unitalpha->activate();

	unitdet3->unbind(LLTexUnit::TT_TEXTURE);
	unitdet3->disable();
	unitdet3->activate();

	unitdet2->unbind(LLTexUnit::TT_TEXTURE);
	unitdet2->disable();
	unitdet2->activate();

	unitdet1->unbind(LLTexUnit::TT_TEXTURE);
	unitdet1->disable();
	unitdet1->activate();

	// Restore texture unit detail0 defaults
	unitdet0->unbind(LLTexUnit::TT_TEXTURE);
	unitdet0->enable(LLTexUnit::TT_TEXTURE);
	unitdet0->activate();
}

void LLDrawPoolTerrain::renderFullShaderPBR(LLViewerRegion* regionp)
{
	static std::vector<LLGLTFMaterial*> materials;
	constexpr U32 MAT_COUNT = LLVLComposition::ASSET_COUNT;
	static S32 detail_basecolor[MAT_COUNT];
	static S32 detail_normal[MAT_COUNT];
	static S32 detail_mrough[MAT_COUNT];
	static S32 detail_emissive[MAT_COUNT];

	LLVLComposition* compp = regionp->getComposition();
	compp->getGLTFMaterials(materials);

	S32 detail_mode = LLPipeline::sRenderPBRTerrainDetail;

	LLTexUnit* basecolor_unitp = NULL;
	LLTexUnit* normal_unitp = NULL;
	LLTexUnit* mrough_unitp = NULL;
	LLTexUnit* emissive_unitp = NULL;
	for (U32 i = 0; i < MAT_COUNT; ++i)
	{
		LLViewerTexture* basecolor_texp = NULL;
		LLViewerTexture* normal_texp = NULL;
		LLViewerTexture* mrough_texp = NULL;
		LLViewerTexture* emissive_texp = NULL;
		const LLFetchedGLTFMaterial* matp = compp->getFetchedMaterial(i);
		if (matp)
		{
			basecolor_texp = matp->mBaseColorTexture;
			normal_texp = matp->mNormalTexture;
			mrough_texp = matp->mMetallicRoughnessTexture;
			emissive_texp = matp->mEmissiveTexture;
		}
		detail_basecolor[i] =
			sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL0_BASE_COLOR + i);
		basecolor_unitp = gGL.getTexUnit(detail_basecolor[i]);
		if (basecolor_texp)
		{
			basecolor_unitp->bind(basecolor_texp);
		}
		else
		{
			basecolor_unitp->bind(LLViewerFetchedTexture::sWhiteImagep);
		}
		basecolor_unitp->setTextureAddressMode(LLTexUnit::TAM_WRAP);
		basecolor_unitp->activate();

		if (detail_mode >= TERRAIN_PBR_DETAIL_NORMAL)
		{
			detail_normal[i] =
				sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL0_NORMAL + i);
			normal_unitp = gGL.getTexUnit(detail_normal[i]);
			if (normal_texp)
			{
				normal_unitp->bind(normal_texp);
			}
			else
			{
				normal_unitp->bind(LLViewerFetchedTexture::sFlatNormalImagep);
			}
			normal_unitp->setTextureAddressMode(LLTexUnit::TAM_WRAP);
			normal_unitp->activate();
		}

		if (detail_mode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
		{
			detail_mrough[i] =
				sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL0_MROUGHNESS + i);
			mrough_unitp = gGL.getTexUnit(detail_mrough[i]);
			if (mrough_texp)
			{
				mrough_unitp->bind(mrough_texp);
			}
			else
			{
				mrough_unitp->bind(LLViewerFetchedTexture::sWhiteImagep);
			}
			mrough_unitp->setTextureAddressMode(LLTexUnit::TAM_WRAP);
			mrough_unitp->activate();
		}

		if (detail_mode >= TERRAIN_PBR_DETAIL_EMISSIVE)
		{
			detail_emissive[i] =
				sShader->enableTexture(LLShaderMgr::TERRAIN_DETAIL0_EMISSIVE + i);
			emissive_unitp = gGL.getTexUnit(detail_emissive[i]);
			if (emissive_texp)
			{
				emissive_unitp->bind(emissive_texp);
			}
			else
			{
				emissive_unitp->bind(LLViewerFetchedTexture::sWhiteImagep);
			}
			emissive_unitp->setTextureAddressMode(LLTexUnit::TAM_WRAP);
			emissive_unitp->activate();
		}
	}

	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader);

	static LLCachedControl<F32> terrain_scale(gSavedSettings,
											  "RenderTerrainPBRScale");
	F32 scale = 1.f / llmax(1.f, terrain_scale);

	// Like for PBR materials, PBR terrain texture transforms are defined by
	// the KHR_texture_transform spec, but with the following notable
	// differences:
	//	1.- The PBR UV origin is defined as the Southwest corner of the region,
	//		with positive U facing East and positive V facing South.
	//	2.- There is an additional scaling factor RenderTerrainPBRScale. If we
	//		have done our math right, RenderTerrainPBRScale should not affect
	//		the overall behavior of KHR_texture_transform.
	//	3.- There is only one texture transform per material, whereas
	//		KHR_texture_transform supports one texture transform per texture
	//		info. I.e. this is not fully compliant with KHR_texture_transform,
	//		but is compliant when all texture infos used by a material have the
	//		same texture transform.
	static LLGLTFMaterial::TextureTransform::PackTight trans_packed[MAT_COUNT];
	for (U32 i = 0; i < MAT_COUNT; ++i)
	{
		LLGLTFMaterial::TextureTransform transform;
		const LLFetchedGLTFMaterial* matp = compp->getFetchedMaterial(i);
		if (matp)
		{
			transform = matp->mTextureTransform[BASECOLIDX];
		}
		// *Note: here we are combining the scale from RenderTerrainPBRScale
		// into the KHR_texture_transform. This only works if the scale is
		// uniform and no other transforms are applied to the terrain UVs.
		transform.mScale.mV[VX] *= scale;
		transform.mScale.mV[VY] *= scale;
		transform.getPackedTight(trans_packed[i]);
	}
	// If this changes, the shaders needs to be changed as well.
	constexpr U32 transform_vec4_count = 5;
	shader->uniform4fv(LLShaderMgr::TERRAIN_TEXTURE_TRANSFORMS,
					   transform_vec4_count, (F32*)trans_packed);

	// Alpha ramp
	S32 ramp = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);
	LLTexUnit* ramp_unitp = gGL.getTexUnit(ramp);
	ramp_unitp->bind(m2DAlphaRampImagep);
	ramp_unitp->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

	// PBR uniforms

	static LLColor4 base_colors[MAT_COUNT];
	static LLColor3 emissive_colors[MAT_COUNT];
	static F32 metallic_factors[MAT_COUNT];
	static F32 roughness_factors[MAT_COUNT];
	static F32 minimum_alphas[MAT_COUNT];
	for (U32 i = 0; i < MAT_COUNT; ++i)
	{
		const LLGLTFMaterial* matp = materials[i];
		base_colors[i] = matp->mBaseColor;
		if (detail_mode >= TERRAIN_PBR_DETAIL_EMISSIVE)
		{
			emissive_colors[i] = matp->mEmissiveColor;
		}
		if (detail_mode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
		{
			metallic_factors[i] = matp->mMetallicFactor;
			roughness_factors[i] = matp->mRoughnessFactor;
		}
		F32 min_alpha = 0.f;
		if (matp->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
		{
			// Dividing the alpha cutoff by transparency here allows the shader
			// to compare against the alpha value of the texture without
			// needing the transparency value.
			min_alpha = matp->mAlphaCutoff / matp->mBaseColor.mV[3];
		}
		minimum_alphas[i] = min_alpha;
	}
	shader->uniform4fv(LLShaderMgr::TERRAIN_BASE_COLOR_FACTORS, MAT_COUNT,
					   (F32*)base_colors);
	if (detail_mode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
	{
		shader->uniform4f(LLShaderMgr::TERRAIN_METALLIC_FACTORS,
						  metallic_factors[0], metallic_factors[1],
						  metallic_factors[2], metallic_factors[3]);
		shader->uniform4f(LLShaderMgr::TERRAIN_ROUGHNESS_FACTORS,
						  roughness_factors[0], roughness_factors[1],
						  roughness_factors[2], roughness_factors[3]);
	}
	if (detail_mode >= TERRAIN_PBR_DETAIL_EMISSIVE)
	{
		shader->uniform3fv(LLShaderMgr::TERRAIN_EMISSIVE_COLORS, MAT_COUNT,
						   (F32*)emissive_colors);
	}
	shader->uniform4f(LLShaderMgr::TERRAIN_MINIMUM_ALPHAS, minimum_alphas[0],
					  minimum_alphas[1], minimum_alphas[2], minimum_alphas[3]);
	// GL_BLEND disabled by default
	drawLoop();

	// Disable multitexture

	sShader->disableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);
	ramp_unitp->unbind(LLTexUnit::TT_TEXTURE);
	ramp_unitp->disable();
	ramp_unitp->activate();

	for (U32 i = 0; i < MAT_COUNT; ++i)
	{
		sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_BASE_COLOR + i);
		if (detail_mode >= TERRAIN_PBR_DETAIL_NORMAL)
		{
			sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_NORMAL + i);
			normal_unitp = gGL.getTexUnit(detail_normal[i]);
		}
		else
		{
			normal_unitp = NULL;
		}
		if (detail_mode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
		{
			sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_MROUGHNESS + i);
			mrough_unitp = gGL.getTexUnit(detail_mrough[i]);
		}
		else
		{
			mrough_unitp = NULL;
		}
		if (detail_mode >= TERRAIN_PBR_DETAIL_EMISSIVE)
		{
			sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_EMISSIVE + i);
			emissive_unitp = gGL.getTexUnit(detail_emissive[i]);
		}
		else
		{
			emissive_unitp = NULL;
		}

		basecolor_unitp = gGL.getTexUnit(detail_basecolor[i]);
		basecolor_unitp->unbind(LLTexUnit::TT_TEXTURE);
		basecolor_unitp->disable();
		basecolor_unitp->activate();

		if (normal_unitp)
		{
			normal_unitp->unbind(LLTexUnit::TT_TEXTURE);
			normal_unitp->disable();
			normal_unitp->activate();
		}

		if (mrough_unitp)
		{
			mrough_unitp->unbind(LLTexUnit::TT_TEXTURE);
			mrough_unitp->disable();
			mrough_unitp->activate();
		}

		if (emissive_unitp)
		{
			emissive_unitp->unbind(LLTexUnit::TT_TEXTURE);
			emissive_unitp->disable();
			emissive_unitp->activate();
		}
	}
}

void LLDrawPoolTerrain::hilightParcelOwners()
{
	if (gUsePBRShaders || mShaderLevel > 1)
	{
		// Use fullbright shader for highlighting
		LLGLSLShader* old_shaderp = sShader;
		sShader->unbind();
		sShader = gUsePBRShaders ? &gDeferredHighlightProgram
								 : &gHighlightProgram;
		sShader->bind();
		gGL.diffuseColor4f(1.f, 1.f, 1.f, 1.f);
		LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.f, -1.f);
		renderOwnership();
		sShader = old_shaderp;
		sShader->bind();
	}
	else
	{
		gPipeline.disableLights();
		renderOwnership();
	}
}

void LLDrawPoolTerrain::renderFull4TU()
{
//MK
	if (gRLenabled && gRLInterface.mContainsCamTextures)
	{
		renderSimple();
		return;
	}
//mk
	LLViewerRegion* regionp =
		mDrawFace[0]->getDrawable()->getVObj()->getRegion();
	if (!regionp) return;	// Paranoia

	static std::vector<LLViewerTexture*> textures;
	LLVLComposition* compp = regionp->getComposition();
	// *HACK: when the terrain is a PBR one, this will return the base color
	// textures, allowing to render something better than grey terrain, when
	// not in PBR rendering mode. HB
	compp->getTextures(textures);

	const F64 divisor = 1.0 / (F64)sDetailScale;
	LLVector3d region_origin_global = regionp->getOriginGlobal();
	F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], divisor) *
				   sDetailScale;
	F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], divisor) *
				   sDetailScale;

	LLVector4 tp0, tp1;

	tp0.set(sDetailScale, 0.f, 0.f, offset_x);
	tp1.set(0.f, sDetailScale, 0.f, offset_y);

	gGL.blendFunc(LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
				  LLRender::BF_SOURCE_ALPHA);

	//-------------------------------------------------------------------------
	// First pass

	//
	// Stage 0: detail texture 0
	//
	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->activate();
	unit0->bind(textures[0]);

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	// Stage 1: generate alpha ramp for detail0/detail1 transition
	//
	LLTexUnit* unit1 = gGL.getTexUnit(1);
	unit1->bind(m2DAlphaRampImagep.get());
	unit1->enable(LLTexUnit::TT_TEXTURE);
	unit1->activate();

	//
	// Stage 2: Interpolate detail1 with existing based on ramp
	//
	LLTexUnit* unit2 = gGL.getTexUnit(2);
	unit2->bind(textures[1]);
	unit2->enable(LLTexUnit::TT_TEXTURE);
	unit2->activate();

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	//
	// Stage 3: Modulate with primary (vertex) color for lighting
	//
	LLTexUnit* unit3 = gGL.getTexUnit(3);
	unit3->bind(textures[1]);
	unit3->enable(LLTexUnit::TT_TEXTURE);
	unit3->activate();

	unit0->activate();

	// GL_BLEND disabled by default
	drawLoop();

	//-------------------------------------------------------------------------
	// Second pass

	// Stage 0: write detail3 into base
	//
	unit0->activate();
	unit0->bind(textures[3]);

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	//
	// Stage 1: generate alpha ramp for detail2/detail3 transition
	//
	unit1->bind(m2DAlphaRampImagep);
	unit1->enable(LLTexUnit::TT_TEXTURE);
	unit1->activate();

	// Set the texture matrix
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.translatef(-2.f, 0.f, 0.f);

	//
	// Stage 2: Interpolate detail2 with existing based on ramp
	//
	unit2->bind(textures[2]);
	unit2->enable(LLTexUnit::TT_TEXTURE);
	unit2->activate();

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	//
	// Stage 3: generate alpha ramp for detail1/detail2 transition
	//
	unit3->bind(m2DAlphaRampImagep);
	unit3->enable(LLTexUnit::TT_TEXTURE);
	unit3->activate();

	// Set the texture matrix
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.translatef(-1.f, 0.f, 0.f);
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	unit0->activate();
	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}

	LLVertexBuffer::unbind();
	// Disable multitexture
	unit3->unbind(LLTexUnit::TT_TEXTURE);
	unit3->disable();
	unit3->activate();

	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	unit2->unbind(LLTexUnit::TT_TEXTURE);
	unit2->disable();
	unit2->activate();

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	unit1->unbind(LLTexUnit::TT_TEXTURE);
	unit1->disable();
	unit1->activate();

	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	// Restore blend state
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	//-------------------------------------------------------------------------
	// Restore texture unit 0 defaults

	unit0->activate();
	unit0->unbind(LLTexUnit::TT_TEXTURE);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
}

void LLDrawPoolTerrain::renderFull2TU()
{
//MK
	if (gRLenabled && gRLInterface.mContainsCamTextures)
	{
		renderSimple();
		return;
	}
//mk
	LLViewerRegion* regionp =
		mDrawFace[0]->getDrawable()->getVObj()->getRegion();
	if (!regionp) return;	// Paranoia

	static std::vector<LLViewerTexture*> textures;
	LLVLComposition* compp = regionp->getComposition();
	// *HACK: when the terrain is a PBR one, this will return the base color
	// textures, allowing to render something better than grey terrain, when
	// not in PBR rendering mode. HB
	compp->getTextures(textures);

	const F64 divisor = 1.0 / (F64)sDetailScale;
	LLVector3d region_origin_global = regionp->getOriginGlobal();
	F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], divisor) *
				   sDetailScale;
	F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], divisor) *
				   sDetailScale;

	LLVector4 tp0, tp1;

	tp0.set(sDetailScale, 0.f, 0.f, offset_x);
	tp1.set(0.f, sDetailScale, 0.f, offset_y);

	gGL.blendFunc(LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
				  LLRender::BF_SOURCE_ALPHA);

	//-------------------------------------------------------------------------
	// Pass 1/4

	//
	// Stage 0: render detail 0 into base
	//
	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->bind(textures[0]);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	drawLoop();

	//-------------------------------------------------------------------------
	// Pass 2/4

	//
	// Stage 0: generate alpha ramp for detail0/detail1 transition
	//
	unit0->bind(m2DAlphaRampImagep);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	//
	// Stage 1: write detail1
	//
	LLTexUnit* unit1 = gGL.getTexUnit(1);
	unit1->bind(textures[1]);
	unit1->enable(LLTexUnit::TT_TEXTURE);
	unit1->activate();

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	unit0->activate();
	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}
	//-------------------------------------------------------------------------
	// Pass 3/4

	//
	// Stage 0: generate alpha ramp for detail1/detail2 transition
	//
	unit0->bind(m2DAlphaRampImagep);

	// Set the texture matrix
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.translatef(-1.f, 0.f, 0.f);
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	//
	// Stage 1: write detail2
	//
	unit1->bind(textures[2]);
	unit1->enable(LLTexUnit::TT_TEXTURE);
	unit1->activate();

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}

	//-------------------------------------------------------------------------
	// Pass 4/4

	//
	// Stage 0: generate alpha ramp for detail2/detail3 transition
	//
	unit0->activate();
	unit0->bind(m2DAlphaRampImagep);
	// Set the texture matrix
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.translatef(-2.f, 0.f, 0.f);
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	// Stage 1: write detail3
	unit1->bind(textures[3]);
	unit1->enable(LLTexUnit::TT_TEXTURE);
	unit1->activate();

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	unit0->activate();
	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}

	// Restore blend state
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	// Disable multitexture

	unit1->unbind(LLTexUnit::TT_TEXTURE);
	unit1->disable();
	unit1->activate();

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	//-------------------------------------------------------------------------
	// Restore texture unit 0 defaults

	unit0->activate();
	unit0->unbind(LLTexUnit::TT_TEXTURE);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
}

void LLDrawPoolTerrain::renderSimple()
{
	// *Note: due to PBR terrain rendering changes, the shader is not bound
	// any more in beginDeferredPass() for PBR, and renderSimple() can be
	// called in this mode due to RLV restrictions. HB
	if (gUsePBRShaders && LLGLSLShader::sCurBoundShaderPtr != sShader)
	{
		sShader->bind();
	}

	LLVector4 tp0, tp1;
	LLVector3 origin_agent =
		mDrawFace[0]->getDrawable()->getVObj()->getRegion()->getOriginAgent();
	const F32 tscale = 1.f / 256.f;
	tp0.set(tscale, 0.f, 0.f, origin_agent.mV[0] * -tscale);
	tp1.set(0.f, tscale, 0.f, origin_agent.mV[1] * -tscale);

	//-------------------------------------------------------------------------
	// Pass 1/1

	// Stage 0: base terrain texture pass
	if (mTexturep)
	{
		mTexturep->addTextureStats(1024.f * 1024.f);
	}

	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->activate();
	unit0->enable(LLTexUnit::TT_TEXTURE);
//MK
	if (gRLenabled && gRLInterface.mContainsCamTextures &&
		gRLInterface.mCamTexturesCustom.notNull())
	{
		unit0->bind(gRLInterface.mCamTexturesCustom);
	}
	else
//mk
	if (mTexturep)
	{
		unit0->bind(mTexturep);
	}

	sShader->uniform4fv(LLShaderMgr::OBJECT_PLANE_S, 1, tp0.mV);
	sShader->uniform4fv(LLShaderMgr::OBJECT_PLANE_T, 1, tp1.mV);

	drawLoop();

	//-------------------------------------------------------------------------
	// Restore texture unit 0 defaults

	unit0->activate();
	unit0->unbind(LLTexUnit::TT_TEXTURE);

	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
}

void LLDrawPoolTerrain::renderOwnership()
{
	LLGLSPipelineAlpha gls_pipeline_alpha;

	llassert(!mDrawFace.empty());

	// Each terrain pool is associated with a single region. We need to peek
	// back into the viewer's data to find out which ownership overlay texture
	// to use.
	LLFace* facep = mDrawFace[0];
	if (!facep) return;	// Paranoia
	LLDrawable* drawablep = facep->getDrawable();
	if (!drawablep) return;	// Paranoia
	const LLViewerObject* objectp = drawablep->getVObj();
	if (!objectp) return;	// Paranoia
	const LLVOSurfacePatch* vo_surface_patchp = (LLVOSurfacePatch*)objectp;
	LLSurfacePatch* surface_patchp = vo_surface_patchp->getPatch();
	if (!surface_patchp) return;	// Paranoia
	LLSurface* surfacep = surface_patchp->getSurface();
	if (!surfacep) return;	// Paranoia
	LLViewerRegion* regionp = surfacep->getRegion();
	if (!regionp) return;	// Paranoia
	LLViewerParcelOverlay* overlayp = regionp->getParcelOverlay();
	if (!overlayp) return;	// Paranoia
	LLViewerTexture* texturep = overlayp->getTexture();
	if (!texturep) return;	// Paranoia

	gGL.getTexUnit(0)->bind(texturep);

	// *NOTE: because the region is 256 meters wide, but has 257 pixels, the
	// texture coordinates for pixel 256x256 is not 1, 1. This makes the
	// ownership map not line up with the selection. We address this with
	// a texture matrix multiply.
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.pushMatrix();

	constexpr F32 TEXTURE_FUDGE = 257.f / 256.f;
	gGL.scalef(TEXTURE_FUDGE, TEXTURE_FUDGE, 1.f);
	for (U32 i = 0, count = mDrawFace.size(); i < count; ++i)
	{
		LLFace* facep = mDrawFace[i];
		if (facep)	// Paranoia
		{
			// Note: mask is ignored for the PBR renderer
			facep->renderIndexed(LLVertexBuffer::MAP_VERTEX |
								 LLVertexBuffer::MAP_TEXCOORD0);
		}
	}

	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
}

void LLDrawPoolTerrain::dirtyTextures(const LLViewerTextureList::dirty_list_t& textures)
{
	LLViewerFetchedTexture* texp =
		LLViewerTextureManager::staticCast(mTexturep);
	if (texp && textures.count(texp))
	{
		for (U32 i = 0, count = mReferences.size(); i < count; ++i)
		{
			LLFace* facep = mReferences[i];
			if (facep)	// Paranoia
			{
				gPipeline.markTextured(facep->getDrawable());
			}
		}
	}
}

void LLDrawPoolTerrain::boostTerrainDetailTextures()
{
	LLFace* facep = mDrawFace[0];
	if (!facep) return;		// Paranoia 1

	LLDrawable* drawablep =  mDrawFace[0]->getDrawable().get();
	if (!drawablep) return;	// Paranoia 2

	LLViewerObject* objectp = drawablep->getVObj().get();
	if (!objectp) return;	// Paranoia 3

	LLViewerRegion* regionp = objectp->getRegion();
	if (!regionp) return;	// Paranoia 4

	LLVLComposition* compp = regionp->getComposition();
	if (compp)				// Paranoia 5
	{
		compp->boostTextures();
	}
}

// Failed attempt at properly restoring terrain after GL restart with core GL
// profile enabled. HB
#if 0
void LLDrawPoolTerrain::rebuildPatches()
{
	for (U32 i = 0, count = mDrawFace.size(); i < count; ++i)
	{
		LLFace* facep = mDrawFace[i];
		if (!facep) continue;	// Paranoia
		LLDrawable* drawablep = facep->getDrawable();
		if (drawablep)			// Paranoia
		{
			gPipeline.markRebuild(drawablep);
			LLViewerObject* objectp = drawablep->getVObj();
			gPipeline.markGLRebuild(objectp);
			LL_DEBUGS("MarkGLRebuild") << "Marked for GL rebuild: " << std::hex
									   << (intptr_t)objectp << std::dec
									   << LL_ENDL;
		}
	}
}
#endif
