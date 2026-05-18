/**
 * @file lltoolselectland.h
 * @brief LLToolSelectLand class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llvector3d.h"

#include "lltool.h"

class LLParcelSelection;

class LLToolSelectLand final : public LLTool
{
protected:
	LOG_CLASS(LLToolSelectLand);

public:
	LLToolSelectLand();

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleDoubleClick(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;

	// Draws the selection rectangle
	void render() override;
	bool isAlwaysRendered() override			{ return true; }

	void handleSelect() override;
	void handleDeselect() override;

protected:
	bool outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);
	void roundXY(LLVector3d& vec);

protected:
	// Holds on to a parcel selection
	LLSafeHandle<LLParcelSelection>	mSelection;

	// Global coords
	LLVector3d						mDragStartGlobal;
	LLVector3d						mDragEndGlobal;
	// Global coords, from drag
	LLVector3d						mWestSouthBottom;
	LLVector3d						mEastNorthTop;

	// Screen coord, from left
	S32								mDragStartX;
	// Screen coord, from bottom
	S32								mDragStartY;
	// Screen coord, from drag
	S32								mDragEndX;
	S32								mDragEndY;

	// Is drag end a valid point in the world ?
	bool							mDragEndValid;
	// Has mouse ever gone outside slop region ?
	bool							mMouseOutsideSlop;
};

extern LLToolSelectLand gToolSelectLand;
