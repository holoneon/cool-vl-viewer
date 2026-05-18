/**
 * @file lloverlaybar.h
 * @brief LLOverlayBar class definition
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

#include "boost/signals2.hpp"

#include "llcontrol.h"
#include "llframetimer.h"
#include "llpanel.h"

#include "llpathfindingmanager.h"

constexpr S32 OVERLAY_BAR_HEIGHT = 20;

class LLButton;
class LLMediaRemoteCtrl;
class LLVoiceRemoteCtrl;

class LLOverlayBar final : public LLPanel
{
protected:
	LOG_CLASS(LLOverlayBar);

private:
	// Navmesh rebaking stuff
	typedef enum
	{
		kRebakeNavMesh_Available,
		kRebakeNavMesh_RequestSent,
		kRebakeNavMesh_InProgress,
		kRebakeNavMesh_NotAvailable,
		kRebakeNavMesh_Default = kRebakeNavMesh_NotAvailable
	} ERebakeNavMeshMode;

public:
	LLOverlayBar(const LLRect& rect);
	~LLOverlayBar() override;

	void refresh() override;
	void draw() override;
	void reshape(S32 width, S32 height, bool call_from_parent = true) override;
	void setVisible(bool visible) override;

	LL_INLINE void setDirty()							{ mDirty = true; }

	// Callback functions used by llmediaremotectrl.cpp:
	static void toggleAudioVolumeFloater(void*);

	// Navmesh rebaking stuff (used by llstatusbar.cpp and
	// hbviewerautomation.cpp).

	LL_INLINE bool isNavmeshDirty() const
	{
		return mRebakeNavMeshMode == kRebakeNavMesh_Available;
	}

	LL_INLINE bool isNavmeshRebaking() const
	{
		return mRebakeNavMeshMode == kRebakeNavMesh_RequestSent ||
			   mRebakeNavMeshMode == kRebakeNavMesh_InProgress;
	}

	LL_INLINE bool canRebakeRegion() const			{ return mCanRebakeRegion; }

	// Lua status bar icon action
 	void setLuaFunctionButton(const std::string& label,
							  const std::string& command,
							  const std::string& tooltip);

private:
	void layoutButtons();

	// Navmesh rebaking stuff
	void setRebakeMode(ERebakeNavMeshMode mode);
	void handleAgentState(bool can_rebake_region);
	void handleRebakeNavMeshResponse(bool status_response);
	void handleNavMeshStatus(const LLPathfindingNavMeshStatus& statusp);
	void handleRegionBoundaryCrossed();
	void createNavMeshStatusListenerForCurrentRegion();

	static void* createMasterRemote(void* userdata);
	static void* createParcelMusicRemote(void* userdata);
	static void* createParcelMediaRemote(void* userdata);
	static void* createSharedMediaRemote(void* userdata);
	static void* createVoiceRemote(void* userdata);

	static void onClickIMReceived(void* data);
	static void onClickSetNotBusy(void* data);
	static void onClickPublicBaking(void* data);
	static void onClickMouselook(void* data);
	static void onClickStandUp(void* data);
	static void onClickResetView(void* data);
 	static void onClickFlycam(void* data);
 	static void onClickRebakeRegion(void* data);
 	static void onClickLuaFunction(void* data);

private:
	LLVoiceRemoteCtrl*		mVoiceRemote;
	LLMediaRemoteCtrl*		mSharedMediaRemote;
	LLMediaRemoteCtrl*		mParcelMediaRemote;
	LLMediaRemoteCtrl*		mParcelMusicRemote;
	LLMediaRemoteCtrl*		mMasterRemote;

	LLButton*				mBtnIMReceiced;
	LLButton*				mBtnSetNotBusy;
	LLButton*				mBtnFlyCam;
	LLButton*				mBtnMouseLook;
	LLButton*				mBtnStandUp;
	LLButton*				mBtnPublicBaking;
	LLButton*				mBtnRebakeRegion;
	LLButton*				mBtnLuaFunction;

	LLCachedControl<S32>	mStatusBarPad;

	S32						mVoiceRemoteWidth;
	S32						mParcelMediaRemoteWidth;
	S32						mSharedMediaRemoteWidth;
	S32						mParcelMusicRemoteWidth;
	S32						mMasterRemoteWidth;

	U32						mLastIMsCount;
	std::string				mIMReceivedlabel;

	std::string				mLuaCommand;

	LLFrameTimer			mUpdateTimer;

	// Navmesh rebaking stuff
	ERebakeNavMeshMode							mRebakeNavMeshMode;
	LLPathfindingNavMesh::navmesh_slot_t		mNavMeshSlot;
	boost::signals2::connection					mRegionCrossingSlot;
	LLPathfindingManager::agent_state_slot_t	mAgentStateSlot;
	LLUUID										mRebakingNotificationID;
	bool										mCanRebakeRegion;

	bool					mBuilt;
	bool					mDirty;
};

extern LLOverlayBar* gOverlayBarp;
