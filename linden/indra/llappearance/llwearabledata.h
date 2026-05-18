/**
 * @file llwearabledata.h
 * @brief LLWearableData class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llavatarappearancedefines.h"
#include "llerror.h"
#include "llwearable.h"

class LLAvatarAppearance;

class LLWearableData
{
protected:
	LOG_CLASS(LLWearableData);

public:
	LLWearableData();
	virtual ~LLWearableData() = default;

	LL_INLINE void setAvatarAppearance(LLAvatarAppearance* appearance)
	{
		mAvatarAppearance = appearance;
	}

	LLWearable* getWearable(LLWearableType::EType type, U32 index);
	const LLWearable* getWearable(LLWearableType::EType type, U32 index) const;

	LLWearable* getTopWearable(LLWearableType::EType type);
	const LLWearable* getTopWearable(LLWearableType::EType type) const;

	LL_INLINE LLWearable* getBottomWearable(LLWearableType::EType type)
	{
		return getWearable(type, 0);
	}

	LL_INLINE const LLWearable* getBottomWearable(LLWearableType::EType t) const
	{
		return getWearable(t, 0);
	}

	U32 getWearableCount(LLWearableType::EType type) const;
	U32 getWearableCount(U32 tex_idx) const;
	bool getWearableIndex(const LLWearable* wearable, U32& index) const;

	U32 getClothingLayerCount() const;
	bool canAddWearable(LLWearableType::EType type) const;

	bool isOnTop(LLWearable* wearable) const;

	LLUUID computeBakedTextureHash(LLAvatarAppearanceDefines::EBakedTextureIndex idx,
								   bool generate_valid_hash = true);

protected:
//MK
	LL_INLINE void setCanWearFunc(bool (*func)(LLWearableType::EType))
	{
		mCanWearFunc = func;
	}

	LL_INLINE void setCanUnwearFunc(bool (*func)(LLWearableType::EType))
	{
		mCanUnwearFunc = func;
	}
//mk

	// Low-level data structure setter - public access is via setWearableItem, etc.

	// These two methods return false when they fail (e.g. because of RLV
	// restrictions)
	bool setWearable(LLWearableType::EType type, U32 index,
					 LLWearable* wearable);
	bool pushWearable(LLWearableType::EType type, LLWearable* wearable,
					  bool trigger_updated = true);

	virtual void wearableUpdated(LLWearable* wearable, bool removed);

	void eraseWearable(LLWearable* wearable);
	void eraseWearable(LLWearableType::EType type, U32 index);
	void clearWearableType(LLWearableType::EType type);
	bool swapWearables(LLWearableType::EType type, U32 index_a, U32 index_b);

	virtual void invalidateBakedTextureHash(LLMD5& hash) const	{}

private:
	void pullCrossWearableValues(LLWearableType::EType type);

public:
	static constexpr U32 MAX_CLOTHING_LAYERS = 60;

protected:
	LLAvatarAppearance* mAvatarAppearance;

	// All wearables of a certain type (e.g. all shirts):
	typedef std::vector<LLWearable*> wearableentry_vec_t;

	// Wearable "categories" arranged by wearable type:
	typedef std::map<LLWearableType::EType,
					 wearableentry_vec_t> wearableentry_map_t;
	wearableentry_map_t mWearableDatas;

private:
	bool (*mCanWearFunc)(LLWearableType::EType type);
	bool (*mCanUnwearFunc)(LLWearableType::EType type);
};
