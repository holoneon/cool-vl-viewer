/** 
 * @file lleconomy.cpp
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

#include "linden_common.h"

#include "lleconomy.h"

#include "llsdutil.h"
#include "llmessage.h"

constexpr S32 MAX_1K_TEXTURE_AREA = 1024 * 1024;

LLEconomy::LLEconomy()
:	mPriceUpload(-1),
	mAnimationUploadCost(-1),
	mSoundUploadCost(-1),
	mTextureUploadCost(-1),
	m2KTextureUploadCost(-1),
	mCreateGroupCost(-1),
	mAttachmentLimit(-1),
	mAnimatedObjectLimit(-1),
	mGroupMembershipLimit(-1),
	mPicksLimit(-1),
	mGotBenefits(false)
{
}

void LLEconomy::setDefaultCosts(bool in_sl)
{
	mPriceUpload = mAnimationUploadCost = mSoundUploadCost =
				   mTextureUploadCost = m2KTextureUploadCost =
						in_sl ? DEFAULT_UPLOAD_COST : 0;
	mCreateGroupCost = in_sl ? DEFAULT_GROUP_COST : 0;
	llinfos << "Price per upload: " << mPriceUpload
			<< " - Price for group creation: " << mCreateGroupCost << llendl;
}

void LLEconomy::processEconomyData(LLMessageSystem* msg)
{
	if (mGotBenefits)
	{
		llinfos << "Received legacy message for economy data after valid user account benefits were set. Ignoring."
				<< llendl;
		return;
	}

	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceUpload, mPriceUpload);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceGroupCreate,
					mCreateGroupCost);
#if 0	// Old economy data, never used...
	S32 i;
	msg->getS32Fast(_PREHASH_Info, _PREHASH_ObjectCapacity, i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_ObjectCount, i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceEnergyUnit, i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceObjectClaim, i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PricePublicObjectDecay, i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PricePublicObjectDelete, i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceRentLight, i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_TeleportMinPrice, i);
	F32 f;
	msg->getF32Fast(_PREHASH_Info, _PREHASH_TeleportPriceExponent, f);
#endif
	llinfos << "Received economy data. Price per upload: " << mPriceUpload
			<< " - Price for group creation: " << mCreateGroupCost << llendl;

	if (mAnimationUploadCost == -1)
	{
		mAnimationUploadCost = mPriceUpload;
	}
	if (mSoundUploadCost == -1)
	{
		mSoundUploadCost = mPriceUpload;
	}
	if (mTextureUploadCost == -1)
	{
		mTextureUploadCost = mPriceUpload;
	}
	if (m2KTextureUploadCost == -1)
	{
		m2KTextureUploadCost = mPriceUpload;
	}
}

static std::vector<std::string> sBenefits;

bool get_S32_value(const LLSD& sd, const char* key, S32& value)
{
	if (sd.has(key))
	{
		value = sd[key].asInteger();
		sBenefits.emplace_back(llformat("  - %s: %d", key, value));
		return true;
	}
	return false;
}

void LLEconomy::setBenefits(const LLSD& data, const std::string& account_type)
{
	LL_DEBUGS("Benefits") << ll_pretty_print_sd(data) << LL_ENDL;
	llinfos << "Account type: " << account_type << " - Setting benefits:"
			<< llendl;
	mAccountType = account_type;
	mBenefits = data;

	mGotBenefits = true;

	get_S32_value(data, "attachment_limit", mAttachmentLimit);
	get_S32_value(data, "animated_object_limit", mAnimatedObjectLimit);
	get_S32_value(data, "picks_limit", mPicksLimit);
	get_S32_value(data, "group_membership_limit", mGroupMembershipLimit);

	if (!get_S32_value(data, "create_group_cost", mCreateGroupCost))
	{
		mGotBenefits = false;
	}

	if (get_S32_value(data, "animation_upload_cost", mAnimationUploadCost))
	{
		if (mAnimationUploadCost > mPriceUpload)
		{
			mPriceUpload = mAnimationUploadCost;
		}
	}
	else
	{
		mGotBenefits = false;
	}

	if (get_S32_value(data, "sound_upload_cost", mSoundUploadCost))
	{
		if (mSoundUploadCost > mPriceUpload)
		{
			mPriceUpload = mSoundUploadCost;
		}
	}
	else
	{
		mGotBenefits = false;
	}

	if (get_S32_value(data, "texture_upload_cost", mTextureUploadCost))
	{
		if (mTextureUploadCost > mPriceUpload)
		{
			mPriceUpload = mTextureUploadCost;
		}
	}
	else
	{
		mGotBenefits = false;
	}

	m2KTextureUploadCost = 0;
	if (data.has("large_texture_upload_cost"))
	{
		const LLSD& costs = data["large_texture_upload_cost"];
		if (costs.isArray())
		{
			LL_DEBUGS("Benefits") << "Large textures upload cost: "
								  << ll_pretty_print_sd(costs) << LL_ENDL;
			// Apparently, LL considered several costs, but only use the
			// lowest. HB
			for (LLSD::array_const_iterator it = costs.beginArray(),
											end = costs.endArray();
				 it != end; ++it)
			{
				S32 cost = it->asInteger();
				if (cost > 0 &&
					(cost < m2KTextureUploadCost || !m2KTextureUploadCost))
				{
					m2KTextureUploadCost = cost;	// Use the lowest cost.
				}
			}
			if (m2KTextureUploadCost > 0)
			{
				sBenefits.emplace_back(llformat("  - large_texture_upload_cost: %d",
												m2KTextureUploadCost));
			}
		}
		else
		{
			get_S32_value(data, "large_texture_upload_cost",
						  m2KTextureUploadCost);
		}
	}
	if (m2KTextureUploadCost <= 0)
	{
		m2KTextureUploadCost = mTextureUploadCost;
	}

	for (size_t i = 0, count = sBenefits.size(); i < count; ++i)
	{
		llinfos << sBenefits[i] << llendl;
	}
	sBenefits.clear();

	llinfos << "Done." << llendl;
}

const LLSD& LLEconomy::getBenefit(const std::string& key) const
{
	static const LLSD empty;
	return mBenefits.has(key) ? mBenefits[key] : empty;
}

S32 LLEconomy::getTextureUploadCost(S32 tex_area) const
{
	return tex_area > MAX_1K_TEXTURE_AREA ? m2KTextureUploadCost
										  : mTextureUploadCost;
}

S32 LLEconomy::getTextureUploadCost(S32 x_size, S32 y_size) const
{
	return x_size * y_size > MAX_1K_TEXTURE_AREA ? m2KTextureUploadCost
												 : mTextureUploadCost;
}
