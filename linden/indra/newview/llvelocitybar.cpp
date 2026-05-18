/**
 * @file llvelocitybar.cpp
 * @brief A user interface widget that displays user energy level
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

#include "llviewerprecompiledheaders.h"

#include "llvelocitybar.h"

#include "llgl.h"

#include "llagent.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"

LLVelocityBar* gVelocityBarp = NULL;

constexpr S32 BAR_TOP = 24;
constexpr S32 BAR_BOTTOM = 20;
constexpr S32 TICK_BOTTOM = 15;
constexpr S32 TICK_WIDTH = 2;

static const char* velocity_str = "Velocity %.2fm/s = %.1fkm/h = %.1fkt";
static const std::string labels[] = { "0", "2", "4", "6", "8", "10", "12",
									  "14", "16", "18", "20", "22", "24", "26",
									  "28", "30", "32m/s" };
constexpr F32 units_to_ticks = 0.5f;	// 1 / 2m/s per tick
constexpr S32 labels_size = (S32)LL_ARRAY_SIZE(labels);

LLVelocityBar::LLVelocityBar(const std::string& name)
:	LLView(name, false),
	mFont(LLFontGL::getFontMonospace())
{
	llassert(gVelocityBarp == NULL);

	gVelocityBarp = this;
	setVisible(false);
	setFollowsBottom();
	setFollowsLeft();
	resize();

	if (mFont)
	{
		// We use a constant width (the width for '0') for speed:
		mHalfCharWidth = mFont->getWidth("0") / 2;
	}
}

//virtual
LLVelocityBar::~LLVelocityBar()
{
	gVelocityBarp = NULL;
}

void LLVelocityBar::resize()
{
	mWindowWidth = gViewerWindowp->getVirtualWindowRect().getWidth();
	LLRect r;
	r.setLeftTopAndSize(VELOCITY_LEFT, VELOCITY_TOP,
						mWindowWidth - 2 * VELOCITY_LEFT,
						VELOCITY_HEIGHT);
	setRect(r);
}

//virtual
void LLVelocityBar::draw()
{
	if (gViewerWindowp->getVirtualWindowRect().getWidth() != mWindowWidth)
	{
		resize();
	}

	S32 left, top, right, bottom;
	S32 width = getRect().getWidth();
	F32 velocity = 0.f;
	if (gAgentAvatarp && gAgentAvatarp->mIsSitting)
	{
		// When sitting (presumably on a vehicle), use the speed of the seat.
		LLViewerObject* vehiclep = (LLViewerObject*)gAgentAvatarp->getParent();
		if (vehiclep && vehiclep->flagUsePhysics())
		{
			velocity = vehiclep->getVelocity().length();
		}
		else
		{
			// For non-physical objects, use the camera speed...
			velocity = gViewerCamera.getAverageSpeed();
		}
	}
	else
	{
		velocity = gAgent.getVelocity().length();
	}

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

#if 0
	// Draw a background box
	gGL.color4f(0.f, 0.f, 0.f, 0.25f);
	gl_rect_2d(0, getRect().getHeight(), width, 0);
#endif

	// Color for the scale and text
	LLColor4 color = LLColor4::white;

	// Draw the scale
	top = BAR_BOTTOM - 1;
	bottom = TICK_BOTTOM;
	constexpr S32 intervals = labels_size - 1;
	for (S32 i = 0; i < labels_size; ++i)
	{
		left = i * width / intervals;
		right = left + TICK_WIDTH;
		gl_rect_2d(left, top, right, bottom, color);
	}

	// Draw labels for the bar
	if (mFont)
	{
		top = BAR_TOP + 15;
		left = 0;
		constexpr F32 ms_to_kt = 3600.f / 1852.f;
		mFont->renderUTF8(llformat(velocity_str, velocity, velocity * 3.6f,
								   velocity * ms_to_kt),
						  0, left, top, color, LLFontGL::LEFT, LLFontGL::TOP);
		top = TICK_BOTTOM;
		for (S32 i = 0; i < labels_size; ++i)
		{
			const std::string& label = labels[i];
			left = 1 + i * width / intervals -
				   (S32)label.size() * mHalfCharWidth;
			mFont->renderUTF8(label, 0, left, top, color, LLFontGL::LEFT,
							  LLFontGL::TOP);
		}
	}

	// Draw the speed bar
	right = (S32)(velocity * units_to_ticks * (F32)width / (F32)intervals);
	if (velocity < 4.f)
	{
		color = LLColor4::blue;		// Walking
	}
	else if (velocity < 6.f)
	{
		color = LLColor4::cyan;		// Running
	}
	else if (velocity <= 16.f)
	{
		color = LLColor4::green;	// Flying
	}
	else if (velocity <= 24.f)
	{
		color = LLColor4::yellow;	// Riding/boosted speed
	}
	else if (velocity <= 32.f)
	{
		color = LLColor4::orange;	// Riding/boosted speed
	}
	else
	{
		color = LLColor4::red;		// Out of scale...
		right = width;
	}
	gl_rect_2d(0, BAR_TOP, right, BAR_BOTTOM, color);
}
