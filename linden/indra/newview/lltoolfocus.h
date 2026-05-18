/**
 * @file lltoolfocus.h
 * @brief A tool to set the build focus point.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "lltool.h"

class LLPickInfo;

class LLToolFocus final : public LLTool
{
protected:
	LOG_CLASS(LLToolFocus);

public:
	LLToolFocus();

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;

	void onMouseCaptureLost() override;

	void handleSelect() override;
	void handleDeselect() override;

	LLTool*	getOverrideTool(MASK mask) override		{ return NULL; }

	static void pickCallback(const LLPickInfo& pick_info);
	bool mouseSteerMode()							{ return mMouseSteering; }

protected:
	// Called from handleMouseUp and onMouseCaptureLost to "let go" of the
	// mouse and make it visible. JC
	void releaseMouse();

protected:
	S32		mAccumX;
	S32		mAccumY;
	S32		mMouseDownX;
	S32		mMouseDownY;
	S32		mMouseUpX;		// Needed for releaseMouse()
	S32		mMouseUpY;
	MASK	mMouseUpMask;
	bool	mOutsideSlopX;
	bool	mOutsideSlopY;
	bool	mValidClickPoint;
	bool	mMouseSteering;
};

extern LLToolFocus gToolFocus;
extern bool gCameraBtnOrbit;
extern bool gCameraBtnPan;
extern bool gCameraBtnZoom;
