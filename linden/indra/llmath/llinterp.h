/**
 * @file llinterp.h
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

#include "llpreprocessor.h"
#include "stdtypes.h"

// There used to be several interpolator class templates derived from a base
// LLInterp class template, with different data types, but only the linear type
// with the F32 data type was ever used in the viewer code, so I removed the
// others, and made LLInterpLinear into a non-virtual, non-template class. HB

class LLInterpLinear
{
public:
	LL_INLINE LLInterpLinear()
	:	mStartVal(0.f),
		mEndVal(0.f),
		mCurVal(0.f),
		mStartTime(0.f),
		mCurTime(0.f),
		mEndTime(1.f),
		mDuration(1.f),
		mCurFrac(0.f),
		mDone(false),
		mActive(false)
	{
	}

	LL_INLINE void start()
	{
		mCurVal = mStartVal;
		mCurTime = mStartTime;
		mDone = mActive = false;
		mCurFrac = 0.f;
	}

	void update(F32 time);

	LL_INLINE const F32& getCurVal() const			{ return mCurVal; }

	LL_INLINE void setStartVal(const F32& val)		{ mStartVal = val; }
	LL_INLINE const F32& getStartVal() const		{ return mStartVal; }

	LL_INLINE void setEndVal(const F32& val)		{ mEndVal = val; }

	LL_INLINE const F32& getEndVal() const			{ return mEndVal; }

	LL_INLINE void setStartTime(F32 time)
	{
		mStartTime = time;
		mDuration = mEndTime - mStartTime;
	}

	LL_INLINE F32 getStartTime() const				{ return mStartTime; }

	LL_INLINE void setEndTime(F32 time)
	{
		mEndTime = time;
		mDuration = mEndTime - mStartTime;
	}

	LL_INLINE F32 getEndTime() const				{ return mEndTime; }

	LL_INLINE bool isActive() const					{ return mActive; }
	LL_INLINE bool isDone() const					{ return mDone; }

	LL_INLINE F32 getCurFrac() const				{ return mCurFrac; }

protected:
	F32		mStartTime;
	F32		mEndTime;
	F32		mDuration;
	F32		mCurTime;
	F32		mCurFrac;

	F32		mStartVal;
	F32		mEndVal;
	F32		mCurVal;

	bool	mActive;
	bool	mDone;
};
