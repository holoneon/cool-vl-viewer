/**
 * @file hbfloaterdebugtags.cpp
 * @brief The HBFloaterDebugTags class definition
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Henri Beauchamp
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

#include "hbfloaterdebugtags.h"

#include "lldir.h"
#include "llerrorcontrol.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "lluictrlfactory.h"

#include "llstartup.h"

strings_set_t HBFloaterDebugTags::sDefaultTagsList;
strings_set_t HBFloaterDebugTags::sAddedTagsList;

//static
void HBFloaterDebugTags::primeTagsFromLogControl()
{
	std::string filename = gDirUtil.getFullPath(LL_PATH_APP_SETTINGS,
												"logcontrol.xml");
	LLSD configuration;
	llifstream file(filename.c_str());
	if (file.is_open())
	{
		LLSDSerialize::fromXML(configuration, file);
		file.close();
	}
	LLError::configure(configuration);

	// Remember the default tags list
	sDefaultTagsList = LLError::getTagsForLevel(LLError::LEVEL_DEBUG);

	for (strings_set_t::iterator it = sAddedTagsList.begin(),
								 end = sAddedTagsList.end();
		 it != end; ++it)
	{
		LLError::setTagLevel(*it, LLError::LEVEL_DEBUG);
	}
}

//static
void HBFloaterDebugTags::setTag(const std::string& tag, bool enable)
{
	if (sAddedTagsList.count(tag))
	{
		if (!enable)
		{
			llinfos << "Removing LL_DEBUGS tag \"" << tag
					<< "\" from logging controls" << llendl;
			sAddedTagsList.erase(tag);
			primeTagsFromLogControl();
		}
	}
	else if (enable)
	{
		llinfos << "Adding LL_DEBUGS tag \"" << tag
				<< "\" to logging controls" << llendl;
		sAddedTagsList.emplace(tag);

		LLError::setTagLevel(tag, LLError::LEVEL_DEBUG);
	}

	// Enable/disable debug message checks depending whether there are
	// debug tags or not.
	LLError::Log::sDebugMessages = !sAddedTagsList.empty() ||
									// Always allow debug messages when the
									// viewer is not yet connected
									!LLStartUp::isLoggedIn();
}

// Floater code proper

HBFloaterDebugTags::HBFloaterDebugTags(const LLSD&)
:	mIsDirty(false)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_debug_tags.xml");
	primeTagsFromLogControl();
}

//virtual
bool HBFloaterDebugTags::postBuild()
{
	mDebugTagsList = getChild<LLScrollListCtrl>("tags_list");
	mDebugTagsList->setCommitCallback(onSelectLine);
	mDebugTagsList->setCallbackUserData(this);

	mIsDirty = true;

	return true;
}

//virtual
void HBFloaterDebugTags::draw()
{
	if (mIsDirty)
	{
		mIsDirty = false;
		refreshList();
	}

	LLFloater::draw();
}

void HBFloaterDebugTags::refreshList()
{
	if (!mDebugTagsList)
	{
		mIsDirty = true;
		return;
	}

	S32 scrollpos = mDebugTagsList->getScrollPos();
	mDebugTagsList->deleteAllItems();

	std::string filename = gDirUtil.getFullPath(LL_PATH_APP_SETTINGS,
												"debug_tags.xml");
	llifstream file(filename.c_str());
	if (file.is_open())
	{
		LLSD list;
		llinfos << "Loading the debug tags list from: " << filename << llendl;
		LLSDSerialize::fromXML(list, file);
		S32 id = 0;
		std::string tag;
		LLScrollListItem* item;
		while (id < (S32)list.size())
		{
			bool has_tag = false;
			bool has_ref = false;
			bool has_other = false;
			LLSD data = list[id];
			if (data.has("columns"))
			{
				for (S32 i = 0; i < (S32)data["columns"].size(); ++i)
				{
					LLSD map = data["columns"][i];
					if (map.has("column"))
					{
						if (map["column"].asString() == "tag")
						{
							has_tag = true;
							tag = map.get("value").asString();
						}
						else if (map["column"].asString() == "references")
						{
							has_ref = true;
						}
						else
						{
							has_other = true;
						}
					}
					else
					{
						// Make sure the entry will be removed
						has_other = true;
						break;
					}
				}
			}
			if (!has_other && has_tag && has_ref)
			{
				data["columns"][2]["column"] = "active";
				data["columns"][2]["type"] = "checkbox";
				bool is_default = sDefaultTagsList.count(tag) != 0;
				bool active = is_default || sAddedTagsList.count(tag) != 0;
				data["columns"][2]["value"] = active;
				if (is_default)
				{
					data["columns"][0]["color"] = LLColor4::red2.getValue();
					data["columns"][1]["color"] = LLColor4::red2.getValue();
				}
				item = mDebugTagsList->addElement(data, ADD_BOTTOM);
				item->setEnabled(!is_default);	// cannot change default
				mDebugTagsList->deselectAllItems(true);
				++id;
			}
			else
			{
				list.erase(id);
			}
		}
		file.close();
	}

	mDebugTagsList->setScrollPos(scrollpos);
}

//static
void HBFloaterDebugTags::onSelectLine(LLUICtrl* ctrl, void* data)
{
	HBFloaterDebugTags* self = (HBFloaterDebugTags*)data;
	if (self && self->mDebugTagsList)
	{
		LLScrollListItem* item = self->mDebugTagsList->getFirstSelected();
		if (item && item->getColumn(0) && item->getColumn(1))
		{
			const std::string tag = item->getColumn(1)->getValue().asString();
			bool value = item->getColumn(0)->getValue().asBoolean();
			setTag(tag, value);
		}
	}
}
