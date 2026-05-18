/**
 * @file llwearablelist.h
 * @brief LLWearableList class header file
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

#include "hbfastmap.h"
#include "llsingleton.h"

#include "llviewerwearable.h"

// Globally constructed; be careful that there is no dependency with gAgent.

// *BUG: mList's system of mapping between asset Ids and wearables is flawed
// since LLWearable* has an associated itemID, and you can have multiple
// inventory items pointing to the same asset (i.e. more than one item Id per
// asset Id). EXT-6252

class LLWearableList final : public LLSingleton<LLWearableList>
{
	friend class LLSingleton<LLWearableList>;

protected:
	LOG_CLASS(LLWearableList);

public:
	LLWearableList()					{}
	~LLWearableList() override;

	void cleanup();

	LL_INLINE S32 getLength()			{ return mList.size(); }

	void getAsset(const LLAssetID& asset_id, const std::string& wearable_name,
				  LLAvatarAppearance* avatarp, LLAssetType::EType asset_type,
				  void (*asset_arrived_callback)(LLViewerWearable*, void*),
				  void* userdata);

	LLViewerWearable* createCopy(LLViewerWearable* old_wearable,
								 const std::string& new_name = std::string());
	LLViewerWearable* createNewWearable(LLWearableType::EType type,
										LLAvatarAppearance* avatarp);

private:
	// Used for the create... functions
	LLViewerWearable* generateNewWearable();

	// Callback
	static void processGetAssetReply(const char* filename,
									 const LLAssetID& asset_id,
									 void* user_data, S32 status,
									 LLExtStat ext_status);

private:
	typedef fast_hmap<LLUUID, LLViewerWearable*> wearable_map_t;
	wearable_map_t mList;
};
