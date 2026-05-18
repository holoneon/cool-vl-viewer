/**
 * @file lltool.h
 * @brief LLTool class header file
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

#include "llcoord.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llmousehandler.h"
#include "llvector3.h"
#include "llvector3d.h"

class LLPanel;
class LLToolComposite;
class LLView;
class LLViewerObject;

class LLTool : public LLMouseHandler
{
protected:
	LOG_CLASS(LLTool);

public:
	LLTool(const std::string& name, LLToolComposite* composite = NULL);
	~LLTool() override;

	// *HACK: to support LLFocusMgr
	LL_INLINE virtual bool isView() const override			{ return false; }

	// Virtual functions inherited from LLMouseHandler

	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;

	LL_INLINE bool handleMiddleMouseDown(S32, S32, MASK) override
	{
		// By default, do not handle it
		return false;
	}

	LL_INLINE bool handleMiddleMouseUp(S32, S32, MASK) override
	{
		// By default, do not handle it
		return false;
	}

	LL_INLINE bool handleScrollWheel(S32, S32, S32) override
	{
		// By default, do not handle it
		return false;
	}

	LL_INLINE bool handleDoubleClick(S32, S32, MASK) override
	{
		// By default, do not handle it
		return false;
	}

	LL_INLINE bool handleRightMouseDown(S32, S32, MASK) override
	{
		// By default, do not handle it
		return false;
	}

	LL_INLINE bool handleRightMouseUp(S32, S32, MASK) override
	{
		// By default, do not handle it
		return false;
	}

	LL_INLINE bool handleToolTip(S32, S32, std::string&, LLRect*) override
	{
		// By default, do not handle it
		return false;
	}

	// Tools should permit tips even when the mouse is down, as that is pretty
	// normal for tools
	LL_INLINE EShowToolTip getShowToolTip() override		{ return SHOW_ALWAYS; }

	LL_INLINE void screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x,
									  S32* local_y) const override
	{
		*local_x = screen_x;
		*local_y = screen_y;
	}

	LL_INLINE void localPointToScreen(S32 local_x, S32 local_y, S32* screen_x,
									  S32* screen_y) const override
	{
		*screen_x = local_x;
		*screen_y = local_y;
	}

	LL_INLINE std::string getName() const override			{ return mName; }

	// New virtual functions

	// Override to return true whenever this tool is meant to edit objects.
	// Used by LLFloaterTools. HB
	LL_INLINE virtual bool isObjectEditTool() const			{ return false; }

	LL_INLINE virtual LLViewerObject* getEditingObject()	{ return NULL; }
	LL_INLINE virtual LLVector3d getEditingPointGlobal()	{ return LLVector3d(); }
	LL_INLINE virtual bool isEditing()						{ return getEditingObject() != NULL; }
	LL_INLINE virtual void stopEditing()					{}

	LL_INLINE virtual bool clipMouseWhenDown()				{ return true; }

	// Does stuff when your tool is selected
	LL_INLINE virtual void handleSelect()					{}
	// Cleans up when your tool is deselected
	LL_INLINE virtual void handleDeselect()					{}

	virtual LLTool* getOverrideTool(MASK mask);

	// Returns true if this is a tool that should always be rendered regardless
	// of selection.
	LL_INLINE virtual bool isAlwaysRendered()				{ return false; }

	// Draws tool specific 3D content in world
	LL_INLINE virtual void render()							{}

	// Draws tool specific 2D overlay
	LL_INLINE virtual void draw()							{}

	LL_INLINE virtual bool handleKey(KEY key, MASK mask)	{ return false; }

	// Note: NOT virtual. Subclasses should call this version.
	void setMouseCapture(bool b);

	LL_INLINE bool hasMouseCapture() override
	{
		return gFocusMgr.getMouseCapture() ==
					(mComposite ? (LLTool*)mComposite : this);
	}

	// Override this one as needed.
	LL_INLINE void onMouseCaptureLost() override			{}

protected:
	// Composite will handle mouse captures.
	LLToolComposite*			mComposite;

	std::string					mName;

public:
	static const std::string	sNameNull;
};
