/**
 * @file lldebugview.cpp
 * @brief A view containing UI elements only visible in build mode.
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

#include "lldebugview.h"

#include "llconsole.h"

#include "llfasttimerview.h"
#include "lltextureview.h"
#include "llvelocitybar.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

// Instance created in LLViewerWindow::initBase()
LLDebugView* gDebugViewp = NULL;

LLDebugView::LLDebugView(const std::string& name, const LLRect& rect)
:	LLView(name, rect, false)
{
	LLRect r(CONSOLE_PADDING_LEFT, rect.getHeight() - 100,
			 rect.getWidth() - CONSOLE_PADDING_RIGHT, 100);
	mDebugConsolep = new LLConsole("Debug console", r, -1,
								   gSavedSettings.getU32("DebugConsoleMaxLines"),
								   0.f);
	if (mDebugConsolep)
	{
		mDebugConsolep->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT |
								   FOLLOWS_BOTTOM);
		mDebugConsolep->setVisible(false);
		addChild(mDebugConsolep);
	}
	else
	{
		llwarns << "Unable to initialize the debug console !" << llendl;
	}

#if LL_FAST_TIMERS_ENABLED
	if (new LLFastTimerView("Fast timers"))
	{
		addChild(gFastTimerViewp);
	}
	else
	{
		llwarns << "Unable to initialize the fast timers view !" << llendl;
	}
#endif

	if (new LLTextureView("Texture view"))
	{
		addChild(gTextureViewp);
	}
	else
	{
		llwarns << "Unable to initialize the texture console !" << llendl;
	}

	if (new LLVelocityBar("Velocity bar"))
	{
		addChild(gVelocityBarp);
	}
	else
	{
		llwarns << "Unable to initialize the velocity bar !" << llendl;
	}
}

LLDebugView::~LLDebugView()
{
	gDebugViewp = NULL;
}
