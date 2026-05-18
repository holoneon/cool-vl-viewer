/**
 * @file llviewervisualparam.h
 * @brief viewer side visual params (with data file parsing)
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

#include "llvisualparam.h"

class LLWearable;

//-----------------------------------------------------------------------------
// LLViewerVisualParamInfo
//-----------------------------------------------------------------------------
class LLViewerVisualParamInfo : public LLVisualParamInfo
{
	friend class LLViewerVisualParam;

public:
	LLViewerVisualParamInfo();

	bool parseXml(LLXmlTreeNode* node) override;

protected:
	std::string	mEditGroup;
	S32			mWearableType;
	F32			mCamDist;
	F32			mCamAngle;		// degrees
	F32			mCamElevation;
	F32			mEditGroupDisplayOrder;
	// When in simple UI, apply this minimum, range 0.f to 100.f
	F32			mSimpleMin;
	// When in simple UI, apply this maximum, range 0.f to 100.f
	F32			mSimpleMax;
	bool		mCrossWearable;
};

//-----------------------------------------------------------------------------
// LLViewerVisualParam - Virtual class
// A viewer side interface class for a generalized parametric modification of
// the avatar mesh
//-----------------------------------------------------------------------------
class alignas(16) LLViewerVisualParam : public LLVisualParam
{
protected:
	LLViewerVisualParam(const LLViewerVisualParam& other);

public:
	LLViewerVisualParam();

	LL_INLINE LLViewerVisualParam* asViewerVisualParam() override
	{
		return this;
	}

	// Special: These functions are overridden by child classes

	LL_INLINE LLViewerVisualParamInfo* getInfo() const
	{
		return (LLViewerVisualParamInfo*)mInfo;
	}

	// This sets mInfo and calls initialization functions
	bool setInfo(LLViewerVisualParamInfo* info);

	virtual LLViewerVisualParam* cloneParam(LLWearable* wearable) const = 0;

#if 0	// Unused methods
	// New Virtual functions
	virtual F32 getTotalDistortion() = 0;
	virtual const LLVector4a& getAvgDistortion() = 0;
	virtual F32 getMaxDistortion() = 0;
	virtual LLVector4a getVertexDistortion(S32 index, LLPolyMesh* mesh) = 0;
	virtual const LLVector4a* getFirstDistortion(U32* idx, LLPolyMesh** m) = 0;
	virtual const LLVector4a* getNextDistortion(U32* idx, LLPolyMesh** m) = 0;
#endif

	// Interface methods
	LL_INLINE F32 getDisplayOrder() const		{ return getInfo()->mEditGroupDisplayOrder; }
	LL_INLINE S32 getWearableType() const		{ return getInfo()->mWearableType; }

	LL_INLINE const std::string& getEditGroup() const
	{
		return getInfo()->mEditGroup;
	}

	LL_INLINE F32 getCameraDistance()	const	{ return getInfo()->mCamDist; }
	LL_INLINE F32 getCameraAngle() const		{ return getInfo()->mCamAngle; }
	LL_INLINE F32 getCameraElevation() const	{ return getInfo()->mCamElevation; }

	LL_INLINE F32 getSimpleMin() const			{ return getInfo()->mSimpleMin; }
	LL_INLINE F32 getSimpleMax() const			{ return getInfo()->mSimpleMax; }

	LL_INLINE bool getCrossWearable() const		{ return getInfo()->mCrossWearable; }
};
