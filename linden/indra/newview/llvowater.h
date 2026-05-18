/** 
 * @file llvowater.h
 * @brief Description of LLVOWater class
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

#include "llvector2.h"

#include "llpipeline.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewertexture.h"

constexpr U32 N_RES	= 16; // Number of subdivisions of wave tile
constexpr U8 WAVE_STEP = 8;

class LLSurface;
class LLHeavenBody;
class LLVOSky;
class LLFace;

class LLVOWater : public LLStaticViewerObject
{
public:
	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0) 
	};

	LLVOWater(const LLUUID& id, LLViewerRegion* regionp,
			  // This is LL_VO_VOID_WATER for LLVOVoidWater.
			  LLPCode pcode = LL_VO_WATER);

	// Initialize data that's only inited once per class.
	static void initClass()								{}
	static void cleanupClass()							{}

	// Nothing to do.
	LL_INLINE void idleUpdate(F64) override				{}

	LLDrawable* createDrawable() override;
	bool updateGeometry(LLDrawable* drawable) override;
	void updateSpatialExtents(LLVector4a& new_min,
							  LLVector4a& new_max) override;

	LL_INLINE void updateTextures() override			{}

	// Generates accurate apparent angle and area
	void setPixelAreaAndAngle() override;

	LL_INLINE U32 getPartitionType() const override
	{
		return LLViewerRegion::PARTITION_WATER;
	}

	 // Whether this object needs to do an idleUpdate.
	LL_INLINE bool isActive() const override			{ return false; }

	LL_INLINE void setUseTexture(bool b)				{ mUseTexture = b; }
	LL_INLINE void setIsEdgePatch(bool b)				{ mIsEdgePatch = b; }
	LL_INLINE bool getUseTexture() const				{ return mUseTexture; }
	LL_INLINE bool getIsEdgePatch() const				{ return mIsEdgePatch; }

protected:
	bool mUseTexture;
	bool mIsEdgePatch;
	S32 mRenderType;
};

class LLVOVoidWater final : public LLVOWater
{
public:
	LL_INLINE LLVOVoidWater(const LLUUID& id, LLViewerRegion* regionp)
	:	LLVOWater(id, regionp, LL_VO_VOID_WATER)
	{
		mRenderType = LLPipeline::RENDER_TYPE_VOIDWATER;
	}

	LL_INLINE U32 getPartitionType() const override
	{
		return LLViewerRegion::PARTITION_VOIDWATER;
	}
};
