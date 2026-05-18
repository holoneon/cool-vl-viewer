/**
 * @file llvisualparam.h
 * @brief Implementation of LLPolyMesh class.
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

#include <functional>

#include "llmemory.h"
#include "llstring.h"
#include "llxmltree.h"
#include "llvector3.h"

class LLDriverParam;
class LLPolyMesh;
class LLPolyMorphTarget;
class LLPolySkeletalDistortion;
class LLPolySkeletalDistortionInfo;
class LLViewerVisualParam;
class LLXmlTreeNode;

enum ESex
{
	SEX_FEMALE =	0x01,
	SEX_MALE =		0x02,
	SEX_BOTH =		0x03  // values chosen to allow use as a bit field.
};

enum EVisualParamGroup
{
	VISUAL_PARAM_GROUP_TWEAKABLE,
	VISUAL_PARAM_GROUP_ANIMATABLE,
	VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT,
	NUM_VISUAL_PARAM_GROUPS
};

enum EParamLocation
{
	LOC_UNKNOWN,
	LOC_AV_SELF,
	LOC_AV_OTHER,
	LOC_WEARABLE
};

constexpr S32 MAX_TRANSMITTED_VISUAL_PARAMS = 255;

//-----------------------------------------------------------------------------
// LLVisualParamInfo
// Contains shared data for VisualParams
//-----------------------------------------------------------------------------
class LLVisualParamInfo
{
	friend class LLVisualParam;

protected:
	LOG_CLASS(LLVisualParamInfo);

public:
	LLVisualParamInfo();
	virtual ~LLVisualParamInfo() = default;

	LL_INLINE virtual LLPolySkeletalDistortionInfo* asPolySkeletalDistortionInfo()
	{
		return NULL;
	}

	virtual bool parseXml(LLXmlTreeNode* node);

	LL_INLINE S32 getID() const									{ return mID; }

protected:
	std::string			mName;			// name (for internal purposes)
	std::string			mDisplayName;	// name displayed to the user
	std::string			mMinName;		// name associated with minimum value
	std::string			mMaxName;		// name associated with maximum value
	S32					mID;			// ID associated with VisualParam
	EVisualParamGroup	mGroup;			// morph group for use in the pie menu
	F32					mMinWeight;		// minimum weight for this morph target
	F32					mMaxWeight;		// maximum weight for this morph target
	F32					mDefaultWeight;
	ESex				mSex;			// Which gender(s) this param applies to.
};

//-----------------------------------------------------------------------------
// LLVisualParam
// VIRTUAL CLASS
// An interface class for a generalized parametric modification of the avatar
// mesh. Contains data that is specific to each Avatar.
//
// IMPORTANT NOTE: several derived classes use 16-bytes aligned structures so
// that the latter can be used with SSE2 maths. new() and delete() are
// therefore redefined here (so that all derived classes will inherit and use
// those as well, ensuring consistency for all constructors and destructors),
// and all derived classes shall therefore be 16-bytes aligned (do use the
// alignas(16) for all of them !). HB
//-----------------------------------------------------------------------------
class alignas(16) LLVisualParam
{
protected:
	LOG_CLASS(LLVisualParam);

	LLVisualParam(const LLVisualParam& other);

public:
	LL_ALIGNED16_NEW_DELETE

	typedef	std::function<LLVisualParam*(S32)> visual_param_mapper;

	LLVisualParam();
	virtual ~LLVisualParam();

	LL_INLINE virtual LLDriverParam* asDriverParam()		{ return NULL; }

	LL_INLINE virtual LLPolyMorphTarget* asPolyMorphTarget()
	{
		return NULL;
	}

	LL_INLINE virtual LLViewerVisualParam* asViewerVisualParam()
	{
		return NULL;
	}

	LL_INLINE virtual LLPolySkeletalDistortion* asPolySkeletalDistortion()
	{
		return NULL;
	}

	// Special: These functions are overridden by child classes
	// (They can not be virtual because they use specific derived Info classes)
	LL_INLINE LLVisualParamInfo* getInfo() const			{ return mInfo; }
	// This sets mInfo and calls initialization functions
	bool setInfo(LLVisualParamInfo* info);

	// Virtual functions

	// Pure virtual
	virtual void apply(ESex avatar_sex) = 0;

	// Default functions
	virtual void setWeight(F32 weight, bool upload_bake);
	virtual void setAnimationTarget(F32 target_value, bool upload_bake);
	virtual void animate(F32 delta, bool upload_bake);
	virtual void stopAnimating(bool upload_bake);

	LL_INLINE virtual bool linkDrivenParams(visual_param_mapper mapper,
											bool only_cross_params)
	{
		return true;
	}

	LL_INLINE virtual void resetDrivenParams()				{}

	// Interface methods

	LL_INLINE S32 getID() const								{ return mID; }
	LL_INLINE void setID(S32 id) 							{ llassert(!mInfo); mID = id; }

	LL_INLINE const std::string& getName() const 			{ return mInfo->mName; }
	LL_INLINE const std::string& getDisplayName() const 	{ return mInfo->mDisplayName; }
	LL_INLINE const std::string& getMaxDisplayName() const	{ return mInfo->mMaxName; }
	LL_INLINE const std::string& getMinDisplayName() const	{ return mInfo->mMinName; }

	LL_INLINE void setDisplayName(const std::string& s)		{ mInfo->mDisplayName = s; }
	LL_INLINE void setMaxDisplayName(const std::string& s)	{ mInfo->mMaxName = s; }
	LL_INLINE void setMinDisplayName(const std::string& s)	{ mInfo->mMinName = s; }

	LL_INLINE EVisualParamGroup getGroup() const			{ return mInfo->mGroup; }
	LL_INLINE F32 getMinWeight() const						{ return mInfo->mMinWeight; }
	LL_INLINE F32 getMaxWeight() const						{ return mInfo->mMaxWeight; }
	LL_INLINE F32 getDefaultWeight() const					{ return mInfo->mDefaultWeight; }
	LL_INLINE ESex getSex() const							{ return mInfo->mSex; }

	LL_INLINE F32 getWeight() const							{ return mIsAnimating ? mTargetWeight : mCurWeight; }
	LL_INLINE F32 getCurrentWeight() const					{ return mCurWeight; }
	LL_INLINE F32 getLastWeight() const						{ return mLastWeight; }
	LL_INLINE void setLastWeight(F32 val)					{ mLastWeight = val; }
	LL_INLINE bool isAnimating() const						{ return mIsAnimating; }

	LL_INLINE bool isTweakable() const
	{
		return mInfo->mGroup == VISUAL_PARAM_GROUP_TWEAKABLE ||
			   mInfo->mGroup == VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT;
	}

	LL_INLINE LLVisualParam* getNextParam()					{ return mNext; }
	void setNextParam(LLVisualParam* next);
	LL_INLINE void clearNextParam()							{ mNext = NULL; }

	LL_INLINE virtual void setAnimating(bool b)				{ mIsAnimating = b && !mIsDummy; }
	LL_INLINE bool getAnimating() const						{ return mIsAnimating; }

	LL_INLINE void setIsDummy(bool is_dummy)				{ mIsDummy = is_dummy; }

	void setParamLocation(EParamLocation loc);
	LL_INLINE EParamLocation getParamLocation() const		{ return mParamLocation; }

protected:
	LLVisualParam*		mNext;			// Next param in a shared chain
	LLVisualParamInfo*	mInfo;

	F32					mCurWeight;		// Current weight
	F32					mLastWeight;	// Last weight
	F32					mTargetWeight;	// Interpolation target

	// Id for storing weight/morphtarget compares compactly
	S32					mID;

	// Where does this visual param live ?
	EParamLocation		mParamLocation;

	// This value has been given an interpolation target
	bool				mIsAnimating;
	// used to prevent dummy visual params from animating
	bool				mIsDummy;
};
