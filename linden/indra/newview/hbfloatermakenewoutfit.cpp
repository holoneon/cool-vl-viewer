/**
 * @file hbfloatermakenewoutfit.cpp
 * @brief The "Make new outfit" floater implementation
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 *
 * Copyright (c) 2011-2024 Henri Beauchamp
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

#include "hbfloatermakenewoutfit.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llaisapi.h"
#include "llviewercontrol.h"
#include "llvoavatarself.h"

//static
uuid_list_t HBFloaterMakeNewOutfit::sFetchingRequests;
uuid_list_t HBFloaterMakeNewOutfit::sUnderpants;
uuid_list_t HBFloaterMakeNewOutfit::sUndershirts;

HBFloaterMakeNewOutfit::HBFloaterMakeNewOutfit(const LLSD&)
:	mIsDirty(true),
	mSaveStatusDirty(true)
{
    LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_make_new_outfit.xml");
}

//virtual
HBFloaterMakeNewOutfit::~HBFloaterMakeNewOutfit()
{
	sFetchingRequests.clear();
}

//virtual
bool HBFloaterMakeNewOutfit::postBuild()
{
	mSaveButton = getChild<LLButton>("save_btn");
	mSaveButton->setClickedCallback(onButtonSave, this);

	childSetAction("cancel_btn", onButtonCancel, this);
	
	mShapeCheck = getChild<LLCheckBoxCtrl>("checkbox_shape");
	mShapeCheck->setCommitCallback(onCommitCheckBox);
	mShapeCheck->setCallbackUserData(this);

	mSkinCheck = getChild<LLCheckBoxCtrl>("checkbox_skin");
	mSkinCheck->setCommitCallback(onCommitCheckBox);
	mSkinCheck->setCallbackUserData(this);

	mHairCheck = getChild<LLCheckBoxCtrl>("checkbox_hair");
	mHairCheck->setCommitCallback(onCommitCheckBox);
	mHairCheck->setCallbackUserData(this);

	mEyesCheck = getChild<LLCheckBoxCtrl>("checkbox_eyes");
	mEyesCheck->setCommitCallback(onCommitCheckBox);
	mEyesCheck->setCallbackUserData(this);

	mAttachmentsList = getChild<LLScrollListCtrl>("attachments_list");
	mAttachmentsList->setCommitOnSelectionChange(true);
	mAttachmentsList->setCommitCallback(onCommitCheckBox);
	mAttachmentsList->setCallbackUserData(this);

	mWearablesList = getChild<LLScrollListCtrl>("wearables_list");
	mWearablesList->setCommitOnSelectionChange(true);
	mWearablesList->setCommitCallback(onCommitWearableList);
	mWearablesList->setCallbackUserData(this);

	mRenameCheck = getChild<LLCheckBoxCtrl>("checkbox_rename");
	mUseNoCopyLinksCheck = getChild<LLCheckBoxCtrl>("checkbox_nocopy_links");
	mUseClothesLinksCheck = getChild<LLCheckBoxCtrl>("checkbox_clothes_links");

	mUseAllLinksCheck = getChild<LLCheckBoxCtrl>("checkbox_all_links");
	mUseAllLinksCheck->setCommitCallback(onCommitCheckBoxLinkAll);
	mUseAllLinksCheck->setCallbackUserData(this);

	bool use_links_always = gSavedSettings.getBool("UseInventoryLinksAlways");
	mUseNoCopyLinksCheck->setEnabled(!use_links_always);
	mUseClothesLinksCheck->setEnabled(!use_links_always);
	mRenameCheck->setEnabled(!use_links_always);

	return true;
}

//virtual
void HBFloaterMakeNewOutfit::draw()
{
	if (mIsDirty && isAgentAvatarValid())
	{
		static LLCachedControl<bool> ais3_fecth(gSavedSettings,
												"UseAISForFetching");
		bool using_ais = ais3_fecth && AISAPI::isAvailable();
		mIsDirty = false;
		LLScrollListItem* listitemp;
		LLViewerInventoryItem* invitemp;

		// Update wearables list
		S32 scrollpos = mWearablesList->getScrollPos();
		S32 selected = mWearablesList->getFirstSelectedIndex();
		mWearablesList->deleteAllItems();
		sUndershirts.clear();
		sUnderpants.clear();
		for (S32 type = (S32)LLWearableType::WT_SHIRT;
			 type < (S32)LLWearableType::WT_COUNT; ++type)
		{
			std::string type_name =
				LLTrans::getString(LLWearableType::getTypeLabel((LLWearableType::EType)type));
			for (U32 index = 0,
					 count = gAgentWearables.getWearableCount((LLWearableType::EType)type);
				 index < count; ++index)
			{
				LLViewerWearable* wearable =
					gAgentWearables.getViewerWearable((LLWearableType::EType)type,
													  index);
				if (!wearable) continue;

				invitemp = gInventory.getItem(wearable->getItemID());
				if (!invitemp) continue;

				const LLUUID& item_id = invitemp->getLinkedUUID();
				if (!invitemp->isFinished())
				{
					if (!sFetchingRequests.count(item_id))
					{
						sFetchingRequests.emplace(item_id);
						invitemp->fetchFromServer();
					}
				}

				LLSD element;
				element["id"] = item_id;
				element["columns"][0]["column"] = "selection";
				element["columns"][0]["type"] = "checkbox";
				element["columns"][0]["value"] = true;
				element["columns"][1]["column"] = "wearable";
				element["columns"][1]["type"] = "text";
				element["columns"][1]["value"] = invitemp->getName();
				element["columns"][1]["font"] = "SANSSERIF_SMALL";
				element["columns"][2]["column"] = "type";
				element["columns"][2]["type"] = "text";
				element["columns"][2]["value"] = type_name;
				element["columns"][2]["font"] = "SANSSERIF_SMALL";
				listitemp = mWearablesList->addElement(element, ADD_BOTTOM);
				if (!listitemp) continue; // Out of memory ?
#if LL_TEEN_WERABLE_RESTRICTIONS
				if (gAgent.isTeen())
				{
					if (type == LLWearableType::WT_UNDERSHIRT)
					{
						sUndershirts.emplace(item_id);
					}
					else if (type == LLWearableType::WT_UNDERPANTS)
					{
						sUnderpants.emplace(item_id);
					}
				}
#endif
			}
		}
		mWearablesList->setScrollPos(scrollpos);
		if (selected >= 0)
		{
			mWearablesList->selectNthItem(selected);
		}

		// Update attachments list
		scrollpos = mAttachmentsList->getScrollPos();
		selected = mAttachmentsList->getFirstSelectedIndex();
		mAttachmentsList->deleteAllItems();
		for (S32 i = 0, count = gAgentAvatarp->mAttachedObjectsVector.size();
			 i < count; ++i)
		{
			LLViewerJointAttachment* vatt =
				gAgentAvatarp->mAttachedObjectsVector[i].second;
			if (!vatt) continue;	// Paranoia
			std::string joint_name = LLTrans::getString(vatt->getName());

			LLViewerObject* vobj =
				gAgentAvatarp->mAttachedObjectsVector[i].first;
			if (!vobj) continue;	// Paranoia

			const LLUUID& item_id = vobj->getAttachmentItemID();
			if (item_id.isNull()) continue;

			invitemp = gInventory.getItem(item_id);
			if (!invitemp) continue;

			// Make sure all attached inventory items are complete, so
			// that we can safely copy them later...
			bool complete = true;
			if (!invitemp->isFinished())
			{
				if (!sFetchingRequests.count(item_id))
				{
					sFetchingRequests.emplace(item_id);
					invitemp->fetchFromServer();
				}
				complete = false;
				// Refresh UI till all items are complete
				mIsDirty = true;
			}

			LLSD element;
			element["id"] = invitemp->getLinkedUUID();
			element["columns"][0]["column"] = "selection";
			element["columns"][0]["type"] = "checkbox";
			element["columns"][0]["value"] = complete;
			element["columns"][1]["column"] = "attachment";
			element["columns"][1]["type"] = "text";
			element["columns"][1]["value"] = invitemp->getName();
			element["columns"][1]["font"] = "SANSSERIF_SMALL";
			element["columns"][2]["column"] = "joint";
			element["columns"][2]["type"] = "text";
			element["columns"][2]["value"] = joint_name;
			element["columns"][2]["font"] = "SANSSERIF_SMALL";
			listitemp = mAttachmentsList->addElement(element, ADD_BOTTOM);
			if (!listitemp) continue; // Out of memory ?
			// *HACK: when using AIS3 for inventory fetches, do not wait for
			// item completeness (it may not even arrive in time), and enable
			// the check box anyway...
			listitemp->setEnabled(using_ais || complete);
		}
		mAttachmentsList->setScrollPos(scrollpos);
		if (selected >= 0)
		{
			mAttachmentsList->selectNthItem(selected);
		}

		// Force a refresh of the Save button status
		mSaveStatusDirty = true;
	}

	if (mSaveStatusDirty)
	{
		mSaveStatusDirty = false;
		mSaveButton->setEnabled(hasCheckedItems());
	}

	LLFloater::draw();
}

bool HBFloaterMakeNewOutfit::hasCheckedItems()
{
	// Check the body parts
	if (mShapeCheck->get() || mSkinCheck->get() || mHairCheck->get() ||
		mEyesCheck->get())
	{
		return true;
	}

	// Check the wearables
	LLScrollListCtrl::items_vec_t items = mWearablesList->getAllData();
	for (S32 i = 0, count = items.size(); i < count; ++i)
	{
		LLScrollListItem* itemp = items[i];	// Cannot be NULL.
		if (itemp->getColumn(0)->getValue().asBoolean())
		{
			return true;
		}
	}

	// Finally, check the attachments
	items = mAttachmentsList->getAllData();
	for (S32 i = 0, count = items.size(); i < count; ++i)
	{
		LLScrollListItem* itemp = items[i];	// Cannot be NULL.
		if (itemp->getColumn(0)->getValue().asBoolean())
		{
			return true;
		}
	}

	return false;
}

void HBFloaterMakeNewOutfit::getIncludedItems(uuid_vec_t& wearables_to_include,
											  uuid_vec_t& attachments_to_include)
{
	// First, deal with body parts check boxes
	wearables_to_include.clear();
	for (S32 type = 0; type <= (S32)LLWearableType::WT_EYES; ++type)
	{
		std::string name =
			LLWearableType::getTypeLabel((LLWearableType::EType)type);
		std::string label = "checkbox_" + name;
		LLStringUtil::toLower(label);
		if (!childGetValue(label.c_str()).asBoolean()) continue;

		if (!gAgentWearables.getWearableCount((LLWearableType::EType)type))
		{
			llwarns << "Avatar not fully rezzed. Missing body part: "
					<< name << llendl;
			continue;
		}

		LLViewerWearable* wearable =
			gAgentWearables.getViewerWearable((LLWearableType::EType)type, 0);
		if (!wearable)
		{
			llwarns << "Could not find wearable item for body part: " << name
					<< llendl;
			continue;
		}

		LLViewerInventoryItem* invitemp =
			gInventory.getItem(wearable->getItemID());
		if (invitemp)
		{
			wearables_to_include.emplace_back(invitemp->getLinkedUUID());
		}
		else
		{
			llwarns << "Could not find inventory item for body part: " << name
					<< llendl;
		}
	}

	// Then, add all selected wearables in the list
	LLScrollListCtrl::items_vec_t items = mWearablesList->getAllData();
	wearables_to_include.reserve(wearables_to_include.size() + items.size());
	for (S32 i = 0, count = items.size(); i < count; ++i)
	{
		LLScrollListItem* itemp = items[i];	// Cannot be NULL.
		if (itemp->getColumn(0)->getValue().asBoolean())
		{
			wearables_to_include.emplace_back(itemp->getValue().asUUID());
		}
	}

	// Finally, deal with the attachments
	items = mAttachmentsList->getAllData();
	attachments_to_include.reserve(items.size());
	for (S32 i = 0, count = items.size(); i < count; ++i)
	{
		LLScrollListItem* itemp = items[i];	// Cannot be NULL.
		if (itemp->getColumn(0)->getValue().asBoolean())
		{
			attachments_to_include.emplace_back(itemp->getValue().asUUID());
		}
	}
}

//static
void HBFloaterMakeNewOutfit::setDirty()
{
	HBFloaterMakeNewOutfit* self = findInstance();
    if (self)
	{
		self->mIsDirty = true;
	}
}

//static
void HBFloaterMakeNewOutfit::onCommitWearableList(LLUICtrl* ctrl,
												  void* user_data)
{
	HBFloaterMakeNewOutfit* self = (HBFloaterMakeNewOutfit*)user_data;
	if (self)
	{
		self->mSaveStatusDirty = true;
#if LL_TEEN_WERABLE_RESTRICTIONS
		// Enforce strict underwears for teens
		LLScrollListItem* itemp = dynamic_cast<LLScrollListItem*>(ctrl);
		if (gAgent.isTeen() && itemp &&
			!itemp->getColumn(0)->getValue().asBoolean())
		{
			LLUUID item_id = itemp->getValue();
			if (sUnderpants.count(item_id))
			{
				if (sUnderpants.size() > 1)
				{
					sUnderpants.erase(item_id);
				}
				else
				{
					itemp->getColumn(0)->setValue(true);
				}
			}
			else if (sUndershirts.count(item_id))
			{
				if (sUndershirts.size() > 1)
				{
					sUndershirts.erase(item_id);
				}
				else
				{
					itemp->getColumn(0)->setValue(true);
				}
			}
		}
#endif
	}
}

//static
void HBFloaterMakeNewOutfit::onCommitCheckBox(LLUICtrl*, void* user_data)
{
	HBFloaterMakeNewOutfit* self = (HBFloaterMakeNewOutfit*)user_data;
	if (self)
	{
		self->mSaveStatusDirty = true;
	}
}

//static
void HBFloaterMakeNewOutfit::onCommitCheckBoxLinkAll(LLUICtrl* ctrl,
													 void* user_data)
{
	HBFloaterMakeNewOutfit* self = (HBFloaterMakeNewOutfit*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (self && check)
	{
		bool enable = !check->get();
		self->mUseNoCopyLinksCheck->setEnabled(enable);
		self->mUseClothesLinksCheck->setEnabled(enable);
		self->mRenameCheck->setEnabled(enable);
	}
}

//static
void HBFloaterMakeNewOutfit::onButtonSave(void* user_data)
{
	HBFloaterMakeNewOutfit* self = (HBFloaterMakeNewOutfit*)user_data;
	if (self)
	{
		std::string folder = self->childGetValue("name_lineeditor").asString();
		bool rename_clothing = self->mRenameCheck->getValue().asBoolean();
		uuid_vec_t wearables, attachments;
		self->getIncludedItems(wearables, attachments);
		gAgentWearables.makeNewOutfit(folder, wearables, attachments,
									  rename_clothing);
		self->close();
	}
}

//static
void HBFloaterMakeNewOutfit::onButtonCancel(void* user_data)
{
	HBFloaterMakeNewOutfit* self = (HBFloaterMakeNewOutfit*)user_data;
	if (self)
	{
		self->close();
	}
}
