/**
 * @file llinventorytype.h
 * @brief Inventory item type, more specific than an asset type.
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

#pragma once

#include <memory>

#include "llassettype.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryType
//
// Class used to encapsulate operations around inventory type.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryType
{
public:
	enum EType : S32
	{
		IT_TEXTURE = 0,
		IT_SOUND = 1,
		IT_CALLINGCARD = 2,
		IT_LANDMARK = 3,
		//IT_SCRIPT = 4,
		//IT_CLOTHING = 5,
		IT_OBJECT = 6,
		IT_NOTECARD = 7,
		IT_CATEGORY = 8,
		IT_ROOT_CATEGORY = 9,
		IT_LSL = 10,
		//IT_LSL_BYTECODE = 11,
		//IT_TEXTURE_TGA = 12,
		//IT_BODYPART = 13,
		//IT_TRASH = 14,
		IT_SNAPSHOT = 15,
		//IT_LOST_AND_FOUND = 16,
		IT_ATTACHMENT = 17,
		IT_WEARABLE = 18,
		IT_ANIMATION = 19,
		IT_GESTURE = 20,
#if LL_MESH_ASSET_SUPPORT
		IT_MESH = 22,
#endif
		IT_SETTINGS = 25,
		IT_MATERIAL = 26,
		IT_GLTF = 27,
		IT_GLTF_BIN = 28,

		IT_COUNT,

		IT_NONE = -1
	};

	enum EIconName : S32
	{
		ICONNAME_TEXTURE,
		ICONNAME_SOUND,
		ICONNAME_CALLINGCARD_ONLINE,
		ICONNAME_CALLINGCARD_OFFLINE,
		ICONNAME_LANDMARK,
		ICONNAME_LANDMARK_VISITED,
		ICONNAME_SCRIPT,
		ICONNAME_CLOTHING,
		ICONNAME_OBJECT,
		ICONNAME_OBJECT_MULTI,
		ICONNAME_NOTECARD,
		ICONNAME_BODYPART,
		ICONNAME_SNAPSHOT,

		ICONNAME_BODYPART_SHAPE,
		ICONNAME_BODYPART_SKIN,
		ICONNAME_BODYPART_HAIR,
		ICONNAME_BODYPART_EYES,
		ICONNAME_CLOTHING_SHIRT,
		ICONNAME_CLOTHING_PANTS,
		ICONNAME_CLOTHING_SHOES,
		ICONNAME_CLOTHING_SOCKS,
		ICONNAME_CLOTHING_JACKET,
		ICONNAME_CLOTHING_GLOVES,
		ICONNAME_CLOTHING_UNDERSHIRT,
		ICONNAME_CLOTHING_UNDERPANTS,
		ICONNAME_CLOTHING_SKIRT,
		ICONNAME_CLOTHING_ALPHA,
		ICONNAME_CLOTHING_TATTOO,
		ICONNAME_CLOTHING_UNIVERSAL,

		ICONNAME_ANIMATION,
		ICONNAME_GESTURE,

		ICONNAME_CLOTHING_PHYSICS,

		ICONNAME_LINKITEM,
		ICONNAME_LINKFOLDER,

#if LL_MESH_ASSET_SUPPORT
		ICONNAME_MESH,
#endif

		ICONNAME_SETTINGS,
		ICONNAME_SETTINGS_SKY,
		ICONNAME_SETTINGS_WATER,
		ICONNAME_SETTINGS_DAY,

		ICONNAME_MATERIAL,

		ICONNAME_INVALID,
		ICONNAME_COUNT,
		ICONNAME_NONE = -1
	};

	static EType lookup(const std::string& name);
	static const std::string& lookup(EType type);
	// translation from a type to a human readable form.
	static const std::string& lookupHumanReadable(EType type);

	// return the default inventory for the given asset type.
	static EType defaultForAssetType(LLAssetType::EType asset_type);

	// true if this type cannot have restricted permissions.
	static bool cannotRestrictPermissions(EType type);

private:
	// Do not instantiate or derive one of these objects
	LLInventoryType();
	~LLInventoryType();
};

class LLTranslationBridge
{
public:
	typedef std::shared_ptr<LLTranslationBridge> ptr_t;

	virtual ~LLTranslationBridge() = default;

	virtual std::string getString(const std::string& xml_desc) = 0;
};

// Helper function that returns true if inventory type and asset type
// are potentially compatible. For example, an attachment must be an
// object, but a wearable can be a bodypart or clothing asset.
bool inventory_and_asset_types_match(LLInventoryType::EType inventory_type,
									 LLAssetType::EType asset_type);
