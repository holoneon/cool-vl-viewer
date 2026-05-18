/**
 * @file llviewerwearable.h
 * @brief LLViewerWearable class header file
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

#include "llavatarappearance.h"
#include "llavatarappearancedefines.h"
#include "llextendedstatus.h"
#include "llwearable.h"
#include "lluuid.h"

class LLViewerWearable : public LLWearable
{
	friend class LLWearableList;

protected:
	LOG_CLASS(LLViewerWearable);

	//--------------------------------------------------------------------
	// Constructors and destructors
	//--------------------------------------------------------------------
private:
	// Private constructor used by LLWearableList
	LLViewerWearable(const LLTransactionID& transactionID);
	LLViewerWearable(const LLAssetID& assetID);

public:
	~LLViewerWearable() override = default;

	LL_INLINE LLViewerWearable* asViewerWearable() override
	{
		return this;
	}

	LL_INLINE const LLViewerWearable* asViewerWearable() const override
	{
		return this;
	}

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
	LL_INLINE const LLUUID& getItemID() const			{ return mItemID; }
	LL_INLINE const LLAssetID& getAssetID() const		{ return mAssetID; }

	LL_INLINE const LLTransactionID& getTransactionID() const
	{
		return mTransactionID;
	}

	void setItemID(const LLUUID& item_id);

	bool isDirty() const;
	bool isOldVersion() const;

	void writeToAvatar(LLAvatarAppearance* avatarp) override;

	LL_INLINE void removeFromAvatar(bool upload_bake)
	{
		LLViewerWearable::removeFromAvatar(mType, upload_bake);
	}

	static void removeFromAvatar(LLWearableType::EType type, bool upload_bake);

	EImportResult importStream(std::istream& input_stream,
							   LLAvatarAppearance* avatarp) override;

	void setParamsToDefaults();
	void setTexturesToDefaults();

	// true when doing preview renders, some updates will be suppressed.
	LL_INLINE void setVolatile(bool is_volatile)		{ mVolatile = is_volatile; }
	LL_INLINE bool getVolatile()						{ return mVolatile; }

	LLUUID getDefaultTextureImageID(LLAvatarAppearanceDefines::ETextureIndex index) override;

	void saveNewAsset() const;
	static void onSaveNewAssetComplete(const LLUUID& asset_uuid,
									   void* user_data, S32 status,
									   LLExtStat ext_status);

	void copyDataFrom(const LLViewerWearable* src);

	friend std::ostream& operator<<(std::ostream &s,
									const LLViewerWearable &w);

	void revertValues() override;
	void saveValues() override;

	LL_INLINE void revertValuesWithoutUpdate()			{ LLWearable::revertValues(); }

	// Something happened that requires the wearable's label to be updated
	// (e.g. worn/unworn).
	void setUpdated() const override;

	// The wearable was worn. make sure the name of the wearable object matches
	// the LLViewerInventoryItem, not the wearable asset itself.
	void refreshName();

	// Update the baked texture hash.
	void addToBakedTextureHash(LLMD5& hash) const override;

protected:
	LLAssetID				mAssetID;
	LLTransactionID			mTransactionID;
	// ID of the inventory item in the agent's inventory:
	LLUUID					mItemID;

	// true when rendering preview images. Can suppress some updates.
	bool					mVolatile;

	// Cache used by getDefaultTextureImageID() for speed
	static std::map<LLAvatarAppearanceDefines::ETextureIndex, LLUUID> sCachedTextures;
};

class LLWearableSaveData
{
public:
	LLWearableSaveData(LLWearableType::EType type);
	~LLWearableSaveData();

	LL_INLINE static void resetSavedWearableCount()		{ sSavedWearableCount = 0; }
	LL_INLINE static bool pendingSavedWearables()		{ return sSavedWearableCount != 0; }

public:
	LLWearableType::EType	mType;
	bool					mResetCOFTimer;

	static bool				sResetCOFTimer;

private:
	static U32				sSavedWearableCount;
};
