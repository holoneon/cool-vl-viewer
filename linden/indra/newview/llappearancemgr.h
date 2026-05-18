/**
 * @file llappearancemgr.h
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llframetimer.h"
#include "llhttpretrypolicy.h"
#include "llpointer.h"
#include "lluuid.h"

#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llviewerwearable.h"

class LLViewerJointAttachment;
class LLWearableHoldingPattern;

//-----------------------------------------------------------------------------
// Misc callbacks and callback data structures
//-----------------------------------------------------------------------------

class LLWearOnAvatarCallback : public LLInventoryCallback
{
protected:
	LOG_CLASS(LLWearOnAvatarCallback);

public:
	LLWearOnAvatarCallback(bool do_replace = true)
	:	mReplace(do_replace)
	{
		// *TODO: track callbacks by (original) item UUID
		++sPendingCallbackCount;
		mCounterGeneration = sCurrentCounterGeneration;
	}

	void fire(const LLUUID& inv_item);

	static bool pendingCallbacks()				{ return sPendingCallbackCount > 0; }
	static void resetPendingCallbacks()			{ sPendingCallbackCount = 0; ++sCurrentCounterGeneration; }

protected:
	bool		mReplace;

private:
	U32			mCounterGeneration;
	static U32	sCurrentCounterGeneration;
	static U32	sPendingCallbackCount;
};

class LLRezAttachmentCallback : public LLInventoryCallback
{
protected:
	LOG_CLASS(LLRezAttachmentCallback);

public:
	LLRezAttachmentCallback(LLViewerJointAttachment* attachmentp,
							bool replace = false)
	: 	mAttach(attachmentp),
		mReplace(replace)
	{
		// *TODO: track callbacks by (original) item UUID
		++sPendingCallbackCount;
		mCounterGeneration = sCurrentCounterGeneration;
	}

	void fire(const LLUUID& inv_item);

	static bool pendingCallbacks()				{ return sPendingCallbackCount > 0; }
	static void resetPendingCallbacks()			{ sPendingCallbackCount = 0; ++sCurrentCounterGeneration; }

protected:
	~LLRezAttachmentCallback()					{}

private:
	LLViewerJointAttachment*	mAttach;
	bool						mReplace;
	U32							mCounterGeneration;
	static U32					sCurrentCounterGeneration;
	static U32					sPendingCallbackCount;
};

struct OnWearStruct
{
	OnWearStruct(const LLUUID& uuid, bool replace = true)
	:	mUUID(uuid),
		mReplace(replace)
	{
	}

	LLUUID	mUUID;
	bool	mReplace;
};

struct OnRemoveStruct
{
	OnRemoveStruct(const LLUUID& uuid)
	:	mUUID(uuid)
	{
	}

	LLUUID mUUID;
};

//-----------------------------------------------------------------------------

class LLAppearanceMgr
{
protected:
	LOG_CLASS(LLAppearanceMgr);

public:
	LLAppearanceMgr()
	:	mNeedsSyncAttachments(false),
		mNeedsSyncWearables(false),
		mForceServerSideRebake(false),
		mRebaking(true),
		mIsRestoringInitialOutfit(true),
		mOutfitRestorationRetried(false),
		mBakeRequestSent(false),
		mRestorationRetryDelayDelta(0),
		mLoadingNotificationID(LLUUID::null)
	{
	}

	typedef std::vector<LLInventoryModel::item_array_t> wearables_by_type_t;

	void wearOutfitByName(const std::string& name);

	void wearInventoryItemOnAvatar(LLInventoryItem* item, bool replace = true);

	void wearInventoryCategory(LLInventoryCategory* category,
							   bool copy, bool append);

	void wearInventoryCategoryOnAvatar(LLInventoryCategory* category,
									   bool append, bool replace = false);

	void removeInventoryCategoryFromAvatar(LLInventoryCategory* category);

	bool wearItemOnAvatar(const LLUUID& item_id_to_wear, bool replace = true);

#if 0	// Not used for now
	bool canAddWearables(const uuid_vec_t& item_ids);
#endif

	void rezAttachment(LLViewerInventoryItem* item,
					   LLViewerJointAttachment* attachment,
					   bool replace = false);

	void checkOutfit();		// periodic outfit checking and syncing
	LL_INLINE bool isRestoringInitialOutfit()	{ return mIsRestoringInitialOutfit; }
	LL_INLINE void resetCOFUpdateTimer()		{ mUpdateCOFTimer.reset(); }

	static void sortItemsByActualDescription(LLInventoryModel::item_array_t& items);

	// Check ordering information on wearables stored in links' descriptions
	// and update if it is invalid
	void updateClothingOrderingInfo(LLUUID cat_id);

	static const LLUUID getCOF(bool create = false);

	S32 getCOFVersion() const;
	void updateCOF() const;

	void requestServerAppearanceUpdate();

	void incrementCofVersion();

	bool isAvatarFullyBaked() const;
	void setRebaking(bool rebaking = true);
	LL_INLINE bool isRebaking()					{ return mRebaking; }

private:
	typedef enum e_restore_outfit_status {
		RETRY,
		FAILED,
		INCOMPLETE,
		DONE
	} ERestoreOutfitStatus;

	// Remove worn items not listed in outfit.xml
	void removeNonMatchingItems();

	// Called from checkOutfit() to restore the initial outfit after login.
	void restoreInitialOutfit(bool from_cof);
	// Try and restore outfit from outfit.xml
	ERestoreOutfitStatus restoreOutfit(bool can_retry);
	// Save the current outfit to outfit.xml
	void saveOutfit();

	// COF stuff...
	// <rant>Lindens be damned for this ugly COF concept !!!</rant>
	ERestoreOutfitStatus restoreOutfitFromCOF(bool can_retry);
	void cleanupCOF(const LLUUID& cof);
	void syncAttachmentLinksInCOF();
	void syncWearableLinksInCOF();
	void slamCOF();

	void getDescendentsOfAssetType(const LLUUID& category,
								   LLInventoryModel::item_array_t& items,
								   LLAssetType::EType type);

	void getDescendentsOfWearableTypes(const LLUUID& category,
									   LLInventoryModel::item_array_t& items);

	void getUserDescendents(const LLUUID& category,
							LLInventoryModel::item_array_t& wear_items,
							LLInventoryModel::item_array_t& obj_items,
							LLInventoryModel::item_array_t& gest_items);

	void serverAppearanceUpdateSuccess(const LLSD& result);
	void serverAppearanceUpdateFailure(const LLSD& http_results);

	// Detach all attachments not in keep_these list; when erase_worn is true,
	// worn attachments UUIDs are removed from keep_these as well.
	static void detachExtraAttachments(uuid_list_t& keep_these,
									   bool erase_worn = false);

	static void wearInventoryCategoryOnAvatarStep2(bool proceed, void* data);
	static void wearInventoryCategoryOnAvatarStep3(LLWearableHoldingPattern* holder);
	static void onWearableAssetFetch(LLViewerWearable* wearable, void* data);

	static void removeInventoryCategoryFromAvatarStep2(bool proceed, void* data);

	static bool onSetWearableDialog(const LLSD& notification,
									const LLSD& response,
									LLViewerWearable* old_wearable);

private:
	U32								mRestorationRetryDelayDelta;
	LLUUID							mLoadingNotificationID;
	LLPointer<LLHTTPRetryPolicy>	mBakeRetryPolicy;
	LLFrameTimer					mUpdateCOFTimer;
	bool							mIsRestoringInitialOutfit;
	bool							mOutfitRestorationRetried;
	bool							mBakeRequestSent;
	bool							mRebaking;

public:
	bool							mNeedsSyncAttachments;
	bool							mNeedsSyncWearables;
	bool							mForceServerSideRebake;
};

extern LLAppearanceMgr gAppearanceMgr;

std::string build_order_string(LLWearableType::EType type, U32 i);
