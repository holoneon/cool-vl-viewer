/**
 * @file lltoolpie.h
 * @brief LLToolPie class header file
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

#include "llsafehandle.h"
#include "lluuid.h"

#include "lltool.h"
#include "llviewerwindow.h" // for LLPickInfo

class LLViewerObject;
class LLObjectSelection;

class LLToolPie final : public LLTool
{
protected:
	LOG_CLASS(LLToolPie);

public:
	LLToolPie();

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleRightMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleDoubleClick(S32 x, S32 y, MASK mask) override;
	bool handleScrollWheel(S32 x, S32 y, S32 clicks) override;

	LL_INLINE void render() override					{}

	void stopEditing() override;

	LL_INLINE void onMouseCaptureLost() override		{}
	void handleDeselect() override;

	LLTool* getOverrideTool(MASK mask) override;

	LL_INLINE LLPickInfo& getPick()						{ return mPick; }
	LL_INLINE U8 getClickAction()						{ return mClickAction; }
	LL_INLINE LLViewerObject* getClickActionObject()	{ return mClickActionObject; }

	LL_INLINE LLObjectSelection* getLeftClickSelection()
	{
		return (LLObjectSelection*)mLeftClickSelection;
	}

	void resetSelection();

	static void leftMouseCallback(const LLPickInfo& pick_info);
	static void rightMouseCallback(const LLPickInfo& pick_info);

	static void selectionPropertiesReceived();


private:
	bool handleLeftClickPick();
	bool handleRightClickPick();

	bool useClickAction(MASK mask, LLViewerObject* object,
						LLViewerObject* parent);

	bool handleMediaClick(const LLPickInfo& info);
	bool handleMediaDblClick(const LLPickInfo& info);
	bool handleMediaHover(const LLPickInfo& info);

private:
	LLPointer<LLViewerObject>		mClickActionObject;
	LLSafeHandle<LLObjectSelection>	mLeftClickSelection;
	LLPickInfo						mPick;
	U8								mClickAction;
	bool							mPieMouseButtonDown;
	bool							mGrabMouseButtonDown;
};

extern LLToolPie gToolPie;
