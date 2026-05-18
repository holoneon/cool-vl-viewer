/**
 * @file lldynamictexture.cpp
 * @brief Implementation of LLDynamicTexture class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "lldynamictexture.h"

#include "llgl.h"
#include "llglslshader.h"
#include "llrender.h"
#include "llvertexbuffer.h"

#include "llpipeline.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewerwindow.h"

// static
LLViewerDynamicTexture::instance_list_t
	LLViewerDynamicTexture::sInstances[LLViewerDynamicTexture::ORDER_COUNT];
S32 LLViewerDynamicTexture::sNumRenders = 0;

LLViewerDynamicTexture::LLViewerDynamicTexture(S32 width, S32 height,
											   S32 components, EOrder order,
											   bool clamp)
:	LLViewerTexture(width, height, components, false),
	mClamp(clamp)
{
	llassert(components >= 1 && components <= 4 && order >= 0 &&
			 order < ORDER_COUNT);
	generateGLTexture();
	sInstances[order].insert(this);
}

LLViewerDynamicTexture::~LLViewerDynamicTexture()
{
	for (S32 order = 0; order < ORDER_COUNT; ++order)
	{
		// Will fail in all but one case.
		sInstances[order].erase(this);
	}
}

//virtual
S8 LLViewerDynamicTexture::getType() const
{
	return LLViewerTexture::DYNAMIC_TEXTURE;
}

void LLViewerDynamicTexture::generateGLTexture()
{
	LLViewerTexture::generateGLTexture();
	generateGLTexture(-1, 0, 0, false);
}

void LLViewerDynamicTexture::generateGLTexture(S32 internal_fmt,
											   U32 primary_fmt,
											   U32 type_format,
											   bool swap_bytes)
{
	if (mComponents < 1 || mComponents > 4)
	{
		llerrs << "Bad number of components in dynamic texture: "
			   << mComponents << llendl;
	}

	LLPointer<LLImageRaw> raw_imagep = new LLImageRaw(mFullWidth, mFullHeight,
													  mComponents);
	if (internal_fmt >= 0)
	{
		setExplicitFormat(internal_fmt, primary_fmt, type_format, swap_bytes);
	}
	createGLTexture(0, raw_imagep, 0, true);
	setAddressMode(mClamp ? LLTexUnit::TAM_CLAMP : LLTexUnit::TAM_WRAP);
	mImageGLp->setGLTextureCreated(false);
}

void LLViewerDynamicTexture::preRender(bool clear_depth)
{
	// Using offscreen render target, just use the bottom left corner
	mOrigin.set(0, 0);

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	// Set up camera
	mCamera.setOrigin(gViewerCamera);
	mCamera.setAxes(gViewerCamera);
	mCamera.setAspect(gViewerCamera.getAspect());
	mCamera.setView(gViewerCamera.getView());
	mCamera.setNear(gViewerCamera.getNear());

	glViewport(mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight);
	if (clear_depth)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

void LLViewerDynamicTexture::postRender(bool success)
{
	if (success)
	{
		if (mImageGLp.isNull() || !mImageGLp->getHasGLTexture() ||
			mImageGLp->getDiscardLevel() != 0)
		{
			generateGLTexture();
		}
		mImageGLp->setSubImageFromFrameBuffer(0, 0, mOrigin.mX, mOrigin.mY,
											  mFullWidth, mFullHeight);
	}

	// Restore viewport
	gViewerWindowp->setupViewport();

	// Restore camera
	gViewerCamera.setOrigin(mCamera);
	gViewerCamera.setAxes(mCamera);
	gViewerCamera.setAspect(mCamera.getAspect());
	gViewerCamera.setViewNoBroadcast(mCamera.getView());
	gViewerCamera.setNear(mCamera.getNear());
}

// Calls update on each dynamic texture in order: "first", "middle" then "last"
//static
bool LLViewerDynamicTexture::updateAllInstances()
{
	sNumRenders = 0;
	if (LLGLManager::sIsDisabled)
	{
		return true;
	}

	bool ret = false;

	// This also unbinds the vertex buffer and calls gGL.flush(). HB
	LLGLSLShader::unbind();

	// Render GLTF material previews first, using the deferred screen target
	// buffer. HB
	if (gUsePBRShaders && !sInstances[ORDER_FIRST].empty())
	{
		LLRenderTarget* targetp = &gPipeline.mAuxillaryRT.mDeferredScreen;
		targetp->bindTarget();
		targetp->clear();
		for (instance_list_t::iterator iter = sInstances[ORDER_FIRST].begin(),
									   end = sInstances[ORDER_FIRST].end();
			 iter != end; ++iter)
		{
			LLViewerDynamicTexture* texp = *iter;
			if (texp->needsRender())
			{
				glClear(GL_DEPTH_BUFFER_BIT);
				gGL.color4f(1.f, 1.f, 1.f, 1.f);
				texp->preRender();	// Must be called outside of startRender()
				bool result = texp->render();
				if (result)
				{
					ret = true;
					++sNumRenders;
				}
				gGL.flush();
				LLVertexBuffer::unbind();
				texp->postRender(result);
			}
		}
		targetp->flush();	// Also calls gGL.flush()
	}

	// Render "normal" dynamic textures, without using a target buffer (i.e.
	// render them on screen); this includes small bake preview textures. HB
	for (instance_list_t::iterator iter = sInstances[ORDER_MIDDLE].begin(),
								   end = sInstances[ORDER_MIDDLE].end();
		 iter != end; ++iter)
	{
		LLViewerDynamicTexture* texp = *iter;
		if (texp->needsRender())
		{
			glClear(GL_DEPTH_BUFFER_BIT);
			gGL.color4f(1.f, 1.f, 1.f, 1.f);
			texp->preRender();	// Must be called outside of startRender()
			bool result = texp->render();
			if (result)
			{
				ret = true;
				++sNumRenders;
			}
			gGL.flush();
			LLVertexBuffer::unbind();
			texp->postRender(result);
		}
	}
	gGL.flush();

	// Render 2K (or maybe only 1K in OpenSim) local bakes last, using an
	// adequately sized target buffer. HB
	if (!sInstances[ORDER_LAST].empty())
	{
		LLRenderTarget* targetp = &gPipeline.mBakeBuffer;
		targetp->bindTarget();
		targetp->clear();
		for (instance_list_t::iterator iter = sInstances[ORDER_LAST].begin(),
									   end = sInstances[ORDER_LAST].end();
			 iter != end; ++iter)
		{
			LLViewerDynamicTexture* texp = *iter;
			if (texp->needsRender())
			{
				glClear(GL_DEPTH_BUFFER_BIT);
				gGL.color4f(1.f, 1.f, 1.f, 1.f);
				texp->preRender();	// Must be called outside of startRender()
				bool result = texp->render();
				if (result)
				{
					ret = true;
					++sNumRenders;
				}
				gGL.flush();
				LLVertexBuffer::unbind();
				texp->postRender(result);
			}
		}
		targetp->flush();	// Also calls gGL.flush()
	}

	return ret;
}
