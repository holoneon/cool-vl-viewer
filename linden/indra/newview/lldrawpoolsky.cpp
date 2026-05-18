/**
 * @file lldrawpoolsky.cpp
 * @brief LLDrawPoolSky class implementation
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

#include "lldrawpoolsky.h"

#include "lldrawable.h"
#include "llface.h"
#include "llpipeline.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewershadermgr.h"
#include "llviewertexture.h"
#include "llvosky.h"

LLDrawPoolSky::LLDrawPoolSky()
:	LLFacePool(POOL_SKY),
	mSkyTex(NULL)
{
}

//virtual
void LLDrawPoolSky::prerender()
{
	mShaderLevel =
		gViewerShaderMgrp->getShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT);
	if (!gUsePBRShaders)
	{
		LLDrawable* drawablep = gSky.mVOSkyp->mDrawable;
		if (drawablep)
		{
			gSky.mVOSkyp->updateGeometry(drawablep);
		}
	}
}

//virtual
void LLDrawPoolSky::render(S32)
{
	if (mDrawFace.empty() ||
		// Do not draw the sky box if we can and are rendering the WL sky dome.
		gPipeline.canUseWindLightShaders() ||
		// Do not render sky under water (background just gets cleared to fog
		// color).
		(mShaderLevel > 0 && LLPipeline::sUnderWaterRender))
	{
		return;
	}

	gGL.flush();

	// Just use the UI shader (generic single texture no lighting)
	gOneTextureNoColorProgram.bind();

	LLVector3 origin = gViewerCamera.getOrigin();
	U32 face_count = mDrawFace.size();

	LLGLSPipelineDepthTestSkyBox gls_skybox(GL_TRUE, GL_FALSE);

	gGL.pushMatrix();
	gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);

	LLVertexBuffer::unbind();
	gGL.diffuseColor4f(1.f, 1.f, 1.f, 1.f);

	for (U32 i = 0; i < face_count; ++i)
	{
		renderSkyFace(i);
	}

	gGL.popMatrix();
}

void LLDrawPoolSky::renderSkyFace(U8 index)
{
	LLFace* facep = mDrawFace[index];
	if (!facep || !facep->getGeomCount())
	{
		return;
	}

	if (index < LLVOSky::FACE_SUN)			// Sky texture, interpolate
	{
		mSkyTex[index].bindTexture(true);	// Bind the current texture
		facep->renderIndexed();
	}
	else if (index == LLVOSky::FACE_MOON)	// Moon
	{
		// SL-14113: write depth for Moon so stars can test if behind it
		LLGLSPipelineDepthTestSkyBox gls_skybox(GL_TRUE, GL_TRUE);
		
		LLGLEnable blend(GL_BLEND);

		LLViewerTexture* texp = facep->getTexture(LLRender::DIFFUSE_MAP);
		if (texp)
		{
			gMoonProgram.bind(); // SL-14113 was gOneTextureNoColorProgram
			gGL.getTexUnit(0)->bind(texp);
			facep->renderIndexed();
		}
	}
	else									// Heavenly body faces, no interp.
	{
		// Reset to previous
		LLGLSPipelineDepthTestSkyBox gls_skybox(GL_TRUE, GL_FALSE);

		LLGLEnable blend(GL_BLEND);

		LLViewerTexture* texp = facep->getTexture(LLRender::DIFFUSE_MAP);
		if (texp)
		{
			gOneTextureNoColorProgram.bind();
			gGL.getTexUnit(0)->bind(texp);
			facep->renderIndexed();
		}
	}
}
