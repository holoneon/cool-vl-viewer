/**
 * @file hbfloatersoundlist.h
 * @brief HBFloaterSoundsList class definition
 *
 * This class implements a floater where all sounds are listed, allowing
 * the user to mute a source or stop any sound.
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 *
 * Copyright (c) 2014, Henri Beauchamp.
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

#include "llfloater.h"

#include "llmutelist.h"

class LLButton;
class LLFlyoutButton;
class LLCheckBoxCtrl;
class LLMessageSystem;
class LLScrollListCtrl;
class LLVector3d;

class HBFloaterSoundsList final
:	public LLFloater,
	public LLFloaterSingleton<HBFloaterSoundsList>,
	public LLMuteListObserver
{
	friend class LLUISingleton<HBFloaterSoundsList,
							   VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBFloaterSoundsList);

public:
	~HBFloaterSoundsList() override;

	static LLVector3d selectedLocation();

	// Used in llviewermessage.cpp to inform us we changed region
	static void newRegion();

	static void processObjectPropertiesFamily(LLMessageSystem* msg);

	// Used as a callback to avatar name resolution, as well as in
	// hbviewerautomation.cpp when changing the blocked sounds list.
	static void setDirty();

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBFloaterSoundsList(const LLSD&);

	bool postBuild() override;
	void draw() override;

	// LLMuteListObserver interface
	void onChange() override;

	void setButtonsStatus();

	void requestInfo(const LLUUID& object_id);

	static void onPlaySoundBtn(LLUICtrl* ctrl, void* userdata);
	static void onBlockSoundBtn(LLUICtrl* ctrl, void* userdata);
	static void onMuteOwnerBtn(void* userdata);
	static void onShowSourceBtn(LLUICtrl* ctrl,void* userdata);
	static void onMuteObjectBtn(LLUICtrl* ctrl, void* userdata);

	static void onDoubleClick(void* userdata);
	static void onSelectSound(LLUICtrl*, void* userdata);

	enum SOUNDS_COLUMN_ORDER
	{
		LIST_SOUND = 0,
		LIST_OBJECT,
		LIST_OWNER,
		LIST_SOURCE_ID,
		LIST_OBJECT_ID,
		LIST_OWNER_ID,
	};

private:
	LLFlyoutButton*		mPlayFlyoutBtn;
	LLFlyoutButton*		mBlockSoundBtn;
	LLButton*			mMuteOwnerBtn;
	LLFlyoutButton*		mShowFlyoutBtn;
	LLFlyoutButton*		mMuteFlyoutBtn;
	LLCheckBoxCtrl*		mFreezeCheck;
	LLScrollListCtrl*	mSoundsList;

	LLUUID				mTrackingID;
	LLVector3d			mTrackingLocation;
	LLVector3d			mSelectedLocation;

	F32					mLastUpdate;

	bool				mIsDirty;
	bool				mTracking;

	std::string			mNoneString;
	std::string			mLoadingString;
	std::string			mAttachmentString;

	uuid_list_t			mIgnoredSounds;
	uuid_list_t			mRequests;

	typedef fast_hmap<LLUUID, std::string> names_map_t;
	static names_map_t	sObjectNames;

	typedef fast_hmap<LLUUID, LLUUID> groups_map_t;
	static groups_map_t	sGroupOwnedObjects;
};
