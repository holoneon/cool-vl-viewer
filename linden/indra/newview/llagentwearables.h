/** 
 * @file llagentwearables.h
 * @brief LLAgentWearables class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include <vector>

#include "llinitdestroyclass.h"
#include "llinventory.h"
#include "llmemory.h"
#include "llstring.h"
#include "lluuid.h"
#include "llwearabledata.h"

#include "llinventorymodel.h"
#include "llviewerinventory.h"

// LL apparently removed silently these restrictions (underwears "always on"
// for teens) from their own viewer, so... Set to 1 to reenable if ever needed
// again. HB
#define LL_TEEN_WERABLE_RESTRICTIONS 0

class LLInitialWearablesFetch;
class LLViewerObject;
class LLViewerWearable;
class LLVOAvatarSelf;
struct HBNewOutfitData;

class LLAgentWearables final : public LLInitClass<LLAgentWearables>,
							   public LLWearableData
{
	friend class LLInitialWearablesFetch;
	friend class LLAddWearableToInventoryCallback;
	friend class LLCreateStandardWearablesDoneCallback;
	friend class LLSendAgentWearablesUpdateCallback;

protected:
	LOG_CLASS(LLAgentWearables);

public:
	//--------------------------------------------------------------------
	// Constructors / destructors / Initializers
	//--------------------------------------------------------------------

	LLAgentWearables();

	// LLInitClass interface
	static void initClass() {}

	void setAvatarObject(LLVOAvatarSelf* avatar);
	void createStandardWearables(bool female); 

protected:
	void createStandardWearablesDone(S32 type, U32 index);
	void createStandardWearablesAllDone();

	//--------------------------------------------------------------------
	// Queries
	//--------------------------------------------------------------------
public:
	bool isWearingItem(const LLUUID& item_id) const;
	bool isWearableModifiable(LLWearableType::EType type, U32 index) const;
	bool isWearableModifiable(const LLUUID& item_id) const;

	bool isWearableCopyable(LLWearableType::EType type, U32 index) const;
	bool areWearablesLoaded() const;
	LL_INLINE bool isSettingOutfit() const			{ return mIsSettingOutfit; }
	void updateWearablesLoaded();
	bool canMoveWearable(const LLUUID& item_id, bool closer_to_body) const;

	// Note: False for shape, skin, eyes, and hair, unless you have MORE than 1.
	bool canWearableBeRemoved(const LLViewerWearable* wearable) const;

	void animateAllWearableParams(F32 delta, bool upload_bake);

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	const LLUUID& getWearableItemID(LLWearableType::EType t, U32 idx) const;
	const LLUUID& getWearableAssetID(LLWearableType::EType t, U32 idx) const;
	const LLViewerWearable* getWearableFromItemID(const LLUUID& item_id) const;
	LLViewerWearable* getWearableFromItemID(const LLUUID& item_id);
	LLViewerWearable* getWearableFromAssetID(const LLUUID& asset_id);
	LLViewerWearable* getViewerWearable(LLWearableType::EType type, U32 idx);
	const LLViewerWearable* getViewerWearable(LLWearableType::EType type,
											  U32 idx) const;
	LLViewerInventoryItem* getWearableInventoryItem(LLWearableType::EType type,
													U32 idx);
	static bool selfHasWearable(LLWearableType::EType type);

	//--------------------------------------------------------------------
	// Setters
	//--------------------------------------------------------------------

private:
	void wearableUpdated(LLWearable* wearable, bool removed) override;

public:
	void setWearableItem(LLInventoryItem* new_item, LLViewerWearable* wearable,
						 bool do_append = false);
	void setWearableOutfit(const LLInventoryItem::item_array_t& items,
						   const std::vector<LLViewerWearable*>& wearables,
						   bool remove);
	void setWearableName(const LLUUID& item_id, const std::string& new_name);

	// *TODO: Move this into llappearance/LLWearableData ?
	LLLocalTextureObject* addLocalTextureObject(LLWearableType::EType type,
												LLAvatarAppearanceDefines::ETextureIndex texture_type,
												U32 index);

protected:
	void setWearableFinal(LLInventoryItem* new_item,
						  LLViewerWearable* new_wearable,
						  bool do_append = false);

	void addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
									 LLViewerWearable* wearable, 
									 const LLUUID& category_id = LLUUID::null,
									 bool notify = true);
	void addWearabletoAgentInventoryDone(LLWearableType::EType type, U32 index,
										 const LLUUID& item_id,
										 LLViewerWearable* wearable);
	void recoverMissingWearable(LLWearableType::EType type, U32 index);
	void recoverMissingWearableDone();

	//--------------------------------------------------------------------
	// Removing wearables
	//--------------------------------------------------------------------
public:
	void removeWearable(LLWearableType::EType type, bool do_remove_all,
						U32 index);
private:
	void removeWearableFinal(LLWearableType::EType type, bool do_remove_all,
							U32 index);
protected:
	static bool onRemoveWearableDialog(const LLSD& notification,
									   const LLSD& response);

	//--------------------------------------------------------------------
	// Server Communication
	//--------------------------------------------------------------------
public:
	// Processes the initial wearables update message
	static void	 processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys,
													void** user_data);

protected:
	void invalidateBakedTextureHash(LLMD5& hash) const override;

	void sendAgentWearablesUpdate();
	void sendAgentWearablesRequest();
	void queryWearableCache();
	void updateServer();
	static void onInitialWearableAssetArrived(LLViewerWearable* wearable,
											  void* userdata);

	//--------------------------------------------------------------------
	// Outfits
	//--------------------------------------------------------------------
public:
	S32 getWearableTypeAndIndex(LLViewerWearable* wearable,
								LLWearableType::EType& type);

	void makeNewOutfit(const std::string& new_folder_name,
					   const uuid_vec_t& wearables_to_include,
					   const uuid_vec_t& attachments_to_include,
					   bool rename_clothing);

private:
	void makeNewOutfitCopy(const LLUUID& cat_id, HBNewOutfitData* datap);
	void makeNewOutfitDone(LLWearableType::EType type, U32 index); 

	//--------------------------------------------------------------------
	// Save Wearables
	//--------------------------------------------------------------------
public:
	void saveWearableAs(LLWearableType::EType type, U32 index,
						const std::string& new_name,
						bool save_in_lost_and_found = false);
	void saveWearable(LLWearableType::EType type, U32 index,
					  bool send_update = true,
					  const std::string& new_name = LLStringUtil::null);
	void saveAllWearables();
	void revertWearable(LLWearableType::EType type, U32 index);

	//--------------------------------------------------------------------
	// Static UI hooks
	//--------------------------------------------------------------------
public:
	static void userRemoveWearable(LLWearableType::EType type, U32 index);
	static void userRemoveWearablesOfType(LLWearableType::EType type);
	static void userRemoveAllClothes();
	static void userRemoveAllClothesStep2(bool proceed, void*);

	typedef std::vector<LLViewerObject*> llvo_vec_t;

	static void userRemoveMultipleAttachments(llvo_vec_t& llvo_array);
	static void userRemoveAllAttachments(bool only_temp_attach = false);
	static void userAttachMultipleAttachments(LLInventoryModel::item_array_t& items);

	// These methods are for overriding the old initial wearables update
	// message logic in SL, for when that message will stop being sent...
	// They are only used by the LLAppearanceMgr::checkOutfit() function.
	LL_INLINE bool initialWearablesUpdateReceived()	{ return mInitialWearablesUpdateReceived; }
	void setInitialWearablesUpdateReceived();
	LL_INLINE void setWearablesLoaded()				{ mWearablesLoaded = true; }

private:
	// All wearables of a certain type (EG all shirts)
	typedef std::vector<LLWearable*> wearableentry_vec_t;
	// Wearable "categories" arranged by wearable type:
	typedef std::map<LLWearableType::EType,
					 wearableentry_vec_t> wearableentry_map_t;
	wearableentry_map_t	mWearableDatas;

	bool				mInitialWearablesUpdateReceived;
	bool				mWearablesLoaded;
	bool				mIsSettingOutfit;
};

extern LLAgentWearables gAgentWearables;
extern bool gWearablesListDirty;
