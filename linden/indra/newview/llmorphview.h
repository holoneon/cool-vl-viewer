/**
 * @file llmorphview.h
 * @brief Container for character morph controls
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

#include "llview.h"
#include "llvector3d.h"

class LLJoint;

class LLMorphView : public LLView
{
public:
	LLMorphView(const LLRect& rect);
	~LLMorphView() override;

	void initialize();
	void shutdown();

	// Inherited method
	void setVisible(bool visible) override;

	LL_INLINE void setCameraTargetJoint(LLJoint* joint)
	{
		mCameraTargetJoint = joint;
	}

	LL_INLINE LLJoint* getCameraTargetJoint()
	{
		return mCameraTargetJoint;
	}

	LL_INLINE void setCameraOffset(const LLVector3d& offset)
	{
		mCameraOffset = offset;
	}

	void setCameraTargetOffset(const LLVector3d& offset)
	{
		mCameraTargetOffset = offset;
	}

	void updateCamera();
	void setCameraDrivenByKeys(bool b);

protected:
	LLJoint*		mCameraTargetJoint;
	LLVector3d		mCameraOffset;
	LLVector3d		mCameraTargetOffset;
	F32				mOldCameraNearClip;

	// Camera rotation
	F32				mCameraPitch;
	F32				mCameraYaw;

	bool			mCameraDrivenByKeys;
};

//
// Globals
//

extern LLMorphView* gMorphViewp;
