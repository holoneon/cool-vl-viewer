/**
 * @file llhudeffectpointat.h
 * @brief LLHUDEffectPointAt class definition
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

#pragma once

#include "llhudeffect.h"

class LLViewerObject;
class LLVOAvatar;

typedef enum e_pointat_type
{
	POINTAT_TARGET_NONE,
	POINTAT_TARGET_SELECT,
	POINTAT_TARGET_GRAB,
	POINTAT_TARGET_CLEAR,
	POINTAT_NUM_TARGETS
} EPointAtType;

class LLHUDEffectPointAt : public LLHUDEffect
{
protected:
	LOG_CLASS(LLHUDEffectPointAt);

public:
	friend class LLHUDObject;

	void markDead() override;
	void setSourceObject(LLViewerObject* objectp) override;

	bool setPointAt(EPointAtType target_type, LLViewerObject* object,
					LLVector3 position);
	void clearPointAtTarget();

	LL_INLINE EPointAtType getPointAtType()			{ return mTargetType; }
	LL_INLINE const LLVector3& getPointAtPosAgent()	{ return mTargetPos; }

	const LLVector3d getPointAtPosGlobal();

protected:
	LLHUDEffectPointAt(U8 type);
	~LLHUDEffectPointAt() override = default;

	void render() override;
	void packData(LLMessageSystem* mesgsys) override;
	void unpackData(LLMessageSystem* mesgsys, S32 blocknum) override;

	// Point-at behavior has either target position or target object with
	// offset
	void setTargetObjectAndOffset(LLViewerObject* objp,
								  const LLVector3d& offset);
	void setTargetPosGlobal(const LLVector3d& target_pos_global);
	bool calcTargetPosition();
	void update() override;

public:
	static bool		sDebugPointAt;

private:
	LLVector3d		mTargetOffsetGlobal;
	LLVector3		mLastSentOffsetGlobal;
	LLVector3		mTargetPos;
	LLFrameTimer	mTimer;
	EPointAtType	mTargetType;
	F32				mKillTime;
	F32				mLastSendTime;
};
