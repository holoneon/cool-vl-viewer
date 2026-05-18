/**
 * @file llviewerthrottle.h
 * @brief LLViewerThrottle class header file
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

#pragma once

#include <vector>

#include "llframetimer.h"
#include "llstring.h"
#include "llthrottle.h"

class LLViewerThrottleGroup
{
protected:
	LOG_CLASS(LLViewerThrottleGroup);

public:
	LLViewerThrottleGroup();
	LLViewerThrottleGroup(const U32 settings[TC_EOF]);

	LLViewerThrottleGroup operator*(F32 frac) const;
	LLViewerThrottleGroup operator+(const LLViewerThrottleGroup& b) const;
	LLViewerThrottleGroup operator-(const LLViewerThrottleGroup& b) const;

	LL_INLINE U32 getTotal()					{ return mThrottleTotal; }
	void sendToSim() const;

	void dump();

protected:
	U32 mThrottles[TC_EOF];
	U32 mThrottleTotal;
};

class LLViewerThrottle
{
protected:
	LOG_CLASS(LLViewerThrottle);

public:
	LLViewerThrottle();

	void setMaxBandwidth(U32 kbps, bool from_event = false);

	void load();
	void save() const;
	void sendToSim() const;

	LL_INLINE U32 getMaxBandwidth() const		{ return mMaxBandwidth; }
	LL_INLINE U32 getCurrentBandwidth() const	{ return mCurrentBandwidth; }

	void updateDynamicThrottle();
	void resetDynamicThrottle();

	LLViewerThrottleGroup getThrottleGroup(U32 bandwidth_kbps);

	LL_INLINE const char* getSettingName() const
	{
		return mBWSettingName.c_str();
	}

protected:
	LLViewerThrottleGroup				mCurrent;
	LLFrameTimer						mUpdateTimer;
	std::string							mBWSettingName;
	std::vector<LLViewerThrottleGroup>	mPresets;
	U32									mMaxBandwidth;
	U32									mCurrentBandwidth;
	F32									mBufferLoadRate;
	F32									mThrottleFrac;
};

extern LLViewerThrottle gViewerThrottle;
