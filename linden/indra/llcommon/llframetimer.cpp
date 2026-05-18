/**
 * @file llframetimer.cpp
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

#include "llcommonmath.h"

#include "llframetimer.h"

// Static members
U64 LLFrameTimer::sStartTotalTime = LLTimer::totalTime();
F64 LLFrameTimer::sFrameTime = 0.0;
U64 LLFrameTimer::sTotalTime = 0;
F64 LLFrameTimer::sTotalSeconds = 0.0;
S32 LLFrameTimer::sFrameCount = 0;
U64 LLFrameTimer::sFrameDeltaTime = 0;

constexpr F64 USEC_TO_SEC_F64 = 0.000001;

//static
void LLFrameTimer::stepFrame()
{
	U64 total_time = LLTimer::totalTime();
	if (total_time < sTotalTime)
	{
		llwarns << "Clock went backwards. Adjusting start time accordingly."
				<< llendl;
		sFrameDeltaTime = 0;
		// Let's approximate the adjusted start time +/- last frame time
		sStartTotalTime += total_time;
		sStartTotalTime -= sTotalTime;
	}
	else
	{
		sFrameDeltaTime = total_time - sTotalTime;
	}
	sTotalTime = total_time;
	sTotalSeconds = U64_to_F64(sTotalTime) * USEC_TO_SEC_F64;
	sFrameTime = U64_to_F64(sTotalTime - sStartTotalTime) * USEC_TO_SEC_F64;

	++sFrameCount;
}

void LLFrameTimer::setExpiryAt(F64 seconds_since_epoch)
{
	mStartTime = sFrameTime;
	mExpiry = seconds_since_epoch - (USEC_TO_SEC_F64 * sStartTotalTime);
}

F64 LLFrameTimer::expiresAt() const
{
	F64 expires_at = U64_to_F64(sStartTotalTime) * USEC_TO_SEC_F64;
	expires_at += mExpiry;
	return expires_at;
}

bool LLFrameTimer::checkExpirationAndReset(F32 expiration)
{
	if (hasExpired())
	{
		reset();
		setTimerExpirySec(expiration);
		return true;
	}
	return false;
}

//static
F32 LLFrameTimer::getFrameDeltaTimeF32()
{
	return (F32)(U64_to_F64(sFrameDeltaTime) * USEC_TO_SEC_F64);
}

// Return seconds since the current frame started
//static
F32 LLFrameTimer::getCurrentFrameTime()
{
	U64 frame_time = LLTimer::totalTime() - sTotalTime;
	return (F32)(U64_to_F64(frame_time) * USEC_TO_SEC_F64);
}

// Glue code to avoid full class .h file #includes
F32 getCurrentFrameTime()
{
	return (F32)(LLFrameTimer::getCurrentFrameTime());
}
