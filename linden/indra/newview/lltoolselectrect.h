/**
 * @file lltoolselectrect.h
 * @brief A tool to select multiple objects with a screen-space rectangle.
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

#include "lltool.h"
#include "lltoolselect.h"

class LLToolSelectRect : public LLToolSelect
{
protected:
	LOG_CLASS(LLToolSelectRect);

public:
	LLToolSelectRect(LLToolComposite* composite);

	virtual bool handleMouseDown(S32 x, S32 y, MASK mask);
	virtual bool handleMouseUp(S32 x, S32 y, MASK mask);
	virtual bool handleHover(S32 x, S32 y, MASK mask);

	// Draws the selection rectangle
	virtual void draw();

	void handlePick(const LLPickInfo& pick);

protected:
	void handleRectangleSelection(S32 x, S32 y, MASK mask);
	bool outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);

protected:
	S32		mDragStartX;		// Screen coords, from left
	S32		mDragStartY;		// Screen coords, from bottom

	S32		mDragEndX;
	S32		mDragEndY;

	S32		mDragLastWidth;
	S32		mDragLastHeight;

	bool	mMouseOutsideSlop;	// Has mouse ever gone outside slop region ?
};
