/**
 * @file llassettype.cpp
 * @brief Implementatino of LLAssetType functionality.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include <time.h>

#include "llassettype.h"
#include "lldictionary.h"

struct AssetEntry : public LLDictionaryEntry
{
	AssetEntry(const char* desc_name,
			   const char* type_name, 	// 8 characters limit !
			   // For decoding to human readable form; put any and as many
			   // printable characters you want in each one
			   const char* human_name,
			   EDragAndDropType dad_type,
			   bool can_link, 			// Can you create a link to this type ?
			   bool can_fetch, 			// Can you fetch this asset by Id ?
			   bool can_know) 			// Can you see this asset Id ?
		:
		LLDictionaryEntry(desc_name),
		mTypeName(type_name),
		mHumanName(human_name),
		mDadType(dad_type),
		mCanLink(can_link),
		mCanFetch(can_fetch),
		mCanKnow(can_know)
	{
		llassert_always(strlen(mTypeName) <= 8);
	}

	const char*			mTypeName;
	const char*			mHumanName;
	EDragAndDropType	mDadType;
	bool				mCanLink;
	bool				mCanFetch;
	bool				mCanKnow;
};

class LLAssetDictionary : public LLDictionary<LLAssetType::EType, AssetEntry>
{
public:
	LLAssetDictionary();
};

// Since it is a small structure, let's initialize it unconditionally (i.e.
// even if we do not log in) at global scope. This saves having to bother with
// a costly LLSingleton (slow, lot's of CPU cycles and cache lines wasted) or
// to find the right place where to construct the class on login... HB
LLAssetDictionary gAssetDictionary;

LLAssetDictionary::LLAssetDictionary()
{
	//       												   DESCRIPTION      TYPE NAME    HUMAN NAME          DRAG&DROP TYPE      CAN LINK ?  CAN FETCH ? CAN KNOW ?
	//      												  |----------------|------------|-------------------|-------------------|-----------|-----------|---------|
	addEntry(LLAssetType::AT_TEXTURE, 			new AssetEntry("TEXTURE",		"texture",	"texture",			DAD_TEXTURE,		true,		false,		true));
	addEntry(LLAssetType::AT_SOUND, 			new AssetEntry("SOUND",			"sound",	"sound",			DAD_SOUND,			true,		true,		true));
	addEntry(LLAssetType::AT_CALLINGCARD, 		new AssetEntry("CALLINGCARD",	"callcard",	"calling card",		DAD_CALLINGCARD,	true,		false,		false));
	addEntry(LLAssetType::AT_LANDMARK, 			new AssetEntry("LANDMARK",		"landmark",	"landmark",			DAD_LANDMARK,		true,		true,		true));
	addEntry(LLAssetType::AT_SCRIPT, 			new AssetEntry("SCRIPT",		"script",	"legacy script",	DAD_NONE,			true,		false,		false));
	addEntry(LLAssetType::AT_CLOTHING, 			new AssetEntry("CLOTHING",		"clothing",	"clothing",			DAD_CLOTHING,		true,		true,		true));
	addEntry(LLAssetType::AT_OBJECT, 			new AssetEntry("OBJECT",		"object",	"object",			DAD_OBJECT,			true,		false,		false));
	addEntry(LLAssetType::AT_NOTECARD, 			new AssetEntry("NOTECARD",		"notecard",	"note card",		DAD_NOTECARD,		true,		false,		true));
	addEntry(LLAssetType::AT_CATEGORY, 			new AssetEntry("CATEGORY",		"category",	"folder",			DAD_CATEGORY,		true,		false,		false));
	addEntry(LLAssetType::AT_LSL_TEXT, 			new AssetEntry("LSL_TEXT",		"lsltext",	"lsl2 script",		DAD_SCRIPT,			true,		false,		false));
	addEntry(LLAssetType::AT_LSL_BYTECODE, 		new AssetEntry("LSL_BYTECODE",	"lslbyte",	"lsl bytecode",		DAD_NONE,			true,		false,		false));
	addEntry(LLAssetType::AT_TEXTURE_TGA, 		new AssetEntry("TEXTURE_TGA",	"txtr_tga",	"tga texture",		DAD_NONE,			true,		false,		false));
	addEntry(LLAssetType::AT_BODYPART, 			new AssetEntry("BODYPART",		"bodypart",	"body part",		DAD_BODYPART,		true,		true,		true));
	addEntry(LLAssetType::AT_SOUND_WAV, 		new AssetEntry("SOUND_WAV",		"snd_wav",	"sound",			DAD_NONE,			true,		false,		false));
	addEntry(LLAssetType::AT_IMAGE_TGA, 		new AssetEntry("IMAGE_TGA",		"img_tga",	"targa image",		DAD_NONE,			true,		false,		false));
	addEntry(LLAssetType::AT_IMAGE_JPEG, 		new AssetEntry("IMAGE_JPEG",	"jpeg",		"jpeg image",		DAD_NONE,			true,		false,		false));
	addEntry(LLAssetType::AT_ANIMATION, 		new AssetEntry("ANIMATION",		"animatn",	"animation",		DAD_ANIMATION,		true,		true,		true));
	addEntry(LLAssetType::AT_GESTURE, 			new AssetEntry("GESTURE",		"gesture",	"gesture",			DAD_GESTURE,		true,		true,		true));
	addEntry(LLAssetType::AT_SIMSTATE, 			new AssetEntry("SIMSTATE",		"simstate",	"simstate",			DAD_NONE,			false,		false,		false));
	addEntry(LLAssetType::AT_LINK, 				new AssetEntry("LINK",			"link",		"sym link",			DAD_LINK,			false,		false,		true));
	addEntry(LLAssetType::AT_LINK_FOLDER, 		new AssetEntry("FOLDER_LINK",	"link_f", 	"sym folder link",	DAD_LINK,			false,		false,		true));
	addEntry(LLAssetType::AT_MARKETPLACE_FOLDER,new AssetEntry("MARKETPLACE",	"market",	"marketplace",		DAD_NONE,			false,		false,		false));
#if LL_MESH_ASSET_SUPPORT
	addEntry(LLAssetType::AT_MESH,              new AssetEntry("MESH",			"mesh",		"mesh",             DAD_MESH,			false,		true,		true));
#endif
	addEntry(LLAssetType::AT_SETTINGS,          new AssetEntry("SETTINGS",		"settings",	"settings",         DAD_SETTINGS,		true,		true,		true));
	addEntry(LLAssetType::AT_MATERIAL,          new AssetEntry("MATERIAL",		"material",	"render material",  DAD_MATERIAL,		true,		true,		true));
	addEntry(LLAssetType::AT_NONE, 				new AssetEntry("NONE",			"-1",		NULL,		  		DAD_NONE,			false,		false,		false));
};

//-----------------------------------------------------------------------------
// LLAssetType static class
//-----------------------------------------------------------------------------

//static
LLAssetType::EType LLAssetType::getType(const std::string& desc_name)
{
	std::string s = desc_name;
	LLStringUtil::toUpper(s);
	return gAssetDictionary.lookup(s);
}

//static
const std::string& LLAssetType::getDesc(LLAssetType::EType asset_type)
{
	const AssetEntry* entry = gAssetDictionary.lookup(asset_type);
	if (entry)
	{
		return entry->mName;
	}
	return badLookup();
}

//static
const char* LLAssetType::lookup(LLAssetType::EType asset_type)
{
	const AssetEntry* entry = gAssetDictionary.lookup(asset_type);
	if (entry)
	{
		return entry->mTypeName;
	}
	return badLookup().c_str();
}

//static
LLAssetType::EType LLAssetType::lookup(const char* name)
{
	return lookup(ll_safe_string(name));
}

//static
LLAssetType::EType LLAssetType::lookup(const std::string& type_name)
{
	for (LLAssetDictionary::const_iterator iter = gAssetDictionary.begin(),
										   end = gAssetDictionary.end();
		 iter != end; ++iter)
	{
		const AssetEntry* entry = iter->second;
		if (type_name == entry->mTypeName)
		{
			return iter->first;
		}
	}
	return AT_NONE;
}

//static
const char* LLAssetType::lookupHumanReadable(LLAssetType::EType asset_type)
{
	const AssetEntry* entry = gAssetDictionary.lookup(asset_type);
	if (entry)
	{
		return entry->mHumanName;
	}
	return badLookup().c_str();
}

//static
LLAssetType::EType LLAssetType::lookupHumanReadable(const char* name)
{
	return lookupHumanReadable(ll_safe_string(name));
}

//static
LLAssetType::EType LLAssetType::lookupHumanReadable(const std::string& readable_name)
{
	for (LLAssetDictionary::const_iterator iter = gAssetDictionary.begin(),
										   end = gAssetDictionary.end();
		 iter != end; ++iter)
	{
		const AssetEntry* entry = iter->second;
		if (entry->mHumanName && (readable_name == entry->mHumanName))
		{
			return iter->first;
		}
	}
	return AT_NONE;
}

//static
bool LLAssetType::lookupCanLink(EType asset_type)
{
	const AssetEntry* entry = gAssetDictionary.lookup(asset_type);
	return entry && entry->mCanLink;
}

// Not adding this to dictionary since we probably will only have these two types
//static
bool LLAssetType::lookupIsLinkType(EType asset_type)
{
	return asset_type == AT_LINK || asset_type == AT_LINK_FOLDER;
}

//static
const std::string& LLAssetType::badLookup()
{
	static const std::string sBadLookup = "llassettype_bad_lookup";
	return sBadLookup;
}

//static
bool LLAssetType::lookupIsAssetFetchByIDAllowed(EType asset_type)
{
	const AssetEntry* entry = gAssetDictionary.lookup(asset_type);
	return entry && entry->mCanFetch;
}

//static
bool LLAssetType::lookupIsAssetIDKnowable(EType asset_type)
{
	const AssetEntry* entry = gAssetDictionary.lookup(asset_type);
	return entry && entry->mCanKnow;
}

//static
EDragAndDropType LLAssetType::lookupDragAndDropType(EType asset_type)
{
	const AssetEntry* entry = gAssetDictionary.lookup(asset_type);
	return entry ? entry->mDadType : DAD_NONE;
}

// Generate a good default description
//static
void LLAssetType::generateDescriptionFor(EType asset_type, std::string& desc)
{
	constexpr S32 BUF_SIZE = 30;
	char time_str[BUF_SIZE];
	time_t now;
	time(&now);
	memset(time_str, '\0', BUF_SIZE);
	strftime(time_str, BUF_SIZE - 1, "%Y-%m-%d %H:%M:%S ", localtime(&now));
	desc.assign(time_str);
	desc.append(lookupHumanReadable(asset_type));
}
