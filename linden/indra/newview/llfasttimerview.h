/**
 * @file llfasttimerview.h
 * @brief LLFastTimerView class definition
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

#pragma once

#include "llfasttimer.h"

#if LL_FAST_TIMERS_ENABLED

# include "llfloater.h"
# include "llframetimer.h"

class LLFontGL;

class LLFastTimerView final : public LLFloater
{
protected:
	LOG_CLASS(LLFastTimerView);

public:
	LLFastTimerView(const std::string& name);
	~LLFastTimerView() override;

	void setVisible(bool visible) override;
	void onClose(bool app_quitting) override;

	void draw() override;

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleScrollWheel(S32 x, S32 y, S32 clicks) override;

	S32 getLegendIndex(S32 y);
	F64 getTime(LLFastTimer::EFastTimerType tidx);

private:
	void resize();
	void setDisplayModeText();
	void setCenterModeText();

private:
	LLFrameTimer	mHighlightTimer;
	LLRect			mBarRect;
	S32*			mBarStart;
	S32*			mBarEnd;
	U64				mAvgCountTotal;
	U64				mMaxCountTotal;
	LLFontGL*		mFont;
	LLWString		mCenterModeText;
	LLWString		mDisplayModeText;
	S32				mDisplayModeTextWidth;
	S32				mDisplayMode;
	S32				mDisplayCenter;
	S32				mDisplayCalls;
	S32				mDisplayHz;
	S32				mScrollIndex;
	S32				mHoverIndex;
	S32				mHoverBarIndex;
	S32				mSubtractHidden;
	S32 			mPrintStats;
	S32				mWindowHeight;
	S32				mWindowWidth;
	bool			mFirstDrawLoop;
};

extern LLFastTimerView* gFastTimerViewp;

#endif	// LL_FAST_TIMERS_ENABLED

#if TRACY_ENABLE

# include "llerror.h"

class LLProcessLauncher;

// Purely static class, used to launch the Tracy profiler executable
class HBTracyProfiler
{
protected:
	LOG_CLASS(HBTracyProfiler);

public:
	HBTracyProfiler() = delete;
	~HBTracyProfiler() = delete;

	static bool running();
	static void launch();
	static void detach();
	static void kill();

private:
	static LLProcessLauncher* sProcess;
};

#endif	// TRACY_ENABLE
