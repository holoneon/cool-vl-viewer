/**
 * @file lleventtimer.h
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

#pragma once

#include "llinstancetracker.h"
#include "lltimer.h"

class LLDate;

// Class for scheduling a function to be called periodically (the timing is
// imprecise since it is conditionned by the duration of each frame).

class LLEventTimer : public LLInstanceTracker<LLEventTimer>
{
	// This class is the only one that should be allowed to call stepFrame()
	friend class LLApp;

public:
	// Period is the amount of time between each call to tick() in seconds
	LL_INLINE LLEventTimer(F32 period)
	:	mPeriod(period)
	{
	}

	LLEventTimer(const LLDate& time);

	// Method to be called at the supplied frequency. Normally returns false;
	// true will delete the timer after the method returns.
	virtual bool tick() = 0;

private:
	// Called exclusively by LLApp::stepFrame()
	static void stepFrame();

protected:
	LLTimer	mEventTimer;
	F32		mPeriod;
};
