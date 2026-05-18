/**
 * @file llgesturemgr.h
 * @brief Manager for playing gestures on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llassetstorage.h"		// LLAssetType
#include "hbfastmap.h"

#include "llviewerinventory.h"

class LLMultiGesture;
class LLGestureStep;
class LLUUID;

class LLGestureManagerObserver
{
public:
	virtual ~LLGestureManagerObserver()	{}
	virtual void changed() = 0;
};

class LLGestureManager
{
protected:
	LOG_CLASS(LLGestureManager);

public:
	LLGestureManager();
	~LLGestureManager();

	// Loads and activates gestures on login
	void load(const LLSD& gestures);

	// Call once per frame to manage gestures
	void update();

	// Loads a gesture out of inventory into the in-memory active form. Note
	// that the inventory item must exist, so we can look up the  asset id.
	void activateGesture(const LLUUID& item_id);

	// Activate a list of gestures
	void activateGestures(LLViewerInventoryItem::item_array_t& items);

	// If you change a gesture, you need to build a new multigesture
	// and call this method.
	void replaceGesture(const LLUUID& item_id,
						LLMultiGesture* new_gesturep,
						const LLUUID& asset_id);
	void replaceGesture(const LLUUID& item_id, const LLUUID& asset_id);

	// Load gesture into in-memory active form. Can be called even if the
	// inventory item isn't loaded yet. inform_server true will send message
	// upstream to update database user_gesture_active table, which isn't
	// necessary on login. deactivate_similar will cause other gestures with
	// the same trigger phrase or keybinding to be deactivated.
	void activateGestureWithAsset(const LLUUID& item_id,
								  const LLUUID& asset_id, bool inform_server,
								  bool deactivate_similar);

	// Takes gesture out of active list and deletes it.
	void deactivateGesture(const LLUUID& item_id);

	// Deactivates all gestures that match either this trigger phrase or this
	// hot key.
	void deactivateSimilarGestures(LLMultiGesture* gesture,
								   const LLUUID& in_item_id);

	bool isGestureActive(const LLUUID& item_id);

	bool isGesturePlaying(const LLUUID& item_id);

	// Forces a gesture to be played, for example, if it is being previewed.
	void playGesture(LLMultiGesture* gesturep);
	void playGesture(const LLUUID& item_id);

	// Stops all requested or playing anims for this gesture. Also removes from
	// playing list.
	void stopGesture(LLMultiGesture* gesturep);
	void stopGesture(const LLUUID& item_id);

	// Trigger the first gesture that matches this key.
	// Returns true if it finds a gesture bound to that key.
	bool triggerGesture(KEY key, MASK mask);

	// Triggers all gestures referenced as substrings in this string
	bool triggerAndReviseString(const std::string& str,
								std::string* revised_string = NULL);

	S32 getPlayingCount() const;

	void addObserver(LLGestureManagerObserver* observer);
	void removeObserver(LLGestureManagerObserver* observer);
	void notifyObservers();

	bool matchPrefix(const std::string& in_str, std::string* out_str);

	// Copy item ids into the vector
	void getItemIDs(uuid_vec_t* ids);

	static void notifyLoadFailed(const LLUUID& item_id, S32 status);

protected:
	// Handle the processing of a single gesture
	void stepGesture(LLMultiGesture* gesturep);

	// Do a single step in a gesture
	void runStep(LLMultiGesture* gesturep, LLGestureStep* step);

	// Used by loadGesture
	static void onLoadComplete(const LLUUID& asset_uuid,
							   LLAssetType::EType type, void* user_data,
							   S32 status, LLExtStat);

public:
	std::vector<LLMultiGesture*>			mPlaying;

	// Maps inventory item_id to gesture
	typedef fast_hmap<LLUUID, LLMultiGesture*> item_map_t;
	// Active gestures. NOTE: The gesture pointer CAN BE NULL. This means that
	// there is a gesture with that item_id, but the asset data is still on its
	// way down from the server.
	item_map_t								mActive;

	std::vector<LLGestureManagerObserver*>	mObservers;

	std::string								mDeactivateSimilarNames;
	S32										mLoadingCount;
	bool									mValid;
};

extern LLGestureManager gGestureManager;
