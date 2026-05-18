/**
 * @file llkeyframefallmotion.cpp
 * @brief Implementation of LLKeyframeFallMotion class.
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

#include "linden_common.h"

#include "llkeyframefallmotion.h"

#include "llcharacter.h"
#include "llmatrix3.h"

LLKeyframeFallMotion::LLKeyframeFallMotion(const LLUUID& id)
:	LLKeyframeMotion(id),
	mCharacter(NULL),
	mVelocityZ(0.f)
{
}

LLMotion::LLMotionInitStatus LLKeyframeFallMotion::onInitialize(LLCharacter* chrp)
{
	// Save character pointer for later use
	mCharacter = chrp;

	// Load keyframe data, setup pose and joint states
	LLMotion::LLMotionInitStatus result = LLKeyframeMotion::onInitialize(chrp);
	if (result != LLMotion::STATUS_SUCCESS)
	{
		return result;
	}

	for (U32 jm = 0, count = mJointMotionList->getNumJointMotions();
		 jm < count; ++jm)
	{
		LLJointState* jstate = mJointStates[jm];
		if (jstate)
		{
			LLJoint* joint = jstate->getJoint();
			if (joint && joint->getName() == "mPelvis")
			{
				mPelvisState = jstate;
				return result;
			}
		}
	}

	return result;
}

bool LLKeyframeFallMotion::onActivate()
{
	mVelocityZ = -mCharacter->getCharacterVelocity().mV[VZ];

	LLVector3 ground_pos;
	LLVector3 ground_normal;
	mCharacter->getGround(mCharacter->getCharacterPosition(), ground_pos,
						  ground_normal);
	ground_normal.normalize();

	LLQuaternion inverse_pelvis_rot = mCharacter->getCharacterRotation();
	inverse_pelvis_rot.transpose();

	// Find ground normal in pelvis space
	ground_normal = ground_normal * inverse_pelvis_rot;

	// Calculate new foward axis
	LLVector3 fwd_axis = LLVector3::x_axis -
						 ground_normal * (ground_normal * LLVector3::x_axis);
	fwd_axis.normalize();
	mRotationToGroundNormal = LLQuaternion(fwd_axis, ground_normal % fwd_axis,
										   ground_normal);

	return LLKeyframeMotion::onActivate();
}

bool LLKeyframeFallMotion::onUpdate(F32 time, U8* joint_mask)
{
	bool result = LLKeyframeMotion::onUpdate(time, joint_mask);
	F32 slerp_amt = clamp_rescale(time / getDuration(), 0.5f, 0.75f, 0.f, 1.f);

	if (mPelvisState.notNull())
	{
		mPelvisState->setRotation(mPelvisState->getRotation() *
								  slerp(slerp_amt, mRotationToGroundNormal,
										LLQuaternion()));
	}

	return result;
}
