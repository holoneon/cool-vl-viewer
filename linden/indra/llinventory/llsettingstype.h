/**
 * @file llsettingstype.h
 * @brief LLSettingsType class header file
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

#pragma once

#include "llinventorytype.h"

// Purely static class
class LLSettingsType
{
	LLSettingsType() = delete;
	~LLSettingsType() = delete;

public:
	static void initClass(LLTranslationBridge::ptr_t& trans);
	static void cleanupClass();

	enum EType
	{
		ST_SKY = 0,
		ST_WATER = 1,
		ST_DAYCYCLE = 2,

		ST_INVALID = 255,
		ST_NONE = -1
	};

	static EType fromInventoryFlags(U32 flags);
	static LLInventoryType::EIconName getIconName(EType type);
	static std::string getDefaultName(EType type);
};
