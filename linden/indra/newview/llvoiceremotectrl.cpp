/**
 * @file llvoiceremotectrl.cpp
 * @brief A remote control for voice chat
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 *
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llvoiceremotectrl.h"

#include "llbutton.h"
#include "lliconctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llfloaterchatterbox.h"
#include "llfloateractivespeakers.h"
#include "lloverlaybar.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llvoicechannel.h"

LLVoiceRemoteCtrl::LLVoiceRemoteCtrl (const std::string& name)
:	LLPanel(name)
{
	setIsChrome(true);

#if EXPANDED_VOICE_CTRL
	if (gSavedSettings.getBool("ShowVoiceChannelPopup"))
	{
		LLUICtrlFactory::getInstance()->buildPanel(this,
												   "panel_voice_remote_expanded.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(this,
												   "panel_voice_remote.xml");
	}
#else
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_voice_remote.xml");
#endif

	setFocusRoot(true);
}

bool LLVoiceRemoteCtrl::postBuild()
{
	mTalkBtn = getChild<LLButton>("push_to_talk");
	mTalkBtn->setClickedCallback(onBtnTalkClicked);
	mTalkBtn->setHeldDownCallback(onBtnTalkHeld);
	mTalkBtn->setMouseUpCallback(onBtnTalkReleased);

	mTalkLockBtn = getChild<LLButton>("ptt_lock");
	mTalkLockBtn->setClickedCallback(onBtnLock);
	mTalkLockBtn->setCallbackUserData(this);

	mSpeakersBtn = getChild<LLButton>("speakers_btn");
	mSpeakersBtn->setClickedCallback(onClickSpeakers);
	mSpeakersBtn->setCallbackUserData(this);

	mIcon = getChild<LLIconCtrl>("voice_volume");

#if EXPANDED_VOICE_CTRL
	childSetAction("show_channel", onClickPopupBtn, this);
	childSetAction("end_call_btn", onClickEndCall, this);

	LLTextBox* text = getChild<LLTextBox>("channel_label");
	if (text)
	{
		text->setUseEllipses(true);
	}

	childSetAction("voice_channel_bg", onClickVoiceChannel, this);
#endif
	return true;
}

void LLVoiceRemoteCtrl::draw()
{
	bool voice_active = false;
	if (gViewerParcelMgr.allowAgentVoice())
	{
		LLVoiceChannel* channelp = LLVoiceChannel::getCurrentVoiceChannel();
		if (channelp)
		{
			voice_active = channelp->isActive();
		}
	}
	mTalkBtn->setEnabled(voice_active);
	mTalkLockBtn->setEnabled(voice_active);

	// Propagate PTT state to button display,
	if (!mTalkBtn->hasMouseCapture())
	{
		mTalkBtn->setToggleState(gVoiceClient.isAgentMicOpen());
	}
	mSpeakersBtn->setToggleState(LLFloaterActiveSpeakers::instanceVisible(LLSD()));
	static LLCachedControl<bool> ptt_enabled(gSavedSettings,
											 "PTTCurrentlyEnabled");
	mTalkLockBtn->setToggleState(!ptt_enabled);

	static S32 last_icon_number = -1;
	S32 icon_number = 4;
	std::string talk_blip_image;
	if (gVoiceClient.getIsSpeaking(gAgentID))
	{
		F32 voice_power = gVoiceClient.getCurrentPower(gAgentID);

		if (voice_power > OVERDRIVEN_POWER_LEVEL)
		{
			talk_blip_image = "icn_voice_ptt-on-lvl3.tga";
			icon_number = 3;
		}
		else
		{
			F32 power = gVoiceClient.getCurrentPower(gAgentID);
			S32 icon_image_idx =
				llmin(2, llfloor((power / OVERDRIVEN_POWER_LEVEL) * 3.f));
			icon_number = icon_image_idx;
			switch (icon_image_idx)
			{
			case 0:
				talk_blip_image = "icn_voice_ptt-on.tga";
				break;

			case 1:
				talk_blip_image = "icn_voice_ptt-on-lvl1.tga";
				break;

			case 2:
				talk_blip_image = "icn_voice_ptt-on-lvl2.tga";
			}
		}
	}
	else
	{
		talk_blip_image = "icn_voice_ptt-off.tga";
	}

	if (icon_number != last_icon_number)
	{
		last_icon_number = icon_number;
		mIcon->setImage(talk_blip_image);
	}

	LLFloater* floaterp =
		LLFloaterChatterBox::getInstance()->getCurrentVoiceFloater();
	std::string active_channel_name;
	if (floaterp)
	{
		active_channel_name = floaterp->getTitle();
	}

#if EXPANDED_VOICE_CTRL
	LLVoiceChannel* curchannelp = LLVoiceChannel::getCurrentVoiceChannel();
	childSetEnabled("end_call_btn",
					LLVoiceClient::voiceEnabled() && curchannelp &&
					curchannelp->isActive() &&
					curchannelp != LLVoiceChannelProximal::getInstance());

	childSetValue("channel_label", active_channel_name);
	childSetToolTip("voice_channel_bg", active_channel_name);

	if (curchannelp)
	{
		LLIconCtrl* iconp = getChild<LLIconCtrl>("voice_channel_icon");
		if (iconp && floaterp)
		{
			iconp->setImage(floaterp->getString("voice_icon"));
		}

		LLButton* buttonp = getChild<LLButton>("voice_channel_bg");
		if (buttonp)
		{
			LLColor4 bg_color;
			if (curchannelp->isActive())
			{
				bg_color = lerp(LLColor4::green, LLColor4::white, 0.7f);
			}
			else if (curchannelp->getState() == LLVoiceChannel::STATE_ERROR)
			{
				bg_color = lerp(LLColor4::red, LLColor4::white, 0.7f);
			}
			else // active, but not connected
			{
				bg_color = lerp(LLColor4::yellow, LLColor4::white, 0.7f);
			}
			buttonp->setImageColor(bg_color);
		}
	}

	LLButton* buttonp = getChild<LLButton>("show_channel");
	if (buttonp)
	{
		if (buttonp->getToggleState())
		{
			buttonp->setImageOverlay(std::string("arrow_down.tga"));
		}
		else
		{
			buttonp->setImageOverlay(std::string("arrow_up.tga"));
		}
	}
#endif

	LLPanel::draw();
}

//static
void LLVoiceRemoteCtrl::onBtnTalkClicked(void *user_data)
{
	// when in toggle mode, clicking talk button turns mic on/off
	if (gSavedSettings.getBool("PushToTalkToggle"))
	{
		gVoiceClient.toggleUserPTTState();
	}
}

//static
void LLVoiceRemoteCtrl::onBtnTalkHeld(void *user_data)
{
	// When not in toggle mode, holding down talk button turns on mic
	if (!gSavedSettings.getBool("PushToTalkToggle"))
	{
		gVoiceClient.setUserPTTState(true);
	}
}

//static
void LLVoiceRemoteCtrl::onBtnTalkReleased(void* user_data)
{
	// When not in toggle mode, releasing talk button turns off mic
	if (!gSavedSettings.getBool("PushToTalkToggle"))
	{
		gVoiceClient.setUserPTTState(false);
	}
}

//static
void LLVoiceRemoteCtrl::onBtnLock(void* user_data)
{
	LLVoiceRemoteCtrl* self = (LLVoiceRemoteCtrl*)user_data;
	if (self)
	{
		gSavedSettings.setBool("PTTCurrentlyEnabled",
							   !self->mTalkLockBtn->getToggleState());
	}
}

//static
void LLVoiceRemoteCtrl::onClickSpeakers(void *user_data)
{
	LLFloaterActiveSpeakers::toggleInstance(LLSD());
}

#if EXPANDED_VOICE_CTRL
//static
void LLVoiceRemoteCtrl::onClickPopupBtn(void* user_data)
{
	LLVoiceRemoteCtrl* self = (LLVoiceRemoteCtrl*)user_data;

	remotep->deleteAllChildren();
	if (gSavedSettings.getBool("ShowVoiceChannelPopup"))
	{
		LLUICtrlFactory::getInstance()->buildPanel(self,
												   "panel_voice_remote_expanded.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(self,
												   "panel_voice_remote.xml");
	}
	if (gOverlayBarp)
	{
		gOverlayBarp->setDirty();
	}
}

//static
void LLVoiceRemoteCtrl::onClickEndCall(void* user_data)
{
	LLVoiceChannel* curchannelp = LLVoiceChannel::getCurrentVoiceChannel();

	if (curchannelp && curchannelp != LLVoiceChannelProximal::getInstance())
	{
		curchannelp->deactivate();
	}
}

//static
void LLVoiceRemoteCtrl::onClickVoiceChannel(void* user_data)
{
	LLFloaterChatterBox::showInstance();
}
#endif
