/**
 * @file lldrawpoolwater.cpp
 * @brief LLDrawPoolWater class implementation
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

#include <array>

#include "lldrawpoolwater.h"

#include "imageids.h"
#include "llcubemap.h"
#include "lldir.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llagent.h"
#include "llappviewer.h"			// For gFrameTimeSeconds
#include "lldrawable.h"
#include "llenvironment.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "llpipeline.h"
//MK
#include "mkrlinterface.h"
//mk
#include "llsky.h"
#include "llviewercontrol.h"
#include "llviewerdisplay.h"		// For gCubeSnapshot
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"
#include "llvowater.h"
#include "llwlwaterparammgr.h"
#include "llworld.h"

static F32 sTime = 0.f;

static bool sDeferredRender = false;

///////////////////////////////////////////////////////////////////////////////
// LLDrawPoolWater class
///////////////////////////////////////////////////////////////////////////////

bool LLDrawPoolWater::sNeedsReflectionUpdate = true;
bool LLDrawPoolWater::sNeedsTexturesReload = true;
LLColor4 LLDrawPoolWater::sWaterFogColor = LLColor4(0.2f, 0.5f, 0.5f, 0.f);

LLDrawPoolWater::LLDrawPoolWater()
:	LLFacePool(POOL_WATER)
{
}

void LLDrawPoolWater::setOpaqueTexture(const LLUUID& tex_id)
{
	if (mOpaqueWaterImagep && mOpaqueWaterImagep->getID() == tex_id)
	{
		// Nothing to do !
		return;
	}
	if (tex_id == DEFAULT_WATER_OPAQUE || tex_id.isNull())
	{
		mOpaqueWaterImagep = LLViewerFetchedTexture::sOpaqueWaterImagep;
	}
	else
	{
		mOpaqueWaterImagep = LLViewerTextureManager::getFetchedTexture(tex_id);
		mOpaqueWaterImagep->setNoDelete();
	}
	mOpaqueWaterImagep->addTextureStats(1024.f * 1024.f);
}

void LLDrawPoolWater::setTransparentTextures(const LLUUID& tex1_id,
											 const LLUUID& tex2_id)
{
	if (!mWaterImagep[0] || mWaterImagep[0]->getID() != tex1_id)
	{
		if (tex1_id == DEFAULT_WATER_TEXTURE || tex1_id.isNull())
		{
			mWaterImagep[0] = LLViewerFetchedTexture::sWaterImagep;
		}
		else
		{
			mWaterImagep[0] =
				LLViewerTextureManager::getFetchedTexture(tex1_id);
		}
		mWaterImagep[0]->setNoDelete();
		mWaterImagep[0]->addTextureStats(1024.f * 1024.f);
	}

	if (mWaterImagep[1] && mWaterImagep[1]->getID() == tex2_id)
	{
		// Nothing left to do
		return;
	}
	if (tex2_id.notNull())
	{
		mWaterImagep[1] = LLViewerTextureManager::getFetchedTexture(tex2_id);
	}
	else
	{
		// Use the same texture as the first one...
		mWaterImagep[1] = mWaterImagep[0];
	}
	mWaterImagep[1]->setNoDelete();
	mWaterImagep[1]->addTextureStats(1024.f * 1024.f);
}

void LLDrawPoolWater::setNormalMaps(const LLUUID& tex1_id,
									const LLUUID& tex2_id)
{
	if (!mWaterNormp[0] || mWaterNormp[0]->getID() != tex1_id)
	{
		if (tex1_id == DEFAULT_WATER_NORMAL || tex1_id.isNull())
		{
			mWaterNormp[0] = LLViewerFetchedTexture::sWaterNormapMapImagep;
		}
		else
		{
			mWaterNormp[0] =
				LLViewerTextureManager::getFetchedTexture(tex1_id);
		}
		mWaterNormp[0]->setNoDelete();
		mWaterNormp[0]->addTextureStats(1024.f * 1024.f);
	}

	if (mWaterNormp[1] && mWaterNormp[1]->getID() == tex2_id)
	{
		// Nothing left to do
		return;
	}
	if (tex2_id.notNull())
	{
		mWaterNormp[1] = LLViewerTextureManager::getFetchedTexture(tex2_id);
	}
	else
	{
		// Use the same texture as the first one...
		mWaterNormp[1] = mWaterNormp[0];
	}
	mWaterNormp[1]->setNoDelete();
	mWaterNormp[1]->addTextureStats(1024.f * 1024.f);
}

//virtual
void LLDrawPoolWater::prerender()
{
	mShaderLevel =
		gViewerShaderMgrp->getShaderLevel(LLViewerShaderMgr::SHADER_WATER);

	const LLSettingsWater::ptr_t& waterp = gEnvironment.getCurrentWater();
	if (waterp)
	{
		sWaterFogColor = LLColor4(waterp->getWaterFogColor(), 0.f);
	}

	if (sNeedsTexturesReload)
	{
		sNeedsTexturesReload = false;
		if (waterp)
		{
			setTransparentTextures(waterp->getTransparentTextureID(),
								   waterp->getNextTransparentTextureID());
			setNormalMaps(waterp->getNormalMapID(),
						  waterp->getNextNormalMapID());
			if (!gUsePBRShaders)
			{
				setOpaqueTexture(waterp->getDefaultOpaqueTextureAssetId());
			}
		}
	}

	LLVOSky* voskyp = gSky.mVOSkyp;
	if (!voskyp) return;

	mLightDir = gEnvironment.getLightDirection();
	mLightDir.normalize();
	const LLSettingsSky::ptr_t& skyp = gEnvironment.getCurrentSky();
	if (!skyp) return;	// Paranoia

	if (gPipeline.mIsSunUp)
	{
		mLightDiffuse = voskyp->getSun().getColorCached();
		mLightColor = gPipeline.mSunLightColor;
		if (!gUsePBRShaders)
		{
			// *HACK for SL-18707: fix colours of light reflections on water at
			// sunrise and sunset.
			mLightColor.normalize();
			mLightColor.mV[0] = 5.f;
			mLightColor.mV[1] = 2.f;
		}
	}
	else if (gPipeline.mIsMoonUp)
	{
		mLightDiffuse = skyp->getMoonDiffuse();
		mLightColor = gPipeline.mMoonLightColor;
	}

	if (mLightDiffuse.normalize() > 0.f)
	{
		F32 ground_proj_sq = mLightDir.mV[0] * mLightDir.mV[0] +
							 mLightDir.mV[1] * mLightDir.mV[1];
		mLightDiffuse *= 1.5f + (6.f * ground_proj_sq);
	}
}

// Do not render water above a configurable altitude.
S32 LLDrawPoolWater::getWaterPasses()
{
	static LLCachedControl<U32> max_alt(gSavedSettings,
										"RenderWaterMaxAltitude");
	if (!max_alt)	// Always render when set to 0
	{
		return 1;
	}
	static LLCachedControl<F32> far_clip(gSavedSettings, "RenderFarClip");
	F32 limit = llmax(F32(far_clip), F32(max_alt));
	return gPipeline.mEyeAboveWater <= limit ? 1 : 0;
}

// Only for use by the EE renderer
//virtual
S32 LLDrawPoolWater::getNumPasses()
{
	return gUsePBRShaders ? 0 : getWaterPasses();
}

// Only for use by the EE renderer
//virtual
void LLDrawPoolWater::render(S32)
{
	LL_FAST_TIMER(FTM_RENDER_WATER);

	if (mDrawFace.empty() || LLViewerOctreeEntryData::getCurrentFrame() <= 1)
	{
		return;
	}

	// Do a quick'n dirty depth sort
	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin(),
										end = mDrawFace.end();
		 iter != end; ++iter)
	{
		LLFace* facep = *iter;
		facep->mDistance = -facep->mCenterLocal.mV[2];
	}

	std::sort(mDrawFace.begin(), mDrawFace.end(),
			  LLFace::CompareDistanceGreater());

	if (!LLPipeline::RenderWaterReflectionType
//MK
		|| (gRLenabled && gRLInterface.mContainsCamTextures))
//mk
	{
		// Render water for low end hardware
		renderOpaqueLegacyWater();
		return;
	}

	LLGLEnable blend(GL_BLEND);

	if (mShaderLevel > 0)
	{
		renderWater();
		return;
	}

	LLVOSky* voskyp = gSky.mVOSkyp;
	if (!voskyp)
	{
		return;
	}

	LLFace* refl_facep = voskyp->getReflFace();

	gPipeline.disableLights();

	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLGLDisable cull_face(GL_CULL_FACE);

	// Set up second pass first
	LLTexUnit* unit1 = gGL.getTexUnit(1);
	unit1->activate();
	unit1->enable(LLTexUnit::TT_TEXTURE);
	unit1->bind(mWaterImagep[0]);

	LLTexUnit* unit2 = gGL.getTexUnit(2);
	unit2->activate();
	unit2->enable(LLTexUnit::TT_TEXTURE);
	unit2->bind(mWaterImagep[1]);

	const LLVector3& camera_up = gViewerCamera.getUpAxis();
	F32 up_dot = camera_up * LLVector3::z_axis;

	LLColor4 water_color;
	if (gViewerCamera.cameraUnderWater())
	{
		water_color.set(1.f, 1.f, 1.f, 0.4f);
	}
	else
	{
		water_color.set(1.f, 1.f, 1.f, 0.5f + 0.5f * up_dot);
	}

	gGL.diffuseColor4fv(water_color.mV);

	// Automatically generate texture coords for detail map
	glEnable(GL_TEXTURE_GEN_S); // Texture unit 1
	glEnable(GL_TEXTURE_GEN_T); // Texture unit 1
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	// Slowly move over time.
	static F32 frame_time = 0.f;
	if (!LLPipeline::sFreezeTime)
	{
		frame_time = gFrameTimeSeconds;
	}
	F32 offset = fmod(frame_time * 2.f, 100.f);
	F32 tp0[4] = { 16.f / 256.f, 0.f, 0.f, offset * 0.01f };
	F32 tp1[4] = { 0.f, 16.f / 256.f, 0.f, offset * 0.01f };
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1);

	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->activate();

	glClearStencil(1);
	glClear(GL_STENCIL_BUFFER_BIT);
	glClearStencil(0);
	LLGLEnable gls_stencil(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin(),
										end = mDrawFace.end();
		 iter != end; ++iter)
	{
		LLFace* facep = *iter;
		if (!voskyp->isReflFace(facep))
		{
			LLViewerTexture* texp = facep->getTexture();
			if (texp && texp->hasGLTexture())
			{
				unit0->bind(texp);
				facep->renderIndexed();
			}
		}
	}

	// Now, disable texture coord generation on texture state 1
	unit1->activate();
	unit1->unbind(LLTexUnit::TT_TEXTURE);
	unit1->disable();
	glDisable(GL_TEXTURE_GEN_S); // Texture unit 1
	glDisable(GL_TEXTURE_GEN_T); // Texture unit 1

	unit2->activate();
	unit2->unbind(LLTexUnit::TT_TEXTURE);
	unit2->disable();
	glDisable(GL_TEXTURE_GEN_S); // Texture unit 2
	glDisable(GL_TEXTURE_GEN_T); // Texture unit 2

	// Disable texture coordinate and color arrays
	unit0->activate();
	unit0->unbind(LLTexUnit::TT_TEXTURE);

	if (voskyp->getCubeMap())
	{
		voskyp->getCubeMap()->enableTexture(0);
		voskyp->getCubeMap()->bind();

		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadIdentity();
		LLMatrix4 camera_mat = gViewerCamera.getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();

		gGL.loadMatrix(camera_rot.getF32ptr());

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		LLOverrideFaceColor overrid(this, 1.f, 1.f, 1.f, 0.5f * up_dot);

		for (std::vector<LLFace*>::iterator iter = mDrawFace.begin(),
											end = mDrawFace.end();
			 iter != end; ++iter)
		{
			LLFace* face = *iter;
			if (!voskyp->isReflFace(face) && face->getGeomCount() > 0)
			{
				face->renderIndexed();
			}
		}

		voskyp->getCubeMap()->disableTexture();

		unit0->unbind(LLTexUnit::TT_TEXTURE);
		unit0->enable(LLTexUnit::TT_TEXTURE);
		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	if (refl_facep)
	{
		glStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFF);
		renderReflection(refl_facep);
	}

	stop_glerror();
}

// Only for use by the EE renderer
//virtual
S32 LLDrawPoolWater::getNumDeferredPasses()
{
	return gUsePBRShaders ? 0 : getWaterPasses();
}

// Only for use by the EE renderer
//virtual
void LLDrawPoolWater::renderDeferred(S32 pass)
{
	LL_FAST_TIMER(FTM_RENDER_WATER);

	if (!LLPipeline::RenderWaterReflectionType)
	{
		// Render opaque water without use of ALM
		render(pass);
		return;
	}
	sDeferredRender = true;
	renderWater();
	sDeferredRender = false;
}

// Only for use by the EE renderer
// For low end hardware
void LLDrawPoolWater::renderOpaqueLegacyWater()
{
	LLVOSky* voskyp = gSky.mVOSkyp;
	if (!voskyp)
	{
		return;
	}

	LLGLSLShader* shader = NULL;
	if (LLPipeline::sUnderWaterRender)
	{
		shader = &gObjectSimpleNonIndexedTexGenWaterProgram;
	}
	else
	{
		shader = &gObjectSimpleNonIndexedTexGenProgram;
	}
	shader->bind();

	// Depth sorting and write to depth buffer since this is opaque, we should
	// see nothing behind the water. No blending because of no transparency.
	// And no face culling so that the underside of the water is also opaque.
	LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
	LLGLDisable no_cull(GL_CULL_FACE);
	LLGLDisable no_blend(GL_BLEND);

	gPipeline.disableLights();

	// Activate the texture binding and bind one texture since all images will
	// have the same texture
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
	{
		unit0->bind(mOpaqueWaterImagep);
	}

	// Automatically generate texture coords for water texture
	if (!shader)
	{
		glEnable(GL_TEXTURE_GEN_S); // Texture unit 0
		glEnable(GL_TEXTURE_GEN_T); // Texture unit 0
		glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	}

	// Use the fact that we know all water faces are the same size to save some
	// computation

	// Slowly move texture coordinates over time so the watter appears to be
	// moving.
	F32 movement_period_secs = 50.f;
	// Slowly move over time.
	static F32 frame_time = 0.f;
	if (!LLPipeline::sFreezeTime)
	{
		frame_time = gFrameTimeSeconds;
	}
	F32 offset = fmod(frame_time, movement_period_secs);

	if (movement_period_secs != 0)
	{
	 	offset /= movement_period_secs;
	}
	else
	{
		offset = 0;
	}

	F32 tp0[4] = { 16.f / 256.f, 0.f, 0.f, offset };
	F32 tp1[4] = { 0.f, 16.f / 256.f, 0.f, offset };

	if (!shader)
	{
		glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0);
		glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1);
	}
	else
	{
		shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_S, 1, tp0);
		shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_T, 1, tp1);
	}

	gGL.diffuseColor3f(1.f, 1.f, 1.f);

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin(),
										end = mDrawFace.end();
		 iter != end; ++iter)
	{
		LLFace* face = *iter;
		if (!voskyp->isReflFace(face))
		{
			face->renderIndexed();
		}
	}

	if (!shader)
	{
		// Reset the settings back to expected values
		glDisable(GL_TEXTURE_GEN_S); // Texture unit 0
		glDisable(GL_TEXTURE_GEN_T); // Texture unit 0
	}

	unit0->unbind(LLTexUnit::TT_TEXTURE);

	stop_glerror();
}

// Only for use by the EE renderer
void LLDrawPoolWater::renderReflection(LLFace* facep)
{
	LLVOSky* voskyp = gSky.mVOSkyp;
	if (!voskyp || !facep->getGeomCount())
	{
		return;
	}

	S8 dr = voskyp->getDrawRefl();
	if (dr < 0)
	{
		return;
	}

	gGL.getTexUnit(0)->bind(dr == 0 ? voskyp->getSunTex()
									: voskyp->getMoonTex());

	LLOverrideFaceColor override_color(this, facep->getFaceColor().mV);
	facep->renderIndexed();
}

// Only for use by the EE renderer
void LLDrawPoolWater::renderWater()
{
	LLVOSky* voskyp = gSky.mVOSkyp;
	if (!voskyp)
	{
		return;
	}

	static LLCachedControl<bool> mip_normal(gSavedSettings,
											"RenderWaterMipNormal");
	LLTexUnit::eTextureFilterOptions mode =
		mip_normal ? LLTexUnit::TFO_ANISOTROPIC : LLTexUnit::TFO_POINT;
	if (mWaterNormp[0])
	{
		mWaterNormp[0]->setFilteringOption(mode);
	}
	if (mWaterNormp[1])
	{
		mWaterNormp[1]->setFilteringOption(mode);
	}

	if (!sDeferredRender)
	{
		gGL.setColorMask(true, true);
	}

	LLGLDisable blend(GL_BLEND);

	LLGLSLShader* shaderp;
    LLGLSLShader* edge_shaderp = NULL;
	if (gPipeline.mEyeAboveWater < 0.f)
	{
		if (sDeferredRender)
		{
			shaderp = &gDeferredUnderWaterProgram;
		}
		else
		{
			shaderp = &gUnderWaterProgram;
		}
	}
	else if (sDeferredRender)
	{
		shaderp = &gDeferredWaterProgram;
	}
	else
	{
		shaderp = &gWaterProgram;
		edge_shaderp = &gWaterEdgeProgram;
	}

	const LLSettingsWater::ptr_t& swaterp = gEnvironment.getCurrentWater();
	const LLSettingsWater* waterp = swaterp.get();
	shadeWater(waterp, shaderp, false);
	shadeWater(waterp, edge_shaderp ? edge_shaderp : shaderp, true);

	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->activate();
	unit0->enable(LLTexUnit::TT_TEXTURE);
	if (!sDeferredRender)
	{
		gGL.setColorMask(true, false);
	}

	stop_glerror();
}

// Only for use by the EE renderer
void LLDrawPoolWater::shadeWater(const LLSettingsWater* waterp,
								 LLGLSLShader* shaderp, bool edge)
{
	if (!waterp) return;	// Paranoia

	shaderp->bind();

	if (sDeferredRender &&
		shaderp->getUniformLocation(LLShaderMgr::DEFERRED_NORM_MATRIX) >= 0)
	{
		LLMatrix4a norm_mat = gGLModelView;
		norm_mat.invert();
		norm_mat.transpose();
		shaderp->uniformMatrix4fv(LLShaderMgr::DEFERRED_NORM_MATRIX, 1,
								  GL_FALSE, norm_mat.getF32ptr());
	}

	shaderp->uniform4fv(LLShaderMgr::SPECULAR_COLOR, 1, mLightColor.mV);

	if (!LLPipeline::sFreezeTime)
	{
		sTime = (F32)LLFrameTimer::getElapsedSeconds() * 0.5f;
	}

	S32 reftex = shaderp->enableTexture(LLShaderMgr::WATER_REFTEX);
	if (reftex > -1)
	{
		LLTexUnit* unit = gGL.getTexUnit(reftex);
		unit->activate();
		unit->bind(&gPipeline.mWaterRef);
		gGL.getTexUnit(0)->activate();
	}

	// Bind normal map
	S32 bump_tex = shaderp->enableTexture(LLShaderMgr::BUMP_MAP);
	LLTexUnit* unitbump = gGL.getTexUnit(bump_tex);
	unitbump->unbind(LLTexUnit::TT_TEXTURE);
	S32 bump_tex2 = shaderp->enableTexture(LLShaderMgr::BUMP_MAP2);
	LLTexUnit* unitbump2 = bump_tex2 > -1 ? gGL.getTexUnit(bump_tex2) : NULL;
	if (unitbump2)
	{
		unitbump2->unbind(LLTexUnit::TT_TEXTURE);
	}

	LLViewerTexture* tex_a = mWaterNormp[0];
	LLViewerTexture* tex_b = mWaterNormp[1];

	F32 blend_factor = waterp->getBlendFactor();
	if (tex_a && (!tex_b || tex_a == tex_b))
	{
		unitbump->bind(tex_a);
		blend_factor = 0.f;	// Only one tex provided, no blending
	}
	else if (tex_b && !tex_a)
	{
		unitbump->bind(tex_b);
		blend_factor = 0.f;	// Only one tex provided, no blending
	}
	else if (tex_a != tex_b)
	{
		unitbump->bind(tex_a);
		if (unitbump2)
		{
			unitbump2->bind(tex_b);
		}
	}

	// Bind reflection texture from render target
	S32 screentex = shaderp->enableTexture(LLShaderMgr::WATER_SCREENTEX);
	// NOTE: there is actually no such uniform in the current water shaders, so
	// diff_tex is set to -1...
	S32 diff_tex = shaderp->enableTexture(LLShaderMgr::DIFFUSE_MAP);

	// Set uniforms for water rendering
	F32 screen_res[] =
	{
		1.f / gGLViewport[2],
		1.f / gGLViewport[3]
	};
	shaderp->uniform2fv(LLShaderMgr::DEFERRED_SCREEN_RES, 1, screen_res);
	shaderp->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);

	LLColor4 fog_color = sWaterFogColor;
	F32 eye_level = gPipeline.mEyeAboveWater;
	F32 fog_density = waterp->getModifiedWaterFogDensity(eye_level < 0.f);
	if (screentex > -1)
	{
		shaderp->uniform1f(LLShaderMgr::WATER_FOGDENSITY, fog_density);
		gGL.getTexUnit(screentex)->bind(&gPipeline.mWaterDis);
	}
	if (mShaderLevel == 1)
	{
		fog_color.mV[VW] = logf(fog_density) / F_LN2;
	}
	shaderp->uniform4fv(LLShaderMgr::WATER_FOGCOLOR, 1, fog_color.mV);

	shaderp->uniform1f(LLShaderMgr::WATER_WATERHEIGHT, eye_level);
	shaderp->uniform1f(LLShaderMgr::WATER_TIME, sTime);
	const LLVector3& camera_origin = gViewerCamera.getOrigin();
	shaderp->uniform3fv(LLShaderMgr::WATER_EYEVEC, 1, camera_origin.mV);
	shaderp->uniform3fv(LLShaderMgr::WATER_SPECULAR, 1, mLightDiffuse.mV);
	shaderp->uniform2fv(LLShaderMgr::WATER_WAVE_DIR1, 1,
						waterp->getWave1Dir().mV);
	shaderp->uniform2fv(LLShaderMgr::WATER_WAVE_DIR2, 1,
						waterp->getWave2Dir().mV);
	shaderp->uniform3fv(LLShaderMgr::WATER_LIGHT_DIR, 1, mLightDir.mV);

	shaderp->uniform3fv(LLShaderMgr::WATER_NORM_SCALE, 1,
						waterp->getNormalScale().mV);
	shaderp->uniform1f(LLShaderMgr::WATER_FRESNEL_SCALE,
						waterp->getFresnelScale());
	shaderp->uniform1f(LLShaderMgr::WATER_FRESNEL_OFFSET,
						waterp->getFresnelOffset());
	shaderp->uniform1f(LLShaderMgr::WATER_BLUR_MULTIPLIER,
						waterp->getBlurMultiplier());

	F32 sun_angle = llmax(0.f, mLightDir.mV[1]);
	shaderp->uniform1f(LLShaderMgr::WATER_SUN_ANGLE, 0.1f + 0.2f * sun_angle);
	shaderp->uniform1i(LLShaderMgr::SUN_UP_FACTOR, gPipeline.mIsSunUp ? 1 : 0);

	shaderp->uniform4fv(LLShaderMgr::LIGHTNORM, 1,
						gEnvironment.getClampedLightNorm().mV);
	shaderp->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1, camera_origin.mV);

	if (eye_level < 0.f)
	{
		shaderp->uniform1f(LLShaderMgr::WATER_REFSCALE,
						   waterp->getScaleBelow());
	}
	else
	{
		shaderp->uniform1f(LLShaderMgr::WATER_REFSCALE,
						   waterp->getScaleAbove());
	}

	LLGLDisable cullface(GL_CULL_FACE);

	LLTexUnit* unitp = diff_tex > -1 ? gGL.getTexUnit(diff_tex) : NULL;
	for (U32 i = 0, count = mDrawFace.size(); i < count; ++i)
	{
		LLFace* facep = mDrawFace[i];
		if (!facep) continue;

		LLVOWater* vowaterp = (LLVOWater*)facep->getViewerObject();
		if (!vowaterp) continue;

		if (unitp)
		{
			unitp->bind(facep->getTexture());
		}

		bool edge_patch = vowaterp->getIsEdgePatch();
		if (edge)
		{
			if (edge_patch)
			{
				facep->renderIndexed();
			}
		}
		else if (!edge_patch)
		{
			sNeedsReflectionUpdate = true;
			facep->renderIndexed();
		}
	}

	unitbump->unbind(LLTexUnit::TT_TEXTURE);
	if (unitbump2)
	{
		unitbump2->unbind(LLTexUnit::TT_TEXTURE);
	}

	shaderp->disableTexture(LLShaderMgr::ENVIRONMENT_MAP,
							LLTexUnit::TT_CUBE_MAP);
	shaderp->disableTexture(LLShaderMgr::WATER_SCREENTEX);
	shaderp->disableTexture(LLShaderMgr::BUMP_MAP);
	shaderp->disableTexture(LLShaderMgr::DIFFUSE_MAP);
	shaderp->disableTexture(LLShaderMgr::WATER_REFTEX);

	shaderp->unbind();

	stop_glerror();
}

//virtual
S32 LLDrawPoolWater::getNumPostDeferredPasses()
{
	return gUsePBRShaders ? getWaterPasses() : 0;
}

// Only for use by the PBR renderer
//virtual
void LLDrawPoolWater::renderPostDeferred(S32)
{
	LL_FAST_TIMER(FTM_RENDER_WATER);

	static LLCachedControl<bool> mip_normal(gSavedSettings,
											"RenderWaterMipNormal");
	LLTexUnit::eTextureFilterOptions mode =
		mip_normal ? LLTexUnit::TFO_ANISOTROPIC : LLTexUnit::TFO_POINT;
	if (mWaterNormp[0])
	{
		mWaterNormp[0]->setFilteringOption(mode);
	}
	if (mWaterNormp[1])
	{
		mWaterNormp[1]->setFilteringOption(mode);
	}

	LLGLDisable blend(GL_BLEND);
	gGL.setColorMask(true, true);

	if (!LLPipeline::sFreezeTime)
	{
		sTime = (F32)LLFrameTimer::getElapsedSeconds() * 0.5f;
	}

	const LLSettingsWater::ptr_t& swaterp = gEnvironment.getCurrentWater();
	const LLSettingsWater* waterp = swaterp.get();
	for (U32 edge = 0; edge < 2; ++edge)
	{
		LLGLSLShader* shaderp;
		if (gPipeline.mEyeAboveWater < 0.f)
		{
			shaderp = &gUnderWaterProgram;
		}
		// Also use the water edge program (i.e. with opaque water) for the
		// reflection probe render pass, else the water appears too bright
		// in mirrors and reflections. HB
		else if (edge || gCubeSnapshot)
		{
			shaderp = &gWaterEdgeProgram;
		}
		else
		{
			shaderp = &gWaterProgram;
		}
		shadeWaterPBR(waterp, shaderp, edge);
	}

	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->activate();
	unit0->enable(LLTexUnit::TT_TEXTURE);

	gGL.setColorMask(true, false);
}

// Only for use by the PBR renderer
void LLDrawPoolWater::shadeWaterPBR(const LLSettingsWater* waterp,
									LLGLSLShader* shaderp, bool edge)
{
	if (!waterp) return;	// Paranoia

	gPipeline.bindDeferredShader(*shaderp, NULL, &gPipeline.mWaterDis);

	// Bind normal map

	S32 bump_tex = shaderp->enableTexture(LLShaderMgr::BUMP_MAP);
	LLTexUnit* unitbump = gGL.getTexUnit(bump_tex);
	unitbump->unbind(LLTexUnit::TT_TEXTURE);

	S32 bump_tex2 = shaderp->enableTexture(LLShaderMgr::BUMP_MAP2);
	LLTexUnit* unitbump2 = bump_tex2 > -1 ? gGL.getTexUnit(bump_tex2) : NULL;
	if (unitbump2)
	{
		unitbump2->unbind(LLTexUnit::TT_TEXTURE);
	}

	LLViewerTexture* tex_a = mWaterNormp[0];
	LLViewerTexture* tex_b = mWaterNormp[1];

	F32 blend_factor = waterp->getBlendFactor();
	if (tex_a && (!tex_b || tex_a == tex_b))
	{
		unitbump->bind(tex_a);
		blend_factor = 0.f;	// Only one tex provided, no blending
	}
	else if (tex_b && !tex_a)
	{
		unitbump->bind(tex_b);
		blend_factor = 0.f;	// Only one tex provided, no blending
	}
	else if (tex_a != tex_b)
	{
		unitbump->bind(tex_a);
		if (unitbump2)
		{
			unitbump2->bind(tex_b);
		}
	}

	shaderp->bindTexture(LLShaderMgr::WATER_EXCLUSIONTEX,
						 &gPipeline.mWaterExclusionMask);

	// Bind reflection texture from render target
	S32 screentex = shaderp->enableTexture(LLShaderMgr::WATER_SCREENTEX);

	// Set uniforms for water rendering
	F32 screen_res[] =
	{
		1.f / gGLViewport[2],
		1.f / gGLViewport[3]
	};
	shaderp->uniform2fv(LLShaderMgr::DEFERRED_SCREEN_RES, 1, screen_res);
	shaderp->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);

	LLColor4 fog_color = sWaterFogColor;
	F32 eye_level = gPipeline.mEyeAboveWater;
	F32 fog_density = waterp->getModifiedWaterFogDensity(eye_level < 0.f);
	if (screentex > -1)
	{
		shaderp->uniform1f(LLShaderMgr::WATER_FOGDENSITY, fog_density);
		gGL.getTexUnit(screentex)->bind(&gPipeline.mWaterDis);
	}
	if (mShaderLevel == 1)
	{
		fog_color.mV[VW] = logf(fog_density) / F_LN2;
	}

	shaderp->uniform1f(LLShaderMgr::WATER_WATERHEIGHT, eye_level);
	shaderp->uniform1f(LLShaderMgr::WATER_TIME, sTime);

	const LLVector3& camera_origin = gViewerCamera.getOrigin();
	shaderp->uniform3fv(LLShaderMgr::WATER_EYEVEC, 1, camera_origin.mV);

	shaderp->uniform4fv(LLShaderMgr::SPECULAR_COLOR, 1, mLightColor.mV);
	shaderp->uniform4fv(LLShaderMgr::WATER_FOGCOLOR, 1, fog_color.mV);
	shaderp->uniform3fv(LLShaderMgr::WATER_FOGCOLOR_LINEAR, 1,
						linearColor3(fog_color).mV);

	shaderp->uniform3fv(LLShaderMgr::WATER_SPECULAR, 1, mLightDiffuse.mV);

	shaderp->uniform2fv(LLShaderMgr::WATER_WAVE_DIR1, 1,
						waterp->getWave1Dir().mV);
	shaderp->uniform2fv(LLShaderMgr::WATER_WAVE_DIR2, 1,
						waterp->getWave2Dir().mV);
	shaderp->uniform3fv(LLShaderMgr::WATER_LIGHT_DIR, 1, mLightDir.mV);

	shaderp->uniform3fv(LLShaderMgr::WATER_NORM_SCALE, 1,
						waterp->getNormalScale().mV);
	shaderp->uniform1f(LLShaderMgr::WATER_FRESNEL_SCALE,
						waterp->getFresnelScale());
	shaderp->uniform1f(LLShaderMgr::WATER_FRESNEL_OFFSET,
						waterp->getFresnelOffset());
	shaderp->uniform1f(LLShaderMgr::WATER_BLUR_MULTIPLIER,
						waterp->getBlurMultiplier());

	shaderp->uniform1i(LLShaderMgr::SUN_UP_FACTOR, gPipeline.mIsSunUp ? 1 : 0);
#if 0	// No more in actual use in current PBR shaders. HB
	F32 sun_angle = llmax(0.f, mLightDir.mV[1]);
	shaderp->uniform1f(LLShaderMgr::WATER_SUN_ANGLE, 0.1f + 0.2f * sun_angle);
	shaderp->uniform1f(LLShaderMgr::WATER_SCALED_ANGLE, 1.f - sun_angle);
#endif

	shaderp->uniform3fv(LLShaderMgr::LIGHTNORM, 1,
						gEnvironment.getClampedLightNorm().mV);
	shaderp->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1, camera_origin.mV);

	if (gPipeline.mEyeAboveWater < 0.f)
	{
		shaderp->uniform1f(LLShaderMgr::WATER_REFSCALE,
						   waterp->getScaleBelow());
	}
	else
	{
		shaderp->uniform1f(LLShaderMgr::WATER_REFSCALE,
						   waterp->getScaleAbove());
	}

	LLGLDisable cullface(GL_CULL_FACE);

	pushWaterPlanes(edge);

	shaderp->disableTexture(LLShaderMgr::ENVIRONMENT_MAP,
							LLTexUnit::TT_CUBE_MAP);
	shaderp->disableTexture(LLShaderMgr::WATER_SCREENTEX);
	shaderp->disableTexture(LLShaderMgr::BUMP_MAP);
	shaderp->disableTexture(LLShaderMgr::WATER_REFTEX);

	gPipeline.unbindDeferredShader(*shaderp);

	unitbump->unbind(LLTexUnit::TT_TEXTURE);
	if (unitbump2)
	{
		unitbump2->unbind(LLTexUnit::TT_TEXTURE);
	}

	stop_glerror();
}

// Only for use by the PBR renderer
void LLDrawPoolWater::pushWaterPlanes(bool edge)
{
	for (U32 i = 0, count = mDrawFace.size(); i < count; ++i)
	{
		LLFace* facep = mDrawFace[i];
		if (!facep) continue;

		LLVOWater* vowaterp = (LLVOWater*)facep->getViewerObject();
		if (!vowaterp) continue;

		bool edge_patch = vowaterp->getIsEdgePatch();
		if (edge)
		{
			if (edge_patch)
			{
				facep->renderIndexed();
			}
		}
		else if (!edge_patch)
		{
			sNeedsReflectionUpdate = true;
			facep->renderIndexed();
		}
	}
}

// Only for use by the PBR renderer
void LLDrawPoolWater::renderSSR()
{
	if (mDrawFace.empty())
	{
		return;
	}

	const LLSettingsWater::ptr_t& waterp = gEnvironment.getCurrentWater();
	if (!waterp) return;	// Paranoia

	static LLCachedControl<bool> mip_normal(gSavedSettings,
											"RenderWaterMipNormal");
	LLTexUnit::eTextureFilterOptions filter_mode =
		mip_normal ? LLTexUnit::TFO_ANISOTROPIC : LLTexUnit::TFO_POINT;

	gPipeline.bindDeferredShader(gSSRWaterProgram);

	F32 blend_factor = (F32)waterp->getBlendFactor();
	LLViewerTexture* tex_a = mWaterNormp[0];
	LLViewerTexture* tex_b = mWaterNormp[1];
	if (tex_a && (!tex_b || (tex_a == tex_b)))
	{
		gSSRWaterProgram.bindTexture(LLViewerShaderMgr::BUMP_MAP, tex_a);
		tex_a->setFilteringOption(filter_mode);
		blend_factor = 0.f;
	}
	else if (tex_b && !tex_a)
	{
		gSSRWaterProgram.bindTexture(LLViewerShaderMgr::BUMP_MAP, tex_b);
		tex_b->setFilteringOption(filter_mode);
		blend_factor = 0.f;
	}
	else if (tex_b != tex_a)
	{
		gSSRWaterProgram.bindTexture(LLViewerShaderMgr::BUMP_MAP, tex_a);
		tex_a->setFilteringOption(filter_mode);
		gSSRWaterProgram.bindTexture(LLViewerShaderMgr::BUMP_MAP2, tex_b);
		tex_b->setFilteringOption(filter_mode);
	}

	gSSRWaterProgram.uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);
	gSSRWaterProgram.uniform1f(LLShaderMgr::WATER_WATERHEIGHT,
							   gPipeline.mEyeAboveWater);
	gSSRWaterProgram.uniform1f(LLShaderMgr::WATER_TIME,
							   gFrameTimeSeconds * 0.5f);

	const LLVector3& camera_origin = gViewerCamera.getOrigin();
	gSSRWaterProgram.uniform3fv(LLShaderMgr::WATER_EYEVEC, 1,
								camera_origin.mV);
	gSSRWaterProgram.uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1,
								camera_origin.mV);

	gSSRWaterProgram.uniform2fv(LLShaderMgr::WATER_WAVE_DIR1, 1,
								waterp->getWave1Dir().mV);
	gSSRWaterProgram.uniform2fv(LLShaderMgr::WATER_WAVE_DIR2, 1,
								waterp->getWave2Dir().mV);

	LLVector3 light_dir = gEnvironment.getLightDirection();
	light_dir.normalize();
	gSSRWaterProgram.uniform3fv(LLShaderMgr::WATER_LIGHT_DIR, 1, light_dir.mV);
	gSSRWaterProgram.uniform3fv(LLShaderMgr::WATER_NORM_SCALE, 1,
								waterp->getNormalScale().mV);

	static LLCachedControl<LLVector3> bmlim(gSavedSettings,
											"RenderSSRBlurMultiplierLimits");
	F32 bm_min = llclamp(bmlim().mV[0], 0.f, 0.5f);
	F32 bm_max = llclamp(bmlim().mV[1], bm_min, 0.5f);
	F32 blur_mult = llclamp(waterp->getBlurMultiplier(), bm_min, bm_max) *
					llclamp(bmlim().mV[2], 0.f, 4.f);
	gSSRWaterProgram.uniform1f(LLShaderMgr::WATER_BLUR_MULTIPLIER,
							   llmin(1.f, blur_mult));

	gSSRWaterProgram.uniform3fv(LLViewerShaderMgr::LIGHTNORM, 1,
								gEnvironment.getClampedLightNorm().mV);

	LLGLDisable cullface(GL_CULL_FACE);

	pushWaterPlanes(false);

	gPipeline.unbindDeferredShader(gSSRWaterProgram);
}

///////////////////////////////////////////////////////////////////////////////
// LLDrawPoolWaterExclusion class (only used in PBR mode)
///////////////////////////////////////////////////////////////////////////////

LLDrawPoolWaterExclusion::LLDrawPoolWaterExclusion()
:	LLRenderPass(LLDrawPool::POOL_WATEREXCLUSION)
{
}

//virtual
void LLDrawPoolWaterExclusion::render(S32 pass)
{
	LL_TRACY_TIMER(TRC_RENDER_WATER_EXCLUSION);

	if (!gPipeline.shadersLoaded())
	{
		return;
	}

	gDrawColorProgram.bind();

	LLGLDepthTest depth(GL_TRUE);
	gDrawColorProgram.uniform4f(LLShaderMgr::DIFFUSE_COLOR,
								1.f, 1.f, 1.f, 1.f);

	LLDrawPoolWater* poolp =
		(LLDrawPoolWater*)gPipeline.getPool(LLDrawPool::POOL_WATER);
	if (poolp)
	{
		// Just treat our water planes as double sided for the purposes of
		// generating the exclusion mask.
		LLGLDisable cullface(GL_CULL_FACE);

		poolp->pushWaterPlanes(false);
		// Take care of the edge water tiles.
		poolp->pushWaterPlanes(true);
	}

	gDrawColorProgram.uniform4f(LLShaderMgr::DIFFUSE_COLOR,
								0.f, 0.f, 0.f, 1.f);

	static LLStaticHashedString water_sign("waterSign");
	gDrawColorProgram.uniform1f(water_sign, 1.f);
	pushBatches(LLRenderPass::PASS_INVISIBLE, false, false);

	gDrawColorProgram.unbind();
}
