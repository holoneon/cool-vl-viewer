/**
 * @file llvector2.cpp
 * @brief LLVector2 class implementation.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "llvector2.h"

#include "llvector3.h"
#include "llvector4.h"
#include "llmatrix4.h"
#include "llmatrix3.h"
#include "llquaternion.h"

// LLVector2

LLVector2 LLVector2::zero(0.f, 0.f);

// Non-member functions

// Sets all values to absolute value of their original values. Returns true if
// data changed.
bool LLVector2::abs()
{
	bool ret = false;

	if (mV[0] < 0.f)
	{
		mV[0] = -mV[0];
		ret = true;
	}

	if (mV[1] < 0.f)
	{
		mV[1] = -mV[1];
		ret = true;
	}

	return ret;
}

F32 angle_between(const LLVector2& a, const LLVector2& b)
{
	LLVector2 an = a;
	LLVector2 bn = b;
	an.normalize();
	bn.normalize();
	F32 cosine = an * bn;
	F32 angle = cosine >= 1.f ? 0.f
							  : (cosine <= -1.f ? F_PI : acosf(cosine));
	return angle;
}

bool are_parallel(const LLVector2& a, const LLVector2& b, float epsilon)
{
	LLVector2 an = a;
	LLVector2 bn = b;
	an.normalize();
	bn.normalize();
	F32 dot = an * bn;
	return 1.f - fabsf(dot) < epsilon;
}

F32	dist_vec(const LLVector2& a, const LLVector2& b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	return sqrtf(x * x + y * y);
}

F32	dist_vec_squared(const LLVector2& a, const LLVector2& b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	return x * x + y * y;
}

F32	dist_vec_squared2D(const LLVector2& a, const LLVector2& b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	return x * x + y * y;
}

LLVector2 lerp(const LLVector2& a, const LLVector2& b, F32 u)
{
	return LLVector2(a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u,
					 a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u);
}
