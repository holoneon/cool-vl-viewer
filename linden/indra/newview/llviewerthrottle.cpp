/**
 * @file llviewerthrottle.cpp
 * @brief LLViewerThrottle class implementation
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
#include "llviewerprecompiledheaders.h"

#include "llviewerthrottle.h"

#include "llevent.h"
#include "lldatapacker.h"
#include "llmessage.h"

#include "llagent.h"
#include "llgridmanager.h"			// For gIsInSecondLife
#include "llviewercontrol.h"
#include "llviewerstats.h"

// Ignore a bogus clang v15.0 warning
#if CLANG_VERSION >= 150000
# pragma clang diagnostic ignored "-Warray-parameter"
#endif

using namespace LLOldEvents;

// consts

// The viewer is allowed to set the under-the-hood bandwidth to 50% greater
// than the prefs UI shows, under the assumption that the viewer won't
// receive all the different message types at once.
constexpr F32 MAX_FRACTIONAL = 1.5f;
constexpr F32 MIN_FRACTIONAL = 0.2f;

constexpr U32 MIN_BANDWIDTH = 256;
constexpr U32 MAX_BANDWIDTH = 32768;
constexpr F32 STEP_FRACTIONAL = 0.1f;
constexpr F32 HIGH_BUFFER_LOAD_TRESHOLD = 1.f;
constexpr F32 LOW_BUFFER_LOAD_TRESHOLD = 0.8f;
constexpr F32 TIGHTEN_THROTTLE_THRESHOLD = 3.f;	// Packet loss % per second
constexpr F32 EASE_THROTTLE_THRESHOLD = 0.5f;	// Packet loss % per second
constexpr F32 DYNAMIC_UPDATE_DURATION = 5.f;	// In seconds

LLViewerThrottle gViewerThrottle;

// Bandwidth settings for different bit rates, they are interpolated /
// extrapolated.
// The values are for: Resend, Land, Wind, Cloud, Task, Texture, Asset
static const U32 BW_PRESET_50[TC_EOF] = { 5, 10, 3, 3, 10, 10, 9 };
static const U32 BW_PRESET_300[TC_EOF] = { 30, 40, 9, 9, 86, 86, 40 };
static const U32 BW_PRESET_500[TC_EOF] = { 50, 70, 14, 14, 136, 136, 80 };
static const U32 BW_PRESET_1000[TC_EOF] = { 100, 100, 20, 20, 310, 310, 140 };
static const U32 BW_PRESET_2000[TC_EOF] = { 200, 200, 25, 25, 450, 800, 300 };
static const U32 BW_PRESET_10000[TC_EOF] = { 1000, 500, 25, 25, 1450, 5000, 2000 };

LLViewerThrottleGroup::LLViewerThrottleGroup()
{
	for (S32 i = 0; i < TC_EOF; ++i)
	{
		mThrottles[i] = 0;
	}
	mThrottleTotal = 0;
}

LLViewerThrottleGroup::LLViewerThrottleGroup(const U32 settings[])
{
	mThrottleTotal = 0;
	for (S32 i = 0; i < TC_EOF; ++i)
	{
		mThrottles[i] = settings[i];
		mThrottleTotal += settings[i];
	}
}

LLViewerThrottleGroup LLViewerThrottleGroup::operator*(F32 frac) const
{
	LLViewerThrottleGroup res;
	res.mThrottleTotal = 0;

	for (S32 i = 0; i < TC_EOF; ++i)
	{
		res.mThrottles[i] = F32(mThrottles[i]) * frac;
		res.mThrottleTotal += res.mThrottles[i];
	}

	return res;
}

LLViewerThrottleGroup LLViewerThrottleGroup::operator+(const LLViewerThrottleGroup& b) const
{
	LLViewerThrottleGroup res;
	res.mThrottleTotal = 0;

	for (S32 i = 0; i < TC_EOF; i++)
	{
		res.mThrottles[i] = mThrottles[i] + b.mThrottles[i];
		res.mThrottleTotal += res.mThrottles[i];
	}

	return res;
}

LLViewerThrottleGroup LLViewerThrottleGroup::operator-(const LLViewerThrottleGroup& b) const
{
	LLViewerThrottleGroup res;
	res.mThrottleTotal = 0;

	for (S32 i = 0; i < TC_EOF; ++i)
	{
		res.mThrottles[i] = mThrottles[i] - b.mThrottles[i];
		res.mThrottleTotal += res.mThrottles[i];
	}

	return res;
}

void LLViewerThrottleGroup::sendToSim() const
{
	llinfos << "Sending throttle settings, total BW " << mThrottleTotal
			<< llendl;
	LLMessageSystem* msg = gMessageSystemp;

	msg->newMessageFast(_PREHASH_AgentThrottle);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
	msg->addU32Fast(_PREHASH_CircuitCode, msg->mOurCircuitCode);

	msg->nextBlockFast(_PREHASH_Throttle);
	msg->addU32Fast(_PREHASH_GenCounter, 0);

	// Pack up the throttle data
	U8 tmp[64];
	LLDataPackerBinaryBuffer dp(tmp, MAX_THROTTLE_SIZE);
	for (S32 i = 0; i < TC_EOF; ++i)
	{
		// Sim wants BPS, not KBPS
		dp.packF32(F32(mThrottles[i] * 1024), "Throttle");
	}
	S32 len = dp.getCurrentSize();
	msg->addBinaryDataFast(_PREHASH_Throttles, tmp, len);

	gAgent.sendReliableMessage();
}

void LLViewerThrottleGroup::dump()
{
	static const std::string groups[TC_EOF] = {
		"Resend", "Land", "Wind", "Cloud", "Task", "Texture", "Asset"
	};
	for (S32 i = 0; i < TC_EOF; ++i)
	{
		LL_DEBUGS("Throttle") << groups[i] << ": " << mThrottles[i] << LL_ENDL;
	}
	LL_DEBUGS("Throttle") << "Total: " << mThrottleTotal << LL_ENDL;
}

LLViewerThrottle::LLViewerThrottle()
:	mMaxBandwidth(0),
	mCurrentBandwidth(0),
	mBufferLoadRate(0),
	mThrottleFrac(1.f),
	mBWSettingName("ThrottleBandwidthKbps")
{
	// Need to be pushed on in bandwidth order
	mPresets.emplace_back(BW_PRESET_50);
	mPresets.emplace_back(BW_PRESET_300);
	mPresets.emplace_back(BW_PRESET_500);
	mPresets.emplace_back(BW_PRESET_1000);
	mPresets.emplace_back(BW_PRESET_2000);
	mPresets.emplace_back(BW_PRESET_10000);
}

void LLViewerThrottle::setMaxBandwidth(U32 kbps, bool from_event)
{
	if (!from_event)
	{
		kbps = llclamp(kbps, MIN_BANDWIDTH, MAX_BANDWIDTH);
		gSavedSettings.setU32(getSettingName(), kbps);
	}
	gViewerThrottle.load();

	if (gAgent.getRegion())
	{
		gViewerThrottle.sendToSim();
	}
}

void LLViewerThrottle::load()
{
	mBWSettingName = gIsInSecondLife ? "ThrottleBandwidthKbps"
									 : "ThrottleBandwidthKbpsOS";
	U32 curr_value = gSavedSettings.getU32(getSettingName());
	if (curr_value < MIN_BANDWIDTH)
	{
		mMaxBandwidth = MIN_BANDWIDTH * 1024;	// Convert to bps
		gSavedSettings.setU32(getSettingName(), MIN_BANDWIDTH);
	}
	else if (curr_value > MAX_BANDWIDTH)
	{
		mMaxBandwidth = MAX_BANDWIDTH * 1024;	// Convert to bps
		gSavedSettings.setU32(getSettingName(), MAX_BANDWIDTH);
	}
	else
	{
		mMaxBandwidth = curr_value * 1024;	// Convert to bps
	}

	resetDynamicThrottle();
	mCurrent.dump();
}

void LLViewerThrottle::save() const
{
	gSavedSettings.setU32(getSettingName(), mMaxBandwidth / 1024);
}

void LLViewerThrottle::sendToSim() const
{
	mCurrent.sendToSim();
}

LLViewerThrottleGroup LLViewerThrottle::getThrottleGroup(U32 bandwidth_kbps)
{
	// Clamp the bandwidth users can set.
	U32 set_bandwidth = llclamp(bandwidth_kbps, MIN_BANDWIDTH, MAX_BANDWIDTH);

	S32 count = mPresets.size();
	S32 i;
	for (i = 0; i < count; ++i)
	{
		if (mPresets[i].getTotal() > set_bandwidth)
		{
			break;
		}
	}

	if (i == 0)
	{
		// We return the minimum if it is less than the minimum
		return mPresets[0];
	}

	if (i == count)
	{
		// Higher than the highest preset, we extrapolate out based on the
		// last two presets. This allows us to keep certain throttle channels
		// from growing in bandwidth
		F32 delta_bw = set_bandwidth - mPresets[count-1].getTotal();
		LLViewerThrottleGroup delta_throttle = mPresets[count - 1] -
											   mPresets[count - 2];
		F32 delta_total = delta_throttle.getTotal();
		F32 delta_frac = delta_bw / delta_total;
		delta_throttle = delta_throttle * delta_frac;
		return mPresets[count - 1] + delta_throttle;
	}

	// In between two presets, just interpolate
	F32 delta_bw = set_bandwidth - mPresets[i - 1].getTotal();
	LLViewerThrottleGroup delta_throttle = mPresets[i] - mPresets[i - 1];
	F32 delta_total = delta_throttle.getTotal();
	F32 delta_frac = delta_bw / delta_total;
	delta_throttle = delta_throttle * delta_frac;
	return mPresets[i - 1] + delta_throttle;
}

// static
void LLViewerThrottle::resetDynamicThrottle()
{
	mThrottleFrac = MAX_FRACTIONAL;
	mCurrentBandwidth = U32(F32(mMaxBandwidth) * MAX_FRACTIONAL);
	mCurrent = getThrottleGroup(mCurrentBandwidth / 1024);
}

void LLViewerThrottle::updateDynamicThrottle()
{
	mBufferLoadRate = llmax(mBufferLoadRate,
							gMessageSystemp->getBufferLoadRate());

	if (mUpdateTimer.getElapsedTimeF32() < DYNAMIC_UPDATE_DURATION)
	{
		return;
	}
	mUpdateTimer.reset();

	if (gViewerStats.mPacketsLostPercentStat.getMean() >
			TIGHTEN_THROTTLE_THRESHOLD || // Already losing packets
		// Let viewer sort through the backlog before it starts dropping
		// packets.
		mBufferLoadRate >= HIGH_BUFFER_LOAD_TRESHOLD)
	{
		if (mThrottleFrac <= MIN_FRACTIONAL ||
			mCurrentBandwidth / 1024 <= MIN_BANDWIDTH)
		{
			return;
		}
		mThrottleFrac -= STEP_FRACTIONAL;
		mThrottleFrac = llmax(MIN_FRACTIONAL, mThrottleFrac);
		mCurrentBandwidth = mMaxBandwidth * mThrottleFrac;
		mCurrent = getThrottleGroup(mCurrentBandwidth / 1024);
		mCurrent.sendToSim();
		llinfos << "Tightening network throttle to " << mCurrentBandwidth << llendl;
	}
	else if (gViewerStats.mPacketsLostPercentStat.getMean() <=
				EASE_THROTTLE_THRESHOLD &&
			 mBufferLoadRate < LOW_BUFFER_LOAD_TRESHOLD)
	{
		if (mThrottleFrac >= MAX_FRACTIONAL ||
			mCurrentBandwidth / 1024 >= MAX_BANDWIDTH)
		{
			return;
		}
		mThrottleFrac += STEP_FRACTIONAL;
		mThrottleFrac = llmin(MAX_FRACTIONAL, mThrottleFrac);
		mCurrentBandwidth = mMaxBandwidth * mThrottleFrac;
		constexpr F32 TO_KPBS = 1.f / 1024.f;
		mCurrent = getThrottleGroup(F32(mCurrentBandwidth) * TO_KPBS);
		mCurrent.sendToSim();
		llinfos << "Easing network throttle to " << mCurrentBandwidth << llendl;
	}

	mBufferLoadRate = 0;
}
