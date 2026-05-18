/**
 * @file llfloatervoicedevicesettings.cpp
 * @author Richard Nelson
 * @brief Voice communication set-up
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

#include "llviewerprecompiledheaders.h"

#include "llfloatervoicedevicesettings.h"

#include "llbutton.h"
#include "llcombobox.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llprefsvoice.h"
#include "llviewercontrol.h"
#include "llvoicechannel.h"
#include "llvoiceclient.h"

LLFloaterVoiceDeviceSettings::LLFloaterVoiceDeviceSettings(const LLSD&)
:	LLFloater("voice settings"),
	mInputDevice(gSavedSettings.getString("VoiceWebRTCInputAudioDevice")),
	mOutputDevice(gSavedSettings.getString("VoiceWebRTCOutputAudioDevice")),
	mSpeakingColor(gSavedSettings.getColor4("SpeakingColor")),
	mOverdrivenColor(gSavedSettings.getColor4("OverdrivenColor")),
	mMicVolume(gSavedSettings.getF32("AudioLevelMic")),
	mLastMicTune(-1.f),
	mDevicesUpdated(false)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_device_settings.xml");
	center();

	// Ask for new enumeration of audio devices
	gVoiceClient.refreshDeviceLists(true);
	// Put voice client in "tuning" mode
	gVoiceClient.tuningStart();
	LLVoiceChannel::suspend();
}

//virtual
LLFloaterVoiceDeviceSettings::~LLFloaterVoiceDeviceSettings()
{
	gVoiceClient.tuningStop();
	LLVoiceChannel::resume();
}

//virtual
bool LLFloaterVoiceDeviceSettings::postBuild()
{
	mCtrlInputDevices = getChild<LLComboBox>("voice_input_device");
	mCtrlInputDevices->setCommitCallback(onCommitInputDevice);
	mCtrlInputDevices->setCallbackUserData(this);

	mCtrlOutputDevices = getChild<LLComboBox>("voice_output_device");
	mCtrlOutputDevices->setCommitCallback(onCommitOutputDevice);
	mCtrlOutputDevices->setCallbackUserData(this);

	mCtrlMicVolume = getChild<LLSliderCtrl>("mic_volume_slider");
	mCtrlMicVolume->setValue(mMicVolume);

	mCtrlInputLoadingText = getChild<LLTextBox>("input_device_loading");
	mCtrlOutputLoadingText = getChild<LLTextBox>("output_device_loading");
	mCtrlWaitText = getChild<LLTextBox>("wait_text");

	mApplyBtn = getChild<LLButton>("apply_btn");
	mApplyBtn->setClickedCallback(onApply, this);
	childSetAction("cancel_btn", onCancel, this);

	mDefaultDeviceName = getString("default_text");

	return true;
}

//virtual
void LLFloaterVoiceDeviceSettings::draw()
{
	refresh();

	// Let the user know that volume indicator is not yet available
	bool is_in_tuning_mode = gVoiceClient.inTuningMode();
	mCtrlWaitText->setVisible(!is_in_tuning_mode);

	LLFloater::draw();

	F32 voice_power = gVoiceClient.tuningGetEnergy();
	S32 discrete_power = 0;

	if (!is_in_tuning_mode)
	{
		discrete_power = 0;
	}
	else
	{
		discrete_power =
			llmin(4, S32(voice_power * 4.f / OVERDRIVEN_POWER_LEVEL));
	}

	if (is_in_tuning_mode)
	{
		for (S32 power_bar_idx = 0; power_bar_idx < 5; power_bar_idx++)
		{
			std::string view_name = llformat("%s%d", "bar", power_bar_idx);
			LLView* bar_view = getChild<LLView>(view_name.c_str());
			if (bar_view)
			{
				if (power_bar_idx < discrete_power)
				{
					LLColor4 color = power_bar_idx >= 3 ? mOverdrivenColor
														: mSpeakingColor;
					gl_rect_2d(bar_view->getRect(), color, true);
				}
				gl_rect_2d(bar_view->getRect(), LLColor4::grey, false);
			}
		}
	}
}

//virtual
void LLFloaterVoiceDeviceSettings::refresh()
{
	bool has_device_list = gVoiceClient.deviceSettingsAvailable();
	mApplyBtn->setEnabled(has_device_list);
	mCtrlInputDevices->setEnabled(has_device_list);
	mCtrlOutputDevices->setEnabled(has_device_list);
	mCtrlInputLoadingText->setVisible(!has_device_list);
	mCtrlOutputLoadingText->setVisible(!has_device_list);
	if (!has_device_list)
	{
		// The combo boxes are disabled, since we cannot get the device
		// settings from the voice server just now. Put the currently set
		// default (ONLY) in the box, and select it.
		mCtrlInputDevices->removeall();
		mCtrlInputDevices->add(mInputDevice, "Default", ADD_BOTTOM);
		mCtrlInputDevices->setSimple(mInputDevice);

		mCtrlOutputDevices->removeall();
		mCtrlOutputDevices->add(mOutputDevice, "Default", ADD_BOTTOM);
		mCtrlOutputDevices->setSimple(mOutputDevice);

		mDevicesUpdated = false;
		return;
	}

	F32 tuned_vol = mCtrlMicVolume->getValue().asReal();
	if (mLastMicTune != tuned_vol)
	{
		// Set mic volume tuning slider based on last mic volume setting
		gVoiceClient.tuningSetMicVolume(tuned_vol);
		mLastMicTune = tuned_vol;
	}

	if (mDevicesUpdated)
	{
		return;	// No lists refresh needed
	}
	mDevicesUpdated = true;

	typedef strings_map_t::const_iterator dev_iterator;

	const strings_map_t& input_devices = gVoiceClient.getCaptureDevices();
	mCtrlInputDevices->removeall();
	mCtrlInputDevices->add(mDefaultDeviceName, "Default", ADD_BOTTOM);
	for (dev_iterator it = input_devices.begin(), end = input_devices.end();
		 it != end; ++it)
	{
		mCtrlInputDevices->add(it->first, it->second, ADD_BOTTOM);
	}
	if (!mCtrlInputDevices->setSelectedByValue(mInputDevice, true))
	{
		mCtrlInputDevices->setSimple(mDefaultDeviceName);
	}

	const strings_map_t& output_devices = gVoiceClient.getRenderDevices();
	mCtrlOutputDevices->removeall();
	mCtrlOutputDevices->add(mDefaultDeviceName, "Default", ADD_BOTTOM);
	for (dev_iterator it = output_devices.begin(), end = output_devices.end();
		 it != end; ++it)
	{
		mCtrlOutputDevices->add(it->first, it->second, ADD_BOTTOM);
	}
	if (!mCtrlOutputDevices->setSelectedByValue(mOutputDevice, true))
	{
		mCtrlOutputDevices->setSimple(mDefaultDeviceName);
	}
}

void LLFloaterVoiceDeviceSettings::apply()
{
	std::string device_id = mCtrlInputDevices->getValue().asString();
	gSavedSettings.setString("VoiceWebRTCInputAudioDevice", device_id);
	mInputDevice = device_id;

	device_id = mCtrlOutputDevices->getValue().asString();
	gSavedSettings.setString("VoiceWebRTCOutputAudioDevice", device_id);
	mOutputDevice = device_id;

	mMicVolume = mCtrlMicVolume->getValue().asReal();
	gSavedSettings.setF32("AudioLevelMic", mMicVolume);
}

//static
void LLFloaterVoiceDeviceSettings::onApply(void* datap)
{
	LLFloaterVoiceDeviceSettings* self = (LLFloaterVoiceDeviceSettings*)datap;
	if (self)
	{
		self->apply();
		self->close();
	}
}

void LLFloaterVoiceDeviceSettings::cancel()
{
	// Note: these setting variables are observed in llviewercontrol.cpp and
	// their changes are automatically propagated to the voice client.
	gSavedSettings.setString("VoiceWebRTCInputAudioDevice", mInputDevice);
	gSavedSettings.setString("VoiceWebRTCOutputAudioDevice", mOutputDevice);
	gSavedSettings.setF32("AudioLevelMic", mMicVolume);
	// Any cancel event on this floater implicitely closes it as well. HB
	close();
}

//static
void LLFloaterVoiceDeviceSettings::onCancel(void* datap)
{
	LLFloaterVoiceDeviceSettings* self = (LLFloaterVoiceDeviceSettings*)datap;
	if (self)
	{
		self->cancel();	// Also calls close()
	}
}

//static
void LLFloaterVoiceDeviceSettings::onCommitInputDevice(LLUICtrl* ctrlp,
													   void* datap)
{
	LLFloaterVoiceDeviceSettings* self = (LLFloaterVoiceDeviceSettings*)datap;
	if (self && ctrlp)
	{
		gVoiceClient.setCaptureDevice(ctrlp->getValue().asString());
	}
}

//static
void LLFloaterVoiceDeviceSettings::onCommitOutputDevice(LLUICtrl* ctrlp,
														void* datap)
{
	LLFloaterVoiceDeviceSettings* self = (LLFloaterVoiceDeviceSettings*)datap;
	if (self && ctrlp)
	{
		gVoiceClient.setRenderDevice(ctrlp->getValue().asString());
	}
}
