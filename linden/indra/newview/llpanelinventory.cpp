/**
 * @file llpanelinventory.cpp
 * @brief LLPanelInventory class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

// Implementation of the panel inventory - used to view and control an object's
// inventory.

#include "llviewerprecompiledheaders.h"

#include <sstream>
#include <utility>					// For std::pair<>

#include "llpanelinventory.h"

#include "llaudioengine.h"
#include "llassetstorage.h"
#include "llcallbacklist.h"
#include "llgl.h"
#include "llinventory.h"
#include "llmenugl.h"
#include "llscrollcontainer.h"
#include "llmessage.h"
#include "lltrans.h"

#include "llagent.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterproperties.h"
#include "llfolderview.h"
#include "llinventoryactions.h"
#include "llinventorybridge.h"		// For set_menu_entries_state()
#include "llinventoryicon.h"		// For getIcon()
#include "llinventorymodel.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
//MK
#include "mkrlinterface.h"
//mk
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltooldraganddrop.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llwearable.h"
#include "roles_constants.h"

//-----------------------------------------------------------------------------
// LLTaskInvFVBridge class
//-----------------------------------------------------------------------------

class LLTaskInvFVBridge : public LLFolderViewEventListener
{
protected:
	LOG_CLASS(LLTaskInvFVBridge);

public:
	LLTaskInvFVBridge(LLPanelInventory* panel, const LLUUID& uuid,
					  const std::string& name, U32 flags = 0);
	~LLTaskInvFVBridge() override								{}

	LL_INLINE LLFontGL::StyleFlags getLabelStyle() const override
	{
		return LLFontGL::NORMAL;
	}

	LL_INLINE std::string getLabelSuffix() const override		{ return LLStringUtil::null; }

	static LLTaskInvFVBridge* createObjectBridge(LLPanelInventory* panelp,
												 LLInventoryObject* objp);
	void showProperties() override;

	S32 getPrice();

	void buyItem();
	static bool commitBuyItem(const LLSD& notification, const LLSD& response);

	// LLFolderViewEventListener functionality
	LL_INLINE const std::string& getName() const override		{ return mName; }
	const std::string& getDisplayName() const override;
	LL_INLINE PermissionMask getPermissionMask() const override	{ return PERM_NONE; }

	LL_INLINE LLFolderType::EType getPreferredType() const override
	{
		return LLFolderType::FT_NONE;
	}

	LL_INLINE const LLUUID& getUUID() const override			{ return mUUID; }
	// *BUG: No creation dates for task inventory
	LL_INLINE time_t getCreationDate() const override			{ return 0; }
	LLUIImagePtr getIcon() const override;
	void openItem() override;
	void previewItem() override;
	LL_INLINE void selectItem() override						{}
	bool isItemRenameable() const override;
	bool renameItem(const std::string& new_name) override;
	bool isItemMovable() override;
	bool isItemRemovable() override;
	bool removeItem() override;
	void removeBatch(std::vector<LLFolderViewEventListener*>& batch) override;
	void move(LLFolderViewEventListener* parent_listener) override;
	bool isItemCopyable() const override;
	LL_INLINE bool copyToClipboard() const override				{ return false; }
	LL_INLINE bool cutToClipboard() const override				{ return false; }
	LL_INLINE bool isClipboardPasteable() const override		{ return false; }
	LL_INLINE void pasteFromClipboard() override				{}
	LL_INLINE void pasteLinkFromClipboard() override			{}
	void buildContextMenu(LLMenuGL& menu, U32 flags) override;
	void performAction(LLFolderView* folderp, LLInventoryModel* modelp,
					   const std::string& action) override;
	LL_INLINE bool isUpToDate() const override					{ return true; }
	LL_INLINE bool hasChildren() const override					{ return false; }

	// New virtual method. Whether this time of item can be edited from an
	// object's contents or not
	LL_INLINE virtual bool canOpen() const						{ return true; }

	LL_INLINE LLInventoryType::EType getInventoryType() const override
	{
		return LLInventoryType::IT_NONE;
	}

	LL_INLINE S32 getSubType() const override					{ return -1; }

	// LLDragAndDropBridge functionality
	bool startDrag(EDragAndDropType* type, LLUUID* id) const override;
	bool dragOrDrop(MASK mask, bool drop, EDragAndDropType cargo_type,
					void* cargo_data, std::string& tooltip_msg) override;

protected:
	LLInventoryItem* findItem() const;

protected:
	LLUUID				mUUID;
	std::string			mName;
	mutable std::string	mDisplayName;
	LLPanelInventory*	mPanel;
	U32					mFlags;
};

LLTaskInvFVBridge::LLTaskInvFVBridge(LLPanelInventory* panelp,
									 const LLUUID& uuid,
									 const std::string& name,
									 U32 flags)
:	mUUID(uuid),
	mName(name),
	mPanel(panelp),
	mFlags(flags)
{
}

LLInventoryItem* LLTaskInvFVBridge::findItem() const
{
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (objp)
	{
		return (LLInventoryItem*)(objp->getInventoryObject(mUUID));
	}
	return NULL;
}

void LLTaskInvFVBridge::showProperties()
{
	LLFloaterProperties::show(mUUID, mPanel->getTaskUUID(), mPanel);
}

struct LLBuyInvItemData
{
	LLUUID mTaskID;
	LLUUID mItemID;
	LLAssetType::EType mType;

	LLBuyInvItemData(const LLUUID& task,
					 const LLUUID& item,
					 LLAssetType::EType type)
	:	mTaskID(task),
		mItemID(item),
		mType(type)
	{
	}
};

void LLTaskInvFVBridge::buyItem()
{
	LLInventoryItem* itemp = findItem();
	if (!itemp || !itemp->getSaleInfo().isForSale())
	{
		return;
	}

	LLBuyInvItemData* datap = new LLBuyInvItemData(mPanel->getTaskUUID(),
												   mUUID, itemp->getType());

	const LLSaleInfo& sale_info = itemp->getSaleInfo();
	const LLPermissions& perm = itemp->getPermissions();
	const std::string owner_name; // no owner name currently... FIXME?

	LLViewerObject* obj;
	if ((obj = gObjectList.findObject(mPanel->getTaskUUID())) &&
		obj->isAttachment())
	{
		gNotifications.add("Cannot_Purchase_an_Attachment");
		llwarns << "Attempted to purchase an attachment" << llendl;
		delete datap;
		return;
	}

	LLSD args;
	args["PRICE"] = llformat("%d",sale_info.getSalePrice());
	args["OWNER"] = owner_name;
	if (sale_info.getSaleType() != LLSaleInfo::FS_CONTENTS)
	{
		const std::string& perm_yes =
			gNotifications.getGlobalString("PermYes");
		const std::string& perm_no = gNotifications.getGlobalString("PermNo");
		U32 next_owner_mask = perm.getMaskNextOwner();
		bool has_perm = (next_owner_mask & PERM_MODIFY) != 0;
		args["MODIFYPERM"] = has_perm ? perm_yes : perm_no;
		has_perm = (next_owner_mask & PERM_COPY) != 0;
		args["COPYPERM"] = has_perm ? perm_yes : perm_no;
		has_perm = (next_owner_mask & PERM_TRANSFER) != 0;
		args["RESELLPERM"] = has_perm ? perm_yes : perm_no;
	}

	std::string alertdesc;
	switch (sale_info.getSaleType())
	{
		case LLSaleInfo::FS_ORIGINAL:
			alertdesc = owner_name.empty() ? "BuyOriginalNoOwner"
										   : "BuyOriginal";
			break;

		case LLSaleInfo::FS_CONTENTS:
			alertdesc = owner_name.empty() ? "BuyContentsNoOwner"
										   : "BuyContents";
			break;

		case LLSaleInfo::FS_COPY:
		default:
			alertdesc = owner_name.empty() ? "BuyCopyNoOwner" : "BuyCopy";
	}

	LLSD payload;
	payload["task_id"] = datap->mTaskID;
	payload["item_id"] = datap->mItemID;
	payload["type"] = datap->mType;
	gNotifications.add(alertdesc, args, payload,
					   LLTaskInvFVBridge::commitBuyItem);
	delete datap;
}

S32 LLTaskInvFVBridge::getPrice()
{
	LLInventoryItem* itemp = findItem();
	return itemp ? itemp->getSaleInfo().getSalePrice() : -1;
}

// static
bool LLTaskInvFVBridge::commitBuyItem(const LLSD& notification,
									  const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLViewerObject* objp =
			gObjectList.findObject(notification["payload"]["task_id"].asUUID());
		if (!objp || !objp->getRegion()) return false;

		LLMessageSystem* msg = gMessageSystemp;
		msg->newMessageFast(_PREHASH_BuyObjectInventory);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
		msg->nextBlockFast(_PREHASH_Data);
		msg->addUUIDFast(_PREHASH_ObjectID,
						 notification["payload"]["task_id"].asUUID());
		msg->addUUIDFast(_PREHASH_ItemID,
						 notification["payload"]["item_id"].asUUID());
		msg->addUUIDFast(_PREHASH_FolderID,
						 gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType((LLAssetType::EType)notification["payload"]["type"].asInteger())));
		msg->sendReliable(objp->getRegion()->getHost());
	}
	return false;
}

const std::string& LLTaskInvFVBridge::getDisplayName() const
{
	LLInventoryItem* itemp = findItem();
	if (itemp)
	{
		mDisplayName.assign(itemp->getName());

		const LLPermissions& perm(itemp->getPermissions());
		bool copy = gAgent.allowOperation(PERM_COPY, perm,
										  GP_OBJECT_MANIPULATE);
		bool mod  = gAgent.allowOperation(PERM_MODIFY, perm,
										  GP_OBJECT_MANIPULATE);
		bool xfer = gAgent.allowOperation(PERM_TRANSFER, perm,
										  GP_OBJECT_MANIPULATE);

		if (!copy)
		{
			mDisplayName.append(" (no copy)");
		}
		if (!mod)
		{
			mDisplayName.append(" (no modify)");
		}
		if (!xfer)
		{
			mDisplayName.append(" (no transfer)");
		}
	}

	return mDisplayName;
}

LLUIImagePtr LLTaskInvFVBridge::getIcon() const
{
	bool item_is_multi = false;
	if (mFlags & LLInventoryItem::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
	{
		item_is_multi = true;
	}

	return LLInventoryIcon::getIcon(LLAssetType::AT_OBJECT,
									LLInventoryType::IT_OBJECT,
									0, item_is_multi);
}

void LLTaskInvFVBridge::openItem()
{
	LL_DEBUGS("Inventory") << "No operation" << LL_ENDL;
}

void LLTaskInvFVBridge::previewItem()
{
	openItem();
}

bool LLTaskInvFVBridge::isItemRenameable() const
{
	if (gAgent.isGodlike()) return true;

	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (objp)
	{
//MK
		if (gRLenabled && !gRLInterface.canDetach(objp))
		{
			return false;
		}
//mk
		LLInventoryItem* itemp =
			(LLInventoryItem*)(objp->getInventoryObject(mUUID));
		if (itemp &&
			gAgent.allowOperation(PERM_MODIFY, itemp->getPermissions(),
								  GP_OBJECT_MANIPULATE, GOD_LIKE))
		{
			return true;
		}
	}

	return false;
}

bool LLTaskInvFVBridge::renameItem(const std::string& new_name)
{
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (objp)
	{
		LLViewerInventoryItem* itemp =
			(LLViewerInventoryItem*)objp->getInventoryObject(mUUID);
		if (itemp &&
			gAgent.allowOperation(PERM_MODIFY, itemp->getPermissions(),
								  GP_OBJECT_MANIPULATE, GOD_LIKE))
		{
			LLPointer<LLViewerInventoryItem> new_itemp =
				new LLViewerInventoryItem(itemp);
			new_itemp->rename(new_name);
			objp->updateInventory(new_itemp);
		}
	}
	return true;
}

bool LLTaskInvFVBridge::isItemMovable()
{
#if 0
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (objp && (objp->permModify() || gAgent.isGodlike()))
	{
		return true;
	}
	return false;
#else
	return true;
#endif
}

bool LLTaskInvFVBridge::isItemRemovable()
{
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (!objp)
	{
		return false;
	}

//MK
	if (gRLenabled && !gRLInterface.canDetach(objp))
	{
		return false;
	}
//mk

	return objp->permModify() || objp->permYouOwner();
}

bool LLTaskInvFVBridge::removeItem()
{
	if (mPanel && isItemRemovable())
	{
		LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
		if (objp)
		{
			if (objp->permModify())
			{
				// Just do it.
				objp->removeInventory(mUUID);
				return true;
			}
			gNotifications.add("CantModifyContentInNoModTask");
		}
	}
	return false;
}

void LLTaskInvFVBridge::removeBatch(std::vector<LLFolderViewEventListener*>& batch)
{
	if (!mPanel)
	{
		return;
	}

	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (!objp)
	{
		return;
	}

	if (!objp->permModify())
	{
		gNotifications.add("CantModifyContentInNoModTask");
	}
	else
	{
		for (S32 i = 0, count = batch.size(); i < count; ++i)
		{
			LLTaskInvFVBridge* itemp = (LLTaskInvFVBridge*)batch[i];
			if (itemp && itemp->isItemRemovable())
			{
				// just do it.
				objp->removeInventory(itemp->getUUID());
			}
		}
	}
}

void LLTaskInvFVBridge::move(LLFolderViewEventListener* parent_listener)
{
}

bool LLTaskInvFVBridge::isItemCopyable() const
{
	LLInventoryItem* itemp = findItem();
	return itemp &&
		   gAgent.allowOperation(PERM_COPY, itemp->getPermissions(),
								 GP_OBJECT_MANIPULATE);
}

bool LLTaskInvFVBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	if (mPanel)
	{
		LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
		if (objp)
		{
			LLInventoryItem* itemp = NULL;
			if ((itemp = (LLInventoryItem*)objp->getInventoryObject(mUUID)))
			{
				const LLPermissions& perm = itemp->getPermissions();
				bool can_copy = gAgent.allowOperation(PERM_COPY, perm,
													  GP_OBJECT_MANIPULATE);
				if (objp->isAttachment() && !can_copy)
				{
					// RN: no copy contents of attachments cannot be dragged
					// out due to a race condition and possible exploit where
					// attached objects do not update their inventory items
					// when their contents are manipulated
					return false;
				}
				if ((can_copy && perm.allowTransferTo(gAgentID)) ||
					objp->permYouOwner()) // || gAgent.isGodlike())
				{
					*type =
						LLAssetType::lookupDragAndDropType(itemp->getType());
					*id = itemp->getUUID();
					return true;
				}
			}
		}
	}
	return false;
}

bool LLTaskInvFVBridge::dragOrDrop(MASK mask, bool drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   std::string& tooltip_msg)
{
	LL_DEBUGS("Inventory") << "No operation" << LL_ENDL;
	return false;
}

//virtual
void LLTaskInvFVBridge::performAction(LLFolderView* folderp,
									  LLInventoryModel* modelp,
									  const std::string& action)
{
	if (action == "task_buy")
	{
		// Check the price of the item.
		S32 price = getPrice();
		if (price == -1)
		{
			llwarns << "Invalid price" << llendl;
		}
		else if (price > 0 && price > gStatusBarp->getBalance())
		{
			LLFloaterBuyCurrency::buyCurrency("This costs", price);
		}
		else
		{
			buyItem();
		}
	}
	else if (action == "task_open")
	{
		openItem();
	}
	else if (action == "task_properties")
	{
		showProperties();
	}
}

void LLTaskInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LLInventoryItem* itemp = findItem();
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if (!itemp)
	{
		set_menu_entries_state(menu, items, disabled_items);
		return;
	}

	 // *TODO: Translate
	if (itemp->getSaleInfo().isForSale() &&
		gAgent.allowOperation(PERM_OWNER, itemp->getPermissions(),
							  GP_OBJECT_MANIPULATE))
	{
		items.emplace_back("Task Buy");

		std::string label("Buy");
		// Check the price of the item.
		S32 price = getPrice();
		if (price == -1)
		{
			llwarns << "Invalid price" << llendl;
		}
		else
		{
			label = llformat("Buy for L$%d", price);
		}

		const LLView::child_list_t *list = menu.getChildList();
		for (LLView::child_list_t::const_iterator itor = list->begin(),
												  end = list->end();
			 itor != end; ++itor)
		{
			std::string name = (*itor)->getName();
			LLMenuItemCallGL* menu_itemp = dynamic_cast<LLMenuItemCallGL*>(*itor);
			if (name == "Task Buy" && menu_itemp)
			{
				menu_itemp->setLabel(label);
			}
		}
	}
	else
	{
		items.emplace_back("Task Open");
		if (!isItemCopyable() || !canOpen())
		{
			disabled_items.emplace_back("Task Open");
		}
	}
	items.emplace_back("Task Properties");
	if (isItemRenameable())
	{
		items.emplace_back("Task Rename");
	}
	if (isItemRemovable())
	{
		items.emplace_back("Task Remove");
	}

	set_menu_entries_state(menu, items, disabled_items);
}

//-----------------------------------------------------------------------------
// LLTaskFolderBridge class
//-----------------------------------------------------------------------------

class LLTaskCategoryBridge : public LLTaskInvFVBridge
{
public:
	LLTaskCategoryBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						 const std::string& name);

	LLUIImagePtr getIcon() const override;
	const std::string& getDisplayName() const override	{ return getName(); }
	bool isItemRenameable() const override				{ return false; }
	bool renameItem(const std::string&) override		{ return false; }
	bool isItemRemovable() override						{ return false; }
	void buildContextMenu(LLMenuGL& menu, U32 flags) override;

#if 0 // For now, return false (default): we will know for sure soon enough.
	// return true if we have or do know know if we have children.
	bool hasChildren() const override;
#endif

	bool startDrag(EDragAndDropType* type, LLUUID* id) const override;
	bool dragOrDrop(MASK mask, bool drop, EDragAndDropType cargo_type,
					void* cargo_data, std::string& tooltip) override;
};

LLTaskCategoryBridge::LLTaskCategoryBridge(LLPanelInventory* panelp,
										   const LLUUID& uuid,
										   const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskCategoryBridge::getIcon() const
{
	static LLUIImagePtr folder_icon =
		LLUI::getUIImage("inv_folder_plain_closed.tga");
	return folder_icon;
}

void LLTaskCategoryBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	items.emplace_back("Task Open");
	set_menu_entries_state(menu, items, disabled_items);
}

bool LLTaskCategoryBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	if (mPanel)
	{
		LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
		if (objp)
		{
			LLInventoryObject* invobj = objp->getInventoryObject(mUUID);
			if (!invobj)
			{
				return false;
			}

			*type = LLAssetType::lookupDragAndDropType(invobj->getActualType());
			if (*type == DAD_NONE ||
				// Cannot drag the root folder (which is the only folder in an
				// object contents). Note that the root folder of an object is
				// currently advertized as DAD_CATEGORY...
				*type == DAD_ROOT_CATEGORY || *type == DAD_CATEGORY)
			{
				return false;
			}

			LLInventoryItem* itemp = (LLInventoryItem*)invobj;
			const LLPermissions& perm = itemp->getPermissions();
			bool can_copy = gAgent.allowOperation(PERM_COPY, perm,
												  GP_OBJECT_MANIPULATE);
			if ((can_copy && perm.allowTransferTo(gAgentID)) ||
				objp->permYouOwner()) // || gAgent.isGodlike())
			{
				*type = LLAssetType::lookupDragAndDropType(itemp->getType());
				*id = itemp->getUUID();
				return true;
			}
		}
	}
	return false;
}

bool LLTaskCategoryBridge::dragOrDrop(MASK mask, bool drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  std::string& tooltip_msg)
{
	bool accept = false;

	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (objp)
	{
		if ((cargo_type == DAD_SETTINGS && !gAgent.hasInventorySettings()) ||
			(cargo_type == DAD_MATERIAL && !gAgent.hasInventoryMaterial()))
		{
			return false;
		}

		switch (cargo_type)
		{
		case DAD_CATEGORY:
			accept = gToolDragAndDrop.dadUpdateInventoryCategory(objp, drop);
			break;
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_LANDMARK:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_CLOTHING:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
#if LL_MESH_ASSET_SUPPORT
		case DAD_MESH:
#endif
		case DAD_SETTINGS:
		case DAD_MATERIAL:
			accept = LLToolDragAndDrop::isInventoryDropAcceptable(objp,
																  (LLViewerInventoryItem*)cargo_data);
			if (accept && drop)
			{
				LLToolDragAndDrop::dropInventory(objp,
												 (LLViewerInventoryItem*)cargo_data,
												 gToolDragAndDrop.getSource(),
												 gToolDragAndDrop.getSourceID());
			}
			break;

		case DAD_SCRIPT:
#if 1		// not yet right for scripts
			// *HACK: In order to resolve SL-22177, we need to block
			// drags from notecards and objects onto other objects.
			// Use the simpler version when we have that right.
			if (LLToolDragAndDrop::isInventoryDropAcceptable(objp,
															 (LLViewerInventoryItem*)cargo_data)
				&& (LLToolDragAndDrop::SOURCE_WORLD != gToolDragAndDrop.getSource())
				&& (LLToolDragAndDrop::SOURCE_NOTECARD != gToolDragAndDrop.getSource()))
			{
				accept = true;
			}
#else
			accept = LLToolDragAndDrop::isInventoryDropAcceptable(objp,
																  (LLViewerInventoryItem*)cargo_data);
#endif
			if (accept && drop)
			{
				LLViewerInventoryItem* itemp =
					(LLViewerInventoryItem*)cargo_data;
				// rez in the script active by default, rez in
				// inactive if the control key is being held down.
				bool active = ((mask & MASK_CONTROL) == 0);
				LLToolDragAndDrop::dropScript(objp, itemp, active,
											  gToolDragAndDrop.getSource(),
											  gToolDragAndDrop.getSourceID());
			}
			break;

		case DAD_CALLINGCARD:
		default:
			break;
		}
	}

	return accept;
}

//-----------------------------------------------------------------------------
// LLTaskTextureBridge class
//-----------------------------------------------------------------------------

class LLTaskTextureBridge : public LLTaskInvFVBridge
{
public:
	LLTaskTextureBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						const std::string& name,
						LLInventoryType::EType it);

	LLUIImagePtr getIcon() const override;
	void openItem() override;

protected:
	LLInventoryType::EType mInventoryType;
};

LLTaskTextureBridge::LLTaskTextureBridge(LLPanelInventory* panelp,
										 const LLUUID& uuid,
										 const std::string& name,
										 LLInventoryType::EType it)
:	LLTaskInvFVBridge(panelp, uuid, name),
	mInventoryType(it)
{
}

LLUIImagePtr LLTaskTextureBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_TEXTURE, mInventoryType,
									0, false);
}

void LLTaskTextureBridge::openItem()
{
//MK
	if (gRLenabled && gRLInterface.contains("viewtexture"))
	{
		return;
	}
//mk
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (!objp || objp->isInventoryPending())
	{
		return;
	}
	open_texture(mUUID, getName(), false, mPanel->getTaskUUID());
}

//-----------------------------------------------------------------------------
// LLTaskSoundBridge class
//-----------------------------------------------------------------------------

class LLTaskSoundBridge : public LLTaskInvFVBridge
{
protected:
	LOG_CLASS(LLTaskSoundBridge);

public:
	LLTaskSoundBridge(LLPanelInventory* panelp, const LLUUID& uuid,
					  const std::string& name);

	LLUIImagePtr getIcon() const override;
	void openItem() override;
	void performAction(LLFolderView* folderp, LLInventoryModel* modelp,
					   const std::string& action) override;
	void buildContextMenu(LLMenuGL& menu, U32 flags) override;
};

LLTaskSoundBridge::LLTaskSoundBridge(LLPanelInventory* panelp,
									 const LLUUID& uuid,
									 const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskSoundBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_SOUND,
									LLInventoryType::IT_SOUND,
									0, false);
}

void LLTaskSoundBridge::openItem()
{
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (objp && !objp->isInventoryPending())
	{
		open_sound(mUUID, getName(), mPanel->getTaskUUID());
	}
}

//virtual
void LLTaskSoundBridge::performAction(LLFolderView* folderp,
									  LLInventoryModel* modelp,
									  const std::string& action)
{
	if (action == "task_play")
	{
		LLInventoryItem* itemp = findItem();
		if (itemp && gAudiop)
		{
			// Play the sound locally.
			LLVector3d lpos_global = gAgent.getPositionGlobal();
			gAudiop->triggerSound(itemp->getAssetUUID(), gAgentID, 1.0,
								  LLAudioEngine::AUDIO_TYPE_UI, lpos_global);
		}
	}
	else
	{
		LLTaskInvFVBridge::performAction(folderp, modelp, action);
	}
}

void LLTaskSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LLInventoryItem* itemp = findItem();
	if (!itemp) return;

	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	// *TODO: Translate
	if (itemp->getPermissions().getOwner() != gAgentID &&
		itemp->getSaleInfo().isForSale())
	{
		items.emplace_back("Task Buy");

		std::string label("Buy");
		// Check the price of the item.
		S32 price = getPrice();
		if (price == -1)
		{
			llwarns << "Invalid price" << llendl;
		}
		else
		{
			label = llformat("Buy for L$%d", price);
		}

		const LLView::child_list_t* listp = menu.getChildList();
		for (LLView::child_list_t::const_iterator it = listp->begin(),
												  end = listp->end();
			 it != end; ++it)
		{
			LLView* viewp = *it;
			LLMenuItemCallGL* menup = dynamic_cast<LLMenuItemCallGL*>(viewp);
			if (menup && viewp->getName() == "Task Buy")
			{
				menup->setLabel(label);
			}
		}
	}
	else
	{
		items.emplace_back("Task Open");
		if (!isItemCopyable())
		{
			disabled_items.emplace_back("Task Open");
		}
	}
	items.emplace_back("Task Properties");
	if (isItemRenameable())
	{
		items.emplace_back("Task Rename");
	}
	if (isItemRemovable())
	{
		items.emplace_back("Task Remove");
	}

	items.emplace_back("Task Play");

#if 0
	menu.appendSeparator();
	menu.append(new LLMenuItemCallGL("Play", &LLTaskSoundBridge::playSound,
									 NULL, (void*)this));
#endif

	set_menu_entries_state(menu, items, disabled_items);
}

//-----------------------------------------------------------------------------
// LLTaskLandmarkBridge class
//-----------------------------------------------------------------------------

class LLTaskLandmarkBridge : public LLTaskInvFVBridge
{
public:
	LLTaskLandmarkBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						 const std::string& name);

	LLUIImagePtr getIcon() const override;
};

LLTaskLandmarkBridge::LLTaskLandmarkBridge(LLPanelInventory* panelp,
										   const LLUUID& uuid,
										   const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskLandmarkBridge::getIcon() const
{
	LLInventoryItem* itemp = findItem();
	bool visited = false;
	if (itemp->getFlags() & LLInventoryItem::II_FLAGS_LANDMARK_VISITED)
	{
		visited = true;
	}
	return LLInventoryIcon::getIcon(LLAssetType::AT_LANDMARK,
									LLInventoryType::IT_LANDMARK,
									visited, false);
}

//-----------------------------------------------------------------------------
// LLTaskCallingCardBridge class
//-----------------------------------------------------------------------------

class LLTaskCallingCardBridge : public LLTaskInvFVBridge
{
public:
	LLTaskCallingCardBridge(LLPanelInventory* panelp, const LLUUID& uuid,
							const std::string& name);

	LLUIImagePtr getIcon() const override;
	bool isItemRenameable() const override					{ return false; }
	bool renameItem(const std::string&) override			{ return false; }
};

LLTaskCallingCardBridge::LLTaskCallingCardBridge(LLPanelInventory* panelp,
												 const LLUUID& uuid,
												 const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskCallingCardBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_CALLINGCARD,
									LLInventoryType::IT_CALLINGCARD,
									0, false);
}

//-----------------------------------------------------------------------------
// LLTaskScriptBridge class
//-----------------------------------------------------------------------------

class LLTaskScriptBridge : public LLTaskInvFVBridge
{
public:
	LLTaskScriptBridge(LLPanelInventory* panelp, const LLUUID& uuid,
					   const std::string& name);

	LLUIImagePtr getIcon() const override;
};

LLTaskScriptBridge::LLTaskScriptBridge(LLPanelInventory* panelp,
									   const LLUUID& uuid,
									   const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskScriptBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_SCRIPT,
									LLInventoryType::IT_LSL,
									0, false);
}

class LLTaskLSLBridge : public LLTaskScriptBridge
{
public:
	LLTaskLSLBridge(LLPanelInventory* panelp, const LLUUID& uuid,
					const std::string& name);

	void openItem() override;
	bool removeItem() override;
};

LLTaskLSLBridge::LLTaskLSLBridge(LLPanelInventory* panelp,
								 const LLUUID& uuid,
								 const std::string& name)
:	LLTaskScriptBridge(panelp, uuid, name)
{
}

void LLTaskLSLBridge::openItem()
{
//MK
	if (gRLenabled && gRLInterface.mContainsViewscript)
	{
		return;
	}
//mk
	if (LLLiveLSLEditor::show(mUUID, mPanel->getTaskUUID()))
	{
		return;
	}
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (!objp || objp->isInventoryPending())
	{
		return;
	}
	if (objp->permModify() || gAgent.isGodlike())
	{
		std::string title("Script: ");
		LLInventoryItem* itemp = findItem();
		if (itemp)
		{
			title.append(itemp->getName());
		}

		S32 left, top;
		gFloaterViewp->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewScriptRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLLiveLSLEditor* editor =
			new LLLiveLSLEditor("lsl ed", rect, title, mPanel->getTaskUUID(),
								mUUID);
		{
			LLHostFloater host;
			editor->open();
		}

		// Keep onscreen
		gFloaterViewp->adjustToFitScreen(editor);
	}
}

bool LLTaskLSLBridge::removeItem()
{
	LLLiveLSLEditor::hide(mUUID, mPanel->getTaskUUID());
	return LLTaskInvFVBridge::removeItem();
}

//-----------------------------------------------------------------------------
// LLTaskObjectBridge class
//-----------------------------------------------------------------------------

class LLTaskObjectBridge : public LLTaskInvFVBridge
{
public:
	LLTaskObjectBridge(LLPanelInventory* panelp, const LLUUID& uuid,
					   const std::string& name);

	LLUIImagePtr getIcon() const override;
};

LLTaskObjectBridge::LLTaskObjectBridge(LLPanelInventory* panelp,
									   const LLUUID& uuid,
									   const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskObjectBridge::getIcon() const
{
	bool item_is_multi = false;
	if (mFlags & LLInventoryItem::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
	{
		item_is_multi = true;
	}

	return LLInventoryIcon::getIcon(LLAssetType::AT_OBJECT,
									LLInventoryType::IT_OBJECT,
									0, item_is_multi);
}

//-----------------------------------------------------------------------------
// LLTaskNotecardBridge class
//-----------------------------------------------------------------------------

class LLTaskNotecardBridge : public LLTaskInvFVBridge
{
public:
	LLTaskNotecardBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						 const std::string& name);

	LLUIImagePtr getIcon() const override;
	void openItem() override;
	bool removeItem() override;
};

LLTaskNotecardBridge::LLTaskNotecardBridge(LLPanelInventory* panelp,
										   const LLUUID& uuid,
										   const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskNotecardBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_NOTECARD,
									LLInventoryType::IT_NOTECARD,
									0, false);
}

void LLTaskNotecardBridge::openItem()
{
	if (LLPreview::show(mUUID))
	{
		return;
	}
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (!objp || objp->isInventoryPending())
	{
		return;
	}
//MK
	if (gRLenabled &&
		(!gRLInterface.canDetach(objp) || gRLInterface.contains("viewnote")))
	{
		return;
	}
//mk

	// Note: even if we are not allowed to modify copyable notecard, we should
	// be able to view it
	LLInventoryItem* itemp =
		dynamic_cast<LLInventoryItem*>(objp->getInventoryObject(mUUID));
	bool item_copy = itemp &&
					 gAgent.allowOperation(PERM_COPY, itemp->getPermissions(),
										   GP_OBJECT_MANIPULATE);
	if (item_copy || objp->permModify() || gAgent.isGodlike())
	{
		S32 left, top;
		gFloaterViewp->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("NotecardEditorRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLPreviewNotecard* preview =
			new LLPreviewNotecard("live notecard editor", rect, getName(),
								  mUUID, mPanel->getTaskUUID());
		// If you are opening a notecard from an object's inventory, it takes
		// focus
		preview->setFocus(true);

		// Keep onscreen
		gFloaterViewp->adjustToFitScreen(preview);
	}
}

bool LLTaskNotecardBridge::removeItem()
{
	LLPreview::hide(mUUID);
	return LLTaskInvFVBridge::removeItem();
}

//-----------------------------------------------------------------------------
// LLTaskGestureBridge class
//-----------------------------------------------------------------------------

class LLTaskGestureBridge : public LLTaskInvFVBridge
{
public:
	LLTaskGestureBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						const std::string& name);

	LLUIImagePtr getIcon() const override;
	void openItem() override;
	bool removeItem() override;
};

LLTaskGestureBridge::LLTaskGestureBridge(LLPanelInventory* panelp,
										 const LLUUID& uuid,
										 const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskGestureBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_GESTURE,
									LLInventoryType::IT_GESTURE,
									0, false);
}

void LLTaskGestureBridge::openItem()
{
	LLViewerObject* objp = gObjectList.findObject(mPanel->getTaskUUID());
	if (!objp || objp->isInventoryPending())
	{
		return;
	}

	open_gesture(mUUID, getName(), mPanel->getTaskUUID());
}

bool LLTaskGestureBridge::removeItem()
{
	// We do not need to deactivate gesture because gestures inside objects can
	// never be active.
	LLPreview::hide(mUUID);
	return LLTaskInvFVBridge::removeItem();
}

//-----------------------------------------------------------------------------
// LLTaskAnimationBridge class
//-----------------------------------------------------------------------------

class LLTaskAnimationBridge : public LLTaskInvFVBridge
{
public:
	LLTaskAnimationBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						  const std::string& name);

	LLUIImagePtr getIcon() const override;
	void openItem() override;
	bool removeItem() override;
};

LLTaskAnimationBridge::LLTaskAnimationBridge(LLPanelInventory* panelp,
											 const LLUUID& uuid,
											 const std::string& name)
:	LLTaskInvFVBridge(panelp, uuid, name)
{
}

LLUIImagePtr LLTaskAnimationBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_ANIMATION,
									LLInventoryType::IT_ANIMATION,
									0, false);
}

void LLTaskAnimationBridge::openItem()
{
	LLViewerObject* objectp = gObjectList.findObject(mPanel->getTaskUUID());
	if (objectp && !objectp->isInventoryPending() &&
		// *TODO: what permissions allow looking at animation ?
		(objectp->permModify() || gAgent.isGodlike()))
	{
		open_animation(mUUID, getName(), 0, mPanel->getTaskUUID());
	}
}

bool LLTaskAnimationBridge::removeItem()
{
	LLPreview::hide(mUUID);
	return LLTaskInvFVBridge::removeItem();
}

//-----------------------------------------------------------------------------
// LLTaskWearableBridge class
//-----------------------------------------------------------------------------

class LLTaskWearableBridge : public LLTaskInvFVBridge
{
public:
	LLTaskWearableBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						 const std::string& name,
						 LLAssetType::EType asset_type, U32 flags);

	LLUIImagePtr getIcon() const override;

protected:
	LLAssetType::EType		mAssetType;
};

LLTaskWearableBridge::LLTaskWearableBridge(LLPanelInventory* panelp,
										   const LLUUID& uuid,
										   const std::string& name,
										   LLAssetType::EType asset_type,
										   U32 flags)
:	LLTaskInvFVBridge(panelp, uuid, name, flags),
	mAssetType(asset_type)
{
}

LLUIImagePtr LLTaskWearableBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(mAssetType,
									LLInventoryType::IT_WEARABLE,
									mFlags, false);
}

#if LL_MESH_ASSET_SUPPORT
//-----------------------------------------------------------------------------
// LLTaskMeshBridge class
//-----------------------------------------------------------------------------

class LLTaskMeshBridge : public LLTaskInvFVBridge
{
public:
	LLTaskMeshBridge(LLPanelInventory* panelp, const LLUUID& uuid,
					 const std::string& name)
	:	LLTaskInvFVBridge(panelp, uuid, name)
	{
	}

	LLUIImagePtr getIcon() const override;
	void openItem() override							{}
};

LLUIImagePtr LLTaskMeshBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_MESH,
									LLInventoryType::IT_MESH,
									0, false);
}
#endif

//-----------------------------------------------------------------------------
// LLTaskSettingsBridge class
//-----------------------------------------------------------------------------

class LLTaskSettingsBridge : public LLTaskInvFVBridge
{
public:
	LLTaskSettingsBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						 const std::string& name, U32 flags)
	:	LLTaskInvFVBridge(panelp, uuid, name,
						  flags & LLInventoryItem::II_FLAGS_SUBTYPE_MASK)
	{
	}

	LLUIImagePtr getIcon() const override;
	bool canOpen() const override						{ return false; }
	void openItem() override							{}
};

LLUIImagePtr LLTaskSettingsBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_SETTINGS,
									LLInventoryType::IT_SETTINGS,
									mFlags, false);
}

//-----------------------------------------------------------------------------
// LLTaskMaterialBridge class
//-----------------------------------------------------------------------------

class LLTaskMaterialBridge : public LLTaskInvFVBridge
{
public:
	LLTaskMaterialBridge(LLPanelInventory* panelp, const LLUUID& uuid,
						 const std::string& name)
	:	LLTaskInvFVBridge(panelp, uuid, name)
	{
	}

	LLUIImagePtr getIcon() const override;
	bool canOpen() const override						{ return true; }
	void openItem() override;
	bool removeItem() override;
};

LLUIImagePtr LLTaskMaterialBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_MATERIAL,
									LLInventoryType::IT_MATERIAL,
									0, false);
}

void LLTaskMaterialBridge::openItem()
{
	LLViewerObject* objectp = gObjectList.findObject(mPanel->getTaskUUID());
	if (!objectp || objectp->isInventoryPending())
	{
		return;
	}

	// Even if we are not allowed to modify a copyable material held inside a
	// no-modify object inventory, we should be able to view it.
	LLInventoryItem* itemp =
		dynamic_cast<LLInventoryItem*>(objectp->getInventoryObject(mUUID));
	bool item_copy = itemp &&
					 gAgent.allowOperation(PERM_COPY, itemp->getPermissions(),
										   GP_OBJECT_MANIPULATE);
	if (item_copy || objectp->permModify() || gAgent.isGodlike())
	{
		open_material(mUUID, getName(), mPanel->getTaskUUID());
	}
}

bool LLTaskMaterialBridge::removeItem()
{
	LLPreview::hide(mUUID);
	return LLTaskInvFVBridge::removeItem();
}

//-----------------------------------------------------------------------------
// LLTaskInvFVBridge class
//-----------------------------------------------------------------------------

LLTaskInvFVBridge* LLTaskInvFVBridge::createObjectBridge(LLPanelInventory* panelp,
														 LLInventoryObject* invobjp)
{
	LLTaskInvFVBridge* bridgep = NULL;
	LLAssetType::EType type = invobjp->getType();
	LLInventoryItem* itemp = NULL;
	switch (type)
	{
	case LLAssetType::AT_TEXTURE:
		itemp = (LLInventoryItem*)invobjp;
		bridgep = new LLTaskTextureBridge(panelp, invobjp->getUUID(),
										  invobjp->getName(),
										  itemp->getInventoryType());
		break;

	case LLAssetType::AT_SOUND:
		bridgep = new LLTaskSoundBridge(panelp, invobjp->getUUID(),
										invobjp->getName());
		break;

	case LLAssetType::AT_LANDMARK:
		bridgep = new LLTaskLandmarkBridge(panelp, invobjp->getUUID(),
										   invobjp->getName());
		break;

	case LLAssetType::AT_CALLINGCARD:
		bridgep = new LLTaskCallingCardBridge(panelp, invobjp->getUUID(),
											  invobjp->getName());
		break;

	case LLAssetType::AT_SCRIPT:	// OLD SCRIPTS DEPRECATED - JC
#if 0
		bridgep = new LLTaskOldScriptBridge(panelp, invobjp->getUUID(),
											invobjp->getName());
#else
		llwarns << "Old script: deprecated !" << llendl;
#endif
		break;

	case LLAssetType::AT_OBJECT:
		bridgep = new LLTaskObjectBridge(panelp, invobjp->getUUID(),
										 invobjp->getName());
		break;

	case LLAssetType::AT_NOTECARD:
		bridgep = new LLTaskNotecardBridge(panelp, invobjp->getUUID(),
										   invobjp->getName());
		break;

	case LLAssetType::AT_ANIMATION:
		bridgep = new LLTaskAnimationBridge(panelp, invobjp->getUUID(),
											invobjp->getName());
		break;

	case LLAssetType::AT_GESTURE:
		bridgep = new LLTaskGestureBridge(panelp, invobjp->getUUID(),
										  invobjp->getName());
		break;

	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
		itemp = (LLInventoryItem*)invobjp;
		bridgep = new LLTaskWearableBridge(panelp, invobjp->getUUID(),
										   invobjp->getName(), type,
										   itemp->getFlags());
		break;

	case LLAssetType::AT_CATEGORY:
		bridgep = new LLTaskCategoryBridge(panelp, invobjp->getUUID(),
										   invobjp->getName());
		break;

	case LLAssetType::AT_LSL_TEXT:
		bridgep = new LLTaskLSLBridge(panelp, invobjp->getUUID(),
									  invobjp->getName());
		break;

#if LL_MESH_ASSET_SUPPORT
	case LLAssetType::AT_MESH:
		bridgep = new LLTaskMeshBridge(panelp, invobjp->getUUID(),
									   invobjp->getName());
		break;
#endif

	case LLAssetType::AT_SETTINGS:
		itemp = (LLInventoryItem*)invobjp;
		bridgep = new LLTaskSettingsBridge(panelp, invobjp->getUUID(),
										   invobjp->getName(),
										   itemp->getFlags());
		break;

	case LLAssetType::AT_MATERIAL:
		bridgep = new LLTaskMaterialBridge(panelp, invobjp->getUUID(),
										   invobjp->getName());
		break;

	default:
		llwarns << "Unhandled inventory type: " << (S32)type << llendl;
		break;
	}

	return bridgep;
}

//-----------------------------------------------------------------------------
// LLPanelInventory class
//-----------------------------------------------------------------------------

LLPanelInventory::LLPanelInventory(const std::string& name, const LLRect& rect)
:	LLPanel(name, rect),
	mScroller(NULL),
	mFolders(NULL),
	mItemsCount(0),
	mHaveInventory(false),
	mIsInventoryEmpty(true),
	mInventoryNeedsUpdate(false)
{
	reset();
	// Callbacks
	init_object_inventory_panel_actions(this);
	gIdleCallbacks.addFunction(idle, this);
}

LLPanelInventory::~LLPanelInventory()
{
	if (!gIdleCallbacks.deleteFunction(idle, this))
	{
		llwarns << "Failed to delete callback" << llendl;
	}
}

void LLPanelInventory::clearContents()
{
	mItemsCount = 0;
	mHaveInventory = false;
	mIsInventoryEmpty = true;

	if (gToolDragAndDrop.getSource() == LLToolDragAndDrop::SOURCE_WORLD)
	{
		gToolDragAndDrop.endDrag();
	}

	if (mScroller)
	{
		// Removes mFolders
		removeChild(mScroller);
		mScroller->die();
		mScroller = NULL;
		mFolders = NULL;
	}
}

void LLPanelInventory::reset()
{
	clearContents();

	setBorderVisible(false);

	LLRect dummy_rect(0, 1, 1, 0);
	mFolders = new LLFolderView("task inventory", NULL, dummy_rect,
								getTaskUUID(), this);
	// This ensures that we never say "searching..." or "no items found"
	mFolders->getFilter()->setShowFolderState(LLInventoryFilter::SHOW_ALL_FOLDERS);

	LLRect scroller_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	mScroller = new LLScrollableContainer("task inventory scroller",
										  scroller_rect, mFolders);
	mScroller->setFollowsAll();
	addChild(mScroller);

	mFolders->setScrollContainer(mScroller);
}

void LLPanelInventory::inventoryChanged(LLViewerObject* objectp,
										LLInventoryObject::object_list_t* invp,
										S32, void*)
{
	if (!objectp) return;

	if (mTaskUUID == objectp->mID)
	{
		mInventoryNeedsUpdate = true;
	}

	if (!invp)
	{
		return;	// Nothing else to do.
	}

	const LLUUID& obj_id = objectp->getID();
	// Refresh any properties floaters that are hanging around. We need to copy
	// the ones that need refreshing onto a temporary object because we cannot
	// iterate through the object inventory twice...
	std::vector<LLFloaterProperties*> floaters;
	for (LLInventoryObject::object_list_t::const_iterator it = invp->begin(),
														  end = invp->end();
		 it != end; ++it)
	{
		LLFloaterProperties* floaterp =
			LLFloaterProperties::find((*it)->getUUID(), obj_id);
		if (floaterp)
		{
			floaters.push_back(floaterp);
		}
	}
	for (S32 i = 0, count = floaters.size(); i < count; ++i)
	{
		floaters[i]->refresh();
	}
}

void LLPanelInventory::updateInventory()
{
	// We are still interested in this task's inventory.
	uuid_list_t selected_items;
	bool inventory_has_focus = false;
	if (mHaveInventory && mFolders->getNumSelectedDescendants())
	{
		mFolders->getSelectionList(selected_items);
		inventory_has_focus = gFocusMgr.childHasKeyboardFocus(mFolders);
	}

	reset();

	LLViewerObject* objectp = gObjectList.findObject(mTaskUUID);
	if (objectp)
	{
		LLInventoryObject* inv_rootp = objectp->getInventoryRoot();
		LLInventoryObject::object_list_t contents;
		objectp->getInventoryContents(contents);
		if (inv_rootp)
		{
			mItemsCount = createFolderViews(inv_rootp, contents);
			mHaveInventory = true;
			mIsInventoryEmpty = false;
			mFolders->setEnabled(true);
		}
		else
		{
			// *TODO: create an empty inventory
			mItemsCount = 0;
			mIsInventoryEmpty = true;
			mHaveInventory = true;
		}
	}
	else
	{
		// *TODO: create an empty inventory
		mItemsCount = 0;
		mIsInventoryEmpty = true;
		mHaveInventory = true;
	}

	// Restore previous selection
	bool first_item = true;
	for (uuid_list_t::iterator it = selected_items.begin(),
							   end = selected_items.end();
		 it != end; ++it)
	{
		LLFolderViewItem* selected_item = mFolders->getItemByID(*it);
		if (selected_item)
		{
			// *HACK: "set" first item then "change" each other one to get
			// keyboard focus right
			if (first_item)
			{
				mFolders->setSelection(selected_item, true,
									   inventory_has_focus);
				first_item = false;
			}
			else
			{
				mFolders->changeSelection(selected_item, true);
			}
		}
	}

	mFolders->arrangeFromRoot();
	mInventoryNeedsUpdate = false;
}

// *FIX: This is currently a very expensive operation, because we have to
// iterate through the inventory one time for each category. This leads to an
// N^2 based on the category count. This could be greatly speeded up with an
// efficient multimap implementation, but we do not have that in our current
// arsenal.
U32 LLPanelInventory::createFolderViews(LLInventoryObject* inv_rootp,
										 LLInventoryObject::object_list_t& contents)
{
	if (!inv_rootp)
	{
		return 0;
	}
	// Create a visible root category.
	LLTaskInvFVBridge* bridgep =
		LLTaskInvFVBridge::createObjectBridge(this, inv_rootp);
	if (!bridgep)
	{
		return 0;
	}
	LLFolderViewFolder* folderp =
		new LLFolderViewFolder(inv_rootp->getName(), bridgep->getIcon(),
							   mFolders, bridgep);
	folderp->addToFolder(mFolders, mFolders);
	folderp->setRegisterLastOpen(false);
	folderp->toggleOpen();

	return createViewsForCategory(&contents, inv_rootp, folderp);
}

typedef std::pair<LLInventoryObject*, LLFolderViewFolder*> obj_folder_pair;

U32 LLPanelInventory::createViewsForCategory(LLInventoryObject::object_list_t* invlist,
											 LLInventoryObject* parent,
											 LLFolderViewFolder* folderp)
{
	U32 total = 0;
	// Find all in the first pass
	std::vector<obj_folder_pair*> child_categories;
	LLTaskInvFVBridge* bridgep;
	for (LLInventoryObject::object_list_t::iterator
			it = invlist->begin(), end = invlist->end();
		 it != end; ++it)
	{
		LLInventoryObject* invobjp = *it;
		if (invobjp && parent->getUUID() == invobjp->getParentUUID())
		{
			bridgep = LLTaskInvFVBridge::createObjectBridge(this, invobjp);
			if (!bridgep)
			{
				continue;
			}
			++total;
			if (invobjp->getType() == LLAssetType::AT_CATEGORY)
			{
				LLFolderViewFolder* vfolderp =
					new LLFolderViewFolder(invobjp->getName(),
										   bridgep->getIcon(), mFolders,
										   bridgep);
				vfolderp->setRegisterLastOpen(false);
				child_categories.push_back(new obj_folder_pair(invobjp,
															   vfolderp));
				vfolderp->addToFolder(folderp, mFolders);
			}
			else
			{
				LLFolderViewItem* vitemp =
					new LLFolderViewItem(invobjp->getName(),
										 bridgep->getIcon(),
										 bridgep->getCreationDate(), mFolders,
										 bridgep);
				vitemp->addToFolder(folderp, mFolders);
			}
		}
	}

	// Now, for each category, do the second pass
	for (S32 i = 0, count = child_categories.size(); i < count; ++i)
	{
		total += createViewsForCategory(invlist, child_categories[i]->first,
										child_categories[i]->second);
		delete child_categories[i];
	}

	return total;
}

void LLPanelInventory::refresh()
{
	bool has_inventory = false;
	LLObjectSelectionHandle selection = gSelectMgr.getSelection();
	LLSelectNode* node = selection->getFirstRootNode(NULL, true);
	if (node && node->mValid)
	{
		LLViewerObject* objp = node->getObject();
		if (objp &&
			(selection->getRootObjectCount() == 1 ||
			 selection->getObjectCount() == 1))
		{
			// Determine if we need to make a request. Start with a default
			// based on if we have inventory at all.
			bool make_request = !mHaveInventory;

			// If the task id is different than what we have stored,
			// then make the request.
			const LLUUID& attach_id = objp->getAttachmentItemID();
			if (mTaskUUID != objp->mID)
			{
				mTaskUUID = objp->mID;
				mAttachmentUUID = attach_id;
				make_request = true;

				// This is a new object so pre-emptively clear the contents
				// Otherwise we show the old stuff until the update comes in
				clearContents();

				// Register for updates from this object,
				registerVOInventoryListener(objp, NULL);
			}
			else if (mAttachmentUUID != attach_id)
			{
				mAttachmentUUID = attach_id;
				if (attach_id.notNull())
				{
					// Server unsubscribes viewer (deselects object) from
					// property updates after "ObjectAttach" so we need to
					// resubscribe.
					gSelectMgr.sendSelect();
				}
			}

			// Based on the node information, we may need to dirty the object
			// inventory and get it again.
			if (node->mValid)
			{
				if (node->mInventorySerial != objp->getInventorySerial() ||
					objp->isInventoryDirty())
				{
					make_request = true;
				}
			}

			// Do the request if necessary.
			if (make_request)
			{
				clearContents();
				requestVOInventory();
			}
			has_inventory = true;
		}
	}
	if (!has_inventory)
	{
		mTaskUUID.setNull();
		mAttachmentUUID.setNull();
		removeVOInventoryListener();
		clearContents();
	}
}

void LLPanelInventory::removeSelectedItem()
{
	if (mFolders)
	{
		mFolders->removeSelectedItems();
	}
}

void LLPanelInventory::startRenamingSelectedItem()
{
	if (mFolders)
	{
		mFolders->startRenamingSelectedItem();
	}
}

// *TODO: Ensure that "Loading contents..." is also displayed while refreshing
// the inventory (after an addition, a removal or the change of an asset).
void LLPanelInventory::draw()
{
	if (mIsInventoryEmpty)
	{
		static LLFontGL* font = LLFontGL::getFontSansSerif();
		if (mTaskUUID.notNull() && !mHaveInventory)
		{
			static const LLWString load = LLTrans::getWString("inv_loading");
			font->render(load, 0, (S32)(getRect().getWidth() * 0.5f), 10,
						 LLColor4(1, 1, 1, 1), LLFontGL::HCENTER,
						 LLFontGL::BOTTOM);
		}
		else if (mHaveInventory)
		{
			static const LLWString empty = LLTrans::getWString("inv_empty");
			font->render(empty, 0, (S32)(getRect().getWidth() * 0.5f), 10,
						 LLColor4(1, 1, 1, 1), LLFontGL::HCENTER,
						 LLFontGL::BOTTOM);
		}
	}

	LLPanel::draw();
}

void LLPanelInventory::deleteAllChildren()
{
	mScroller = NULL;
	mFolders = NULL;
	LLView::deleteAllChildren();
}

bool LLPanelInventory::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data, EAcceptance* accept,
										 std::string& tooltip_msg)
{
	if (!mHaveInventory || !mFolders)
	{
		return false;
	}

	LLFolderViewItem* folderp = mFolders->getNextFromChild(NULL);
	if (!folderp)
	{
		return false;
	}

	// Try to pass on unmodified mouse coordinates
	S32 local_x = x - mFolders->getRect().mLeft;
	S32 local_y = y - mFolders->getRect().mBottom;

	if (mFolders->pointInView(local_x, local_y))
	{
		return mFolders->handleDragAndDrop(local_x, local_y, mask, drop,
										   cargo_type, cargo_data, accept,
										   tooltip_msg);
	}

	// Force mouse coordinates to be inside folder rectangle
	return mFolders->handleDragAndDrop(5, 1, mask, drop, cargo_type,
									   cargo_data, accept, tooltip_msg);
}

//static
void LLPanelInventory::idle(void* user_data)
{
	LLPanelInventory* self = (LLPanelInventory*)user_data;
	if (self && self->mInventoryNeedsUpdate)
	{
		self->updateInventory();
	}
}
