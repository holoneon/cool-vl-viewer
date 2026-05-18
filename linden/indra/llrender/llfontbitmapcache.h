/**
 * @file llfontbitmapcache.h
 * @brief Storage for previously rendered glyphs.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 *
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

#include "llimagegl.h"

enum EFontGlyphType : U32
{
	Grayscale = 0,
	Color,
	Count,
	Unspecified,
};

// Maintain a collection of bitmaps containing rendered glyphs.
// Generalizes the single-bitmap logic from LLFontFreetype and LLFontGL.
class LLFontBitmapCache : public LLRefCount
{
public:
	LLFontBitmapCache();

	// This must be called once, before caching any glyphs.
 	void init(S32 max_char_width, S32 max_char_height);

	void reset();

	void nextOpenPos(S32 width, S32& pos_x, S32& pos_y, U32 bitmap_type,
					 U32& bitmap_num);

	void destroyGL();

 	LLImageRaw* getImageRaw(U32 bitmap_type, U32 bitmap_num) const;
 	LLImageGL* getImageGL(U32 bitmap_type, U32 bitmap_num) const;  

	LL_INLINE S32 getMaxCharWidth() const			{ return mMaxCharWidth; }
	LL_INLINE S32 getBitmapWidth() const			{ return mBitmapWidth; }
	LL_INLINE S32 getBitmapHeight() const			{ return mBitmapHeight; }

	LL_INLINE U32 getNumBitmaps(U32 bitmap_type) const
	{
		return bitmap_type < EFontGlyphType::Count ?
					(U32)mImageRawVec[bitmap_type].size() : 0;
	}

	LL_INLINE U32 getCacheGeneration() const		{ return mGeneration; }

private:
	LL_INLINE static U32 getNumComponents(U32 bitmap_type)
	{
		return bitmap_type == EFontGlyphType::Color ? 4 : 2;
	}

private:
	std::vector<LLPointer<LLImageRaw> >	mImageRawVec[EFontGlyphType::Count];
	std::vector<LLPointer<LLImageGL> >	mImageGLVec[EFontGlyphType::Count];

	S32									mCurrentOffsetX[EFontGlyphType::Count];
	S32									mCurrentOffsetY[EFontGlyphType::Count];

	S32									mBitmapWidth;
	S32									mBitmapHeight;

	S32									mMaxCharWidth;
	S32									mMaxCharHeight;

	U32									mGeneration;
};
