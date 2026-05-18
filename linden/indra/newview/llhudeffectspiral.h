/**
 * @file llhudeffectspiral.h
 * @brief LLHUDEffectSpiral class definition
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

#include "llframetimer.h"
#include "llinterp.h"

#include "llviewerpartsim.h"

class LLVector3d;
class LLViewerObject;

constexpr U32 NUM_TRAIL_POINTS = 40;

class LLHUDEffectSpiral : public LLHUDEffect
{
	friend class LLHUDObject;

protected:
	LOG_CLASS(LLHUDEffectSpiral);

public:
	void markDead() override;
	void setTargetObject(LLViewerObject* objectp) override;

	LL_INLINE void setVMag(F32 vmag)			{ mVMag = vmag; }
	LL_INLINE void setVOffset(F32 offset)		{ mVOffset = offset; }
	LL_INLINE void setInitialRadius(F32 radius)	{ mInitialRadius = radius; }
	LL_INLINE void setFinalRadius(F32 radius)	{ mFinalRadius = radius; }
	LL_INLINE void setScaleBase(F32 scale)		{ mScaleBase = scale; }
	LL_INLINE void setScaleVar(F32 scale)		{ mScaleVar = scale; }
	LL_INLINE void setSpinRate(F32 rate)		{ mSpinRate = rate; }
	LL_INLINE void setFlickerRate(F32 rate)		{ mFlickerRate = rate; }

	// Start the effect playing locally.
	void triggerLocal();

	// Factorized code to create the standard beam effect from the agent to an
	// object or a global position with the standard agent effect color. HB
	static void agentBeamToObject(LLViewerObject* objectp);
	static void agentBeamToPosition(const LLVector3d& pos);
	// Swirling particles at global position, with optional duration (0 to mark
	// dead once sent) and optional immediate sending to server. HB
	static void swirlAtPosition(const LLVector3d& pos, F32 duration = -1.f,
								bool send_now = false);
	// Sphere effect at global position, for 0.25s (used by LLToolPie only). HB
	static void sphereAtPosition(const LLVector3d& pos);

protected:
	LLHUDEffectSpiral(U8 type);
	~LLHUDEffectSpiral() override = default;

	void update() override;
	LL_INLINE void render() override			{}
	void packData(LLMessageSystem* mesgsys) override;
	void unpackData(LLMessageSystem* mesgsys, S32 blocknum) override;

private:
	LLPointer<LLViewerPartSource>	mPartSourcep;

	F32								mKillTime;
	F32								mVMag;
	F32								mVOffset;
	F32								mInitialRadius;
	F32								mFinalRadius;
	F32								mSpinRate;
	F32								mFlickerRate;
	F32								mScaleBase;
	F32								mScaleVar;
	LLFrameTimer					mTimer;
	LLInterpLinear					mFadeInterp;
};
