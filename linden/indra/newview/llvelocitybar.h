/** 
 * @file llvelocitybar.h
 * @brief A user interface widget that displays the user velocity
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

#include "llview.h"

// Initial rectangle
constexpr S32 VELOCITY_LEFT = 20;
constexpr S32 VELOCITY_TOP = 140;
constexpr S32 VELOCITY_HEIGHT = 45;

class LLFontGL;

class LLVelocityBar final : public LLView
{
protected:

public:
	LLVelocityBar(const std::string& name);
	~LLVelocityBar() override;

	void draw() override;

private:
	void resize();

private:
	LLFontGL*	mFont;
	S32			mWindowWidth;
	S32			mHalfCharWidth;
};

extern LLVelocityBar* gVelocityBarp;
