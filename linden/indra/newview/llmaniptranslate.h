/**
 * @file llmaniptranslate.h
 * @brief LLManipTranslate class definition
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

#include "llmanip.h"
#include "lltimer.h"
#include "llvector4.h"
#include "llquaternion.h"

class LLManipTranslate final : public LLManip
{
protected:
	LOG_CLASS(LLManipTranslate);

public:
	class ManipulatorHandle
	{
	public:
		LLVector3	mStartPosition;
		LLVector3	mEndPosition;
		EManipPart	mManipID;
		F32			mHotSpotRadius;

		ManipulatorHandle(LLVector3 start_pos, LLVector3 end_pos,
						  EManipPart id, F32 radius)
		:	mStartPosition(start_pos),
			mEndPosition(end_pos),
			mManipID(id),
			mHotSpotRadius(radius)
		{
		}
	};

	LLManipTranslate(LLToolComposite* composite);

	static bool getSnapEnabled();
	static bool getSnapToMouseCursor();
	static F32 getGridDrawSize();
	static U32 getGridTexName();
	static void destroyGL();
	static void restoreGL();

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;
	void render() override;
	void handleSelect() override;

	void highlightManipulators(S32 x, S32 y) override;
	bool handleMouseDownOnPart(S32 x, S32 y, MASK mask) override;
	bool canAffectSelection() override;

protected:
	enum EHandleType
	{
		HANDLE_CONE,
		HANDLE_BOX,
		HANDLE_SPHERE
	};

	void renderArrow(S32 which_arrow, S32 selected_arrow, F32 box_size,
					 F32 arrow_size, F32 handle_size, bool reverse_direction);
	void renderTranslationHandles();
	void renderText();
	void renderSnapGuides();
	void renderGrid(F32 x, F32 y, F32 size, F32 r, F32 g, F32 b, F32 a);
	void renderGridVert(F32 x_trans, F32 y_trans, F32 r, F32 g, F32 b,
						F32 alpha);
	void highlightIntersection(LLVector3 normal, LLVector3 selection_center,
							   LLQuaternion grid_rotation,
							   LLColor4 inner_color);
	F32 getMinGridScale();

private:
	struct compare_manipulators
	{
		LL_INLINE bool operator()(const ManipulatorHandle* const a,
								  const ManipulatorHandle* const b) const
		{
			if (a->mEndPosition.mV[VZ] != b->mEndPosition.mV[VZ])
			{
				return (a->mEndPosition.mV[VZ] < b->mEndPosition.mV[VZ]);
			}
			return a->mManipID < b->mManipID;
		}
	};

private:
	S32			mLastHoverMouseX;
	S32			mLastHoverMouseY;
	S32			mMouseDownX;
	S32			mMouseDownY;
	F32			mAxisArrowLength;		// pixels
	F32			mConeSize;				// meters, world space
	F32			mArrowLengthMeters;		// meters
	F32			mPlaneManipOffsetMeters;
	F32			mSnapOffsetMeters;
	F32			mSubdivisions;
	LLVector3	mManipNormal;
	LLVector3d	mDragCursorStartGlobal;
	LLVector3d	mDragSelectionStartGlobal;
	LLTimer		mUpdateTimer;
	LLVector4	mManipulatorVertices[18];
	LLVector3	mSnapOffsetAxis;
	LLQuaternion mGridRotation;
	LLVector3	mGridOrigin;
	LLVector3	mGridScale;
	LLVector3	mArrowScales;
	LLVector3	mPlaneScales;
	LLVector4	mPlaneManipPositions;
	bool		mMouseOutsideSlop;		// true after mouse goes outside slop region
	bool		mCopyMadeThisDrag;
	bool		mInSnapRegime;
};
