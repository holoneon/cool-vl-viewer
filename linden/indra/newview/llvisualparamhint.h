/**
 * @file llvisualparamhint.h
 * @brief A dynamic texture class for displaying avatar visual params effects
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

// Note: probably because of obscure pre-historical reasons, this file is
// named "lltoolmorph.h" in LL's viewer sources. I renamed it based on the
// class it declares instead. HB

#pragma once

#include "llui.h"
#include "llviewervisualparam.h"

#include "lldynamictexture.h"

class LLJoint;
class LLPolyMesh;
class LLViewerJointMesh;
class LLViewerObject;

class LLVisualParamHint final : public LLViewerDynamicTexture
{
protected:
	LOG_CLASS(LLVisualParamHint);

protected:
	~LLVisualParamHint() override;

public:
	LLVisualParamHint(S32 pos_x,  S32 pos_y, S32 width, S32 height,
					  LLViewerJointMesh* meshp, LLViewerVisualParam* paramp,
					  LLWearable* wearablep, F32 param_weight,
					  LLJoint* jointp);

	S8 getType() const override;
	bool needsRender() override;
	void preRender(bool clear_depth) override;
	bool render() override;

	void setWearable(LLWearable* wearablep, LLViewerVisualParam* paramp);

	LL_INLINE void requestUpdate(S32 delay_frames)
	{
		mNeedsUpdate = true;
		mDelayFrames = delay_frames;
	}

	LL_INLINE void setUpdateDelayFrames(S32 delay)	{ mDelayFrames = delay; }

	void draw();

	LL_INLINE LLViewerVisualParam* getVisualParam()	{ return mVisualParam; }
	LL_INLINE F32 getVisualParamWeight()			{ return mVisualParamWeight; }
	LL_INLINE bool getVisible()						{ return mIsVisible; }

	LL_INLINE void setAllowsUpdates(bool b)			{ mAllowsUpdates = b; }

	LL_INLINE const LLRect& getRect()				{ return mRect; }

	// Requests updates for all instances (excluding two possible exceptions)
	//  Grungy but efficient.
	static void requestHintUpdates(LLVisualParamHint* exception1 = NULL,
								   LLVisualParamHint* exception2 = NULL);

protected:
	LLViewerJointMesh*		mJointMesh;			// mesh that this distortion applies to
	LLViewerVisualParam*	mVisualParam;		// visual param applied by this hint
	LLWearable*				mWearablePtr;		// wearable we're editing
	LLJoint*				mCamTargetJoint;	// joint to target with preview camera
	LLUIImagePtr			mBackgroundp;
	LLRect					mRect;
	S32						mDelayFrames;		// updates are blocked for this many frames
	F32						mVisualParamWeight;	// weight for this visual parameter
	F32						mLastParamWeight;

	bool					mNeedsUpdate;		// does this texture need to be re-rendered?
	bool					mAllowsUpdates;		// updates are blocked unless this is true
	bool					mIsVisible;			// is this distortion hint visible ?

	typedef std::set<LLVisualParamHint*> instance_list_t;
	static instance_list_t	sInstances;
};

// This class resets avatar data at the end of an update cycle
class LLVisualParamReset final : public LLViewerDynamicTexture
{
public:
	LLVisualParamReset();

	bool render() override;
	S8 getType() const override;

public:
	static bool sDirty;
};
