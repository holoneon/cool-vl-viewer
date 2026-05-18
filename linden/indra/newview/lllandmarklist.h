/**
 * @file lllandmarklist.h
 * @brief Landmark asset list class
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

#include <functional>
#include <map>					// For multimap

#include "llassetstorage.h"
#include "hbfastmap.h"
#include "llerror.h"
#include "lllandmark.h"
#include "lluuid.h"

class LLLineEditor;
class LLInventoryItem;

class LLLandmarkList
{
protected:
	LOG_CLASS(LLLandmarkList);

public:
	~LLLandmarkList();

	bool assetExists(const LLUUID& asset_id);

	typedef std::function<void(LLLandmark*)> loaded_callback_t;
	LLLandmark* getAsset(const LLUUID& asset_id,
						 loaded_callback_t cb = nullptr);

	static void processGetAssetReply(const LLUUID& asset_id,
									 LLAssetType::EType type,
									 void* user_data, S32 status,
									 LLExtStat ext_status);

	// Returns true if loading the landmark with given asset_id has been
	// requested but is not complete yet.
	bool isAssetInLoadedCallbackMap(const LLUUID& asset_id);

private:
	void onRegionHandle(const LLUUID& landmark_id);
	void makeCallbacks(const LLUUID& landmark_id);
	void eraseCallbacks(const LLUUID& asset_id);
	void markBadAsset(const LLUUID& asset_id);

private:
	typedef fast_hmap<LLUUID, LLLandmark*> landmark_list_t;
	landmark_list_t				mList;

	uuid_list_t					mBadList;
	uuid_list_t					mWaitList;

	typedef fast_hmap<LLUUID, F32> landmark_requested_list_t;
	landmark_requested_list_t	mRequestedList;

	// *TODO: make the callback multimap a template class and make use of it
	// here and in LLLandmark.
	typedef std::multimap<LLUUID, loaded_callback_t> loaded_callback_map_t;
	loaded_callback_map_t		mLoadedCallbackMap;
};

extern LLLandmarkList gLandmarkList;
