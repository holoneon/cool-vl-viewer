/**
 * @file llinventoryicon.h
 * @brief Class definition of the inventory icon.
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
#include "llui.h"

// Purely static class
class LLInventoryIcon
{
public:
	LLInventoryIcon() = delete;
	~LLInventoryIcon() = delete;

	// Note: in the methods below, 'misc_flag' have different meanings
	// depending on item type.

	static const std::string& getIconName(LLAssetType::EType asset_type,
										  LLInventoryType::EType inv_type =
											LLInventoryType::IT_NONE,
										  U32 misc_flag = 0,
										  bool item_is_multi = false);
	static const std::string& getIconName(LLInventoryType::EIconName idx);

	static const LLUIImagePtr getIcon(LLAssetType::EType asset_type,
									  LLInventoryType::EType inv_type =
										LLInventoryType::IT_NONE,
									  U32 misc_flag = 0,
									  bool item_is_multi = false);
	static const LLUIImagePtr getIcon(LLInventoryType::EIconName idx);

protected:
	static const LLInventoryType::EIconName getIconIdx(LLAssetType::EType asset_type,
													   LLInventoryType::EType inv_type,
													   U32 misc_flag,
													   bool item_is_multi);

	static LLInventoryType::EIconName assignWearableIcon(U32 misc_flag);
	static LLInventoryType::EIconName assignSettingsIcon(U32 misc_flag);
};
