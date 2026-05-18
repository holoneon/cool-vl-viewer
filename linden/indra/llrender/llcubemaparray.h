/**
 * @file llcubemaparray.h
 * @brief LLCubeMapArray class definition
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 *
 * Copyright (c) 2022, Linden Research, Inc.
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

#include "llgl.h"
#include "llimagegl.h"

class LLVector3;

class LLCubeMapArray : public LLRefCount
{
	friend class LLTexUnit;

protected:
	LL_INLINE ~LLCubeMapArray()
	{
		destroyGL();
	}

public:
	LL_INLINE LLCubeMapArray()
	:	mTextureStage(0),
		mTexName(0),
		mResolution(0),
		mCount(0),
		mHDR(false)
	{
	}

	LLCubeMapArray(LLCubeMapArray& lhs, U32 resolution, U32 count);

	// Allocates a cube map array
	// res - resolution of each cube face
	// components - number of components per pixel
	// count - number of cube maps in the array
	// use_mips - if true, mipmaps will be allocated for this cube map array
	// and anisotropic filtering will be used.
	// hdr - true to use high precision image formats
	void allocate(U32 res, U32 components, U32 count, bool use_mips, bool hdr);

	void bind(S32 stage);
	void unbind();

	LL_INLINE U32 getGLName() const			{ return mImage->getTexName(); }

	void destroyGL();

	// Returns the resolution of the cubemaps in the array.
	LL_INLINE U32 getResolution() const		{ return mResolution; }
	// Returns the number of cubemaps in the array
	LL_INLINE U32 getCount() const			{ return mCount; }

protected:
	// Note: the first member variable is 32 bits in order to align on 64 bits
	// for the next variables, counting the 32 bits counter from LLRefCount. HB
	S32						mTextureStage;

	LLPointer<LLImageGL>	mImage;
	U32						mTexName;		// For GL image alloc tracking.

	U32						mResolution;
	U32						mCount;

	bool					mHDR;

public:
	static GLenum			sTargets[6];

	// Look and up vectors for each cube face (agent space)
	static LLVector3		sLookVecs[6];
	static LLVector3		sUpVecs[6];

	// Look and up vectors for each cube face (clip space)
	static LLVector3		sClipToCubeLookVecs[6];
	static LLVector3		sClipToCubeUpVecs[6];
};
