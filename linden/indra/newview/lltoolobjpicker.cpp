/**
 * @file lltoolobjpicker.cpp
 * @brief LLToolObjPicker class implementation
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

// LLToolObjPicker is a transient tool, useful for a single object pick.

#include "llviewerprecompiledheaders.h"

#include "lltoolobjpicker.h"

#include "llagent.h"
#include "lltoolmgr.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"

LLToolObjPicker gToolObjPicker;

LLToolObjPicker::LLToolObjPicker()
:	LLTool("ObjPicker", NULL),
	mExitCallback(NULL),
	mExitCallbackData(NULL),
	mPicked(false)
{
}

// Returns true if an object was selected
//virtual
bool LLToolObjPicker::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLView* viewp = gViewerWindowp->getRootView();
	bool handled = viewp->handleMouseDown(x, y, mask);

	mHitObjectID.setNull();

	if (!handled)
	{
		// didn't click in any UI object, so must have clicked in the world
		gViewerWindowp->pickAsync(x, y, mask, pickCallback);
		handled = true;
	}
	else
	{
		if (hasMouseCapture())
		{
			setMouseCapture(false);
		}
		else
		{
			llwarns << "PickerTool doesn't have mouse capture on mouseDown"
					<< llendl;
		}
	}

	// Pass mousedown to base class
	LLTool::handleMouseDown(x, y, mask);

	return handled;
}

//static
void LLToolObjPicker::pickCallback(const LLPickInfo& pick_info)
{
	gToolObjPicker.mHitObjectID = pick_info.mObjectID;
	gToolObjPicker.mPicked = pick_info.mObjectID.notNull();
}

//virtual
bool LLToolObjPicker::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLView* viewp = gViewerWindowp->getRootView();
	// Let the UI handle this
	bool handled = viewp->handleHover(x, y, mask);
	LLTool::handleMouseUp(x, y, mask);
	if (hasMouseCapture())
	{
		setMouseCapture(false);
	}
	else
	{
		llwarns_sparse << "No capture on mouse up" << llendl;
	}
	return handled;
}

//virtual
bool LLToolObjPicker::handleHover(S32 x, S32 y, MASK mask)
{
	LLView* viewp = gViewerWindowp->getRootView();
	bool handled = viewp->handleHover(x, y, mask);
	if (!handled)
	{
		// Used to do pick on hover. Now we just always display the cursor.
		ECursorType cursor = UI_CURSOR_ARROWLOCKED;

		cursor = UI_CURSOR_TOOLPICKOBJECT3;

		gWindowp->setCursor(cursor);
	}
	return handled;
}

//virtual
void LLToolObjPicker::onMouseCaptureLost()
{
	if (mExitCallback)
	{
		mExitCallback(mExitCallbackData);

		mExitCallback = NULL;
		mExitCallbackData = NULL;
	}

	mPicked = false;
	mHitObjectID.setNull();
}


//virtual
void LLToolObjPicker::handleSelect()
{
	LLTool::handleSelect();
	setMouseCapture(true);
}

//virtual
void LLToolObjPicker::handleDeselect()
{
	if (hasMouseCapture())
	{
		LLTool::handleDeselect();
		setMouseCapture(false);
	}
}
