/**
 * @file llemote.cpp
 * @brief Implementation of LLEmote class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llemote.h"

#include "llcharacter.h"

LLEmote::LLEmote(const LLUUID& id)
:	LLMotion(id)
{
	mCharacter = NULL;

	// RN: flag face joint as highest priority for now, until we implement a
	// proper animation track
	mJointSignature[0][LL_FACE_JOINT_NUM] = 0xff;
	mJointSignature[1][LL_FACE_JOINT_NUM] = 0xff;
	mJointSignature[2][LL_FACE_JOINT_NUM] = 0xff;
}

LLMotion::LLMotionInitStatus LLEmote::onInitialize(LLCharacter* character)
{
	mCharacter = character;
	return STATUS_SUCCESS;
}

bool LLEmote::onActivate()
{
	if (!mCharacter)
	{
		return true;
	}

	LLVisualParam* default_param;
	default_param = mCharacter->getVisualParam("Express_Closed_Mouth");
	if (default_param)
	{
		default_param->setWeight(default_param->getMaxWeight(), false);
	}

	mParam = mCharacter->getVisualParam(mName.c_str());
	if (mParam)
	{
		mParam->setWeight(0.f, false);
		mCharacter->updateVisualParams();
	}

	return true;
}

bool LLEmote::onUpdate(F32 time, U8* joint_mask)
{
	if (mParam && mCharacter)
	{
		F32 weight = mParam->getMinWeight() +
					 mPose.getWeight() *
					 (mParam->getMaxWeight() - mParam->getMinWeight());
		mParam->setWeight(weight, false);

		// Cross fade against the default parameter
		LLVisualParam* default_param =
			mCharacter->getVisualParam("Express_Closed_Mouth");
		if (default_param)
		{
			F32 default_param_weight = default_param->getMinWeight() +
									   (1.f - mPose.getWeight()) *
									   (default_param->getMaxWeight() -
										default_param->getMinWeight());

			default_param->setWeight(default_param_weight, false);
		}

		mCharacter->updateVisualParams();
	}

	return true;
}

void LLEmote::onDeactivate()
{
	if (!mCharacter) return;

	if (mParam)
	{
		mParam->setWeight(mParam->getDefaultWeight(), false);
	}

	LLVisualParam* default_param;
	default_param = mCharacter->getVisualParam("Express_Closed_Mouth");
	if (default_param)
	{
		default_param->setWeight(default_param->getMaxWeight(), false);
	}

	mCharacter->updateVisualParams();
}
