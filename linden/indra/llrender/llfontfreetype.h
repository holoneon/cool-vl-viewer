/**
 * @file llfontfreetype.h
 * @brief Font library wrapper
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include <unordered_map>
#include <utility>

#include "hbfastmap.h"
#include "llfontbitmapcache.h"
#include "llimagegl.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llstl.h"

// Hack. FT_Face is just a typedef for a pointer to a struct, but there is no
// simple forward declarations file for FreeType and the main include file is
// 200K. We forward declare the struct here. JC
struct FT_FaceRec_;
typedef struct FT_FaceRec_* LLFT_Face;

class LLFontManager
{
public:
	static void initClass();
	static void cleanupClass();

public:
	LLFontManager();
	~LLFontManager();
};

struct LLFontGlyphInfo
{
	LL_INLINE LLFontGlyphInfo(U32 index, U32 glyph_type, U32 bitmap_type,
							  S32 bitmap_num, S32 pos_x, S32 pos_y, S32 width,
							  S32 height, S32 x_bearing, S32 y_bearing,
							  F32 x_advance, F32 y_advance, S32 lsb_delta,
							  S32 rsb_delta)
	:	mGlyphIndex(index),
		mGlyphType(glyph_type),
		mBitmapEntry(std::make_pair(bitmap_type, bitmap_num)),
		mXBitmapOffset(pos_x),
		mYBitmapOffset(pos_y),
		mWidth(width),
		mHeight(height),
		mXBearing(x_bearing),
		mYBearing(y_bearing),
		mXAdvance(x_advance),
		mYAdvance(y_advance),
		mLsbDelta(lsb_delta),
		mRsbDelta(rsb_delta)
	{
	}

	LL_INLINE LLFontGlyphInfo(const LLFontGlyphInfo& fgi)
	:	mGlyphIndex(fgi.mGlyphIndex),
		mGlyphType(fgi.mGlyphType),
		mBitmapEntry(fgi.mBitmapEntry),
		mXBitmapOffset(fgi.mXBitmapOffset),
		mYBitmapOffset(fgi.mYBitmapOffset),
		mWidth(fgi.mWidth),
		mHeight(fgi.mHeight),
		mXBearing(fgi.mXBearing),
		mYBearing(fgi.mYBearing),
		mXAdvance(fgi.mXAdvance),
		mYAdvance(fgi.mYAdvance),
		mLsbDelta(fgi.mLsbDelta),
		mRsbDelta(fgi.mRsbDelta)
	{
	}

	// Which bitmap in the bitmap cache contains this glyph
	std::pair<U32, S32> mBitmapEntry;

	U32 mGlyphIndex;
	U32 mGlyphType;

	// Metrics in pixels
	S32 mWidth;
	S32 mHeight;
	F32 mXAdvance;
	F32 mYAdvance;

	// Information for actually rendering
	S32 mXBitmapOffset;			// Offset to the origin in the bitmap
	S32 mYBitmapOffset;			// Offset to the origin in the bitmap
	S32 mXBearing;				// Distance from baseline to left in pixels
	S32 mYBearing;				// Distance from baseline to top in pixels
	S32 mLsbDelta;				// Subpixel left side bearing delta
	S32 mRsbDelta;				// Subpixel right side bearing delta
};

extern LLFontManager* gFontManagerp;

class LLFontFreetype final : public LLRefCount
{
protected:
	LOG_CLASS(LLFontFreetype);

public:
	LLFontFreetype();
	~LLFontFreetype() override;

	// is_fallback should be true for fallback fonts that aren't used
	// to render directly (Unicode backup, primarily)
	bool loadFace(const std::string& filename, F32 point_size, F32 vert_dpi,
				  F32 horz_dpi, bool is_fallback);

	typedef std::function<bool(llwchar)> char_functor_t;
	void addFallbackFont(const LLPointer<LLFontFreetype>& fallback_font,
						 const char_functor_t& functor = NULL);

	// Global font metrics - in units of pixels
	LL_INLINE F32 getLineHeight() const				{ return mLineHeight; }
	LL_INLINE F32 getAscenderHeight() const			{ return mAscender; }
	LL_INLINE F32 getDescenderHeight() const		{ return mDescender; }

// For a lowercase "g":
//
//	------------------------------
//	                     ^     ^
//						 |     |
//				xxx x    |Ascender
//	           x   x     v     |
//	---------   xxxx-------------- Baseline
//	^		       x	       |
//  | Descender    x           |
//	v			xxxx           |LineHeight
//  -----------------------    |
//                             v
//	------------------------------

	enum
	{
		FIRST_CHAR = 32,
		NUM_CHARS = 127 - 32,
		LAST_CHAR_BASIC = 127,

		// Need full 8-bit ascii range for spanish
		NUM_CHARS_FULL = 255 - 32,
		LAST_CHAR_FULL = 255
	};

	LLFontGlyphInfo* getGlyphInfo(llwchar wch,
								  U32 glyph_type =
									EFontGlyphType::Unspecified) const;

	LL_INLINE F32 getXAdvance(const LLFontGlyphInfo* glyph) const
	{
		return mFTFace && glyph ? glyph->mXAdvance : 0.f;
	}

	F32 getXAdvance(llwchar wc) const;

	F32 getXKerning(const LLFontGlyphInfo* left_glyph_info,
					const LLFontGlyphInfo* right_glyph_info) const;

	// Gets the kerning between the two characters
	LL_INLINE F32 getXKerning(llwchar char_left, llwchar char_right) const
	{
		return getXKerning(getGlyphInfo(char_left), getGlyphInfo(char_right));
	}

	void reset(F32 vert_dpi, F32 horz_dpi);

	void destroyGL();

	LL_INLINE const std::string& getName() const	{ return mName; }

	LL_INLINE const LLPointer<LLFontBitmapCache> getFontBitmapCache() const
	{
		return mFontBitmapCachep;
	}

	LL_INLINE void setStyle(U8 style)				{ mStyle = style; }
	LL_INLINE U8 getStyle() const					{ return mStyle; }

private:
	void resetBitmapCache();

	void setSubImageLuminanceAlpha(U32 x, U32 y, U32 bitmap_num,
								   U32 width, U32 height,
								   U8* datap, S32 stride = 0) const;
	void setSubImageBGRA(U32 x, U32 y, U32 bitmap_num, U32 width, U32 height,
						 U8* datap, S32 stride) const;

	// Has a glyph for this character
	bool hasGlyph(const llwchar wch) const;
	// Add a new character to the font if necessary
	LLFontGlyphInfo* addGlyph(llwchar wch, U32 glyph_type) const;
	// Add a glyph from this font to the other (returns the glyph_index, NULL
	// if not found)
	LLFontGlyphInfo* addGlyphFromFont(const LLFontFreetype* fontp,
									  llwchar wch, U32 glyph_index,
									  U32 glyph_type =
										EFontGlyphType::Grayscale) const;

	void insertGlyphInfo(llwchar wch, LLFontGlyphInfo* gip) const;

	void renderGlyph(U32 bitmap_type, U32 glyph_index) const;

private:
	std::string								mName;

	mutable LLPointer<LLFontBitmapCache>	mFontBitmapCachep;

	typedef std::pair<LLPointer<LLFontFreetype>, char_functor_t> font_t;
	// A list of fallback fonts to look for glyphs in (for Unicode chars)
	typedef std::vector<font_t> font_vector_t;
	font_vector_t							mFallbackFonts;

	// Note: the same glyph can be present with multiple representations (but
	// the pointer is always unique).
	typedef std::unordered_multimap<llwchar, LLFontGlyphInfo*> glyph_info_map_t;
	// Information about glyph location in bitmap
	mutable glyph_info_map_t				mCharGlyphInfoMap;

	typedef flat_hmap<U64, F32> kerning_cache_map_t;
	mutable kerning_cache_map_t				mKerningCache;

	LLFT_Face								mFTFace;

	mutable S32								mRenderGlyphCount;

	F32										mPointSize;
	F32										mAscender;
	F32										mDescender;
	F32										mLineHeight;

	U8										mStyle;

	bool									mIsFallback;
};
