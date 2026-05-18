/** 
 * @file llfloaternewim.cpp
 * @brief Panel allowing the user to create a new IM session.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
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

#include "llviewerprecompiledheaders.h"

#include "llfloaternewim.h"

#include "lllineeditor.h"
#include "llnamelistctrl.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llavatartracker.h"
#include "llimmgr.h"
#include "llmutelist.h"

LLFloaterNewIM::LLFloaterNewIM()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_new_im.xml");
}

//virtual
bool LLFloaterNewIM::postBuild()
{
	childSetAction("start_btn", onStart, this);
	childSetAction("close_btn", onClickClose, this);

	mGroupList = getChild<LLNameListCtrl>("group_list");
	mGroupList->setCommitOnSelectionChange(true);
	mGroupList->setCommitCallback(onSelectGroup);
	mGroupList->setCallbackUserData(this);
	mGroupList->setDoubleClickCallback(&LLFloaterNewIM::onStart);
	mGroupList->setCallbackUserData(this);

	mAgentList = getChild<LLNameListCtrl>("user_list");
	mAgentList->setCommitOnSelectionChange(true);
	mAgentList->setCommitCallback(onSelectAgent);
	mAgentList->setCallbackUserData(this);
	mAgentList->setDoubleClickCallback(&LLFloaterNewIM::onStart);
	mAgentList->setCallbackUserData(this);

	LLSearchEditor*	editp = getChild<LLSearchEditor>("filter_search");
	editp->setSearchCallback(onSearchEdit, this);

	setDefaultBtn("start_btn");

	return true;
}

//virtual
bool LLFloaterNewIM::handleKeyHere(KEY key, MASK mask)
{
	bool handled = LLFloater::handleKeyHere(key, mask);
	if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		handled = true;
		// Close talk panel on escape
		if (gIMMgrp)
		{
			gIMMgrp->toggle(NULL);
		}
	}

	// Might need to call base class here if not handled
	return handled;
}

//virtual
bool LLFloaterNewIM::canClose()
{
	if (getHost())
	{
		LLMultiFloater* hostp = (LLMultiFloater*)getHost();
		// If we are the only tab in the im view, go ahead and close
		return hostp->getFloaterCount() == 1;
	}
	return true;
}

//virtual
void LLFloaterNewIM::close(bool app_quitting)
{
	LLMultiFloater* hostp = (LLMultiFloater*)getHost();
	if (hostp)
	{
		hostp->close();
	}
	else
	{
		LLFloater::close(app_quitting);
	}
}

void LLFloaterNewIM::refreshLists()
{
	S32 old_group_scroll_pos = mGroupList->getScrollPos();
	S32 old_agent_scroll_pos = mAgentList->getScrollPos();
	mGroupList->deleteAllItems();
	mAgentList->deleteAllItems();

	// Add groups
	for (S32 i = 0, count = gAgent.mGroups.size(); i < count; ++i)
	{
		LLGroupData* groupp = &(gAgent.mGroups[i]);
		addGroup(groupp->mID, groupp->mName);
	}

	// Build a set of buddies in the current buddy list.
	LLCollectAllBuddies collector;
	gAvatarTracker.applyFunctor(collector);
	LLCollectAllBuddies::buddy_map_t::iterator it;
	LLCollectAllBuddies::buddy_map_t::iterator end;
	it = collector.mOnline.begin();
	end = collector.mOnline.end();
	for ( ; it != end; ++it)
	{
		addAgent(it->second, true);
	}
	it = collector.mOffline.begin();
	end = collector.mOffline.end();
	for ( ; it != end; ++it)
	{
		addAgent(it->second, false);
	}

	mGroupList->setScrollPos(old_group_scroll_pos);
	mAgentList->setScrollPos(old_agent_scroll_pos);
}

void LLFloaterNewIM::addAgent(const LLUUID& id, bool online)
{
	static const EInstantMessage default_session = IM_NOTHING_SPECIAL;

	uuid_vec_t selection = mAgentList->getSelectedIDs();

	std::string fullname;
	bool has_name = gCacheNamep && gCacheNamep->getFullName(id, fullname);
	if (has_name)
	{
		if (!LLAvatarName::sLegacyNamesForFriends &&
			LLAvatarNameCache::useDisplayNames())
		{
			LLAvatarName avatar_name;
			if (LLAvatarNameCache::get(id, &avatar_name))
			{
				if (LLAvatarNameCache::useDisplayNames() == 2)
				{
					fullname = avatar_name.mDisplayName;
				}
				else
				{
					fullname = avatar_name.getNames();
				}
			}
		}
	}
	if (has_name && !mFilterString.empty())
	{
		std::string lcname = fullname;
		LLStringUtil::toLower(lcname);
		if (lcname.find(mFilterString) == std::string::npos)
		{
			// Friend name does not match the filter, skip it. HB
			return;
		}
	}

	LLSD row;
	row["id"] = id;
	row["columns"][0]["value"] = fullname;
	row["columns"][0]["font"] = "SANSSERIF";
	row["columns"][0]["font-style"] = online ? "BOLD" : "NORMAL";
	LLScrollListItem* itemp = mAgentList->addElement(row);
	itemp->setUserdata((void*)&default_session);

	mAgentList->selectMultiple(selection);
	if (mAgentList->getFirstSelectedIndex() == -1)
	{
		mAgentList->selectFirstItem();
	}
}

void LLFloaterNewIM::addGroup(const LLUUID& id, const std::string& name)
{
	static const EInstantMessage group_session = IM_SESSION_GROUP_START;

	if (!mFilterString.empty() && !name.empty())
	{
		std::string lcname = name;
		LLStringUtil::toLower(lcname);
		if (lcname.find(mFilterString) == std::string::npos)
		{
			// Group name does not match the filter, skip this group. HB
			return;
		}
	}

	uuid_vec_t selection = mGroupList->getSelectedIDs();

	LLSD row;
	row["id"] = id;
	row["target"] = "GROUP";
	row["columns"][0]["value"] = name;
	row["columns"][0]["font"] = "SANSSERIF";
	bool muted = LLMuteList::isMuted(id, LLMute::flagTextChat);
	row["columns"][0]["font-style"] = muted ? "NORMAL" : "BOLD";
	LLScrollListItem* itemp = mGroupList->addElement(row, ADD_SORTED);
	itemp->setUserdata((void*)&group_session);
	itemp->setEnabled(!muted);

	mGroupList->selectMultiple(selection);
	if (mGroupList->getFirstSelectedIndex() == -1)
	{
		mGroupList->selectFirstItem();
	}
}

//static
void LLFloaterNewIM::onSelectGroup(LLUICtrl*, void* userdata)
{
	LLFloaterNewIM* self = (LLFloaterNewIM*)userdata;
	if (!self) return;	// Paranoia
 	LLScrollListItem* itemp = self->mAgentList->getFirstSelected();
	if (itemp)
	{
		itemp->setSelected(false);
	}
}

//static
void LLFloaterNewIM::onSelectAgent(LLUICtrl*, void* userdata)
{
	LLFloaterNewIM* self = (LLFloaterNewIM*)userdata;
	if (!self) return;	// Paranoia
 	LLScrollListItem* itemp = self->mGroupList->getFirstSelected();
	if (itemp)
	{
		itemp->setSelected(false);
	}
}

//static
void LLFloaterNewIM::onSearchEdit(const std::string& search_string,
								  void* userdata)
{
	LLFloaterNewIM* self = (LLFloaterNewIM*)userdata;
	if (self)
	{
		self->mFilterString = search_string;
		LLStringUtil::trim(self->mFilterString);
		LLStringUtil::toLower(self->mFilterString);
		self->refreshLists();
	}
}

//static
void LLFloaterNewIM::onStart(void* userdata)
{
	if (!gIMMgrp) return;

	LLFloaterNewIM* self = (LLFloaterNewIM*)userdata;

	LLScrollListItem* itemp = self->mGroupList->getFirstSelected();
	if (!itemp)
	{
		itemp = self->mAgentList->getFirstSelected();
	}
	if (!itemp)
	{
		make_ui_sound("UISndInvalidOp");
		return;
	}

	const LLScrollListCell* cellp = itemp->getColumn(0);
	std::string name = cellp->getValue();

	// Do a live determination of what type of session it should be.
	EInstantMessage type;
	EInstantMessage* t = (EInstantMessage*)itemp->getUserdata();
	if (t)
	{
		type = *t;
	}
	else
	{
		type = LLIMMgr::defaultIMTypeForAgent(itemp->getUUID());
	}
	if (type != IM_SESSION_GROUP_START)
	{
		if (gCacheNamep)
		{
			// Needed to avoid catching a display name, which would make us use
			// a wrong IM log file... HB
			gCacheNamep->getFullName(itemp->getUUID(), name);
		}
	}
	else if (LLMuteList::isMuted(itemp->getUUID(), LLMute::flagTextChat))
	{
		make_ui_sound("UISndInvalidOp");
		return;
	}

	gIMMgrp->addSession(name, type, itemp->getUUID());
	make_ui_sound("UISndStartIM");
}

// static
void LLFloaterNewIM::onClickClose(void *userdata)
{
	if (gIMMgrp)
	{
		gIMMgrp->setFloaterOpen(false);
	}
}
