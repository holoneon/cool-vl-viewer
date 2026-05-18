/**
 * @file llrand.h
 * @brief Information, functions, and typedefs for randomness.
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

#pragma once

#include "llpreprocessor.h"

#include "stdtypes.h"

// *NOTE: The system rand implementation is probably not correct.
#define LL_USE_SYSTEM_RAND 0

/**
 * Use the boost random number generators if you want a stateful
 * random numbers. If you want more random numbers, use the
 * C-functions since they will generate faster/better randomness
 * across the process.
 *
 * I tested some of the boost random engines, and picked a good double
 * generator and a good integer generator. I also took some timings
 * for them on Linux using gcc 3.3.5. The harness also did some other
 * fairly trivial operations to try to limit compiler optimizations,
 * so these numbers are only good for relative comparisons.
 *
 * usec/inter		algorithm
 * 0.21				boost::minstd_rand0
 * 0.039			boost:lagged_fibonacci19937
 * 0.036			boost:lagged_fibonacci607
 * 0.44				boost::hellekalek1995
 * 0.44				boost::ecuyer1988
 * 0.042			boost::rand48
 * 0.043			boost::mt11213b
 * 0.028			stdlib random()
 * 0.05				stdlib lrand48()
 * 0.034			stdlib rand()
 * 0.020			the old & lame LLRand
 */

// Generates a float from [0, RAND_MAX).
S32 ll_rand();

// Generates a float from [0, val) or (val, 0].
S32 ll_rand(S32 val);

// Generates a float from [0, 1.0).
F32 ll_frand();

// Generates a float from [0, val) or (val, 0].
F32 ll_frand(F32 val);

// Generates a double from [0, 1.0).
F64 ll_drand();

// Generates a double from [0, val) or (val, 0].
F64 ll_drand(F64 val);
