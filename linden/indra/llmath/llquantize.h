/**
 * @file llquantize.h
 * @brief useful routines for quantizing floats to various length ints
 * and back out again
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

constexpr U16 U16MAX = 65535;
alignas(16) const F32 F_U16MAX_4A[4] = { 65535.f, 65535.f, 65535.f, 65535.f };

constexpr F32 OOU16MAX = 1.f / (F32)(U16MAX);
alignas(16) const F32 F_OOU16MAX_4A[4] = { OOU16MAX, OOU16MAX, OOU16MAX, OOU16MAX };

constexpr U8 U8MAX = 255;
alignas(16) const F32 F_U8MAX_4A[4] = { 255.f, 255.f, 255.f, 255.f };

constexpr F32 OOU8MAX = 1.f / (F32)(U8MAX);
alignas(16) const F32 F_OOU8MAX_4A[4] = { OOU8MAX, OOU8MAX, OOU8MAX, OOU8MAX };

LL_INLINE U16 F32_to_U16_ROUND(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// Make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// Round the value and return the U16
	return (U16)(ll_round(val * U16MAX));
}

LL_INLINE U16 F32_to_U16(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// Make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// Return the U16
	return (U16)(llfloor(val * U16MAX));
}

LL_INLINE F32 U16_to_F32(U16 ival, F32 lower, F32 upper)
{
	F32 val = ival * OOU16MAX;
	F32 delta = (upper - lower);
	val *= delta;
	val += lower;

	F32 max_error = delta * OOU16MAX;

	// Make sure that zero's come through as zero
	if (fabsf(val) < max_error)
	{
		val = 0.f;
	}

	return val;
}

LL_INLINE U8 F32_to_U8_ROUND(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// Make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// Return the rounded U8
	return (U8)(ll_round(val * U8MAX));
}

LL_INLINE U8 F32_to_U8(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// Make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// Return the U8
	return (U8)(llfloor(val * U8MAX));
}

LL_INLINE F32 U8_to_F32(U8 ival, F32 lower, F32 upper)
{
	F32 val = ival * OOU8MAX;
	F32 delta = (upper - lower);
	val *= delta;
	val += lower;

	F32 max_error = delta * OOU8MAX;

	// Make sure that zero's come through as zero
	if (fabsf(val) < max_error)
	{
		val = 0.f;
	}

	return val;
}
