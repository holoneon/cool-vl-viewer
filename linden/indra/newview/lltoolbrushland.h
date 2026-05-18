/**
 * @file lltoolbrushland.h
 * @brief LLToolBrushLand class header file
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

#include "hbfastset.h"
#include "lleditmenuhandler.h"

#include "lltool.h"

class LLSurface;
class LLViewerRegion;

// A toolbrush that modifies the land.

class LLToolBrushLand final : public LLTool, public LLEditMenuHandler
{
protected:
	LOG_CLASS(LLToolBrushLand);

public:
	LLToolBrushLand();

	// x, y in window coords with 0, 0 = left, bottom
	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;

	void onMouseCaptureLost() override;

	void handleSelect() override;
	void handleDeselect() override;

	// Returns true if this is a tool that should always be rendered regardless
	// of selection.
	LL_INLINE bool isAlwaysRendered() override	{ return true; }

	// Draws the area that will be affected.
	void render() override;

	// This is where the land modification actually occurs
	static void onIdle(void* brush_tool);

	void modifyLandInSelectionGlobal();

	void undo() override;
	LL_INLINE bool canUndo() const override		{ return true; }

protected:
	void brush();
	void modifyLandAtPointGlobal(const LLVector3d& spot, MASK mask);

	typedef fast_hset<LLViewerRegion*> region_list_t;
	void determineAffectedRegions(region_list_t& regions,
								  const LLVector3d& spot) const;

	void renderOverlay(LLSurface& land, const LLVector3& pos_region,
					   const LLVector3& pos_world);

	// Does region allow terraform, or are we a god ?
	bool canTerraform(LLViewerRegion* regionp) const;

	// Modal dialog alerting you cannot terraform the region
	void alertNoTerraform(LLViewerRegion* regionp);

private:
	U8 getBrushIndex();

protected:
	region_list_t	mLastAffectedRegions;

	F32				mStartingZ;
	S32				mMouseX;
	S32				mMouseY;
	bool			mGotHover;
	bool			mBrushSelected;
};

extern LLToolBrushLand gToolBrushLand;
