/**
 * @file llwearabletype.h
 * @brief LLWearableType class header file
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

#pragma once

#include "llassettype.h"
#include "llinventorytype.h"

// Purely static class
class LLWearableType
{
	LLWearableType() = delete;
	~LLWearableType() = delete;

public:
	enum EType
	{
		WT_SHAPE = 0,
		WT_SKIN,
		WT_HAIR,
		WT_EYES,
		WT_SHIRT,
		WT_PANTS,
		WT_SHOES,
		WT_SOCKS,
		WT_JACKET,
		WT_GLOVES,
		WT_UNDERSHIRT,
		WT_UNDERPANTS,
		WT_SKIRT,
		WT_ALPHA,
		WT_TATTOO,
		WT_PHYSICS,
		WT_UNIVERSAL,
		WT_COUNT,

		WT_INVALID = 255,
		WT_NONE = -1,
	};

	static void initClass(LLTranslationBridge::ptr_t trans);
	static void cleanupClass();

	static const std::string& getTypeName(EType type);
	static std::string getCapitalizedTypeName(EType type);
	static const std::string& getTypeDefaultNewName(EType type);
	static const std::string& getTypeLabel(EType type);
	static LLAssetType::EType getAssetType(EType type);
	static EType typeNameToType(const std::string& type_name);
	static LLInventoryType::EIconName getIconName(EType type);
	static bool getDisableCameraSwitch(EType type);
	static bool getAllowMultiwear(EType type);
	static EType inventoryFlagsToWearableType(U32 flags);
};
