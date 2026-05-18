/**
 * @file llinterp.cpp
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

#include "llinterp.h"

void LLInterpLinear::update(F32 time)
{
	F32 target_frac = (time - mStartTime) / mDuration;
	F32 dfrac = target_frac - mCurFrac;
	if (target_frac >= 0.f)
	{
		mActive = true;
	}

	if (target_frac > 1.f)
	{
		mCurVal = mEndVal;
		mCurFrac = 1.f;
		mCurTime = time;
		mDone = true;
		return;
	}

	target_frac = llmax(0.f, target_frac);

	if (dfrac >= 0.f)
	{
		F32 total_frac = 1.f - mCurFrac;
		F32 inc_frac = dfrac / total_frac;
		mCurVal = inc_frac * mEndVal + (1.f - inc_frac) * mCurVal;
		mCurTime = time;
	}
	else
	{
		F32 total_frac = mCurFrac - 1.f;
		F32 inc_frac = dfrac / total_frac;
		mCurVal = inc_frac * mStartVal + (1.f - inc_frac) * mCurVal;
		mCurTime = time;
	}

	mCurFrac = target_frac;
}
