/**
 * @file llaisapi.h
 * @brief classes and functions definitions for interfacing with the v3+ ais
 * inventory service.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "llcoproceduremanager.h"
#include "llcorehttputil.h"
#include "hbfastmap.h"
#include "llhttpretrypolicy.h"

#include "llviewerinventory.h"

// Purely static class
class AISAPI
{
	AISAPI() = delete;
	~AISAPI() = delete;

protected:
	LOG_CLASS(AISAPI);

public:
	typedef enum : U32
	{
		COPYINVENTORY,
		SLAMFOLDER,
		REMOVECATEGORY,
		REMOVEITEM,
		PURGEDESCENDENTS,
		UPDATECATEGORY,
		UPDATEITEM,
		COPYLIBRARYCATEGORY,
		CREATEINVENTORY,
		// Note: keep all fetch types >= FETCHITEM or change AISUpdate(). HB
		FETCHITEM,
		FETCHCATEGORYCHILDREN,
		FETCHCATEGORYCATEGORIES,
		FETCHCATEGORYSUBSET,
		FETCHCOF,
		FETCHORPHANS,
		FETCHCATEGORYLINKS
	} ECommandType;

	typedef std::function<void(const LLUUID& item_id)> completion_t;

	static bool isAvailable(bool override_setting = false);

	static void createInventory(const LLUUID& parent_id,
								const LLSD& new_inventory,
								completion_t callback = completion_t());

	static void slamFolder(const LLUUID& folder_id, const LLSD& new_inventory,
						   completion_t callback = completion_t());

	static void removeCategory(const LLUUID& cat_id,
							   completion_t callback = completion_t());

	static void removeItem(const LLUUID& item_id,
						   completion_t callback = completion_t());

	static void purgeDescendents(const LLUUID& cat_id,
								 completion_t callback = completion_t());

	static void updateCategory(const LLUUID& cat_id, const LLSD& updates,
							   completion_t callback = completion_t());

	static void updateItem(const LLUUID& item_id, const LLSD& updates,
						   completion_t callback = completion_t());

	static void fetchItem(const LLUUID& item_id, bool library = false,
						  completion_t callback = completion_t());

	static void fetchCategoryChildren(const LLUUID& cat_id,
									  bool library = false,
									  bool recursive = false,
									  completion_t callback = completion_t(),
									  U32 depth = 0);

	static void fetchCategoryCategories(const LLUUID& cat_id,
										bool library = false,
										bool recursive = false,
										completion_t callback = completion_t(),
										U32 depth = 0);

	static void fetchCategorySubset(const LLUUID& cat_id,
									const uuid_vec_t& children,
									bool library = false,
									bool recursive = false,
									completion_t callback = completion_t(),
									U32 depth = 0);

	static void fetchCategoryLinks(const LLUUID& cat_id,
								   completion_t callback);

	static void fetchCOF(completion_t callback = completion_t());

	static void fetchOrphans(completion_t callback = completion_t());

	static void copyLibraryCategory(const LLUUID& source_id,
									const LLUUID& dest_id,
									bool copy_subfolders,
									completion_t callback = completion_t());

private:
	typedef std::function<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t,
							   const std::string, LLSD,
							   LLCore::HttpOptions::ptr_t,
							   LLCore::HttpHeaders::ptr_t)> invokationFn_t;

	static void enqueueAISCommand(const std::string& proc_name,
								  LLCoprocedureManager::coprocedure_t proc);
	static void onIdle(void*);

	static bool getInvCap(std::string& cap);
	static bool getLibCap(std::string& cap);

	static void invokeAISCommandCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
									 invokationFn_t invoke, std::string url,
									 LLUUID target_id, LLSD body,
									 completion_t callback, U32 type);

private:
	typedef std::pair<std::string,
					  LLCoprocedureManager::coprocedure_t> ais_query_item_t;
	static std::list<ais_query_item_t> sPostponedQuery;
};

class AISUpdate
{
protected:
	LOG_CLASS(AISUpdate);

public:
	AISUpdate(const LLSD& update, U32 command_type, const LLSD& request_body);

	void parseUpdate(const LLSD& update);
	void parseMeta(const LLSD& update);
	void parseContent(const LLSD& update);
	void parseUUIDArray(const LLSD& content, const std::string& name,
						uuid_list_t& ids);
	void parseLink(const LLSD& link_map, U32 depth);
	void parseItem(const LLSD& link_map);
	void parseCategory(const LLSD& link_map, U32 depth);
	void parseDescendentCount(const LLUUID& cat_id, bool links_only,
							  const LLSD& embedded);
	void parseEmbedded(const LLSD& embedded, U32 depth);
	void parseEmbeddedLinks(const LLSD& links, U32 depth);
	void parseEmbeddedItems(const LLSD& links);
	void parseEmbeddedCategories(const LLSD& links, U32 depth);
	void parseEmbeddedItem(const LLSD& item);
	void parseEmbeddedCategory(const LLSD& category, U32 depth);
	void doUpdate();

private:
	void checkTimeout();
	void clearParseResults();

private:
	typedef fast_hmap<LLUUID, S32> uuid_int_map_t;
	uuid_int_map_t			mCatDescendentDeltas;
	uuid_int_map_t			mCatDescendentsKnown;
	uuid_int_map_t			mCatVersionsUpdated;

	typedef fast_hmap<LLUUID, LLPointer<LLViewerInventoryItem> >
		deferred_item_map_t;
	deferred_item_map_t		mItemsCreated;
	deferred_item_map_t		mItemsUpdated;
	deferred_item_map_t		mItemsLost;

	typedef fast_hmap<LLUUID, LLPointer<LLViewerInventoryCategory> >
		deferred_category_map_t;
	deferred_category_map_t	mCategoriesCreated;
	deferred_category_map_t	mCategoriesUpdated;

	// These keep track of UUIDs mentioned in meta values. Useful for filtering
	// out which content we are interested in.
	uuid_list_t				mObjectsDeletedIds;
	uuid_list_t				mItemIds;
	uuid_list_t				mCategoryIds;

	LLTimer					mTimer;
	U32						mFetchDepth;
	U32						mType;
	bool					mFetch;
};
