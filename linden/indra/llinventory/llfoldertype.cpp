/**
 * @file llfoldertype.cpp
 * @brief Implementatino of LLFolderType functionality.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llfoldertype.h"

#include "lldictionary.h"
#include "llsingleton.h"

//static
bool LLFolderType::sCanDeleteCOF = false;

struct FolderEntry : public LLDictionaryEntry
{
	FolderEntry(const std::string& type_name,	// 8 character limit !
				// Can the viewer change categories of this type ?
				bool is_protected)
	:	LLDictionaryEntry(type_name),
		mIsProtected(is_protected)
	{
		llassert_always(type_name.length() <= 8);
	}

	const bool mIsProtected;
};

class LLFolderDictionary : public LLSingleton<LLFolderDictionary>,
						   public LLDictionary<LLFolderType::EType, FolderEntry>
{
	friend class LLSingleton<LLFolderDictionary>;

public:
	LLFolderDictionary();

protected:
	virtual LLFolderType::EType notFound() const
	{
		return LLFolderType::FT_NONE;
	}
};

LLFolderDictionary::LLFolderDictionary()
{
	//       													    TYPE NAME	PROTECTED
	//      													   |-----------|---------|
	addEntry(LLFolderType::FT_TEXTURE, 				new FolderEntry("texture",	true));
	addEntry(LLFolderType::FT_SOUND, 				new FolderEntry("sound",	true));
	addEntry(LLFolderType::FT_CALLINGCARD, 			new FolderEntry("callcard",	true));
	addEntry(LLFolderType::FT_LANDMARK, 			new FolderEntry("landmark",	true));
	addEntry(LLFolderType::FT_CLOTHING, 			new FolderEntry("clothing",	true));
	addEntry(LLFolderType::FT_OBJECT, 				new FolderEntry("object",	true));
	addEntry(LLFolderType::FT_NOTECARD, 			new FolderEntry("notecard",	true));
	addEntry(LLFolderType::FT_ROOT_INVENTORY, 		new FolderEntry("root_inv",	true));
	addEntry(LLFolderType::FT_ROOT_INVENTORY_OS,	new FolderEntry("root_os",	true));
	addEntry(LLFolderType::FT_LSL_TEXT, 			new FolderEntry("lsltext",	true));
	addEntry(LLFolderType::FT_BODYPART, 			new FolderEntry("bodypart",	true));
	addEntry(LLFolderType::FT_TRASH, 				new FolderEntry("trash",	true));
	addEntry(LLFolderType::FT_SNAPSHOT_CATEGORY, 	new FolderEntry("snapshot", true));
	addEntry(LLFolderType::FT_LOST_AND_FOUND, 		new FolderEntry("lstndfnd",	true));
	addEntry(LLFolderType::FT_ANIMATION, 			new FolderEntry("animatn",	true));
	addEntry(LLFolderType::FT_GESTURE, 				new FolderEntry("gesture",	true));
	addEntry(LLFolderType::FT_MESH, 				new FolderEntry("mesh",		false));

#if 0	// Viewer 2 folder: useless for v1 !
	addEntry(LLFolderType::FT_FAVORITE, 			new FolderEntry("favorite",	false));
#endif

	addEntry(LLFolderType::FT_CURRENT_OUTFIT, 		new FolderEntry("current",	true));

#if 0	// Viewer 2 folders: useless for v1 !
	addEntry(LLFolderType::FT_OUTFIT, 				new FolderEntry("outfit",	false));
	addEntry(LLFolderType::FT_MY_OUTFITS, 			new FolderEntry("my_otfts",	false));

	addEntry(LLFolderType::FT_BASIC_ROOT,			new FolderEntry("basic_rt", true));
#endif

	addEntry(LLFolderType::FT_MARKETPLACE_LISTINGS,	new FolderEntry("merchant",	true));
	addEntry(LLFolderType::FT_MARKETPLACE_STOCK,	new FolderEntry("stock",	false));
	addEntry(LLFolderType::FT_MARKETPLACE_VERSION,	new FolderEntry("version",	false));

	addEntry(LLFolderType::FT_INBOX, 				new FolderEntry("inbox",	false));

	addEntry(LLFolderType::FT_SETTINGS,				new FolderEntry("settings",	false));

	addEntry(LLFolderType::FT_MATERIAL,				new FolderEntry("material",	false));

	// NOTE: OpenSim servers refuse to delete the Suitcase folder, meaning it would
	// reapear at next login if deleted in the viewer...
	addEntry(LLFolderType::FT_SUITCASE,				new FolderEntry("suitcase",	true));

	addEntry(LLFolderType::FT_NONE, 				new FolderEntry("-1",		false));
};

//static
LLFolderType::EType LLFolderType::lookup(const std::string& name)
{
	return LLFolderDictionary::getInstance()->lookup(name);
}

//static
const std::string& LLFolderType::lookup(LLFolderType::EType folder_type)
{
	const FolderEntry* entry = LLFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mName;
	}
	return badLookup();
}

//static
// Only basic v1 folders are protected (i.e. we allow to destroy all the stupid
// and useless v2 folders).
bool LLFolderType::lookupIsProtectedType(EType folder_type)
{
	if (folder_type == FT_CURRENT_OUTFIT && sCanDeleteCOF)
	{
		return false;
	}
	const LLFolderDictionary* dict = LLFolderDictionary::getInstance();
	const FolderEntry* entry = dict->lookup(folder_type);
	return entry && entry->mIsProtected;
}

//static
LLAssetType::EType LLFolderType::folderTypeToAssetType(LLFolderType::EType folder_type)
{
	if (LLAssetType::lookup(LLAssetType::EType(folder_type)) == LLAssetType::badLookup())
	{
		llwarns << "Converting to unknown asset type " << folder_type << llendl;
	}
	return (LLAssetType::EType)folder_type;
}

//static
LLFolderType::EType LLFolderType::assetTypeToFolderType(LLAssetType::EType asset_type)
{
	if (LLFolderType::lookup(LLFolderType::EType(asset_type)) == LLFolderType::badLookup())
	{
		llwarns << "Converting to unknown folder type " << asset_type << llendl;
	}
	return (LLFolderType::EType)asset_type;
}

//static
const std::string &LLFolderType::badLookup()
{
	static const std::string sBadLookup = "llfoldertype_bad_lookup";
	return sBadLookup;
}
