/**
 * @file lleventtimer.cpp
 * @brief Cross-platform objects for doing timing
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "lleventtimer.h"

#include "lldate.h"
#include "lltimer.h"

LLEventTimer::LLEventTimer(const LLDate& time)
:	mPeriod(F32(time.secondsSinceEpoch() - LLTimer::getEpochSeconds()))
{
}

//static
void LLEventTimer::stepFrame()
{
	std::vector<LLEventTimer*> completed_timers;

	for (auto& timer : instance_snapshot())
	{
		F32 et = timer.mEventTimer.getElapsedTimeF32();
		if (timer.mEventTimer.getStarted() && et > timer.mPeriod)
		{
			timer.mEventTimer.reset();
			if (timer.tick())
			{
				completed_timers.push_back(&timer);
			}
		}
	}

	for (U32 i = 0, count = completed_timers.size(); i < count; ++i)
	{
		delete completed_timers[i];
	}
}
