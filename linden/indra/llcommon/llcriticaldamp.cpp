/**
 * @file llcriticaldamp.cpp
 * @brief Implementation of the critical damping functionality.
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

#include "linden_common.h"

#include "llcriticaldamp.h"

// Static members
LLFrameTimer LLCriticalDamp::sInternalTimer;
std::map<F32, F32> LLCriticalDamp::sInterpolants;
F32 LLCriticalDamp::sTimeDelta = 0.f;

//static
void LLCriticalDamp::updateInterpolants()
{
	sTimeDelta = sInternalTimer.getElapsedTimeAndResetF32();

	for (std::map<F32, F32>::iterator it = sInterpolants.begin(),
									  end = sInterpolants.end();
		 it != end; ++it)
	{
		it->second = llclamp(1.f - powf(2.f, -sTimeDelta / it->first),
							 0.f, 1.f);
	}
}

//static
F32 LLCriticalDamp::getInterpolant(F32 time_constant, bool use_cache)
{
	if (time_constant == 0.f)
	{
		return 1.f;
	}

	if (use_cache)
	{
		std::map<F32, F32>::iterator it = sInterpolants.find(time_constant);
		if (it != sInterpolants.end())
		{
			return it->second;
		}
	}

	F32 interpolant = llclamp(1.f - powf(2.f, -sTimeDelta / time_constant),
							  0.f, 1.f);
	if (use_cache)
	{
		sInterpolants[time_constant] = interpolant;
	}

	return interpolant;
}
