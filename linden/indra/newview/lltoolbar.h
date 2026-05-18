/**
 * @file lltoolbar.h
 * @brief Large friendly buttons at bottom of screen.
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

#include "llpanel.h"

#include "llframetimer.h"

constexpr S32 TOOL_BAR_HEIGHT = 20;

class LLButton;

class LLToolBar final : public LLPanel
{
public:
	LLToolBar(const LLRect& rect);
	~LLToolBar() override;

	bool postBuild() override;

	bool handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
						   EDragAndDropType cargo_type, void* cargo_data,
						   EAcceptance* accept, std::string& tooltip) override;

	void reshape(S32 width, S32 height, bool call_from_parent = true) override;

	static void toggle();
	static bool isVisible();

	// Move buttons to appropriate locations based on rect.
	void layoutButtons();

	// Per-frame refresh call
	void refresh() override;

	// Callbacks
	static void onClickChat(void* data);
	static void onClickIM(void*);
	static void onClickFriends(void* data);
	static void onClickGroups(void* data);
	static void onClickFly(void*);
	static void onClickSnapshot(void* data);
	static void onClickSearch(void* data);
	static void onClickBuild(void* data);
	static void onClickRadar(void* data);
	static void onClickMiniMap(void* data);
	static void onClickMap(void* data);
	static void onClickInventory(void* data);

	static F32 sInventoryAutoOpenTime;

private:
	LLButton*				mChatButton;
	LLButton*				mIMButton;
	LLButton*				mFriendsButton;
	LLButton*				mGroupsButton;
	LLButton*				mFlyButton;
	LLButton*				mSnapshotButton;
	LLButton*				mSearchButton;
	LLButton*				mBuildButton;
	LLButton*				mRadarButton;
	LLButton*				mMiniMapButton;
	LLButton*				mMapButton;
	LLButton*				mInventoryButton;
	LLFrameTimer			mInventoryAutoOpenTimer;

	bool					mInventoryAutoOpen;
};

extern LLToolBar* gToolBarp;
