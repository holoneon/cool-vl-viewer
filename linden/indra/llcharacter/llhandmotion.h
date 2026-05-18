/**
 * @file llhandmotion.h
 * @brief Implementation of LLHandMotion class.
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

#include "llmotion.h"
#include "lltimer.h"

#define MIN_REQUIRED_PIXEL_AREA_HAND 10000.f;

class LLHandMotion final : public LLMotion
{
protected:
	LOG_CLASS(LLHandMotion);

public:
	typedef enum e_hand_pose
	{
		HAND_POSE_SPREAD,
		HAND_POSE_RELAXED,
		HAND_POSE_POINT,
		HAND_POSE_FIST,
		HAND_POSE_RELAXED_L,
		HAND_POSE_POINT_L,
		HAND_POSE_FIST_L,
		HAND_POSE_RELAXED_R,
		HAND_POSE_POINT_R,
		HAND_POSE_FIST_R,
		HAND_POSE_SALUTE_R,
		HAND_POSE_TYPING,
		HAND_POSE_PEACE_R,
		HAND_POSE_PALM_R,
		NUM_HAND_POSES
	} eHandPose;

	LLHandMotion(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)		{ return new LLHandMotion(id); }

	// Motions must specify whether or not they loop
	LL_INLINE bool getLoop() override						{ return true; }

	// Motions must report their total duration
	LL_INLINE F32 getDuration() override					{ return 0.f; }

	// Motions must report their "ease in" duration
	LL_INLINE F32 getEaseInDuration() override				{ return 0.f; }

	// Motions must report their "ease out" duration.
	LL_INLINE F32 getEaseOutDuration() override				{ return 0.f; }

	// Called to determine when a motion should be activated/deactivated based
	// on avatar pixel coverage
	LL_INLINE F32 getMinPixelArea() override				{ return MIN_REQUIRED_PIXEL_AREA_HAND; }

	// Motions must report their priority
	LL_INLINE LLJoint::JointPriority getPriority() override	{ return LLJoint::MEDIUM_PRIORITY; }

	LL_INLINE LLMotionBlendType getBlendType() override		{ return NORMAL_BLEND; }

	// Run-time (post constructor) initialization, called after parameters have
	// been set.
	LLMotionInitStatus onInitialize(LLCharacter* character) override
	{
		mCharacter = character;
		return STATUS_SUCCESS;
	}

	// Called when a motion is activated. Must return true to indicate success,
	// or else it will be deactivated.
	bool onActivate() override;

	// Called per time step. Must return true while it is active, and must
	// return false when the motion is completed.
	bool onUpdate(F32 time, U8* joint_mask) override;

	// Called when a motion is deactivated
	LL_INLINE void onDeactivate() override					{}

	LL_INLINE bool canDeprecate() override					{ return false; }

	static std::string getHandPoseName(eHandPose pose);
	static eHandPose getHandPose(const std::string& posename);

public:
	LLCharacter*	mCharacter;
	F32				mLastTime;
	eHandPose		mCurrentPose;
	eHandPose		mNewPose;
};
