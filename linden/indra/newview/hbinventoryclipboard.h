/**
 * @file hbinventoryclipboard.h
 * @brief HBInventoryClipboard class declaration
 * This is a full rewrite/expansion of LL's original LLInventoryClipboard class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012-2023, Henri Beauchamp.
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

#include "hbfastmap.h"
#include "llinventorytype.h"
#include "lluuid.h"

class LLInventoryItem;

// Purely static class
class HBInventoryClipboard
{
public:
	HBInventoryClipboard() = delete;
	~HBInventoryClipboard() = delete;

	///////////////////////////////////////////////////////////////////////////
	// Inventory objects management

	// Empties out the objects clipboard
	LL_INLINE static void reset()
	{
		sObjects.clear();
		sCutObjects.clear();
	}

	// Adds to the current list.
	LL_INLINE static void add(const LLUUID& object_id)	
	{
		sObjects.emplace_back(object_id);
	}

	// Add for Cut operation
	LL_INLINE static void addCut(const LLUUID& object_id)
	{
		sCutObjects.emplace_back(object_id);
	}

	// Stores a single inventory object
	LL_INLINE static void store(const LLUUID& object_id)
	{
		reset();
		add(object_id);
	}

	// Stores an array of objects
	static void store(const uuid_vec_t& inventory_objects);

	// Gets the objects in the clipboard by copying them into the vector.
	static void retrieve(uuid_vec_t& inventory_objects);

	// Gets the objects in the clipboard by copying them into the vector.
	static void retrieveCuts(uuid_vec_t& inventory_objects);

	// These methods return true when object_id is in the corresponding
	// clipboard
	static bool isCopied(const LLUUID& object_id);
	static bool isCut(const LLUUID& object_id);

	// The following three methods return true if the clipboard contains
	// something that can be pasted.
	LL_INLINE static bool hasCopiedContents()	{ return !sObjects.empty(); }

	LL_INLINE static bool hasCutContents()
	{
		return !sCutObjects.empty();
	}

	LL_INLINE static bool hasContents()
	{
		return !sObjects.empty() || !sCutObjects.empty();
	}

	///////////////////////////////////////////////////////////////////////////
	// Inventory assets management

	// Empties out the assets clipboard
	LL_INLINE static void resetAssets()			{ sAssets.clear(); }

	// Adds to the current list of assets. Also copies the asset Id to the text
	// clipboard unless false is passed for 'copy_id_to_text_clipboard'.
	// Note: if the asset Id is null, it is not stored/copied.
	static void addAsset(const LLUUID& asset_id, LLInventoryType::EType type,
						 bool copy_id_to_text_clipboard = true);

	// Stores a single asset Id. Also copies the asset Id to the text clipboard
	// unless false is passed for 'copy_id_to_text_clipboard'.
	// Note: if the asset Id is null, it is not stored/copied.
	static void storeAsset(const LLUUID& asset_id, LLInventoryType::EType type,
						   bool copy_id_to_text_clipboard = true);

	// Stores the asset Id associated with the passed inventory item. Also
	// copies the asset Id to the text clipboard unless false is passed for
	// 'copy_id_to_text_clipboard'.
	// Note: if the asset Id is null, it is not stored/copied.
	static void storeAsset(const LLInventoryItem* itemp,
						   bool copy_id_to_text_clipboard = true);

	// Gets the assets of the specified inventory type stored in the clipboard
	// by copying their UUID into the vector.
	static void retrieveAssets(uuid_vec_t& inventory_assets,
							   LLInventoryType::EType type);

	// Returns true if assets of the specified inventory type are stored.
	static bool hasAssets(LLInventoryType::EType type);

private:
	static uuid_vec_t	sObjects;
	static uuid_vec_t	sCutObjects;

	typedef fast_hmap<LLUUID, LLInventoryType::EType> assets_map_t;
	static assets_map_t	sAssets;
};
