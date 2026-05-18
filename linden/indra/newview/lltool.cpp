/**
 * @file lltool.cpp
 * @brief LLTool class implementation
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

#include "lltool.h"

#include "llview.h"
#include "llwindow.h"				// For gDebugClicks

#include "llagent.h"
#include "lltoolcomp.h"
#include "lltoolfocus.h"
#include "llviewerjoystick.h"
#include "llviewerwindow.h"

//static
const std::string LLTool::sNameNull("null");

LLTool::LLTool(const std::string& name, LLToolComposite* composite)
:	mComposite(composite),
	mName(name)
{
}

LLTool::~LLTool()
{
	if (hasMouseCapture())
	{
		llwarns << "Tool deleted holding mouse capture. Mouse capture removed."
				<< llendl;
		gFocusMgr.removeMouseCaptureWithoutCallback(this);
	}
}

bool LLTool::handleHover(S32 x, S32 y, MASK mask)
{
	gWindowp->setCursor(UI_CURSOR_ARROW);
	LL_DEBUGS("UserInput") << "hover handled by a tool" << LL_ENDL;
	// by default, do nothing, say we handled it
	return true;
}

bool LLTool::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		llinfos << "Left mouse down" << llendl;
	}
	// by default, didn't handle it
	gAgent.setControlFlags(AGENT_CONTROL_LBUTTON_DOWN);
	return true;
}

bool LLTool::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		llinfos << "Left mouse up" << llendl;
	}
	// by default, didn't handle it
	gAgent.setControlFlags(AGENT_CONTROL_LBUTTON_UP);
	return true;
}

void LLTool::setMouseCapture(bool b)
{
	if (b)
	{
		gFocusMgr.setMouseCapture(mComposite ? mComposite : this);
	}
	else if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}
}

LLTool* LLTool::getOverrideTool(MASK mask)
{
	// NOTE: if in flycam mode, ALT-ZOOM camera should be disabled
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return NULL;
	}
	if (mask & MASK_ALT)
	{
		return &gToolFocus;
	}
	return NULL;
}
