/**
 * @file   llmaterialid.h
 * @brief  Header file for llmaterialid
 * @author Stinson@lindenlab.com
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Linden Research, Inc.
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

#include <string>

#include "llsd.h"
#include "lluuid.h"

class LLMaterialID final
{
public:
	LLMaterialID();
	LLMaterialID(const LLSD& matidp);
	LLMaterialID(const LLSD::Binary& matidp);
	LLMaterialID(const void* memoryp);
	LLMaterialID(const LLMaterialID& other_mat_id);
	LLMaterialID(const LLUUID& uuid);

	// Allow the use of the C++11 default move constructor
	LLMaterialID(LLMaterialID&& other) noexcept = default;

	LL_INLINE bool operator==(const LLMaterialID& other_mat_id) const
	{
		return compareToOtherMaterialID(other_mat_id) == 0;
	}

	LL_INLINE bool operator!=(const LLMaterialID& other_mat_id) const
	{
		return compareToOtherMaterialID(other_mat_id) != 0;
	}

	LL_INLINE bool operator<(const LLMaterialID& other_mat_id) const
	{
		return compareToOtherMaterialID(other_mat_id) < 0;
	}

	LL_INLINE bool operator<=(const LLMaterialID& other_mat_id) const
	{
		return compareToOtherMaterialID(other_mat_id) <= 0;
	}

	LL_INLINE bool operator>(const LLMaterialID& other_mat_id) const
	{
		return compareToOtherMaterialID(other_mat_id) > 0;
	}

	LL_INLINE bool operator>=(const LLMaterialID& other_mat_id) const
	{
		return compareToOtherMaterialID(other_mat_id) >= 0;
	}

	LL_INLINE bool isNull() const
	{
		return compareToOtherMaterialID(LLMaterialID::null) == 0;
	}

	LL_INLINE bool notNull() const
	{
		return compareToOtherMaterialID(LLMaterialID::null) != 0;
	}

	LLMaterialID& operator=(const LLMaterialID& other_mat_id)
	{
		copyFromOtherMaterialID(other_mat_id);
		return *this;
	}

	LL_INLINE const U8* get() const					{ return mID; }

	void set(const void* memoryp);
	void clear();

	LLUUID asUUID() const;
	LLSD asLLSD() const;
	std::string asString() const;

	friend std::ostream& operator<<(std::ostream& s,
									const LLMaterialID& material_id);

	// Returns a 64 bits digest of the material Id, by XORing its two 64 bits
	// long words. HB
	LL_INLINE U64 getDigest64() const
	{
		U64* tmp = (U64*)mID;
		return tmp[0] ^ tmp[1];
	}

public:
	static const LLMaterialID null;

private:
	void parseFromBinary(const LLSD::Binary& matidp);
	void copyFromOtherMaterialID(const LLMaterialID& other_mat_id);
	S32 compareToOtherMaterialID(const LLMaterialID& other_mat_id) const;

public:
	U8 mID[UUID_BYTES];
};

// std::hash implementation for LLMaterialID
namespace std
{
	template<> struct hash<LLMaterialID>
	{
		LL_INLINE size_t operator()(const LLMaterialID& id) const noexcept
		{
			return id.getDigest64();
		}
	};
}

// For use with boost::unordered_map and boost::unordered_set
LL_INLINE size_t hash_value(const LLMaterialID& id) noexcept
{
	return id.getDigest64();
}
