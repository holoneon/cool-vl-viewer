/**
 * @file llmaterialtable.h
 * @brief Table of material information for the viewer UI
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

#include "lluuid.h"
#include "llstring.h"

#include <list>

// NOTE: this is a simplified version of the material table with all the server
// related code removed (since never used by the viewer). HB

// Material types
constexpr U8 LL_MCODE_STONE   = 0;
constexpr U8 LL_MCODE_METAL   = 1;
constexpr U8 LL_MCODE_GLASS   = 2;
constexpr U8 LL_MCODE_WOOD    = 3;
constexpr U8 LL_MCODE_FLESH   = 4;
constexpr U8 LL_MCODE_PLASTIC = 5;
constexpr U8 LL_MCODE_RUBBER  = 6;
constexpr U8 LL_MCODE_LIGHT   = 7;
constexpr U8 LL_MCODE_END     = 8;
constexpr U8 LL_MCODE_MASK    = 0x0F;

class LLMaterialInfo
{
public:
	LL_INLINE LLMaterialInfo(U8 code, const char* name, F32 fric, F32 rest)
	:	mMCode(code),
		mName(name),
		mFriction(fric),
		mRestitution(rest)
	{
	}

public:
	std::string	mName;
	F32         mFriction;
	F32         mRestitution;
	U8		    mMCode;
};

class LLMaterialTable
{
public:
	LLMaterialTable();

	void initTableTransNames(strings_map_t namemap);

	U8 getMCode(const std::string& name) const;	// 0 if not found
	const std::string& getName(U8 mcode) const;

	// Physics values (used as default in the Build tools floater)
	F32 getFriction(U8 mcode) const;
	F32 getRestitution(U8 mcode) const;

	LL_INLINE bool isCollisionSound(const LLUUID& sound_id) const
	{
		return mCollisionsSounds.count(sound_id);
	}

public:
	typedef std::list<LLMaterialInfo> info_list_t;
	info_list_t				mMaterialInfoList;

	uuid_list_t				mCollisionsSounds;
};

extern LLMaterialTable gMaterialTable;
