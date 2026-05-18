/**
 * @file llsettingstype.cpp
 * @brief LLSettingsType class implementation
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 *
 * Copyright (c) 2001-2019, Linden Research, Inc.
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

#include "linden_common.h"

#include "llsettingstype.h"

#include "lldictionary.h"
#include "llinventory.h"

static LLTranslationBridge::ptr_t sTrans;

void LLSettingsType::initClass(LLTranslationBridge::ptr_t& trans)
{
	sTrans = trans;
}

void LLSettingsType::cleanupClass()
{
	sTrans.reset();
}

struct SettingsEntry : public LLDictionaryEntry
{
	SettingsEntry(const std::string& name, const std::string& default_new_name,
				  LLInventoryType::EIconName icon_name)
	:	LLDictionaryEntry(name),
		mDefaultNewName(default_new_name),
		mIconName(icon_name)
	{
		if (sTrans)
		{
			mLabel = sTrans->getString(name);
		}
		else
		{
			llwarns << "No translation bridge: SettingsEntry '" << name
					<< "' will be left untranslated." << llendl;
		}
		if (mLabel.empty())
		{
			mLabel = name;
		}
	}

	LLInventoryType::EIconName	mIconName;
	std::string					mLabel;
	std::string					mDefaultNewName;
};

class LLSettingsDictionary : public LLDictionary<LLSettingsType::EType, SettingsEntry>
{
public:
	LLSettingsDictionary();
};

// Since it is a small structure, let's initialize it unconditionally (i.e.
// even if we do not log in) at global scope. This saves having to bother with
// a costly LLSingleton (slow, lot's of CPU cycles and cache lines wasted) or
// to find the right place where to construct the class on login... HB
LLSettingsDictionary gSettingsDictionary;

LLSettingsDictionary::LLSettingsDictionary()
{
	addEntry(LLSettingsType::ST_SKY,      new SettingsEntry("sky",     "New Sky",      LLInventoryType::ICONNAME_SETTINGS_SKY));
	addEntry(LLSettingsType::ST_WATER,    new SettingsEntry("water",   "New Water",    LLInventoryType::ICONNAME_SETTINGS_WATER));
	addEntry(LLSettingsType::ST_DAYCYCLE, new SettingsEntry("day",     "New Daycycle", LLInventoryType::ICONNAME_SETTINGS_DAY));
	addEntry(LLSettingsType::ST_NONE,     new SettingsEntry("none",    "New Settings", LLInventoryType::ICONNAME_SETTINGS));
	addEntry(LLSettingsType::ST_INVALID,  new SettingsEntry("invalid", "New Settings", LLInventoryType::ICONNAME_SETTINGS));
}

LLSettingsType::EType LLSettingsType::fromInventoryFlags(U32 flags)
{
	return (LLSettingsType::EType)(flags & LLInventoryItem::II_FLAGS_SUBTYPE_MASK);
}

LLInventoryType::EIconName LLSettingsType::getIconName(LLSettingsType::EType type)
{
	const SettingsEntry* entry = gSettingsDictionary.lookup(type);
	return entry ? entry->mIconName : getIconName(ST_INVALID);
}

std::string LLSettingsType::getDefaultName(LLSettingsType::EType type)
{
	const SettingsEntry* entry = gSettingsDictionary.lookup(type);
	return entry ? entry->mDefaultNewName : getDefaultName(ST_INVALID);
}
