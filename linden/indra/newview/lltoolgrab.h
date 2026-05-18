/**
 * @file lltoolgrab.h
 * @brief LLToolGrab class header file
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

#include "llquaternion.h"
#include "lluuid.h"
#include "llvector3.h"

#include "lltool.h"
#include "llviewerwindow.h"		// For LLPickInfo

class LLView;
class LLTextBox;
class LLViewerObject;
class LLPickInfo;

class LLToolGrabBase : public LLTool
{
protected:
	LOG_CLASS(LLToolGrabBase);

public:
	LLToolGrabBase(LLToolComposite* composite = NULL);

	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleDoubleClick(S32 x, S32 y, MASK mask) override;

	LL_INLINE void render() override			{}		// 3D elements
	LL_INLINE void draw() override				{}		// 2D elements

	void handleSelect() override;
	void handleDeselect() override;

	LLViewerObject* getEditingObject() override;
	LLVector3d getEditingPointGlobal() override;
	bool isEditing() override;
	void stopEditing() override;

	void onMouseCaptureLost() override;

	// Capture the mouse and start grabbing.
	bool handleObjectHit(const LLPickInfo& info);

	static void pickCallback(const LLPickInfo& pick_info);

private:
	LLVector3d getGrabPointGlobal();
	void startGrab();
	void stopGrab();

	void startSpin();
	void stopSpin();

	void handleHoverSpin(S32 x, S32 y, MASK mask);
	void handleHoverActive(S32 x, S32 y, MASK mask);
	void handleHoverNonPhysical(S32 x, S32 y, MASK mask);
	void handleHoverInactive(S32 x, S32 y, MASK mask);
	void handleHoverFailed(S32 x, S32 y, MASK mask);

private:
	enum EGrabMode { GRAB_INACTIVE, GRAB_ACTIVE_CENTER, GRAB_NONPHYSICAL,
					 GRAB_LOCKED, GRAB_NOOBJECT };

	EGrabMode		mMode;

	// Send simulator time between hover movements
	LLTimer			mGrabTimer;

	// Meters from CG of object
	LLVector3		mGrabOffsetFromCenterInitial;
	// In cursor hidden drag, how far is grab offset from camera
	LLVector3d		mGrabHiddenOffsetFromCamera;

	LLVector3d		mDragStartPointGlobal;	// Projected into world
	LLVector3d		mDragStartFromCamera;	// Drag start relative to camera

	LLPickInfo		mGrabPick;

	S32				mLastMouseX;
	S32				mLastMouseY;
	// Since cursor hidden, how far have you moved ?
	S32				mAccumDeltaX;
	S32				mAccumDeltaY;

	S32             mLastFace;
	LLVector2       mLastUVCoords;
	LLVector2       mLastSTCoords;
	LLVector3       mLastIntersection;
	LLVector3       mLastNormal;
	LLVector3       mLastBinormal;
	LLVector3       mLastGrabPos;

	LLQuaternion	mSpinRotation;

	// Has mouse moved off center at all ?
	bool			mHasMoved;
	// Nas mouse moved outside center 5 pixels ?
	bool			mOutsideSlop;

	bool			mVerticalDragging;
	bool			mSpinGrabbing;
	bool			mClickedInMouselook;
};

class LLToolGrab final : public LLToolGrabBase
{
protected:
	LOG_CLASS(LLToolGrab);

public:
	LLToolGrab() = default;
};

extern LLToolGrab gToolGrab;
extern LLTool* gGrabTransientTool;
extern bool gGrabBtnVertical;
extern bool gGrabBtnSpin;
