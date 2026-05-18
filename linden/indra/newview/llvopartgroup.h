/**
 * @file llvopartgroup.h
 * @brief Group of particle systems
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

#include "llframetimer.h"
#include "llvertexbuffer.h"
#include "llcolor3.h"
#include "llvector3.h"

#include "llviewerobject.h"
#include "llviewerpartsim.h"
#include "llviewerregion.h"

class LLViewerPartGroup;

class LLVOPartGroup : public LLAlphaObject
{
public:
	LLVOPartGroup(const LLUUID& id, LLViewerRegion* regionp,
				  // This is LL_VO_HUD_PART_GROUP for LLVOHUDPartGroup
				  LLPCode pcode = LL_VO_PART_GROUP);

	LL_INLINE LLVOPartGroup* asVOPartGroup() override	{ return this; }

	LL_INLINE bool isActive() const override			{ return false; }
	// Nothing to do.
	LL_INLINE void idleUpdate(F64) override				{}

	F32 getBinRadius() override;
	void updateSpatialExtents(LLVector4a& min, LLVector4a& max) override;

	LL_INLINE U32 getPartitionType() const override
	{
		return LLViewerRegion::PARTITION_PARTICLE;
	}

	bool lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
							  S32 face, bool pick_transparent = false,
							  bool pick_rigged = false,
							  S32* face_hit = NULL,
							  LLVector4a* intersection = NULL,
							  // Unused pointers:
							  LLVector2* tex_coord = NULL,
							  LLVector4a* normal = NULL,
							  LLVector4a* tangent = NULL) override;

	void setPixelAreaAndAngle() override;

	LL_INLINE void updateTextures() override			{}
	LL_INLINE void updateFaceSize(S32) override			{}

	LLDrawable* createDrawable() override;
	bool updateGeometry(LLDrawable* drawable) override;
	void getGeometry(const LLViewerPart& part,
					 LLStrider<LLVector4a>& verticesp);
	void getGeometry(S32 idx,
					 LLStrider<LLVector4a>& verticesp,
					 LLStrider<LLVector3>& normalsp,
					 LLStrider<LLVector2>& texcoordsp,
					 LLStrider<LLColor4U>& colorsp,
					 LLStrider<LLColor4U>& emissivep,
					 LLStrider<U16>& indicesp) override;

	F32 getPartSize(S32 idx) override;
	bool getBlendFunc(S32 idx, U32& src, U32& dst) override;
	const LLUUID& getPartOwner(S32 idx);
	const LLUUID& getPartSource(S32 idx);

	LL_INLINE void setViewerPartGroup(LLViewerPartGroup* group)
	{
		mViewerPartGroupp = group;
	}

	LL_INLINE LLViewerPartGroup* getViewerPartGroup()	{ return mViewerPartGroupp; }

protected:
	~LLVOPartGroup() override = default;
	virtual const LLVector3& getCameraPosition() const;

protected:
	LLViewerPartGroup* mViewerPartGroupp;
};

class LLVOHUDPartGroup final : public LLVOPartGroup
{
public:
	LL_INLINE LLVOHUDPartGroup(const LLUUID& id, LLViewerRegion* regionp)
	:	LLVOPartGroup(id, regionp, LL_VO_HUD_PART_GROUP)
	{
	}

protected:
	LLDrawable* createDrawable() override;

	LL_INLINE U32 getPartitionType() const override
	{
		return LLViewerRegion::PARTITION_HUD_PARTICLE;
	}

	LL_INLINE const LLVector3& getCameraPosition() const override
	{
		return LLVector3::x_axis_neg;
	}
};
