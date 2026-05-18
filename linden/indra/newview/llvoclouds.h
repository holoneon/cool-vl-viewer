/**
 * @file llvoclouds.h
 * @brief Description of LLVOClouds class
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

#include "llcolor4u.h"

#include "llviewerobject.h"

class LLCloudGroup;
class LLViewerCloudGroup;
class LLViewerTexture;

class LLVOClouds final : public LLAlphaObject
{
public:
	LLVOClouds(const LLUUID& id, LLViewerRegion* regionp);

	// Initialize data that's only inited once per class.
	static void initClass();

	void updateDrawable(bool force_damped) override;

	LLDrawable* createDrawable() override;

	bool updateGeometry(LLDrawable* drawable) override;

	void getGeometry(S32 idx, LLStrider<LLVector4a>& verticesp,
					 LLStrider<LLVector3>& normalsp,
					 LLStrider<LLVector2>& texcoordsp,
					 LLStrider<LLColor4U>& colorsp,
					 LLStrider<LLColor4U>& emissivep,
					 LLStrider<U16>& indicesp) override;

	// Whether this object needs to do an idleUpdate:
	LL_INLINE bool isActive() const override			{ return true; }

	LL_INLINE void updateFaceSize(S32 idx) override 	{}

	F32 getPartSize(S32 idx) override;

	void updateTextures() override;

	// Generates accurate apparent angle and area:
	void setPixelAreaAndAngle() override;

	void idleUpdate(F64 time) override;

	U32 getPartitionType() const override;

	LL_INLINE void setCloudGroup(LLCloudGroup* cgp)		{ mCloudGroupp = cgp; }

protected:
	~LLVOClouds() override = default;

protected:
	LLCloudGroup*	mCloudGroupp;
	LLColor3		mCloudsColor;
};

extern LLUUID gCloudTextureID;
