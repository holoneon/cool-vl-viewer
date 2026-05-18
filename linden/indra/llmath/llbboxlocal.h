/**
 * @file llbboxlocal.h
 * @brief General purpose bounding box class.
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

#include "llmatrix4.h"
#include "llvector3.h"

class LLMatrix4;

class LLBBoxLocal
{
	friend LLBBoxLocal operator*(const LLBBoxLocal& a, const LLMatrix4& b);

public:
	LLBBoxLocal() = default;

	LL_INLINE LLBBoxLocal(const LLVector3& min, const LLVector3& max)
	:	mMin(min),
		mMax(max)
	{
	}

	// Default copy constructor is OK.

	LL_INLINE const LLVector3& getMin() const			{ return mMin; }
	LL_INLINE void setMin(const LLVector3& min)			{ mMin = min; }

	LL_INLINE const LLVector3& getMax() const			{ return mMax; }
	LL_INLINE void setMax(const LLVector3& max)			{ mMax = max; }

	LL_INLINE LLVector3 getCenter() const				{ return (mMax - mMin) * 0.5f + mMin; }
	LL_INLINE LLVector3 getExtent() const				{ return mMax - mMin; }

	LL_INLINE void addPoint(const LLVector3& p)
	{
		mMin.mV[VX] = llmin(p.mV[VX], mMin.mV[VX]);
		mMin.mV[VY] = llmin(p.mV[VY], mMin.mV[VY]);
		mMin.mV[VZ] = llmin(p.mV[VZ], mMin.mV[VZ]);
		mMax.mV[VX] = llmax(p.mV[VX], mMax.mV[VX]);
		mMax.mV[VY] = llmax(p.mV[VY], mMax.mV[VY]);
		mMax.mV[VZ] = llmax(p.mV[VZ], mMax.mV[VZ]);
	}

	LL_INLINE void addBBox(const LLBBoxLocal& b)
	{
		addPoint(b.mMin);
		addPoint(b.mMax);
	}

	LL_INLINE void expand(F32 delta)
	{
		mMin.mV[VX] -= delta;
		mMin.mV[VY] -= delta;
		mMin.mV[VZ] -= delta;
		mMax.mV[VX] += delta;
		mMax.mV[VY] += delta;
		mMax.mV[VZ] += delta;
	}

private:
	LLVector3 mMin;
	LLVector3 mMax;
};

LL_INLINE LLBBoxLocal operator*(const LLBBoxLocal& a, const LLMatrix4& b)
{
	return LLBBoxLocal(a.mMin * b, a.mMax * b);
}
