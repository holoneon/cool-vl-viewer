/**
 * @file llkeyframewalkmotion.h
 * @brief Implementation of LLKeframeWalkMotion class.
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

#include "llcharacter.h"
#include "llkeyframemotion.h"
#include "llpreprocessor.h"
#include "llvector3d.h"

#define MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST (20.f)
#define MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST (20.f)

class LLKeyframeWalkMotion final : public LLKeyframeMotion
{
	friend class LLWalkAdjustMotion;

protected:
	LOG_CLASS(LLKeyframeWalkMotion);

public:
	LLKeyframeWalkMotion(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)
	{
		return new LLKeyframeWalkMotion(id);
	}

	LLMotionInitStatus onInitialize(LLCharacter* character) override;
	bool onActivate() override;
	void onDeactivate() override;
	bool onUpdate(F32 time, U8* joint_mask) override;

public:
	LLCharacter*	mCharacter;
	F32				mRealTimeLast;
	F32				mAdjTimeLast;
};

class LLWalkAdjustMotion final : public LLMotion
{
protected:
	LOG_CLASS(LLWalkAdjustMotion);

public:
	LLWalkAdjustMotion(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)		{ return new LLWalkAdjustMotion(id); }

public:
	LLMotionInitStatus onInitialize(LLCharacter* character) override;
	bool onActivate() override;
	void onDeactivate() override;
	bool onUpdate(F32 time, U8* joint_mask) override;

	LL_INLINE LLJoint::JointPriority getPriority() override	{ return LLJoint::HIGH_PRIORITY; }
	LL_INLINE bool getLoop() override						{ return true; }
	LL_INLINE F32 getDuration() override					{ return 0.f; }
	LL_INLINE F32 getEaseInDuration() override				{ return 0.f; }
	LL_INLINE F32 getEaseOutDuration() override				{ return 0.f; }
	LL_INLINE F32 getMinPixelArea() override				{ return MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST; }
	LL_INLINE LLMotionBlendType getBlendType() override		{ return ADDITIVE_BLEND; }

public:
	LLCharacter*			mCharacter;
	LLJoint*				mLeftAnkleJoint;
	LLJoint*				mRightAnkleJoint;
	LLPointer<LLJointState>	mPelvisState;
	LLJoint*				mPelvisJoint;
	LLVector3d				mLastLeftAnklePos;
	LLVector3d				mLastRightAnklePos;
	F32						mLastTime;
	F32						mAvgCorrection;
	F32						mSpeedAdjust;
	F32						mAnimSpeed;
	F32						mAvgSpeed;
	F32						mRelativeDir;
	LLVector3				mPelvisOffset;
	F32						mAnkleOffset;
};

class LLFlyAdjustMotion final : public LLMotion
{
protected:
	LOG_CLASS(LLFlyAdjustMotion);

public:
	LLFlyAdjustMotion(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)		{ return new LLFlyAdjustMotion(id); }

public:
	LLMotionInitStatus onInitialize(LLCharacter* character) override;
	bool onActivate() override;
	LL_INLINE void onDeactivate() override					{}
	bool onUpdate(F32 time, U8* joint_mask) override;

	LL_INLINE LLJoint::JointPriority getPriority() override	{ return LLJoint::HIGHER_PRIORITY; }
	LL_INLINE bool getLoop() override						{ return true; }
	LL_INLINE F32 getDuration() override					{ return 0.f; }
	LL_INLINE F32 getEaseInDuration() override				{ return 0.f; }
	LL_INLINE F32 getEaseOutDuration() override				{ return 0.f; }
	LL_INLINE F32 getMinPixelArea() override				{ return MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST; }
	LL_INLINE LLMotionBlendType getBlendType() override		{ return ADDITIVE_BLEND; }

protected:
	LLCharacter*			mCharacter;
	LLPointer<LLJointState>	mPelvisState;
	F32						mRoll;
};
