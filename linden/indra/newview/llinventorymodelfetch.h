/**
 * @file llinventorymodelfetch.h
 * @brief LLInventoryModelFetch class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include <deque>

#include "llsingleton.h"
#include "lluuid.h"

class LLInventoryItem;
class LLTimer;

// This class handles background fetches, which are fetches of inventory
// folder. Fetches can be recursive or not.

class LLInventoryModelFetch final : public LLSingleton<LLInventoryModelFetch>
{
	friend class LLSingleton<LLInventoryModelFetch>;

protected:
	LOG_CLASS(LLInventoryModelFetch);

public:
	LLInventoryModelFetch();

	// Start and stop background breadth-first fetching of inventory contents.
	// This gets triggered when performing a filter-search.
	void start(const LLUUID& cat_id = LLUUID::null, bool recursive = true);
	void scheduleFolderFetch(const LLUUID& cat_id, bool force = false);
	void scheduleItemFetch(const LLUUID& item_id, bool force = false);

	LL_INLINE bool backgroundFetchActive() const
	{
		return mBackgroundFetchActive;
	}

	// Completing the fetch once per session should be sufficient:
	LL_INLINE bool isEverythingFetched() const
	{
		return mAllRecursiveFoldersFetched;
	}

	LL_INLINE bool libraryFetchStarted() const
	{
		return mRecursiveLibraryFetchStarted;
	}

	bool libraryFetchCompleted() const;

	LL_INLINE bool libraryFetchInProgress() const
	{
		return mRecursiveLibraryFetchStarted && !libraryFetchCompleted();
	}

	LL_INLINE bool inventoryFetchStarted() const
	{
		return mRecursiveInventoryFetchStarted;
	}

	bool inventoryFetchCompleted() const;

	LL_INLINE bool inventoryFetchInProgress() const
	{
		return mRecursiveInventoryFetchStarted && !inventoryFetchCompleted();
	}

    void findLostItems();

	void incrFetchCount(S32 fetching);
	void incrFetchFolderCount(S32 fetching);

	bool isBulkFetchProcessingComplete() const;
	bool isFolderFetchProcessingComplete() const;
	void setAllFoldersFetched();

	void addRequestAtFront(const LLUUID& id, bool recursive, bool is_category);
	void addRequestAtBack(const LLUUID& id, bool recursive, bool is_category);

	void onAISContentsCallback(const uuid_vec_t& content_ids,
							  const LLUUID& response_id);
	void onAISFolderCallback(const LLUUID& cat_id,
							 const LLUUID& response_id, U32 fetch_type);

	LL_INLINE static void setUseAISFetching(bool b)	{ sUseAISFetching = b; }
	static bool useAISFetching();

	// Helpers for force-fetching inventory items and folders. HB
	static void forceFetchFolder(const LLUUID& cat_id);
	static void forceFetchItem(const LLUUID& item_id);
	// Use this when you got the item pointer (faster).
	static void forceFetchItem(const LLInventoryItem* itemp);

private:
	typedef enum : U32
	{
		FT_DEFAULT = 0,
		FT_FORCED,			  	// Non-recursively even if already loaded
		FT_CONTENT_RECURSIVE,	// Request content recursively
		FT_FOLDER_AND_CONTENT,	// Request folder, then content recursively
		FT_RECURSIVE,			// Request everything recursively
	} EFetchType;

	struct FetchQueueInfo
	{
		FetchQueueInfo(const LLUUID& id, U32 fetch_type, bool is_category)
		:	mUUID(id),
			mFetchType(fetch_type),
			mIsCategory(is_category)
		{
		}

		LLUUID	mUUID;
		U32		mFetchType;
		bool	mIsCategory;
	};

	void bulkFetch(const std::string& url);
	void bulkFetchAIS();
	void bulkFetchAIS(const FetchQueueInfo& fetch_info);

	void backgroundFetch();
	static void backgroundFetchCB(void*);	// Background fetch idle method

	bool fetchQueueContainsNoDescendentsOf(const LLUUID& cat_id) const;

private:
	typedef std::deque<FetchQueueInfo> fetch_queue_t;
	fetch_queue_t	mFetchFolderQueue;
	fetch_queue_t	mFetchItemQueue;

	uuid_list_t		mExpectedFolderIds;

	LLTimer			mFetchTimer;

#if 0	// Not yet used by the Cool VL Viewer. HB
	typedef boost::signals2::signal<void()> signal_t;
	signal_t		mFoldersFetchedSignal;
#endif

	S32				mFetchCount;
	S32				mLastFetchCount;
	S32				mFetchFolderCount;

 	bool			mRecursiveInventoryFetchStarted;
	bool			mRecursiveLibraryFetchStarted;
	bool			mAllRecursiveFoldersFetched;
	bool			mBackgroundFetchActive;
	bool			mFolderFetchActive;

	static bool		sUseAISFetching;
};
