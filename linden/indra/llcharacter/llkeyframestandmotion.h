/**
 * @file llkeyframestandmotion.h
 * @brief Implementation of LLKeyframeStandMotion class.
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

#pragma once

#include "lljointsolverrp3.h"
#include "llkeyframemotion.h"

class LLKeyframeStandMotion final : public LLKeyframeMotion
{
protected:
	LOG_CLASS(LLKeyframeStandMotion);

public:
	LLKeyframeStandMotion(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)
	{
		return new LLKeyframeStandMotion(id);
	}

	LLMotionInitStatus onInitialize(LLCharacter* character) override;
	bool onActivate() override;
	bool onUpdate(F32 time, U8* joint_mask) override;
	void onDeactivate() override;

public:
	LLJoint					mPelvisJoint;

	LLJoint					mHipLeftJoint;
	LLJoint					mKneeLeftJoint;
	LLJoint					mAnkleLeftJoint;
	LLJoint					mTargetLeft;

	LLJoint					mHipRightJoint;
	LLJoint					mKneeRightJoint;
	LLJoint					mAnkleRightJoint;
	LLJoint					mTargetRight;

	LLCharacter*			mCharacter;

	LLPointer<LLJointState>	mPelvisState;

	LLPointer<LLJointState>	mHipLeftState;
	LLPointer<LLJointState>	mKneeLeftState;
	LLPointer<LLJointState>	mAnkleLeftState;

	LLPointer<LLJointState>	mHipRightState;
	LLPointer<LLJointState>	mKneeRightState;
	LLPointer<LLJointState>	mAnkleRightState;

	LLJointSolverRP3		mIKLeft;
	LLJointSolverRP3		mIKRight;

	LLVector3				mPositionLeft;
	LLVector3				mPositionRight;
	LLVector3				mNormalLeft;
	LLVector3				mNormalRight;
	LLQuaternion			mRotationLeft;
	LLQuaternion			mRotationRight;

	LLQuaternion			mLastGoodPelvisRotation;
	LLVector3				mLastGoodPosition;

	S32						mFrameNum;

	bool					mTrackAnkles;
	bool					mFlipFeet;
};
