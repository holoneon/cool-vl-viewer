/**
 * @file lltoolview.h
 * @brief UI container for tools.
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

#include "llpanel.h"

class LLButton;
class LLTool;
class LLToolView;

// Utility class, container for the package of information we need about
// each tool.  The package can either point directly to a tool, or indirectly
// to another view of tools.
class LLToolContainer
{
public:
	LLToolContainer(LLToolView* parent);
	~LLToolContainer();

public:
	LLToolView*	mParent;		// LLToolView that owns this container
	LLButton*	mButton;
	LLPanel*	mPanel;
	LLTool*		mTool;			// If not NULL, this is a tool ref
};

// A view containing automatically arranged button icons representing tools.
// The icons sit on top of panels containing options for each tool.
class LLToolView : public LLView
{
public:
	LLToolView(const std::string& name, const LLRect& rect);
	~LLToolView();

	// Handles juggling tool button highlights, panel visibility
	virtual void draw();

	static void onClickToolButton(void* container);

	LLView* getCurrentHoverView();

private:
	// Returns rect for button to add, zero-based index
	LLRect getButtonRect(S32 button_index);

	LLToolContainer* findToolContainer(LLTool* tool);

private:
	typedef std::vector<LLToolContainer*> contain_list_t;
	contain_list_t 	mContainList;
	S32				mButtonCount;	// Used to compute rectangles
};
