/**
 * @file llvoiceremotectrl.h
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

#pragma once

#define EXPANDED_VOICE_CTRL 0

#include "llpanel.h"

class LLButton;
class LLIconCtrl;

class LLVoiceRemoteCtrl final : public LLPanel
{
public:
	LLVoiceRemoteCtrl(const std::string& name);

	bool postBuild() override;
	void draw() override;

	static void onBtnLock(void* user_data);
	static void onBtnTalkHeld(void* user_data);
	static void onBtnTalkReleased(void* user_data);
	static void onBtnTalkClicked(void* user_data);
	static void onClickSpeakers(void* user_data);
#if EXPANDED_VOICE_CTRL
	static void onClickPopupBtn(void* user_data);
	static void onClickVoiceChannel(void* user_data);
	static void onClickEndCall(void* user_data);
#endif

protected:
	LLButton*	mTalkBtn;
	LLButton*	mTalkLockBtn;
	LLButton*	mSpeakersBtn;
	LLIconCtrl*	mIcon;
};
