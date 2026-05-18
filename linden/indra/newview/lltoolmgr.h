/**
 * @file lltoolmgr.h
 * @brief LLToolMgr class header file
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

#include "llkeyboard.h"

class LLTool;
class LLToolset;

// Key bindings for common operations
const MASK MASK_VERTICAL	= MASK_CONTROL;
const MASK MASK_SPIN		= MASK_CONTROL | MASK_SHIFT;
const MASK MASK_ZOOM		= MASK_NONE;
const MASK MASK_ORBIT		= MASK_CONTROL;
const MASK MASK_PAN			= MASK_CONTROL | MASK_SHIFT;
const MASK MASK_COPY		= MASK_SHIFT;

class LLToolMgr
{
protected:
	LOG_CLASS(LLToolMgr);

public:
	LLToolMgr();
	~LLToolMgr();

	// Must be called after gSavedSettings set up.
	void initTools();

	// Returns active tool, taking into account keyboard state
	LLTool* getCurrentTool();

	LL_INLINE bool isCurrentTool(LLTool* tool)
	{
		return tool == getCurrentTool();
	}

	// Returns active tool when overrides are deactivated
	LL_INLINE LLTool* getBaseTool()				{ return mBaseTool; }

	bool inEdit();
	void toggleBuildMode();
	// Determines if we are in Build mode or not
	bool inBuildMode();

	void setTransientTool(LLTool* tool);
	void clearTransientTool();
	LL_INLINE bool usingTransientTool()			{ return mTransientTool != NULL; }


	void setCurrentToolset(LLToolset* current);
	LL_INLINE LLToolset* getCurrentToolset()	{ return mCurrentToolset; }

	void onAppFocusGained();
	void onAppFocusLost();

	LL_INLINE void clearSavedTool()				{ mSavedTool = NULL; }

protected:
	friend class LLToolset;	// To allow access to setCurrentTool();

	void setCurrentTool(LLTool* tool);

	// Calls getcurrenttool() to calculate active tool and call handleSelect()
	// and handleDeselect() immediately when active tool changes
	LL_INLINE void	updateToolStatus()			{ getCurrentTool(); }

protected:
	LLTool*		mBaseTool;
	// The current tool at the time application focus was lost:
	LLTool*		mSavedTool;
	LLTool*		mTransientTool;
	LLTool*		mOverrideTool;	// Tool triggered by keyboard override
	LLTool*		mSelectedTool;	// Last known active tool
	LLToolset*	mCurrentToolset;
};

// Sets of tools for various modes
class LLToolset
{
public:
	LL_INLINE LLToolset()
	:	mSelectedTool(NULL)
	{
	}

	LL_INLINE LLTool* getSelectedTool()			{ return mSelectedTool; }

	void addTool(LLTool* tool);

	void selectTool(LLTool* tool);
	void selectToolByIndex(U32 index);
	LL_INLINE void selectFirstTool()			{ selectToolByIndex(0); }
	void selectNextTool();
	void selectPrevTool();

	void handleScrollWheel(S32 clicks);

#if 0	// Not used
	LL_INLINE bool isToolSelected(U32 idx)
	{
		return idx < (U32)mToolList.size() && mToolList[idx] == mSelectedTool;
	}
#endif

protected:
	LLTool*		mSelectedTool;

	typedef std::vector<LLTool*> tool_list_t;
	tool_list_t	mToolList;
};

// Globals
extern LLToolMgr	gToolMgr;

extern LLTool*		gToolNull;
extern LLToolset*	gBasicToolset;
extern LLToolset*	gCameraToolset;
extern LLToolset*	gMouselookToolset;
extern LLToolset*	gFaceEditToolset;
