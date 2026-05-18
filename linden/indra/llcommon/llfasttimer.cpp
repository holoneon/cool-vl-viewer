/**
 * @file llfasttimer.cpp
 * @brief Implementation of the fast timer.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llfasttimer.h"

#if LL_FAST_TIMERS_ENABLED

#include "llsys.h"

// Statics member variables
LLFastTimer::EFastTimerType LLFastTimer::sCurType = LLFastTimer::FTM_OTHER;
LLFastTimer::EFastTimerType LLFastTimer::sType[FTM_MAX_DEPTH];
S32 LLFastTimer::sCurDepth = 0;
U64 LLFastTimer::sStart[FTM_MAX_DEPTH];
U64 LLFastTimer::sCounter[LLFastTimer::FTM_NUM_TYPES];
U64 LLFastTimer::sCountHistory[FTM_HISTORY_NUM][LLFastTimer::FTM_NUM_TYPES];
U64 LLFastTimer::sCountAverage[LLFastTimer::FTM_NUM_TYPES];
U64 LLFastTimer::sCalls[LLFastTimer::FTM_NUM_TYPES];
U64 LLFastTimer::sCallHistory[FTM_HISTORY_NUM][LLFastTimer::FTM_NUM_TYPES];
U64 LLFastTimer::sCallAverage[LLFastTimer::FTM_NUM_TYPES];
S32 LLFastTimer::sCurFrameIndex = -1;
S32 LLFastTimer::sLastFrameIndex = -1;
bool LLFastTimer::sPauseHistory = false;
bool LLFastTimer::sResetHistory = false;
bool LLFastTimer::sFastTimersEnabled = true;
U64 LLFastTimer::sClockResolution = 0;

//static
void LLFastTimer::reset()
{
	// Good place to calculate the clock frequency
	if (sClockResolution == 0)
	{
#if LL_FASTTIMER_USE_RDTSC
		// getCPUFrequency returns MHz and sClockResolution must be in Hz
		sClockResolution =
			U64(LLCPUInfo::getInstance()->getMHz() * 1000000.0);
#else
		sClockResolution = SEC_TO_MICROSEC_U64;
#endif
	}

	if (!sFastTimersEnabled)
	{
		return;
	}

	if (sCurDepth != 0)
	{
		llerrs << "Reset when sCurDepth != 0 (" << sCurDepth
			   << ") with type number " << sCurType << llendl;
	}
	if (sPauseHistory)
	{
		sResetHistory = true;
	}
	else if (sResetHistory)
	{
		sCurFrameIndex = -1;
		sResetHistory = false;
	}
	else if (sCurFrameIndex >= 0)
	{
		S32 hidx = sCurFrameIndex % FTM_HISTORY_NUM;
		for (S32 i = 0; i < FTM_NUM_TYPES; ++i)
		{
			sCountHistory[hidx][i] = sCounter[i];
			sCountAverage[i] = (sCountAverage[i] * sCurFrameIndex +
								sCounter[i]) / (sCurFrameIndex + 1);
			sCallHistory[hidx][i] = sCalls[i];
			sCallAverage[i] = (sCallAverage[i] * sCurFrameIndex +
							   sCalls[i]) / (sCurFrameIndex + 1);
		}
		sLastFrameIndex = sCurFrameIndex;
	}
	else
	{
		for (S32 i = 0; i < FTM_NUM_TYPES; ++i)
		{
			sCountAverage[i] = 0;
			sCallAverage[i] = 0;
		}
	}

	++sCurFrameIndex;

	for (S32 i = 0; i < FTM_NUM_TYPES; ++i)
	{
		sCounter[i] = 0;
		sCalls[i] = 0;
	}
	sCurDepth = 0;
}

//static
void LLFastTimer::enabledFastTimers(bool enable)
{
	sFastTimersEnabled = enable;
	if (!enable)
	{
		sCurType = FTM_OTHER;
	}
}

#if LL_FAST_TIMERS_CHECK_MAX_DEPTH
S32 LLFastTimer::sMaxDepth = 0;

//static
void LLFastTimer::checkMaxDepth()
{
	if (sCurDepth > sMaxDepth)
	{
		sMaxDepth = sCurDepth;
		if (sMaxDepth > FTM_MAX_DEPTH)
		{
			llwarns << "Fast timers configured max depth is too small !"
					<< llendl;
		}
		llinfos << "Fast timers new max depth = " << sMaxDepth << llendl;
	}
}
#endif

#else	// LL_FAST_TIMERS_ENABLED

// To avoid the LNK4221 warning under Windows while compiling...
# if LL_WINDOWS && !LL_CLANG
namespace
{
	void* dummy;
}
# endif

#endif	// LL_FAST_TIMERS_ENABLED
