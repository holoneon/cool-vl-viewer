/**
 * @file llfloatervoicedevicesettings.h
 * @author Richard Nelson
 * @brief Voice communication set-up wizard
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * Copyright (c) 2024 Henri Beauchamp (largely rewritten).
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#pragma once

#include "llfloater.h"

class LLButton;
class LLComboBox;
class LLSliderCtrl;
class LLTextBox;

class LLFloaterVoiceDeviceSettings final
:	public LLFloater,
	public LLFloaterSingleton<LLFloaterVoiceDeviceSettings>
{
	friend class LLUISingleton<LLFloaterVoiceDeviceSettings,
							   VisibilityPolicy<LLFloater> >;

public:
	void apply();
	void cancel();

	// LLFloater override
	void refresh() override;

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	LLFloaterVoiceDeviceSettings(const LLSD&);
	~LLFloaterVoiceDeviceSettings() override;

	// LLFloater overrides
	bool postBuild() override;
	void draw() override;

	static void onApply(void* datap);
	static void onCancel(void* datap);
	static void onCommitInputDevice(LLUICtrl* ctrlp, void* datap);
	static void onCommitOutputDevice(LLUICtrl* ctrlp, void* datap);

private:
	LLButton*		mApplyBtn;
	LLComboBox*		mCtrlInputDevices;
	LLComboBox*		mCtrlOutputDevices;
	LLSliderCtrl*	mCtrlMicVolume;
	LLTextBox*		mCtrlWaitText;
	LLTextBox*		mCtrlInputLoadingText;
	LLTextBox*		mCtrlOutputLoadingText;
	std::string		mInputDevice;
	std::string		mOutputDevice;
	std::string		mDefaultDeviceName;
	LLColor4		mSpeakingColor;
	LLColor4		mOverdrivenColor;
	F32				mMicVolume;
	F32				mLastMicTune;
	bool			mDevicesUpdated;
};
