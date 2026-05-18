/**
 * @file lleditingmotion.h
 * @brief Implementation of LLEditingMotion class.
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
#include "llmotion.h"
#include "llvector3d.h"

#define EDITING_EASEIN_DURATION	0.0f
#define EDITING_EASEOUT_DURATION 0.5f
#define EDITING_PRIORITY LLJoint::HIGH_PRIORITY
#define MIN_REQUIRED_PIXEL_AREA_EDITING 500.f

class LLEditingMotion final : public LLMotion
{
protected:
	LOG_CLASS(LLEditingMotion);

public:
	LLEditingMotion(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)		{ return new LLEditingMotion(id); }

	// Motions must specify whether or not they loop
	LL_INLINE bool getLoop() override						{ return true; }

	// Motions must report their total duration
	LL_INLINE F32 getDuration() override					{ return 0.f; }

	// Motions must report their "ease in" duration
	LL_INLINE F32 getEaseInDuration() override				{ return EDITING_EASEIN_DURATION; }

	// Motions must report their "ease out" duration.
	LL_INLINE F32 getEaseOutDuration() override				{ return EDITING_EASEOUT_DURATION; }

	// Motions must report their priority
	LL_INLINE LLJoint::JointPriority getPriority() override	{ return EDITING_PRIORITY; }

	LL_INLINE LLMotionBlendType getBlendType() override		{ return NORMAL_BLEND; }

	// Called to determine when a motion should be activated/deactivated based
	// on avatar pixel coverage.
	LL_INLINE F32 getMinPixelArea() override				{ return MIN_REQUIRED_PIXEL_AREA_EDITING; }

	// Run-time (post constructor) initialization, called after parameters have
	// been set. Must return true to indicate success and be available for
	// activation.
	LLMotionInitStatus onInitialize(LLCharacter* character) override;

	// Called when a motion is activated must return true to indicate success,
	// or else it will be deactivated.
	bool onActivate() override;

	// Called per time step. Must return true while it is active, and must
	// return false when the motion is completed.
	bool onUpdate(F32 time, U8* joint_mask) override;

	// Called when a motion is deactivated
	LL_INLINE void onDeactivate() override					{}

public:
	LLJoint					mParentJoint;
	LLJoint					mShoulderJoint;
	LLJoint					mElbowJoint;
	LLJoint					mWristJoint;
	LLJoint					mTarget;

	// Joint states to be animated
	LLPointer<LLJointState>	mParentState;
	LLPointer<LLJointState>	mShoulderState;
	LLPointer<LLJointState>	mElbowState;
	LLPointer<LLJointState>	mWristState;
	LLPointer<LLJointState>	mTorsoState;

	LLCharacter*			mCharacter;

	LLVector3				mWristOffset;
	LLVector3				mLastSelectPt;

	LLJointSolverRP3		mIKSolver;

	static S32				sHandPose;
	static S32				sHandPosePriority;
};
