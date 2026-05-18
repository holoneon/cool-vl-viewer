/**
 * @file llcubemaparray.cpp
 * @brief LLCubeMapArray class implementation
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

#include "linden_common.h"

#include "llcubemaparray.h"

#include "llrender.h"
#include "llvector3.h"

// Defined in llimagegl.cpp
extern void image_bound(U32 width, U32 height, U32 pixformat, U32 count = 1,
						U32 texname = 0);
extern void image_unbound(U32 tex_name);

// MUST match order of OpenGL face-layers
GLenum LLCubeMapArray::sTargets[6] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

LLVector3 LLCubeMapArray::sLookVecs[6] =
{
	LLVector3(1.f, 0.f, 0.f),
	LLVector3(-1.f, 0.f, 0.f),
	LLVector3(0.f, 1.f, 0.f),
	LLVector3(0.f, -1.f, 0.f),
	LLVector3(0.f, 0.f, 1.f),
	LLVector3(0.f, 0.f, -1.f)
};

LLVector3 LLCubeMapArray::sUpVecs[6] =
{
	LLVector3(0.f, -1.f, 0.f),
	LLVector3(0.f, -1.f, 0.f),
	LLVector3(0.f, 0.f, 1.f),
	LLVector3(0.f, 0.f, -1.f),
	LLVector3(0.f, -1.f, 0.f),
	LLVector3(0.f, -1.f, 0.f)
};

LLVector3 LLCubeMapArray::sClipToCubeLookVecs[6] =
{
	LLVector3(0.f, 0.f, -1.f),
	LLVector3(0.f, 0.f, 1.f),
	LLVector3(1.f, 0.f, 0.f),
	LLVector3(1.f, 0.f, 0.f),
	LLVector3(1.f, 0.f, 0.f),
	LLVector3(-1.f, 0.f, 0.f)
};

LLVector3 LLCubeMapArray::sClipToCubeUpVecs[6] =
{
	LLVector3(-1.f, 0.f, 0.f),
	LLVector3(1.f, 0.f, 0.f),
	LLVector3(0.f, 1.f, 0.f),
	LLVector3(0.f, -1.f, 0.f),
	LLVector3(0.f, 0.f, -1.f),
	LLVector3(0.f, 0.f, 1.f)
};

LLCubeMapArray::LLCubeMapArray(LLCubeMapArray& lhs, U32 resolution, U32 count)
:	mTextureStage(-1),
	mResolution(resolution),
	mCount(count)
{
	// Allocate a new cubemap array with the same criteria as the incoming
	// cubemap array.
	U32 components = lhs.mImage->getComponents();
	allocate(resolution, components, count, lhs.mImage->getUseMipMaps(),
			 lhs.mHDR);
	U32 format = components == 4 ? GL_RGBA : GL_RGB;
	U32 min_count = llmin(count, lhs.mCount);
	for (U32 i = 0; i < min_count; ++i)
	{
		LLPointer<LLImageRaw> srcp = new LLImageRaw(lhs.mResolution,
													lhs.mResolution,
													components);
		glGetTexImage(GL_TEXTURE_CUBE_MAP_ARRAY, 0, format, GL_UNSIGNED_BYTE,
					  srcp->getData());
		LLPointer<LLImageRaw> scaledp = srcp->scaled(resolution, resolution);
		glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, i, resolution,
						resolution, 1, format, GL_UNSIGNED_BYTE,
						scaledp->getData());
	}
}

void LLCubeMapArray::allocate(U32 resolution, U32 components, U32 count,
							  bool use_mips, bool hdr)
{
	mResolution = resolution;
	mCount = count;
	mHDR = hdr;

	LLImageGL::generateTextures(1, &mTexName);

	mImage = new LLImageGL(resolution, resolution, components, use_mips);
	mImage->setTexName(mTexName);
	mImage->setTarget(sTargets[0], LLTexUnit::TT_CUBE_MAP_ARRAY);

	mImage->setUseMipMaps(use_mips);
	mImage->setHasMipMaps(use_mips);

	bind(0);

	U32 format = components == 4 ? (hdr ? GL_RGBA16F : GL_RGBA8)
								 : (hdr ? GL_R11F_G11F_B10F : GL_RGB8);

	// Account for the actual cube map array size.
	image_unbound(mTexName);
	image_bound(resolution, resolution, format, 6 * count, mTexName);

	U32 mip = 0;
	while (resolution >= 1)
	{
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, format, resolution,
					 resolution, count * 6, 0, GL_RGBA, GL_UNSIGNED_BYTE,
					 NULL);
		if (!use_mips)
		{
			break;
		}
		resolution /= 2;
		++mip;
	}

	mImage->setAddressMode(LLTexUnit::TAM_CLAMP);

	if (use_mips)
	{
		mImage->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
#if 0	// Latest AMD drivers do not appreciate this method of allocating
		// mipmaps
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);
#endif
	}
	else
	{
		mImage->setFilteringOption(LLTexUnit::TFO_BILINEAR);
	}

	unbind();
}

void LLCubeMapArray::destroyGL()
{
	image_unbound(mTexName);
	mTexName = 0;
	mImage = NULL;
}

void LLCubeMapArray::bind(S32 stage)
{
	mTextureStage = stage;
	if (mTextureStage >= 0)
	{
		gGL.getTexUnit(stage)->bindManual(LLTexUnit::TT_CUBE_MAP_ARRAY,
										  getGLName(),
										  mImage->getUseMipMaps());
	}
}

void LLCubeMapArray::unbind()
{
	if (mTextureStage >= 0)
	{
		gGL.getTexUnit(mTextureStage)->unbind(LLTexUnit::TT_CUBE_MAP_ARRAY);
	}
	mTextureStage = -1;
}
