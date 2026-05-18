/**
 * @file llhudview.cpp
 * @brief 2D HUD overlay
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llhudview.h"

#include "lltracker.h"

// Instance created in LLViewerWindow::initBase()
LLHUDView* gHUDViewp = NULL;

LLHUDView::LLHUDView(const std::string& name, const LLRect& rect)
:	LLView(name, rect, false)
{
}

LLHUDView::~LLHUDView()
{
	gHUDViewp = NULL;
}

//virtual
void LLHUDView::draw()
{
	gTracker.drawHUDArrow();
	LLView::draw();
}

//virtual
bool LLHUDView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (gTracker.handleMouseDown(x, y))
	{
		return true;
	}
	return LLView::handleMouseDown(x, y, mask);
}

const LLColor4& LLHUDView::colorFromType(S32 type)
{
	return type == 0 ? LLColor4::green : LLColor4::black;
}
