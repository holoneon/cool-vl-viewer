/**
 * @file llmorphview.cpp
 * @brief Container for Morph functionality
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

#include "llmorphview.h"

#include "llanimationstates.h"
#include "lljoint.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llfirstuse.h"
#include "llfloatercustomize.h"
#include "llfloatertools.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llvisualparamhint.h"
#include "llvoavatarself.h"

// Instance created in LLViewerWindow::initWorldUI()
LLMorphView* gMorphViewp = NULL;

constexpr F32 MORPH_NEAR_CLIP = 0.1f;

LLMorphView::LLMorphView(const LLRect& rect)
:	LLView("morph view", rect, false, FOLLOWS_ALL),
	mCameraTargetJoint(NULL),
	mCameraOffset(-0.5f, 0.05f, 0.07f),
	mCameraTargetOffset(0.f, 0.f, 0.05f),
	mOldCameraNearClip(0.f),
	mCameraPitch(0.f),
	mCameraYaw(0.f),
	mCameraDrivenByKeys(false)
{
}

LLMorphView::~LLMorphView()
{
	gMorphViewp = NULL;
}

void LLMorphView::initialize()
{
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;

	if (!isAgentAvatarValid())
	{
		gAgent.changeCameraToDefault();
		return;
	}

	gAgentAvatarp->stopMotion(ANIM_AGENT_BODY_NOISE);
	gAgentAvatarp->mSpecialRenderMode = 3;

	// Set up camera for close look at avatar
	mOldCameraNearClip = gViewerCamera.getNear();
	gViewerCamera.setNear(MORPH_NEAR_CLIP);
}

void LLMorphView::shutdown()
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->startMotion(ANIM_AGENT_BODY_NOISE);
		gAgentAvatarp->mSpecialRenderMode = 0;
		// Reset camera
		gViewerCamera.setNear(mOldCameraNearClip);
	}
}

void LLMorphView::setVisible(bool visible)
{
	if (visible &&
		(!gAgentWearables.getWearableCount(LLWearableType::WT_SHAPE) ||
		 !gAgentWearables.getWearableCount(LLWearableType::WT_HAIR) ||
		 !gAgentWearables.getWearableCount(LLWearableType::WT_EYES) ||
		 !gAgentWearables.getWearableCount(LLWearableType::WT_SKIN)))
	{
		// Do not let the user edit wearables if avatar is cloud due to missing
		// parts.
		visible = false;
		llwarns << "Cannot edit appearance while mandatory wearables are missing from outfit."
				<< llendl;
	}

	if (gFloaterViewp && visible != getVisible())
	{
		LLView::setVisible(visible);
		if (visible)
		{
			llassert(!gFloaterCustomizep);
			gFloaterCustomizep = new LLFloaterCustomize();
			gFloaterCustomizep->fetchInventory();
			gFloaterCustomizep->open();
			gFloaterCustomizep->switchToDefaultSubpart();

			initialize();

			// First run dialog
			LLFirstUse::useAppearance();
		}
		else
		{
			if (gFloaterCustomizep)
			{
				gFloaterViewp->removeChild(gFloaterCustomizep);
				delete gFloaterCustomizep;
				gFloaterCustomizep = NULL;
			}

			shutdown();
		}
	}
}

void LLMorphView::updateCamera()
{
	if (!isAgentAvatarValid())
	{
		return;
	}

	if (!mCameraTargetJoint)
	{
		setCameraTargetJoint(gAgentAvatarp->getJoint(LL_JOINT_KEY_HEAD));
	}

	LLJoint* root_joint = gAgentAvatarp->getRootJoint();
	if (!root_joint)
	{
		return;
	}

	const LLQuaternion& avatar_rot = root_joint->getWorldRotation();

	LLVector3d joint_pos =
		gAgent.getPosGlobalFromAgent(mCameraTargetJoint->getWorldPosition());
	LLVector3d target_pos = joint_pos + mCameraTargetOffset * avatar_rot;

	LLQuaternion camera_rot_yaw(mCameraYaw, LLVector3::z_axis);
	LLQuaternion camera_rot_pitch(mCameraPitch, LLVector3::y_axis);

	LLVector3d camera_pos = joint_pos +
							mCameraOffset * camera_rot_pitch *
							camera_rot_yaw * avatar_rot;

	gAgent.setCameraPosAndFocusGlobal(camera_pos, target_pos, gAgentID);
}

void LLMorphView::setCameraDrivenByKeys(bool b)
{
	if (mCameraDrivenByKeys != b)
	{
		if (b)
		{
			// Reset to the default camera position specified by mCameraPitch,
			// mCameraYaw, etc.
			updateCamera();
		}
		mCameraDrivenByKeys = b;
	}
}
