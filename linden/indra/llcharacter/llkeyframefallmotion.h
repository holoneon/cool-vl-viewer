/**
 * @file llkeyframefallmotion.h
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

class LLKeyframeFallMotion final : public LLKeyframeMotion
{
protected:
	LOG_CLASS(LLKeyframeFallMotion);

public:
	LLKeyframeFallMotion(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)
	{
		return new LLKeyframeFallMotion(id);
	}

	LLMotionInitStatus onInitialize(LLCharacter* character) override;
	bool onActivate() override;

	LL_INLINE F32 getEaseInDuration() override
	{
		if (mVelocityZ == 0.f)
		{
			// We have already hit the ground
			return 0.4f;
		}
		return mCharacter->getPreferredPelvisHeight() / mVelocityZ;
	}

	bool onUpdate(F32 activeTime, U8* joint_mask) override;

protected:
	LLQuaternion			mRotationToGroundNormal;
	LLPointer<LLJointState>	mPelvisState;
	LLCharacter*			mCharacter;
	F32						mVelocityZ;
};
