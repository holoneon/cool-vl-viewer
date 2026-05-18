/**
 * @file llprogressview.h
 * @brief LLProgressView class definition
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

#include "llframetimer.h"
#include "llpanel.h"

class LLButton;
class LLProgressBar;
class LLTextBox;

class LLProgressView final : public LLPanel
{
public:
	LLProgressView(const std::string& name, const LLRect& rect);
	~LLProgressView() override;

	bool postBuild() override;

	void draw() override;

	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleKeyHere(KEY key, MASK mask) override;
	void setVisible(bool visible) override;

	void setText(const std::string& text);
	void setPercent(F32 percent);

	void setMessage(const std::string& msg);

	void setCancelButtonVisible(bool b, const std::string& label);

	static void onCancelButtonClicked(void*);
	static void onClickMessage(void*);

protected:
	std::string				mMessage;
	LLButton*				mCancelBtn;
	LLTextBox*				mProgressText;
	LLTextBox*				mMessageText;
	LLProgressBar*			mProgressBar;
	F32						mPercentDone;
	LLRect					mOutlineRect;
	LLFrameTimer			mFadeTimer;
	LLFrameTimer			mProgressTimer;
	bool					mMouseDownInActiveArea;
	bool					mURLInMessage;

	static LLProgressView*	sInstance;
};

extern S32 gStartImageWidth;
extern S32 gStartImageHeight;
