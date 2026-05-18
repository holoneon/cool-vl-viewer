/**
 * @file hbpanellandenvironment.h
 * @brief Configuration of environmemt settings for land (parcel or region).
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 *
 * Copyright (c) 2019-2023 Henri Beauchamp
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

#include "llpanel.h"

#include "llenvironment.h"
#include "llparcelselection.h"

class LLButton;
class LLCheckBoxCtrl;
class LLFloaterEditEnvSettings;
class LLMultiSliderCtrl;
class HBSettingsDropTarget;
class LLSliderCtrl;
class LLTextBox;
class LLViewerInventoryItem;
class LLViewerRegion;

class HBPanelLandEnvironment final : public LLPanel
{
	friend class HBSettingsDropTarget;
	friend class LLPanelEstateInfo;

protected:
	LOG_CLASS(HBPanelLandEnvironment);

	bool postBuild() override;
	void draw() override;

public:
	HBPanelLandEnvironment(LLSafeHandle<LLParcelSelection>& parcelp);
	HBPanelLandEnvironment(U64 region_handle);

	~HBPanelLandEnvironment() override;

	void refresh() override;
	void setEnabled(bool enabled) override;

	void setRegionHandle(U64 handle);
	LLViewerRegion* getRegion();
	LLParcel* getParcel();
	S32 getParcelId();

protected:
	void onChoosenItem(LLViewerInventoryItem* itemp, S32 track);
	void resetOverride();

private:
	bool isAgentRegion();

	void refreshFromRegion();
	void refreshFromParcel();

	void onEnvironmentChanged(LLEnvironment::EEnvSelection env, S32 version);
	void onEnvironmentReceived(S32 parcel_id,
							   LLEnvironment::envinfo_ptr_t info);

	void commitDayParametersChanges();

	void applyDayCycle(LLSettingsDay::ptr_t dayp, bool edited = false);

	void updateApparentTimeOfDay();
	void updateAltitudeLabels();
	void updateTrackNames();
	std::string getNameForTrack(S32 track);

	void loadInventoryItem(LLUUID inv_item_id);
	void onAssetLoaded(const std::string& name, const LLUUID& asset_id,
					   LLSettingsBase::ptr_t settings, S32 status);

	bool closeEditFloater(bool force = false);

	static void invPickerCallback(const std::vector<std::string>&,
								  const uuid_vec_t& ids, void* userdata, bool);

	static void onBtnDefault(void* userdata);
	static void onBtnInventory(void* userdata);
	static void onBtnCustom(void* userdata);
	static void onBtnReset(void* userdata);
	static void onAllowOverride(LLUICtrl* ctrl, void* userdata);
	static void onDayParametersChanged(LLUICtrl*, void* userdata);
	static void onAltSliderCommit(LLUICtrl* ctrl, void* userdata);
	static void onAltSliderMouseUp(S32, S32, void* userdata);

protected:
	LLEnvironment::envinfo_ptr_t		mCurrentEnvironment;

private:
	LLButton*							mUseDefaultBtn;
	LLButton*							mUseInventoryBtn;
	LLButton*							mUseCustomBtn;
	LLButton*							mResetAltitudesBtn;
	LLSliderCtrl*						mDayLengthSlider;
	LLSliderCtrl*						mDayOffsetSlider;
	LLCheckBoxCtrl*						mAllowOverrideCheck;
	LLMultiSliderCtrl*					mAltitudesSlider;
	LLTextBox*							mApparentDayLengthText;
	LLTextBox*							mAltitude2ValueText;
	LLTextBox*							mAltitude3ValueText;
	LLTextBox*							mAltitude4ValueText;

	std::vector<HBSettingsDropTarget*>	mDropTargets;

	LLHandle<LLFloater>					mEditFloaterHandle;

	typedef boost::signals2::connection connection_t;
	connection_t						mChangeConnection;
	connection_t						mCommitConnection;

	LLSafeHandle<LLParcelSelection>&	mParcel;
	U64									mRegionHandle;
	F32									mLastParametersChange;
	F32									mLastTimeOfDayUpdate;
	S32									mCurEnvVersion;
	S32									mLastParcelId;
	bool								mIsRegion;
	bool								mDayParametersDirty;
	bool								mEnvOverrideCheck;
	bool								mLastEnabledState;
};
