/**
 * @file llcommonmath.h
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
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

#include <cmath>
#include <cstdlib>

#include "llpreprocessor.h"
#include "stdtypes.h"

// llsd.cpp uses llisnan() and llsdutil.cpp uses is_approx_equal_fraction(),
// they were moved from llmath.h in llcommon so that llcommon does not depend
// on llmath. HB

#define llisnan(val)	std::isnan(val)
#define llfinite(val)	std::isfinite(val)

// Originally llmath.h contained two complete implementations of
// is_approx_equal_fraction(), with signatures as below, bodies identical save
// where they specifically mentioned F32/F64. Unifying these into a template
// makes sense, but to preserve the compiler's overload-selection behavior, we
// still wrap the template implementation with the specific overloaded
// signatures.

template <typename FTYPE>
LL_INLINE bool is_approx_equal_fraction_impl(FTYPE x, FTYPE y, U32 frac_bits)
{
	FTYPE diff = (FTYPE)fabs(x - y);

	S32 diff_int = (S32)diff;
	S32 frac_tolerance = (S32)((diff - (FTYPE)diff_int) * (1 << frac_bits));

	// If integer portion is not equal, not enough bits were used for packing
	// so error out since either the use case is not correct OR there is an
	// issue with pack/unpack. should fail in either case.
	// For decimal portion, make sure that the delta is no more than 1 based on
	// the number of bits used for packing decimal portion.
	return diff_int == 0 && frac_tolerance <= 1;
}

// F32 flavor
LL_INLINE bool is_approx_equal_fraction(F32 x, F32 y, U32 frac_bits)
{
	return is_approx_equal_fraction_impl<F32>(x, y, frac_bits);
}

// F64 flavor
LL_INLINE bool is_approx_equal_fraction(F64 x, F64 y, U32 frac_bits)
{
	return is_approx_equal_fraction_impl<F64>(x, y, frac_bits);
}

// Formerly in u64.h - Converts an U64 to the closest F64 value.
LL_INLINE F64 U64_to_F64(U64 value)
{
	S64 top_bits = (S64)(value >> 1);
	F64 result = (F64)top_bits;
	result *= 2.f;
	result += (U32)(value & 0x01);
	return result;
}

// The functions below used to be in the now removed lldefs.h header and have
// been moved here for coherency (these are math functions, not constants). HB

// Specific inlines for basic types.
//
// defined for all:
//   llmin(a, b)
//   llmax(a, b)
//   llclamp(a, minimum, maximum)
//
// defined for F32, F64:
//   llclampf(a)     // clamps a to [0.0 .. 1.0]
//
// defined for U16, U32, U64, S16, S32, S64, :
//   llclampb(a)     // clamps a to [0 .. 255]

template <typename T1, typename T2>
LL_INLINE T1 llmax(T1 d1, T2 d2)
{
	return d1 > (T1)d2 ? d1 : (T1)d2;
}

template <typename T1, typename T2, typename T3>
LL_INLINE auto llmax(T1 d1, T2 d2, T3 d3)
{
	T1 r = d1 > d2 ? d1 : d2;
	return r > d3 ? r : d3;
}

template <typename T1, typename T2, typename T3, typename T4>
LL_INLINE T1 llmax(T1 d1, T2 d2, T3 d3, T4 d4)
{
	T1 r1 = d1 > (T1)d2 ? d1 : (T1)d2;
	T1 r2 = (T1)d3 > (T1)d4 ? (T1)d3 : (T1)d4;
	return r1 > r2 ? r1 : r2;
}

template <typename T1, typename T2>
LL_INLINE T1 llmin(T1 d1, T2 d2)
{
	return d1 < (T1)d2 ? d1 : (T1)d2;
}

template <typename T1, typename T2, typename T3>
LL_INLINE T1 llmin(T1 d1, T2 d2, T3 d3)
{
	T1 r = d1 < (T1)d2 ? d1 : (T1)d2;
	return r < d3 ? r : (T1)d3;
}

template <typename T1, typename T2, typename T3, typename T4>
LL_INLINE T1 llmin(T1 d1, T2 d2, T3 d3, T4 d4)
{
	T1 r1 = d1 < (T1)d2 ? d1 : (T1)d2;
	T1 r2 = (T1)d3 < (T1)d4 ? (T1)d3 : (T1)d4;
	return r1 < r2 ? r1 : r2;
}

template <typename T1, typename T2, typename T3>
LL_INLINE T1 llclamp(T1 a, T2 minval, T3 maxval)
{
	if (a < (T1)minval)
	{
		return (T1)minval;
	}
	if (a > (T1)maxval)
	{
		return (T1)maxval;
	}
	return a;
}

template <class T>
LL_INLINE T llclampf(T a)
{
	if (a < (T)0)
	{
		return (T)0;
	}
	if (a > (T)1)
	{
		return (T)1;
	}
	return a;
}

template <class T>
LL_INLINE T llclampb(T a)
{
	if (a < (T)0)
	{
		return (T)0;
	}
	if (a > (T)255)
	{
		return (T)255;
	}
	return a;
}
