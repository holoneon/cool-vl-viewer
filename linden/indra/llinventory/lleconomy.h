/**
 * @file lleconomy.h
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llsingleton.h"

class LLSD;
class LLMessageSystem;
class LLVector3;

// These are the defaults for SL
constexpr S32 DEFAULT_UPLOAD_COST = 10;
constexpr S32 DEFAULT_GROUP_COST = 100;
constexpr S32 DEFAULT_MAX_PICKS = 10;

class LLEconomy final : public LLSingleton<LLEconomy>
{
	friend class LLSingleton<LLEconomy>;

protected:
	LOG_CLASS(LLEconomy);

public:
	LLEconomy();

	void setDefaultCosts(bool in_sl);
	void processEconomyData(LLMessageSystem* msg);
	void setBenefits(const LLSD& data, const std::string& account_type);

	LL_INLINE S32 getPriceUpload() const			{ return mPriceUpload; }
	LL_INLINE S32 getAnimationUploadCost() const	{ return mAnimationUploadCost; }
	LL_INLINE S32 getSoundUploadCost() const		{ return mSoundUploadCost; }
	LL_INLINE S32 getCreateGroupCost() const		{ return mCreateGroupCost; }
	LL_INLINE S32 getAttachmentLimit() const		{ return mAttachmentLimit; }
	LL_INLINE S32 getAnimatedObjectLimit() const	{ return mAnimatedObjectLimit; }
	LL_INLINE S32 getGroupMembershipLimit() const	{ return mGroupMembershipLimit; }

	// A texture upload cost depends on its size, now...
	LL_INLINE S32 getTextureUploadCost() const		{ return mTextureUploadCost; }
	LL_INLINE S32 get2KTextureUploadCost() const	{ return m2KTextureUploadCost; }
	S32 getTextureUploadCost(S32 tex_area) const;
	S32 getTextureUploadCost(S32 x_size, S32 y_size) const;

	LL_INLINE S32 getPicksLimit() const
	{
		return mPicksLimit > -1 ? mPicksLimit : DEFAULT_MAX_PICKS;
	}

	const LLSD& getBenefit(const std::string& key) const;

private:
	std::string	mAccountType;

	LLSD		mBenefits;

	// Note: mPriceUpload is now llmax(mAnimationUploadCost, mSoundUploadCost,
	// mTextureUploadCost) when benefits are implemented in the grid (when not
	// all four costs are equal).
	S32			mPriceUpload;
	S32			mAnimationUploadCost;
	S32			mSoundUploadCost;
	S32			mTextureUploadCost;
	S32			m2KTextureUploadCost;
	S32			mCreateGroupCost;
	S32			mAttachmentLimit;
	S32			mAnimatedObjectLimit;
	S32			mGroupMembershipLimit;
	S32			mPicksLimit;

	bool		mGotBenefits;
};
