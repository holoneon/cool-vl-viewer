/**
 * @file llwind.h
 * @brief LLWind class header file
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

#include "llbitpack.h"
#include "llmath.h"
#include "llvector3.h"
#include "llvector3d.h"

// Hack to make wind speeds more realistic
constexpr F32 WIND_SCALE_HACK = 2.f;

class LLVector3;
class LLBitPack;
class LLGroupHeader;

class LLWind
{
public:
	LLWind();
	~LLWind();

	void renderVectors();

	// For all three methods below, "location" is region-local
	LLVector3 getVelocity(const LLVector3& location);
	LLVector3 getCloudVelocity(const LLVector3& location);
	LLVector3 getVelocityNoisy(const LLVector3& location, F32 dim);

	void decompress(LLBitPack& bitpack, LLGroupHeader* group_headerp);
	LLVector3 getAverage();

	LL_INLINE void setCloudDensityPointer(F32* d)		{ mCloudDensityp = d; }

	LL_INLINE void setOriginGlobal(const LLVector3d& p)	{ mOriginGlobal = p; }
	// Variable region size support
	LL_INLINE void setRegionWidth(F32 width)			{ mRegionWidth = width; }

private:
	void init();

private:
	S32			mSize;
	F32			mRegionWidth;	// Variable region size support
	F32*		mVelX;
	F32*		mVelY;
	F32*		mCloudVelX;
	F32*		mCloudVelY;
	F32*		mCloudDensityp;
	LLVector3d	mOriginGlobal;
};
