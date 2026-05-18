/**
 * @file llpathfindinglinkset.h
 * @brief Definition of a pathfinding linkset that contains various properties required for havok pathfinding.
 *
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

#include "llpathfindingobject.h"

class LLSD;

constexpr S32 MIN_WALKABILITY_VALUE = 0;
constexpr S32 MAX_WALKABILITY_VALUE = 100;

class LLPathfindingLinkset : public LLPathfindingObject
{
protected:
	LOG_CLASS(LLPathfindingLinkset);

public:
	typedef enum
	{
		kUnknown,
		kWalkable,
		kStaticObstacle,
		kDynamicObstacle,
		kMaterialVolume,
		kExclusionVolume,
		kDynamicPhantom
	} ELinksetUse;

	LLPathfindingLinkset(const LLSD& terrain_data);
	LLPathfindingLinkset(const LLUUID& id, const LLSD& data);
	LLPathfindingLinkset(const LLPathfindingLinkset& obj);

	LLPathfindingLinkset& operator=(const LLPathfindingLinkset& obj);

	LL_INLINE LLPathfindingLinkset* asLinkset() override
	{
		return this;
	}

	LL_INLINE const LLPathfindingLinkset* asLinkset() const override
	{
		return this;
	}

	LL_INLINE U32 getLandImpact() const					{ return mLandImpact; }
	LL_INLINE bool isTerrain() const					{ return mIsTerrain; }
	LL_INLINE bool isModifiable() const					{ return mIsModifiable; }
	LL_INLINE bool canBeVolume() const					{ return mCanBeVolume; }
	bool isPhantom() const;

	static ELinksetUse getLinksetUseWithToggledPhantom(ELinksetUse use);

	LL_INLINE ELinksetUse getLinksetUse() const			{ return mLinksetUse; }

	LL_INLINE bool isScripted() const					{ return mIsScripted; }
	LL_INLINE bool hasIsScripted() const				{ return mHasIsScripted; }

	LL_INLINE S32 getWalkabilityCoefficientA() const	{ return mWalkabilityCoefficientA; }
	LL_INLINE S32 getWalkabilityCoefficientB() const	{ return mWalkabilityCoefficientB; }
	LL_INLINE S32 getWalkabilityCoefficientC() const	{ return mWalkabilityCoefficientC; }
	LL_INLINE S32 getWalkabilityCoefficientD() const	{ return mWalkabilityCoefficientD; }

	bool showUnmodifiablePhantomWarning(ELinksetUse use) const;
	bool showPhantomToggleWarning(ELinksetUse use) const;
	bool showCannotBeVolumeWarning(ELinksetUse use) const;
	LLSD encodeAlteredFields(ELinksetUse use, S32 a, S32 b, S32 c,
							 S32 d) const;

private:
	typedef enum
	{
		kNavMeshGenerationIgnore,
		kNavMeshGenerationInclude,
		kNavMeshGenerationExclude
	} ENavMeshGenerationCategory;

	void parseLinksetData(const LLSD& data);
	void parsePathfindingData(const LLSD& data);

	static bool isPhantom(ELinksetUse use);
	static ELinksetUse getLinksetUse(bool phantom, ENavMeshGenerationCategory category);
	static ENavMeshGenerationCategory getNavMeshGenerationCategory(ELinksetUse use);
	static LLSD convertCategoryToLLSD(ENavMeshGenerationCategory category);
	static ENavMeshGenerationCategory convertCategoryFromLLSD(const LLSD& llsd);

private:
	S32			mWalkabilityCoefficientA;
	S32			mWalkabilityCoefficientB;
	S32			mWalkabilityCoefficientC;
	S32			mWalkabilityCoefficientD;
	U32			mLandImpact;
	ELinksetUse	mLinksetUse;
	bool		mIsTerrain;
	bool		mIsModifiable;
	bool		mCanBeVolume;
	bool		mIsScripted;
	bool		mHasIsScripted;
};
