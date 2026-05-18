/**
 * @file lllandmarklist.cpp
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

#include "llviewerprecompiledheaders.h"

#include "lllandmarklist.h"

#include "llassetstorage.h"
#include "llfilesystem.h"
#include "llnotifications.h"
#include "llmessage.h"

#include "llagent.h"
#include "llappviewer.h"		// For gFrameTimeSeconds
#include "llviewerstats.h"

// Global
LLLandmarkList gLandmarkList;

// This limit is mostly arbitrary, but it should be below DEFAULT_QUEUE_SIZE
// pool size, which is 4096, to not overfill the pool if user has more than 4K
// of landmarks, and it should leave some space for other simultaneous asset
// requests.
constexpr S32 MAX_SIMULTANEOUS_REQUESTS = 512;

LLLandmarkList::~LLLandmarkList()
{
	for (auto it = mList.begin(), end = mList.end(); it != end; ++it)
	{
		delete it->second;
	}
	mList.clear();
}

LLLandmark* LLLandmarkList::getAsset(const LLUUID& asset_id,
									 loaded_callback_t cb)
{
	LLLandmark* landmark = get_ptr_in_map(mList, asset_id);
	if (landmark)
	{
		LLVector3d pos;
		if (cb && !landmark->getGlobalPos(pos))
		{
			// The landmark is not yet completely loaded
			mLoadedCallbackMap.emplace(asset_id, cb);
		}
		return landmark;
	}
	else if (gAssetStoragep && !mBadList.count(asset_id) &&
			 !mWaitList.count(asset_id))
	{
		constexpr F32 rerequest_time = 30.f; // 30 seconds between requests
		landmark_requested_list_t::iterator iter =
			mRequestedList.find(asset_id);
		if (iter == mRequestedList.end() ||
			gFrameTimeSeconds - iter->second >= rerequest_time)
		{
			if (cb)
			{
				mLoadedCallbackMap.emplace(asset_id, cb);
			}

			if (mRequestedList.size() > MAX_SIMULTANEOUS_REQUESTS)
			{
				// Postpone download until queue is not full any more
				mWaitList.emplace(asset_id);
				return NULL;
			}

			// Add to mRequestedList before calling getAssetData()
			mRequestedList[asset_id] = gFrameTimeSeconds;
			// Note that getAssetData() can callback immediately and cleans
			// mRequestedList.
			gAssetStoragep->getAssetData(asset_id, LLAssetType::AT_LANDMARK,
										 processGetAssetReply, NULL);
		}
	}
	return NULL;
}

//static
void LLLandmarkList::processGetAssetReply(const LLUUID& asset_id,
										  LLAssetType::EType type,
										  void* user_data, S32 status,
										  LLExtStat ext_status)
{
	if (status == 0)
	{
		LLFileSystem file(asset_id);
		S32 file_length = file.getSize();
		if (file_length <= 0)
		{
			llwarns << "Bad cached file length for asset Id " << asset_id
					<< ": " << file_length << llendl;
			gNotifications.add("UnableToLoadLandmark");
			gLandmarkList.markBadAsset(asset_id);
		}
		else
		{
			std::vector<char> buffer(file_length + 1);
			file.read((U8*)&buffer[0], file_length);
			buffer[file_length] = 0;

			LLLandmark* landmark =
				LLLandmark::constructFromString(&buffer[0], buffer.size());
			if (landmark)
			{
				gLandmarkList.mList[asset_id] = landmark;
				gLandmarkList.mRequestedList.erase(asset_id);

				LLVector3d pos;
				if (!landmark->getGlobalPos(pos))
				{
					LLUUID region_id;
					if (landmark->getRegionID(region_id))
					{
						// NOTE: the callback will be called when we get the
						// region handle.
						LLLandmark::requestRegionHandle(gMessageSystemp,
														gAgent.getRegionHost(),
														region_id,
														std::bind(&LLLandmarkList::onRegionHandle,
																  &gLandmarkList,
																  asset_id));
					}
					else
					{
						gLandmarkList.makeCallbacks(asset_id);
					}
				}
			}
			else
			{
				llwarns << "Corrupted cached file for asset Id " << asset_id
						<< llendl;
				gNotifications.add("UnableToLoadLandmark");
				gLandmarkList.markBadAsset(asset_id);
			}
		}
	}
	else
	{
		gViewerStats.incStat(LLViewerStats::ST_DOWNLOAD_FAILED);

		if (status == LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE)
		{
			gNotifications.add("LandmarkMissing");
		}
		else
		{
			gNotifications.add("UnableToLoadLandmark");
		}

		gLandmarkList.markBadAsset(asset_id);
	}

	// LLAssetStorage::getAssetData() may fire our callback immediately,
	// causing a recursion which is suboptimal for very large wait list.
	//'scheduling' indicates that we are inside request and should not be
	// launching more requests.
	static bool scheduling = false;
	if (!scheduling && gAssetStoragep && !gLandmarkList.mWaitList.empty())
	{
		scheduling = true;
		while (!gLandmarkList.mWaitList.empty() &&
			   gLandmarkList.mRequestedList.size() < MAX_SIMULTANEOUS_REQUESTS)
		{
			// Start a new download from the wait list
			uuid_list_t::iterator iter = gLandmarkList.mWaitList.begin();
			LLUUID id = *iter;
			gLandmarkList.mWaitList.erase(iter);
			// Add to mRequestedList before calling getAssetData()
			gLandmarkList.mRequestedList[id] = gFrameTimeSeconds;
			// Note that getAssetData() can callback immediately and cleans
			// mRequestedList.
			gAssetStoragep->getAssetData(id, LLAssetType::AT_LANDMARK,
										 processGetAssetReply, NULL);
		}
		scheduling = false;
	}
}

void LLLandmarkList::eraseCallbacks(const LLUUID& id)
{
	loaded_callback_map_t::iterator it;
	while ((it = mLoadedCallbackMap.find(id)) != mLoadedCallbackMap.end())
	{
		mLoadedCallbackMap.erase(it);
	}
}

void LLLandmarkList::markBadAsset(const LLUUID& asset_id)
{
	mBadList.emplace(asset_id);
	mRequestedList.erase(asset_id);
	eraseCallbacks(asset_id);
}

bool LLLandmarkList::assetExists(const LLUUID& asset_id)
{
	return mList.count(asset_id) != 0 || mBadList.count(asset_id) != 0;
}

bool LLLandmarkList::isAssetInLoadedCallbackMap(const LLUUID& asset_id)
{
	return mLoadedCallbackMap.count(asset_id) != 0;
}

void LLLandmarkList::onRegionHandle(const LLUUID& landmark_id)
{
	// Calculate the landmark global position. This should succeed since the
	// region handle is available.
	LLLandmark* landmark = getAsset(landmark_id);
	if (!landmark)
	{
		llwarns << "Got region handle but landmark " << landmark_id
				<< " is not found." << llendl;
		markBadAsset(landmark_id);
		return;
	}

	LLVector3d pos;
	if (!landmark->getGlobalPos(pos))
	{
		llwarns << "Got region handle but the global position for landmark "
				<< landmark_id << " is still unknown." << llendl;
		eraseCallbacks(landmark_id);
		return;
	}

	makeCallbacks(landmark_id);
}

void LLLandmarkList::makeCallbacks(const LLUUID& landmark_id)
{
	LLLandmark* landmark = getAsset(landmark_id);
	if (!landmark)
	{
		llwarns << "Landmark " << landmark_id << " not found." << llendl;
	}

	// Invoke all the callbacks here.
	loaded_callback_map_t::iterator it;
	while ((it = mLoadedCallbackMap.find(landmark_id)) !=
				mLoadedCallbackMap.end())
	{
		if (landmark)
		{
			it->second(landmark);
		}

		mLoadedCallbackMap.erase(it);
	}
}
