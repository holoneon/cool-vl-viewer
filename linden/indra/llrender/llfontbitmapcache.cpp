/**
 * @file llfontbitmapcache.cpp
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

#include "linden_common.h"

#include "llfontbitmapcache.h"

#include "llgl.h"

LLFontBitmapCache::LLFontBitmapCache()
:	mMaxCharWidth(0),
	mMaxCharHeight(0),
	mBitmapWidth(0),
	mBitmapHeight(0),
	mGeneration(0)

{
	// Note: we cheat for speed, avoiding a for() loop. This works as long as
	// EFontGlyphType::Count == 2. HB
	mCurrentOffsetX[0] = mCurrentOffsetX[1] = 1;
	mCurrentOffsetY[0] = mCurrentOffsetY[1] = 1;
}

void LLFontBitmapCache::reset()
{
	mBitmapWidth = mBitmapHeight = 0;
	// Note: we cheat for speed, avoiding a for()  loop. This works as long as
	// EFontGlyphType::Count == 2. HB
	mImageRawVec[0].clear();
	mImageRawVec[1].clear();
	mImageGLVec[0].clear();
	mImageGLVec[1].clear();
	mCurrentOffsetX[0] = mCurrentOffsetX[1] = 1;
	mCurrentOffsetY[0] = mCurrentOffsetY[1] = 1;
	++mGeneration;
}

void LLFontBitmapCache::init(S32 max_char_width, S32 max_char_height)
{
	reset();
	mMaxCharWidth = max_char_width;
	mMaxCharHeight = max_char_height;

	S32 image_width = mMaxCharWidth * 20;
	S32 pow_iw = 2;
	while (pow_iw < image_width)
	{
		pow_iw *= 2;
	}
	image_width = llmin(pow_iw, 512); // Never bigger than 512x512 !
	mBitmapWidth = mBitmapHeight = image_width;
}

LLImageRaw* LLFontBitmapCache::getImageRaw(U32 b_type, U32 b_num) const
{
	if (b_type >= EFontGlyphType::Count ||
		(size_t)b_num >= mImageRawVec[b_type].size())
	{
		return NULL;
	}
	return mImageRawVec[b_type][b_num].get();
}

LLImageGL* LLFontBitmapCache::getImageGL(U32 b_type, U32 b_num) const
{
	if (b_type >= EFontGlyphType::Count ||
		(size_t)b_num >= mImageGLVec[b_type].size())
	{
		return NULL;
	}
	return mImageGLVec[b_type][b_num].get();
}

void LLFontBitmapCache::nextOpenPos(S32 width, S32& pos_x, S32& pos_y,
									U32 b_type, U32& bitmap_num)
{
	if (b_type >= EFontGlyphType::Count)
	{
		return;
	}
	bool no_image = mImageRawVec[b_type].empty();
	if (no_image || mCurrentOffsetX[b_type] + width + 1 > mBitmapWidth)
	{
		if (no_image ||
			mCurrentOffsetY[b_type] + 2 * mMaxCharHeight + 2 > mBitmapHeight)
		{
			// We Are out of space in the current image, or no image has been
			// allocated yet. Make a new one.
			S32 image_width = mMaxCharWidth * 20;
			S32 pow_iw = 2;
			while (pow_iw < image_width)
			{
				pow_iw *= 2;
			}
			image_width = llmin(pow_iw, 512); // Never bigger than 512x512 !
			S32 image_height = image_width;

			mBitmapWidth = image_width;
			mBitmapHeight = image_height;

			U32 num_components = getNumComponents(b_type);

			mImageRawVec[b_type].emplace_back(new LLImageRaw(mBitmapWidth,
															 mBitmapHeight,
															 num_components));
			bitmap_num = mImageRawVec[b_type].size() - 1;
			LLImageRaw* imagep = getImageRaw(b_type, bitmap_num);
			if (num_components == 2)
			{
				imagep->clear(255, 0);
			}

			// Make corresponding GL image.
			mImageGLVec[b_type].emplace_back(new LLImageGL(imagep, false,
														   false));
			LLImageGL* glimgp = getImageGL(b_type, bitmap_num);

			// Start at beginning of the new image.
			mCurrentOffsetX[b_type] = 1;
			mCurrentOffsetY[b_type] = 1;

			// Attach corresponding GL texture.
			gGL.getTexUnit(0)->bind(glimgp);
			// Was: setMipFilterNearest(true, true);
			glimgp->setFilteringOption(LLTexUnit::TFO_POINT);
		}
		else
		{
			// Move to next row in current image.
			mCurrentOffsetX[b_type] = 1;
			mCurrentOffsetY[b_type] += mMaxCharHeight + 1;
		}
	}

	pos_x = mCurrentOffsetX[b_type];
	pos_y = mCurrentOffsetY[b_type];
	bitmap_num = getNumBitmaps(b_type) - 1;

	mCurrentOffsetX[b_type] += width + 1;
	++mGeneration;
}

void LLFontBitmapCache::destroyGL()
{
	for (U32 j = 0; j < EFontGlyphType::Count; ++j)
	{
		for (size_t i = 0, count = mImageGLVec[j].size(); i < count; ++i)
		{
			mImageGLVec[j][i]->destroyGLTexture();
		}
	}
}
