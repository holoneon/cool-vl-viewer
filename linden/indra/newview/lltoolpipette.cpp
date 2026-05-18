/** 
 * @file lltoolpipette.cpp
 * @brief LLToolPipette class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

/**
 * A tool to pick texture entry info from objects in world (color/texture)
 */

#include "llviewerprecompiledheaders.h"

#include "lltoolpipette.h" 

#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"

LLToolPipette gToolPipette;

LLToolPipette::LLToolPipette()
:	LLTool("Pipette"),
	mSelectCallback(NULL),
	mUserData(NULL),
	mSuccess(true)
{ 
}

bool LLToolPipette::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mSuccess = true;
	mTooltipMsg.clear();
	setMouseCapture(true);
	gViewerWindowp->pickAsync(x, y, mask, pickCallback);
	return true;
}

bool LLToolPipette::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mSuccess = true;
	gSelectMgr.unhighlightAll();
	// *NOTE: This assumes the pipette tool is a transient tool.
	gToolMgr.clearTransientTool();
	setMouseCapture(false);
	return true;
}

bool LLToolPipette::handleHover(S32 x, S32 y, MASK mask)
{
	gViewerWindowp->setCursor(mSuccess ? UI_CURSOR_PIPETTE : UI_CURSOR_NO);
	if (hasMouseCapture()) // mouse button is down
	{
		gViewerWindowp->pickAsync(x, y, mask, pickCallback);
		return true;
	}
	return false;
}

bool LLToolPipette::handleToolTip(S32 x, S32 y, std::string& msg,
								  LLRect* sticky_rect_screen)
{
	if (mTooltipMsg.empty())
	{
		return false;
	}
	// Keep tooltip message up when mouse in this part of screen
	sticky_rect_screen->setCenterAndSize(x, y, 20, 20);
	msg = mTooltipMsg;
	return true;
}

void LLToolPipette::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj	= pick_info.getObject();
	gSelectMgr.unhighlightAll();

	// If we clicked on a face of a valid prim, save off texture entry data
	if (hit_obj && 
		hit_obj->getPCode() == LL_PCODE_VOLUME &&
		pick_info.mObjectFace != -1)
	{
		// *TODO: this should highlight the selected face only
		gSelectMgr.highlightObjectOnly(hit_obj);
		gToolPipette.mTextureEntry = *hit_obj->getTE(pick_info.mObjectFace);
		if (gToolPipette.mSelectCallback)
		{
			gToolPipette.mSelectCallback(gToolPipette.mTextureEntry,
										 gToolPipette.mUserData);
		}
	}
}

void LLToolPipette::setSelectCallback(select_callback callback, void* user_data)
{
	mSelectCallback = callback;
	mUserData = user_data;
}

void LLToolPipette::setResult(bool success, const std::string& msg)
{
	mTooltipMsg = msg;
	mSuccess = success;
}
