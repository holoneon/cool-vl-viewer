/**
 * @file lltoolselectland.cpp
 * @brief LLToolSelectLand class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lltoolselectland.h"

// indra includes
#include "llparcel.h"

// Viewer includes
#include "llagent.h"
#include "llviewercontrol.h"
#include "llfloatertools.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltoolview.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"

LLToolSelectLand gToolSelectLand;

LLToolSelectLand::LLToolSelectLand()
:	LLTool("Parcel"),
	mDragEndValid(false),
	mMouseOutsideSlop(false),
	mDragStartX(0),
	mDragStartY(0),
	mDragEndX(0),
	mDragEndY(0)
{
}

bool LLToolSelectLand::handleMouseDown(S32 x, S32 y, MASK mask)
{
	bool hit_land = gViewerWindowp->mousePointOnLandGlobal(x, y,
														  &mDragStartGlobal);
	if (hit_land)
	{
		setMouseCapture(true);

		mDragStartX = x;
		mDragStartY = y;
		mDragEndX = x;
		mDragEndY = y;

		mDragEndValid = true;
		mDragEndGlobal = mDragStartGlobal;

		sanitize_corners(mDragStartGlobal, mDragEndGlobal, mWestSouthBottom,
						 mEastNorthTop);

		mWestSouthBottom -= LLVector3d(PARCEL_GRID_STEP_METERS / 2,
									   PARCEL_GRID_STEP_METERS / 2, 0);
		mEastNorthTop += LLVector3d(PARCEL_GRID_STEP_METERS / 2,
									PARCEL_GRID_STEP_METERS / 2, 0);

		roundXY(mWestSouthBottom);
		roundXY(mEastNorthTop);

		mMouseOutsideSlop = true; //false;

		gViewerParcelMgr.deselectLand();
	}

	return hit_land;
}

bool LLToolSelectLand::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	LLVector3d pos_global;
	bool hit_land = gViewerWindowp->mousePointOnLandGlobal(x, y, &pos_global);
	if (hit_land)
	{
		// Auto-select this parcel
		gViewerParcelMgr.selectParcelAt(pos_global);
		return true;
	}
	return false;
}

bool LLToolSelectLand::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (hasMouseCapture())
	{
		setMouseCapture(false);

		if (mMouseOutsideSlop && mDragEndValid)
		{
			// Take the drag start and end locations, then map the southwest
			// point down to the next grid location, and the northeast point up
			// to the next grid location.

			sanitize_corners(mDragStartGlobal, mDragEndGlobal,
							 mWestSouthBottom, mEastNorthTop);

			mWestSouthBottom -= LLVector3d(PARCEL_GRID_STEP_METERS / 2,
										   PARCEL_GRID_STEP_METERS / 2, 0);
			mEastNorthTop += LLVector3d(PARCEL_GRID_STEP_METERS / 2,
										PARCEL_GRID_STEP_METERS / 2, 0);

			roundXY(mWestSouthBottom);
			roundXY(mEastNorthTop);

			// Don't auto-select entire parcel.
			mSelection = gViewerParcelMgr.selectLand(mWestSouthBottom,
													 mEastNorthTop, false);
		}

		mMouseOutsideSlop = false;
		mDragEndValid = false;

		return true;
	}
	return false;
}

bool LLToolSelectLand::handleHover(S32 x, S32 y, MASK mask)
{
	if (hasMouseCapture())
	{
		if (mMouseOutsideSlop || outsideSlop(x, y, mDragStartX, mDragStartY))
		{
			mMouseOutsideSlop = true;

			// Must do this every frame, in case the camera moved or the land
			// moved since last frame.

			// If doesn't hit land, doesn't change old value
			LLVector3d land_global;
			bool hit_land = gViewerWindowp->mousePointOnLandGlobal(x, y,
																  &land_global);
			if (hit_land)
			{
				mDragEndValid = true;
				mDragEndGlobal = land_global;

				sanitize_corners(mDragStartGlobal, mDragEndGlobal,
								 mWestSouthBottom, mEastNorthTop);

				mWestSouthBottom -= LLVector3d(PARCEL_GRID_STEP_METERS / 2,
											   PARCEL_GRID_STEP_METERS / 2, 0);
				mEastNorthTop += LLVector3d(PARCEL_GRID_STEP_METERS / 2,
											PARCEL_GRID_STEP_METERS / 2, 0);

				roundXY(mWestSouthBottom);
				roundXY(mEastNorthTop);

				LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (active, land)"
									   << LL_ENDL;
				gWindowp->setCursor(UI_CURSOR_ARROW);
			}
			else
			{
				mDragEndValid = false;
				LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (active, no land)"
									   << LL_ENDL;
				gWindowp->setCursor(UI_CURSOR_NO);
			}

			mDragEndX = x;
			mDragEndY = y;
		}
		else
		{
			LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (active, in slop)"
								   << LL_ENDL;
			gWindowp->setCursor(UI_CURSOR_ARROW);
		}
	}
	else
	{
		LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (inactive)"
							   << LL_ENDL;
		gWindowp->setCursor(UI_CURSOR_ARROW);
	}

	return true;
}

void LLToolSelectLand::render()
{
	if (hasMouseCapture() && /*mMouseOutsideSlop &&*/ mDragEndValid)
	{
		gViewerParcelMgr.renderRect(mWestSouthBottom, mEastNorthTop);
	}
}

void LLToolSelectLand::handleSelect()
{
	if (gFloaterToolsp)
	{
		gFloaterToolsp->setStatusText("selectland");
	}
}

void LLToolSelectLand::handleDeselect()
{
	mSelection = NULL;
}

void LLToolSelectLand::roundXY(LLVector3d &vec)
{
	vec.mdV[VX] = ll_round(vec.mdV[VX], (F64)PARCEL_GRID_STEP_METERS);
	vec.mdV[VY] = ll_round(vec.mdV[VY], (F64)PARCEL_GRID_STEP_METERS);
}

// true if x,y outside small box around start_x,start_y
bool LLToolSelectLand::outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y)
{
	S32 dx = x - start_x;
	S32 dy = y - start_y;
	return dx <= -2 || 2 <= dx || dy <= -2 || 2 <= dy;
}
