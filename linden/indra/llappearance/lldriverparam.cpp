/**
 * @file lldriverparam.cpp
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

#include "linden_common.h"

#include "lldriverparam.h"

#include "llavatarappearance.h"
#include "llwearabledata.h"

//-----------------------------------------------------------------------------
// LLDriverParamInfo
//-----------------------------------------------------------------------------

bool LLDriverParamInfo::parseXml(LLXmlTreeNode* nodep)
{
	llassert(nodep->hasName("param") && nodep->getChildByName("param_driver"));

	if (!LLViewerVisualParamInfo::parseXml(nodep))
	{
		return false;
	}

	LLXmlTreeNode* param_nodep = nodep->getChildByName("param_driver");
	if (!param_nodep)
	{
		return false;
	}

	static LLStdStringHandle id_str = LLXmlTree::addAttributeString("id");
	static LLStdStringHandle min1_str = LLXmlTree::addAttributeString("min1");
	static LLStdStringHandle max1_str = LLXmlTree::addAttributeString("max1");
	static LLStdStringHandle max2_str = LLXmlTree::addAttributeString("max2");
	static LLStdStringHandle min2_str = LLXmlTree::addAttributeString("min2");

	for (LLXmlTreeNode* childp = param_nodep->getChildByName("driven");
		 childp; childp = param_nodep->getNextNamedChild())
	{
		S32 driven_id;
		if (!childp->getFastAttributeS32(id_str, driven_id))
		{
			llerrs << "<driven> Unable to resolve driven parameter: "
				   << driven_id << llendl;
		}

		F32 min1 = mMinWeight;
		F32 max1 = mMaxWeight;
		F32 max2 = max1;
		F32 min2 = max1;

		//	driven    ________							//
		//	^        /|       |\						//
		//	|       / |       | \						//
		//	|      /  |       |  \						//
		//	|     /   |       |   \						//
		//	|    /    |       |    \					//
		//-------|----|-------|----|-------> driver		//
		//  | min1   max1    max2  min2

		childp->getFastAttributeF32(min1_str, min1); // Optional
		childp->getFastAttributeF32(max1_str, max1); // Optional
		childp->getFastAttributeF32(max2_str, max2); // Optional
		childp->getFastAttributeF32(min2_str, min2); // Optional

		// Push these on the front of the deque, so that we can construct them
		// in order later (faster)
		mDrivenInfoList.emplace_front(driven_id, min1, max1, max2, min2);
	}

	return true;
}

//-----------------------------------------------------------------------------
// LLDriverParam
//-----------------------------------------------------------------------------

LLDriverParam::LLDriverParam(LLAvatarAppearance* appearance,
							 LLWearable* wearable)
:	LLViewerVisualParam(),
	mDefaultVec(),
	mDriven(),
	mCurrentDistortionParam(NULL),
	mAvatarAppearance(appearance),
	mWearablep(wearable)
{
	llassert(mAvatarAppearance);
	llassert(mWearablep == NULL || mAvatarAppearance->isSelf());
	mDefaultVec.clear();
}

LLDriverParam::LLDriverParam(const LLDriverParam& other)
:	LLViewerVisualParam(other),
	mDefaultVec(other.mDefaultVec),
	mDriven(other.mDriven),
	mCurrentDistortionParam(other.mCurrentDistortionParam),
	mAvatarAppearance(other.mAvatarAppearance),
	mWearablep(other.mWearablep)
{
	llassert(mAvatarAppearance);
	llassert(mWearablep == NULL || mAvatarAppearance->isSelf());
}

bool LLDriverParam::setInfo(LLDriverParamInfo* infop)
{
	llassert(mInfo == NULL);
	if (infop->mID < 0)
	{
		return false;
	}
	mInfo = infop;
	mID = infop->mID;
	infop->mDriverParam = this;

	setWeight(getDefaultWeight(), false);

	return true;
}

//virtual
LLViewerVisualParam* LLDriverParam::cloneParam(LLWearable* wearable) const
{
	llassert(wearable);
	return new LLDriverParam(*this);
}

void LLDriverParam::setWeight(F32 weight, bool upload_bake)
{
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	if (mIsAnimating)
	{
		// allow overshoot when animating
		mCurWeight = weight;
	}
	else
	{
		mCurWeight = llclamp(weight, min_weight, max_weight);
	}

	//	driven    ________
	//	^        /|       |\       ^
	//	|       / |       | \      |
	//	|      /  |       |  \     |
	//	|     /   |       |   \    |
	//	|    /    |       |    \   |
	//-------|----|-------|----|-------> driver
	//  | min1   max1    max2  min2

	for (entry_list_t::iterator iter = mDriven.begin(), end = mDriven.end();
		 iter != end; ++iter)
	{
		LLDrivenEntry* entryp = &(*iter);
		LLDrivenEntryInfo* infop = entryp->mInfo;

		LLViewerVisualParam* paramp = entryp->mParam;

		F32 driven_weight = 0.f;
		F32 driven_min = paramp->getMinWeight();
		F32 driven_max = paramp->getMaxWeight();

		if (mIsAnimating)
		{
			// Driven param does not interpolate (textures, for example)
			if (!paramp->getAnimating())
			{
				continue;
			}
			if (mCurWeight < infop->mMin1)
			{
				if (infop->mMin1 == min_weight)
				{
					if (infop->mMin1 == infop->mMax1)
					{
						driven_weight = driven_max;
					}
					else
					{
						// Up-slope extrapolation
						F32 t = (mCurWeight - infop->mMin1) /
								(infop->mMax1 - infop->mMin1);
						driven_weight = driven_min +
										t * (driven_max - driven_min);
					}
				}
				else
				{
					driven_weight = driven_min;
				}

				setDrivenWeight(entryp, driven_weight, upload_bake);
				continue;
			}
			else if (mCurWeight > infop->mMin2)
			{
				if (infop->mMin2 == max_weight)
				{
					if (infop->mMin2 == infop->mMax2)
					{
						driven_weight = driven_max;
					}
					else
					{
						// Down-slope extrapolation
						F32 t = (mCurWeight - infop->mMax2) /
								(infop->mMin2 - infop->mMax2);
						driven_weight = driven_max +
										t * (driven_min - driven_max);
					}
				}
				else
				{
					driven_weight = driven_min;
				}

				setDrivenWeight(entryp, driven_weight, upload_bake);
				continue;
			}
		}

		driven_weight = getDrivenWeight(entryp, mCurWeight);
		setDrivenWeight(entryp, driven_weight, upload_bake);
	}
}

S32 LLDriverParam::getDrivenParamsCount() const
{
	return mDriven.size();
}

const LLViewerVisualParam* LLDriverParam::getDrivenParam(S32 index) const
{
	if (0 > index || index >= (S32)mDriven.size())
	{
		return NULL;
	}
	return mDriven[index].mParam;
}

void LLDriverParam::setAnimationTarget(F32 target_value, bool upload_bake)
{
	LLVisualParam::setAnimationTarget(target_value, upload_bake);

	for (entry_list_t::iterator iter = mDriven.begin(), end = mDriven.end();
		 iter != end; ++iter)
	{
		LLDrivenEntry* entryp = &(*iter);
		F32 driven_weight = getDrivenWeight(entryp, mTargetWeight);

		// this isn't normally necessary, as driver params handle interpolation
		// of their driven params but texture params need to know to assume
		// their final value at beginning of interpolation
		entryp->mParam->setAnimationTarget(driven_weight, upload_bake);
	}
}

void LLDriverParam::stopAnimating(bool upload_bake)
{
	LLVisualParam::stopAnimating(upload_bake);

	for (entry_list_t::iterator iter = mDriven.begin(), end = mDriven.end();
		 iter != end; ++iter)
	{
		LLDrivenEntry* entryp = &(*iter);
		entryp->mParam->setAnimating(false);
	}
}

//virtual
bool LLDriverParam::linkDrivenParams(visual_param_mapper mapper,
									 bool only_cross_params)
{
	bool success = true;
	for (LLDriverParamInfo::entry_info_list_t::iterator
			iter = getInfo()->mDrivenInfoList.begin(),
			end = getInfo()->mDrivenInfoList.end();
		 iter != end; ++iter)
	{
		LLDrivenEntryInfo* driven_infop = &(*iter);
		S32 driven_id = driven_infop->mDrivenID;

		// Check for already existing links. Do not overwrite.
		bool found = false;
		for (entry_list_t::iterator driven_iter = mDriven.begin(),
									driven_end = mDriven.end();
			 driven_iter != driven_end; ++driven_iter)
		{
			if (driven_iter->mInfo->mDrivenID == driven_id)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			LLViewerVisualParam* paramp =
				(LLViewerVisualParam*)mapper(driven_id);
			if (paramp)
			{
				paramp->setParamLocation(this->getParamLocation());
			}
			if (paramp && (!only_cross_params || paramp->getCrossWearable()))
			{
				mDriven.emplace_back(paramp, driven_infop);
			}
			else
			{
				success = false;
			}
		}
	}

	return success;
}

void LLDriverParam::resetDrivenParams()
{
	mDriven.clear();
	mDriven.reserve(getInfo()->mDrivenInfoList.size());
}

void LLDriverParam::updateCrossDrivenParams(LLWearableType::EType driven_type)
{
	bool needs_update = getWearableType() == driven_type;

	// If the driver has a driven entry for the passed-in wearable type, we
	// need to refresh the value
	for (entry_list_t::iterator iter = mDriven.begin(), end = mDriven.end();
		 iter != end; ++iter)
	{
		LLDrivenEntry* drivenp = &(*iter);
		LLViewerVisualParam* paramp = drivenp ? drivenp->mParam : NULL;
		if (paramp && paramp->getCrossWearable() &&
			paramp->getWearableType() == driven_type)
		{
			needs_update = true;
		}
	}

	if (!needs_update)
	{
		return;
	}

	// If we have gotten here, we have added a new wearable of type
	// 'driven_type'. Thus this wearable needs to get updates from the driver
	// wearable. The call to setVisualParamWeight seems redundant, but is
	// necessary as the number of driven wearables has changed since the last
	// update.
	LLWearableType::EType t = (LLWearableType::EType)getWearableType();
	LLWearable* wp = mAvatarAppearance->getWearableData()->getTopWearable(t);
	if (wp)
	{
		wp->setVisualParamWeight(mID, wp->getVisualParamWeight(mID), false);
	}
}

F32 LLDriverParam::getDrivenWeight(const LLDrivenEntry* entryp,
								   F32 input_weight)
{
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	const LLDrivenEntryInfo* infop = entryp->mInfo;

	F32 driven_weight = 0.f;
	F32 driven_min = entryp->mParam->getMinWeight();
	F32 driven_max = entryp->mParam->getMaxWeight();

	if (input_weight <= infop->mMin1)
	{
		if (infop->mMin1 == infop->mMax1 && infop->mMin1 <= min_weight)
		{
			driven_weight = driven_max;
		}
		else
		{
			driven_weight = driven_min;
		}
	}
	else if (input_weight <= infop->mMax1)
	{
		F32 t = (input_weight - infop->mMin1) / (infop->mMax1 - infop->mMin1);
		driven_weight = driven_min + t * (driven_max - driven_min);
	}
	else if (input_weight <= infop->mMax2)
	{
		driven_weight = driven_max;
	}
	else if (input_weight <= infop->mMin2)
	{
		F32 t = (input_weight - infop->mMax2) / (infop->mMin2 - infop->mMax2);
		driven_weight = driven_max + t * (driven_min - driven_max);
	}
	else if (infop->mMax2 >= max_weight)
	{
		driven_weight = driven_max;
	}
	else
	{
		driven_weight = driven_min;
	}

	return driven_weight;
}

void LLDriverParam::setDrivenWeight(LLDrivenEntry* entryp, F32 driven_weight,
									bool upload_bake)
{
	if (mWearablep && mAvatarAppearance->isValid() &&
		entryp->mParam->getCrossWearable() &&
		mAvatarAppearance->getWearableData()->isOnTop(mWearablep))
	{
		// Call setWeight through LLVOAvatarSelf so other wearables can be
		// updated with the correct values
		mAvatarAppearance->setVisualParamWeight((LLVisualParam*)entryp->mParam,
												driven_weight, upload_bake);
	}
	else
	{
		entryp->mParam->setWeight(driven_weight, upload_bake);
	}
}
