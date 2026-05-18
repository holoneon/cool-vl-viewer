/**
 * @file llplane.h
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

#include "llvector4.h"

// A simple way to specify a plane is to give its normal,
// and it's nearest approach to the origin.
//
// Given the equation for a plane : A*x + B*y + C*z + D = 0
// The plane normal = [A, B, C]
// The closest approach = D / sqrtf(A*A + B*B + C*C)

class alignas(16) LLPlane
{
public:
	LL_INLINE LLPlane() = default;

	LL_INLINE LLPlane(const LLVector3& p0, F32 d)
	{
		setVec(p0, d);
	}

	LL_INLINE LLPlane(const LLVector3& p0, const LLVector3& n)
	{
		setVec(p0, n);
	}

	LL_INLINE void setVec(const LLVector3& p0, F32 d)
	{
		mV.set(p0[0], p0[1], p0[2], d);
	}

	LL_INLINE void setVec(const LLVector3& p0, const LLVector3& n)
	{
		F32 d = -(p0 * n);
		setVec(n, d);
	}
	LL_INLINE void setVec(const LLVector3& p0, const LLVector3& p1,
						  const LLVector3& p2)
	{
		LLVector3 u, v, w;
		u = p1 - p0;
		v = p2 - p0;
		w = u % v;
		w.normalize();
		F32 d = -(w * p0);
		setVec(w, d);
	}

	LL_INLINE LLPlane& operator=(const LLVector4& v2)
	{
		mV.set(v2[0], v2[1], v2[2], v2[3]);
		return *this;
	}

	LL_INLINE LLPlane& operator=(const LLVector4a& v2)
	{
		mV.set(v2[0], v2[1], v2[2], v2[3]);
		return *this;
	}

	LL_INLINE void set(const LLPlane& p2)				{ mV = p2.mV; }

	LL_INLINE F32 dist(const LLVector3& v2) const
	{
		return mV[0] * v2[0] + mV[1] * v2[1] + mV[2] * v2[2] + mV[3];
	}

	LL_INLINE LLSimdScalar dot3(const LLVector4a& b) const
	{
		return mV.dot3(b);
	}

	// Read-only access a single float in this vector. Do not use in proximity
	// to any function call that manipulates the data at the whole vector level
	// or you will incur a substantial penalty. Consider using the splat
	// functions instead
	LL_INLINE F32 operator[](S32 idx) const				{ return mV[idx]; }

	// preferable when index is known at compile time
	template<int N> LL_INLINE void getAt(LLSimdScalar& v) const
	{
		v = mV.getScalarAt<N>();
	}

	// Reset the vector to 0, 0, 0, 1
	LL_INLINE void clear()								{ mV.set(0.f, 0.f, 0.f, 1.f); }

	LL_INLINE void getVector3(LLVector3& vec) const		{ vec.set(mV[0], mV[1], mV[2]); }

	// Retrieve the mask indicating which of the x, y, or z axis are greater
	// or equal to zero.
	LL_INLINE U8 calcPlaneMask() const
	{
		return mV.greaterEqual(LLVector4a::getZero()).getGatheredBits() &
							   LLVector4Logical::MASK_XYZ;
	}

	// Check if two planes are nearly same
	LL_INLINE bool equal(const LLPlane& p) const		{ return mV.equals4(p.mV); }

private:
	LLVector4a mV;
};
