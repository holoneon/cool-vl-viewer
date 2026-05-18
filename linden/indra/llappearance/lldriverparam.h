/**
 * @file lldriverparam.h
 * @brief A visual parameter that drives (controls) other visual parameters.
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

#include <deque>

#include "llmemory.h"
#include "llviewervisualparam.h"
#include "llwearabletype.h"

class LLAvatarAppearance;
class LLDriverParam;
class LLWearable;

struct LLDrivenEntryInfo
{
	LLDrivenEntryInfo(S32 id, F32 min1, F32 max1, F32 max2, F32 min2)
	:	mDrivenID(id),
		mMin1(min1),
		mMax1(max1),
		mMax2(max2),
		mMin2(min2)
	{
	}

	S32 mDrivenID;
	F32 mMin1;
	F32 mMax1;
	F32 mMax2;
	F32 mMin2;
};

struct LLDrivenEntry
{
	LL_INLINE LLDrivenEntry(LLViewerVisualParam* paramp,
							LLDrivenEntryInfo* infop)
	:	mParam(paramp),
		mInfo(infop)
	{
	}

	LLViewerVisualParam*	mParam;
	LLDrivenEntryInfo*		mInfo;
};

class LLDriverParamInfo final : public LLViewerVisualParamInfo
{
	friend class LLDriverParam;

protected:
	LOG_CLASS(LLDriverParamInfo);

public:
	LL_INLINE LLDriverParamInfo()
	:	mDriverParam(NULL)
	{
	}

	bool parseXml(LLXmlTreeNode* node) override;

protected:
	typedef std::deque<LLDrivenEntryInfo> entry_info_list_t;
	entry_info_list_t	mDrivenInfoList;

	LLDriverParam*		mDriverParam; // backpointer
};

class alignas(16) LLDriverParam final : public LLViewerVisualParam
{
public:
	// No default constructor. Force construction with LLAvatarAppearance.
	LLDriverParam() = delete;

	LLDriverParam(LLAvatarAppearance* avatarp, LLWearable* wearablep = NULL);

	LL_INLINE LLDriverParam* asDriverParam() override	{ return this; }

	// Special: These functions are overridden by child classes

	LL_INLINE LLDriverParamInfo* getInfo() const		{ return (LLDriverParamInfo*)mInfo; }

	// This sets mInfo and calls initialization functions
	bool setInfo(LLDriverParamInfo* infop);

	LL_INLINE LLAvatarAppearance* getAvatarAppearance()	{ return mAvatarAppearance; }

	LL_INLINE const LLAvatarAppearance* getAvatarAppearance() const
	{
		return mAvatarAppearance;
	}

	void updateCrossDrivenParams(LLWearableType::EType driven_type);

	LLViewerVisualParam* cloneParam(LLWearable* wearablep) const override;

	// LLVisualParam Virtual functions

	// Apply is called separately for each driven param:
	LL_INLINE void apply(ESex) override					{}
	void setWeight(F32 weight, bool upload_bake) override;
	void setAnimationTarget(F32 target_value, bool upload_bake) override;
	void stopAnimating(bool upload_bake) override;
	bool linkDrivenParams(visual_param_mapper mapper,
						  bool only_cross_params) override;
	void resetDrivenParams() override;

	// LLViewerVisualParam Virtual functions

	S32 getDrivenParamsCount() const;
	const LLViewerVisualParam* getDrivenParam(S32 index) const;

	typedef std::vector<LLDrivenEntry> entry_list_t;
	LL_INLINE entry_list_t& getDrivenList()					{ return mDriven; }
	LL_INLINE void  setDrivenList(entry_list_t& list)		{ mDriven = list; }

protected:
	LLDriverParam(const LLDriverParam& other);

	F32 getDrivenWeight(const LLDrivenEntry* entryp, F32 input_weight);
	void setDrivenWeight(LLDrivenEntry* entryp, F32 driven_weight,
						 bool upload_bake);

protected:
	LLVector4a				mDefaultVec;	// Temp holder

	entry_list_t			mDriven;

	LLViewerVisualParam*	mCurrentDistortionParam;
	LLWearable*				mWearablep;

	// Backlink only; do not make this an LLPointer.
	LLAvatarAppearance*		mAvatarAppearance;
};
