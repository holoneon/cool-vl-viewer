/**
 * @file llvisualparamhint.cpp
 * @brief A dynamic texture class for displaying avatar visual params effects
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

// Note: probably because of obscure pre-historical reasons, this file is
// named "lltoolmorph.cpp" in LL's viewer sources. I renamed it based on the
// class it implements instead. HB

#include "llviewerprecompiledheaders.h"

#include "llvisualparamhint.h"

#include "llrender.h"
#include "llwearable.h"

#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
#include "llmorphview.h"
#include "llpipeline.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"
#include "llvoavatarself.h"

//static
LLVisualParamHint::instance_list_t LLVisualParamHint::sInstances;
bool LLVisualParamReset::sDirty = false;

//-----------------------------------------------------------------------------
// LLVisualParamHint() class
//-----------------------------------------------------------------------------

// static
LLVisualParamHint::LLVisualParamHint(S32 pos_x, S32 pos_y,
									 S32 width, S32 height,
									 LLViewerJointMesh* meshp,
									 LLViewerVisualParam* paramp,
									 LLWearable* wearablep,
									 F32 param_weight,
									 LLJoint* jointp)
:	LLViewerDynamicTexture(width, height, 3, ORDER_MIDDLE, true),
	mNeedsUpdate(true),
	mAllowsUpdates(true),
	mIsVisible(false),
	mJointMesh(meshp),
	mVisualParam(paramp),
	mWearablePtr(wearablep),
	mVisualParamWeight(param_weight),
	mDelayFrames(0),
	mRect(pos_x, pos_y + height, pos_x + width, pos_y),
	mLastParamWeight(0.f),
	mCamTargetJoint(jointp)
{
	LLVisualParamHint::sInstances.insert(this);
	mBackgroundp = LLUI::getUIImage("avatar_thumb_bkgrnd.j2c");

	if (!mCamTargetJoint)
	{
		llwarns << "Missing camera target joint !" << llendl;
	}

	llassert(mCamTargetJoint);
	llassert(width != 0);
	llassert(height != 0);
}

LLVisualParamHint::~LLVisualParamHint()
{
	LLVisualParamHint::sInstances.erase(this);
}

//virtual
S8 LLVisualParamHint::getType() const
{
	return LLViewerDynamicTexture::LL_VISUAL_PARAM_HINT;
}

// Requests updates for all instances (excluding two possible exceptions).
// Grungy but efficient.
//static
void LLVisualParamHint::requestHintUpdates(LLVisualParamHint* exception1,
										   LLVisualParamHint* exception2)
{
	S32 delay_frames = 0;
	for (instance_list_t::iterator iter = sInstances.begin();
		 iter != sInstances.end(); ++iter)
	{
		LLVisualParamHint* vphintp = *iter;
		if (vphintp != exception1 && vphintp != exception2)
		{
			vphintp->mNeedsUpdate = true;
			if (vphintp->mAllowsUpdates)
			{
				vphintp->mDelayFrames = delay_frames++;
			}
			else
			{
				vphintp->mDelayFrames = 0;
			}
		}
	}
}

bool LLVisualParamHint::needsRender()
{
	return mNeedsUpdate && mDelayFrames-- <= 0 && mAllowsUpdates &&
		   isAgentAvatarValid() && !gAgentAvatarp->getIsAppearanceAnimating();
}

void LLVisualParamHint::preRender(bool clear_depth)
{
	if (isAgentAvatarValid())
	{
		mLastParamWeight = mVisualParam->getWeight();
		if (mWearablePtr)
		{
			mWearablePtr->setVisualParamWeight(mVisualParam->getID(),
											   mVisualParamWeight, false);
			LLViewerWearable* wearable = mWearablePtr->asViewerWearable();
			if (wearable)
			{
				wearable->setVolatile(true);
			}
		}
		else
		{
			llwarns << "mWearablePtr is NULL: cannot set wearable visual param weight."
					<< llendl;
		}
		gAgentAvatarp->setVisualParamWeight(mVisualParam->getID(),
											mVisualParamWeight, false);
		gAgentAvatarp->setVisualParamWeight("Blink_Left", 0.f);
		gAgentAvatarp->setVisualParamWeight("Blink_Right", 0.f);
		gAgentAvatarp->updateComposites();
		gAgentAvatarp->updateVisualParams();
#if 0	// This is a NOP !
		gAgentAvatarp->updateGeometry(gAgentAvatarp->mDrawable);
#endif
		gAgentAvatarp->updateLOD();

		LLViewerDynamicTexture::preRender(clear_depth);
	}
}

bool LLVisualParamHint::render()
{
	if (!isAgentAvatarValid()) return true;

	LLVisualParamReset::sDirty = true;

	gGL.pushUIMatrix();
	gGL.loadUIIdentity();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho(0.f, mFullWidth, 0.f, mFullHeight, -1.f, 1.f);

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadIdentity();

	gUIProgram.bind();

	LLGLSUIDefault gls_ui;
	//LLGLState::verify(true);
	mBackgroundp->draw(0, 0, mFullWidth, mFullHeight);

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();

	mNeedsUpdate = false;
	mIsVisible = true;

	LLQuaternion avatar_rot;
	LLJoint* rootp = gAgentAvatarp->getRootJoint();
	if (rootp)
	{
		avatar_rot = rootp->getWorldRotation();
	}

	LLVector3 target_joint_pos;
	if (mCamTargetJoint)
	{
		target_joint_pos = mCamTargetJoint->getWorldPosition();
	}
	LLVector3 target_offset(0.f, 0.f, mVisualParam->getCameraElevation());
	LLVector3 target_pos = target_joint_pos + (target_offset * avatar_rot);

	F32 cam_angle_radians = mVisualParam->getCameraAngle() * DEG_TO_RAD;
	LLVector3 snapshot_offset(mVisualParam->getCameraDistance() *
							  cosf(cam_angle_radians),
							  mVisualParam->getCameraDistance() *
							  sinf(cam_angle_radians),
							  mVisualParam->getCameraElevation());
	LLVector3 camera_pos = target_joint_pos + snapshot_offset * avatar_rot;

	gGL.flush();

	gViewerCamera.setAspect((F32)mFullWidth / (F32)mFullHeight);
	gViewerCamera.setOriginAndLookAt(camera_pos,		// Camera
									 LLVector3::z_axis,	// Up
									 target_pos);		// Point of interest

	gViewerCamera.setPerspective(false, mOrigin.mX, mOrigin.mY, mFullWidth,
								  mFullHeight, false);

	// Do not let environment settings influence our scene lighting. Note:
	// 'false' is needed here so that gPipeline.enableLightsPreview() is not
	// called when in forward rendering mode to avoid under-exposed render. HB
	HBPreviewLighting preview_light(false);

	gPipeline.previewAvatar(gAgentAvatarp);

	gAgentAvatarp->setVisualParamWeight(mVisualParam->getID(),
										mLastParamWeight);
	if (mWearablePtr)
	{
		mWearablePtr->setVisualParamWeight(mVisualParam->getID(),
										   mLastParamWeight, false);
	}
	else
	{
		llwarns << "mWearablePtr is NULL: cannot set wearable visual param weight."
				<< llendl;
	}

	LLViewerWearable* wearablep = mWearablePtr->asViewerWearable();
	if (wearablep)
	{
		wearablep->setVolatile(false);
	}

	gAgentAvatarp->updateVisualParams();

	gGL.color4f(1.f, 1.f, 1.f, 1.f);
	mImageGLp->setGLTextureCreated(true);

	gGL.popUIMatrix();

	return true;
}

void LLVisualParamHint::draw()
{
	if (!mIsVisible) return;

	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->bind(this);

	gGL.color4f(1.f, 1.f, 1.f, 1.f);

	LLGLSUIDefault gls_ui;
	gGL.begin(LLRender::TRIANGLES);
	{
		gGL.texCoord2i(0, 1);
		gGL.vertex2i(0, mFullHeight);
		gGL.texCoord2i(0, 0);
		gGL.vertex2i(0, 0);
		gGL.texCoord2i(1, 0);
		gGL.vertex2i(mFullWidth, 0);
		gGL.texCoord2i(0, 1);
		gGL.vertex2i(0, mFullHeight);
		gGL.texCoord2i(1, 0);
		gGL.vertex2i(mFullWidth, 0);
		gGL.texCoord2i(1, 1);
		gGL.vertex2i(mFullWidth, mFullHeight);
	}
	gGL.end();

	unit0->unbind(LLTexUnit::TT_TEXTURE);
}

void LLVisualParamHint::setWearable(LLWearable* wearablep,
									LLViewerVisualParam* paramp)
{
	mWearablePtr = wearablep;
	mVisualParam = paramp;
}

//-----------------------------------------------------------------------------
// LLVisualParamReset() class
//-----------------------------------------------------------------------------

LLVisualParamReset::LLVisualParamReset()
:	LLViewerDynamicTexture(1, 1, 1, ORDER_RESET, false)
{
}

//virtual
S8 LLVisualParamReset::getType() const
{
	return LLViewerDynamicTexture::LL_VISUAL_PARAM_RESET;
}

bool LLVisualParamReset::render()
{
	if (sDirty && isAgentAvatarValid())
	{
		gAgentAvatarp->updateComposites();
		gAgentAvatarp->updateVisualParams();
#if 0	// This is a NOP !  HB
		gAgentAvatarp->updateGeometry(gAgentAvatarp->mDrawable);
#endif
		sDirty = false;
	}

	return false;
}
