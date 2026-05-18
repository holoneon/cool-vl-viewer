/**
 * @file llscrollingpanelparam.h
 * @brief the scrolling panel containing a list of visual param
 *  	  panels
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

#include "llbutton.h"
#include "llscrollingpanellist.h"
#include "llsliderctrl.h"
#include "lltextbox.h"

class LLJoint;
class LLPanelEditWearable;
class LLViewerJointMesh;
class LLVisualParamHint;
class LLViewerVisualParam;
class LLWearable;

class LLScrollingPanelParam : public LLScrollingPanel
{
protected:
	LOG_CLASS(LLScrollingPanelParam);

public:
	LLScrollingPanelParam(LLPanelEditWearable* panel,
						  LLViewerJointMesh* mesh,
						  LLViewerVisualParam* param,
						  bool allow_modify,
						  LLWearable* wearable,
						  LLJoint* jointp,
						  bool use_hints = true);
	virtual ~LLScrollingPanelParam();

	virtual void		draw();
	virtual void		setVisible(bool visible);
	virtual void		updatePanel(bool allow_modify);

	static void			onSliderMouseDown(LLUICtrl* ctrl, void* userdata);
	static void			onSliderMoved(LLUICtrl* ctrl, void* userdata);
	static void			onSliderMouseUp(LLUICtrl* ctrl, void* userdata);

	static void			onHintMinMouseDown(void* userdata);
	static void			onHintMinHeldDown(void* userdata);
	static void			onHintMaxMouseDown(void* userdata);
	static void			onHintMaxHeldDown(void* userdata);
	static void			onHintMinMouseUp(void* userdata);
	static void			onHintMaxMouseUp(void* userdata);

	void				onHintMouseDown(LLVisualParamHint* hint);
	void				onHintHeldDown(LLVisualParamHint* hint);

	F32					weightToPercent(F32 weight);
	F32					percentToWeight(F32 percent);

public:
	LLPanelEditWearable*			mPanelParams;
	LLWearable*						mWearable;
	LLViewerVisualParam* 			mParam;
	LLPointer<LLVisualParamHint>	mHintMin;
	LLPointer<LLVisualParamHint>	mHintMax;
	LLButton*						mLess;
	LLButton*						mMore;
	LLSliderCtrl*					mSlider;
	LLTextBox*						mMinParamText;
	LLTextBox*						mMaxParamText;

	static S32 				sUpdateDelayFrames;

protected:
	LLTimer				mMouseDownTimer;	// timer for how long mouse has been held down on a hint.
	F32					mLastHeldTime;

	bool				mAllowModify;
};
