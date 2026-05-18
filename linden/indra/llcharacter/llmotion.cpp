/**
 * @file llmotion.cpp
 * @brief Implementation of LLMotion class.
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

#include "llmotion.h"

#include "llcriticaldamp.h"

LLMotion::LLMotion(const LLUUID &id)
:	mStopped(true),
	mActive(false),
	mID(id),
	mActivationTimestamp(0.f),
	mStopTimestamp(0.f),
	mSendStopTimestamp(F32_MAX),
	mResidualWeight(0.f),
	mFadeWeight(1.f),
	mDeactivateCallback(nullptr),
	mDeactivateCallbackUserData(nullptr)
{
	for (S32 i = 0; i < 3; ++i)
	{
		memset(&mJointSignature[i][0], 0,
			   sizeof(U8) * LL_CHARACTER_MAX_ANIMATED_JOINTS);
	}
}

void LLMotion::fadeOut()
{
	if (mFadeWeight > 0.01f)
	{
		mFadeWeight = lerp(mFadeWeight, 0.f, LLCriticalDamp::getInterpolant(0.15f));
	}
	else
	{
		mFadeWeight = 0.f;
	}
}

void LLMotion::fadeIn()
{
	if (mFadeWeight < 0.99f)
	{
		mFadeWeight = lerp(mFadeWeight, 1.f,
						   LLCriticalDamp::getInterpolant(0.15f));
	}
	else
	{
		mFadeWeight = 1.f;
	}
}

void LLMotion::addJointState(const LLPointer<LLJointState>& joint_state)
{
	mPose.addJointState(joint_state);
	S32 priority = joint_state->getPriority();
	if (priority == LLJoint::USE_MOTION_PRIORITY)
	{
		priority = getPriority();
	}

	U32 usage = joint_state->getUsage();

	// For now, usage is everything
	S32 joint_num = joint_state->getJoint()->getJointNum();
	if (joint_num < 0 || joint_num >= (S32)LL_CHARACTER_MAX_ANIMATED_JOINTS)
	{
		LL_DEBUGS("Avatar") << "Joint number (" << joint_num
							<< ") is outside of the legal range [0-"
							<< LL_CHARACTER_MAX_ANIMATED_JOINTS << "]"
							<< LL_ENDL;
	}

	mJointSignature[0][joint_num] = (usage & LLJointState::POS) ? (0xff >> (7 - priority)) : 0;
	mJointSignature[1][joint_num] = (usage & LLJointState::ROT) ? (0xff >> (7 - priority)) : 0;
	mJointSignature[2][joint_num] = (usage & LLJointState::SCALE) ? (0xff >> (7 - priority)) : 0;
}

void LLMotion::setDeactivateCallback(void (*cb)(void*), void* userdata)
{
	mDeactivateCallback = cb;
	mDeactivateCallbackUserData = userdata;
}

//virtual
void LLMotion::setStopTime(F32 time)
{
	mStopTimestamp = time;
	mStopped = true;
}

bool LLMotion::isBlending() const
{
	return mPose.getWeight() < 1.f;
}

void LLMotion::activate(F32 time)
{
	mActivationTimestamp = time;
	mStopped = false;
	mActive = true;
	onActivate();
}

void LLMotion::deactivate()
{
	mActive = false;
	mPose.setWeight(0.f);

	if (mDeactivateCallback)
	{
		(*mDeactivateCallback)(mDeactivateCallbackUserData);
		mDeactivateCallback = NULL; // only call callback once
		mDeactivateCallbackUserData = NULL;
	}

	onDeactivate();
}
