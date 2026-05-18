/**
 * @file llcubemap.h
 * @brief LLCubeMap class definition
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

#include <vector>

#include "llgl.h"
#include "llimagegl.h"

class LLVector3;

// Environment map hack !
class LLCubeMap : public LLRefCount
{
	friend class LLTexUnit;

protected:
	LOG_CLASS(LLCubeMap);

	~LLCubeMap() override = default;

public:
	LLCubeMap(bool init_as_srgb = false);

	void init(const std::vector<LLPointer<LLImageRaw> >& rawimages);

	void initGL();
	void destroyGL();

	void initRawData(const std::vector<LLPointer<LLImageRaw> >& rawimages);
	void initGLData();

	void bind();
	void enableTexture(S32 stage);
	void disableTexture();

	void setMatrix(S32 stage);
	void restoreMatrix();

	LL_INLINE U32 getGLName() const		{ return mImages[0]->getTexName(); }

	// The methods below are used by the PBR renderer only.

	LL_INLINE U32 getResolution() const
	{
		return mImages[0].notNull() ? mImages[0]->getWidth(0) : 0;
	}

	// Initializes as an undefined cubemap at the given resolution used for
	// render-to-cubemap operations. Avoids usage of LLImageRaw.
	void initReflectionMap(U32 resolution, U32 components = 3);

	// Initializes from environment map images. Similar to init(), but takes
	// ownership of rawimages and makes this cubemap respect the resolution of
	// rawimages. Raw images must point to array of six square images that are
	// all the same resolution.
	void initEnvironmentMap(const std::vector<LLPointer<LLImageRaw> >& images);

	// Generates mip maps for this Cube Map using GL. NOTE: the cube map MUST
	// already be resident in VRAM.
	void generateMipMaps();

protected:
	// Note: the first member variable is 32 bits in order to align on 64 bits
	// for the next variables, counting the 32 bits counter from LLRefCount. HB

	S32						mTextureStage;

	U32						mTargets[6];

	LLPointer<LLImageGL>	mImages[6];
	LLPointer<LLImageRaw>	mRawImages[6];

	S32						mMatrixStage;

	bool					mIsSRGB;
};
