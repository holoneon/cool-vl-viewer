/**
 * @file llvlmanager.h
 * @brief LLVLManager class definition
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

// This class manages the data coming in for viewer layers from the network.

#include <vector>

#include "llerror.h"

const char CLOUD_LAYER_CODE			= '8';
const char WIND_LAYER_CODE			= '7';
const char LAND_LAYER_CODE			= 'L';
const char WATER_LAYER_CODE			= 'W';

const char AURORA_CLOUD_LAYER_CODE	= ':';
const char AURORA_WIND_LAYER_CODE	= '9';
const char AURORA_LAND_LAYER_CODE	= 'M';
const char AURORA_WATER_LAYER_CODE	= 'X';

class LLVLData;
class LLViewerRegion;

class LLVLManager
{
protected:
	LOG_CLASS(LLVLManager);

public:
	LLVLManager();
	~LLVLManager();

	void addLayerData(LLVLData* vl_datap, S32 mesg_size);

	void unpackData(S32 num_packets = 10);

	LL_INLINE S32 getLandBits() const		{ return mLandBits; }
	LL_INLINE S32 getWindBits() const		{ return mWindBits; }
	LL_INLINE S32 getCloudBits() const		{ return mCloudBits; }

	LL_INLINE S32 getTotalBytes() const
	{
		return (mLandBits + mWindBits + mCloudBits) / 8;
	}

	LL_INLINE void resetBitCounts()
	{
		mLandBits = mWindBits = mCloudBits = 0;
	}

	void cleanupData(LLViewerRegion* regionp);
protected:

	std::vector<LLVLData*>	mPacketData;
	U32						mLandBits;
	U32						mWindBits;
	U32						mCloudBits;
};

class LLVLData
{
public:
	LL_INLINE LLVLData(LLViewerRegion* regionp, S8 type, U8* data, S32 size)
	:	mRegionp(regionp),
		mType(type),
		mData(data),
		mSize(size)
	{
	}

	LL_INLINE ~LLVLData()
	{
		delete[] mData;
		mData = NULL;
		mRegionp = NULL;
	}

public:
	LLViewerRegion*	mRegionp;
	U8*				mData;
	S32				mSize;
	S8				mType;
};

extern LLVLManager gVLManager;
