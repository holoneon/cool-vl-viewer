/**
 * @file llfontfreetype.cpp
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

#include "linden_common.h"

// Import NanoSVG header-only implementation
#if LL_WINDOWS
# pragma warning (push)
# pragma warning (disable : 4702)
#endif
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"
#if LL_WINDOWS
# pragma warning (pop)
#endif

// Freetype stuff
#include "ft2build.h"
#include "freetype/fttypes.h"
#include "freetype/ftmodapi.h"
#include "freetype/otsvg.h"

#include "llfontfreetype.h"

#include "llfontgl.h"
#include "llfontbitmapcache.h"
#include "llgl.h"
#include "llimage.h"
#include "llmath.h"
#include "llstl.h"					// For DeletePairedPointer()
#include "llstring.h"

FT_Render_Mode gFontRenderMode = FT_RENDER_MODE_NORMAL;

LLFontManager* gFontManagerp = NULL;

FT_Library gFTLibrary = NULL;

///////////////////////////////////////////////////////////////////////////////
// Static callbacks for SVG rendering with Freetype via NanoSVG. This is
// implemented in a purely static LLFontFreeTypeSvgRenderer class in LL's
// sources (llfontfreetypesvg.cpp/h files), but it is only used in this
// llfontfreetype.cpp file, so let's keep things simple and just use static
// functions in here instead. HB
// See: https://freetype.org/freetype2/docs/reference/ft2-svg_fonts.html
///////////////////////////////////////////////////////////////////////////////

// Note: about mError, FreeType currently (2.12.1) ignores the error value
// returned by the preset glyph slot callback so we return it at render time.
// See: https://github.com/freetype/freetype/blob/5faa1df8b93ebecf0f8fd5fe8fda7b9082eddced/src/base/ftobjs.c#L1170
struct LLSvgRenderData
{
	NSVGimage*	mNSvgImage = NULL;
	FT_UInt		mGlyphIndex = 0;
	F32			mScale = 0.f;
	FT_Error	mError = FT_Err_Ok;
};

static FT_Error svg_on_init(FT_Pointer* statep)
{
	// The SVG driver hook state is shared across all callback invocations;
	// since our state is lightweight we store it in the glyph instead.
	*statep = NULL;
	return FT_Err_Ok;
}

static void svg_on_free(FT_Pointer*)
{
}

static void svg_on_data_finalize(void* objectp)
{
	FT_GlyphSlot slotp = (FT_GlyphSlot)objectp;
	LLSvgRenderData* datap = (LLSvgRenderData*)slotp->generic.data;
	slotp->generic.data = NULL;
	slotp->generic.finalizer = NULL;
	delete datap;
}

static FT_Error svg_on_preset_glypth_slot(FT_GlyphSlot slotp,
										  FT_Bool cache, FT_Pointer*)
{
	FT_SVG_Document docp = (FT_SVG_Document)slotp->other;
	if (!slotp->generic.data)
	{
		slotp->generic.data = new LLSvgRenderData();
		slotp->generic.finalizer = svg_on_data_finalize;
	}
	LLSvgRenderData* datap = (LLSvgRenderData*)slotp->generic.data;
	if (!cache)
	{
		datap->mGlyphIndex = slotp->glyph_index;
		datap->mError = FT_Err_Ok;
	}

	if (!datap->mNSvgImage)
	{
		// Note: nsvgParse modifies the input string so we need a temporary
		// copy.
		size_t doc_len = docp->svg_document_length;
		char* bufferp = new char[doc_len + 1];
		memcpy((void*)bufferp, (const void*)docp->svg_document, doc_len);
		bufferp[doc_len] = '\0';
		datap->mNSvgImage = nsvgParse(bufferp, "px", 0.0);
		delete[] bufferp;
	}
	if (!datap->mNSvgImage)
	{
		datap->mError = FT_Err_Invalid_SVG_Document;
		return FT_Err_Invalid_SVG_Document;
	}

	// We do not currently support transformations so test for an identity
	// rotation matrix + zero translation.
	if (docp->transform.xx != (1 << 16) || docp->transform.yx != 0 ||
		docp->transform.xy != 0 || docp->transform.yy != (1 << 16) ||
		docp->delta.x > 0 || docp->delta.y > 0)
	{
		datap->mError = FT_Err_Unimplemented_Feature;
		return FT_Err_Unimplemented_Feature;
	}

	F32 svg_width = datap->mNSvgImage->width;
	F32 svg_height = datap->mNSvgImage->height;
	if (svg_width <= 0.f || svg_height <= 0.f)
	{
		svg_width = docp->units_per_EM;
		svg_height = docp->units_per_EM;
	}

	F32 svg_x_scale = (F32)docp->metrics.x_ppem / floorf(svg_width);
	F32 svg_y_scale = (F32)docp->metrics.y_ppem / floorf(svg_height);
	F32 svg_scale = llmin(svg_x_scale, svg_y_scale);
	datap->mScale = svg_scale;

	slotp->bitmap.width = floorf(svg_width) * svg_scale;
	slotp->bitmap.rows = floorf(svg_height) * svg_scale;
	slotp->bitmap_left = (docp->metrics.x_ppem - slotp->bitmap.width) / 2;
	slotp->bitmap_top = slotp->face->size->metrics.ascender / 64.f;
	slotp->bitmap.pitch = slotp->bitmap.width * 4;
	slotp->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

	// Copied as-is from fcft (MIT license)

	// Compute all the bearings and set them correctly. The outline is scaled
	// already, we just need to use the bounding box.
	F32 h_bearing_x = 0;
	F32 h_bearing_y = -slotp->bitmap_top;
	F32 v_bearing_x = slotp->metrics.horiBearingX / 64.f -
					  slotp->metrics.horiAdvance / 128.f;
	F32 v_bearing_y = (slotp->metrics.vertAdvance / 64.f -
					   slotp->metrics.height / 64.f) * 0.5f;
	// Do conversion in two steps to avoid 'bad function cast' warning
	slotp->metrics.width = slotp->bitmap.width * 64.f;
	slotp->metrics.height = slotp->bitmap.rows * 64.f;
	slotp->metrics.horiBearingX = h_bearing_x * 64.f;
	slotp->metrics.horiBearingY = h_bearing_y * 64.f;
	slotp->metrics.vertBearingX = v_bearing_x * 64.f;
	slotp->metrics.vertBearingY = v_bearing_y * 64.f;
	if (slotp->metrics.vertAdvance == 0)
	{
		slotp->metrics.vertAdvance = slotp->bitmap.rows * (1.2f * 64.f);
	}
	return FT_Err_Ok;
}

static FT_Error svg_on_render(FT_GlyphSlot slotp, FT_Pointer*)
{
	LLSvgRenderData* datap = (LLSvgRenderData*)slotp->generic.data;
	if (datap->mError != FT_Err_Ok)
	{
		return datap->mError;
	}

	// Render to glyph bitmap
	NSVGrasterizer* rasterizerp = nsvgCreateRasterizer();
	nsvgRasterize(rasterizerp, datap->mNSvgImage, 0, 0, datap->mScale,
				  slotp->bitmap.buffer, slotp->bitmap.width,
				  slotp->bitmap.rows, slotp->bitmap.pitch);
	nsvgDeleteRasterizer(rasterizerp);
	nsvgDelete(datap->mNSvgImage);
	datap->mNSvgImage = NULL;

	// Convert from RGBA to BGRA
	U32* pixbuffp = (U32*)slotp->bitmap.buffer;
	U8* bytebuffp = slotp->bitmap.buffer;
	for (size_t y = 0, h = slotp->bitmap.rows; y < h; ++y)
	{
		for (size_t x = 0, w = slotp->bitmap.pitch / 4; x < w; ++x)
		{
			size_t p_idx = y * w + x;
			size_t b_idx = p_idx * 4;
			U8 alpha = bytebuffp[b_idx + 3];
			// Store as ARGB.
			// *TODO: do we still have to care about endianness ?
			pixbuffp[y * w + x] = alpha << 24 |
								  (bytebuffp[b_idx] * alpha / 255) << 16 |
								  (bytebuffp[b_idx + 1] * alpha / 255) << 8 |
								  (bytebuffp[b_idx + 2] * alpha / 255);
		}
	}

	slotp->format = FT_GLYPH_FORMAT_BITMAP;
	slotp->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

	return FT_Err_Ok;
}

///////////////////////////////////////////////////////////////////////////////
// LLFontManager class
///////////////////////////////////////////////////////////////////////////////

//static
void LLFontManager::initClass()
{
	if (!gFontManagerp)
	{
		gFontManagerp = new LLFontManager;
	}
}

//static
void LLFontManager::cleanupClass()
{
	delete gFontManagerp;
	gFontManagerp = NULL;
}

LLFontManager::LLFontManager()
{
	int error = FT_Init_FreeType(&gFTLibrary);
	if (error)
	{
		llerrs << "Freetype initialization failure !" << llendl;
	}

	SVG_RendererHooks hooks =
	{
		svg_on_init,
		svg_on_free,
		svg_on_render,
		svg_on_preset_glypth_slot
	};
	FT_Property_Set(gFTLibrary, "ot-svg", "svg-hooks", &hooks);
}

LLFontManager::~LLFontManager()
{
	FT_Done_FreeType(gFTLibrary);
}

///////////////////////////////////////////////////////////////////////////////
// LLFontFreetype class
///////////////////////////////////////////////////////////////////////////////

LLFontFreetype::LLFontFreetype()
:	mFontBitmapCachep(new LLFontBitmapCache),
	mIsFallback(false),
	mAscender(0.f),
	mDescender(0.f),
	mLineHeight(0.f),
	mFTFace(NULL),
	mRenderGlyphCount(0),
	mStyle(0),
	mPointSize(0)
{
}

LLFontFreetype::~LLFontFreetype()
{
	// Clean up freetype libs.
	if (mFTFace)
	{
		FT_Done_Face(mFTFace);
		mFTFace = NULL;
	}

	// Delete glyph info
	std::for_each(mCharGlyphInfoMap.begin(), mCharGlyphInfoMap.end(),
				  DeletePairedPointer());
	mCharGlyphInfoMap.clear();

	// mFontBitmapCachep will be cleaned up by LLPointer destructor.
	// mFallbackFonts cleaned up by LLPointer destructor
}

bool LLFontFreetype::loadFace(const std::string& filename, F32 point_size,
							  F32 vert_dpi, F32 horz_dpi, bool is_fallback)
{
	// Do not leak face objects. This is also needed to deal with changed font
	// file names.
	if (mFTFace)
	{
		FT_Done_Face(mFTFace);
		mFTFace = NULL;
	}

	int error = FT_New_Face(gFTLibrary, filename.c_str(), 0, &mFTFace);
	if (error)
	{
		return false;
	}

	error = FT_Select_Charmap(mFTFace, FT_ENCODING_UNICODE);
	if (error)
	{
		// Note: failures *will* happen (and are harmless) for Windows TTF
		// fonts. So, do not spam the log with them... HB
		LL_DEBUGS("Freetype") << "Failure to select Unicode char map for font: "
							  << filename << LL_ENDL;
	}

	mIsFallback = is_fallback;
	// Please, keep the following calculation in this order; while it would be
	// better to use "point_size * vert_dpi / 72.0f" to lower math rounding
	// errors, the latter gives a different result than what viewers are used
	// to give and would mean having to change font vertical justification in
	// the UI code and/or XML menu definitions...
	F32 pixels_per_em = (point_size / 72.f) * vert_dpi; // Size in inches * dpi

	error = FT_Set_Char_Size(mFTFace,					// handle to face object
							 0,							// char_width in 1/64th of pt
							 (S32)(point_size * 64.f),	// char_height in 1/64th of pt
							 (U32)horz_dpi,				// horizontal device res
							 (U32)vert_dpi);			// vertical device res

	if (error)
	{
		LL_DEBUGS("Freetype") << "Failure to set characters size for font: "
							  << filename << LL_ENDL;
		// Clean up freetype libs.
		FT_Done_Face(mFTFace);
		mFTFace = NULL;
		return false;
	}

	F32 y_max, y_min, x_max, x_min;
	F32 ems_per_unit = 1.f / mFTFace->units_per_EM;
	F32 pixels_per_unit = pixels_per_em * ems_per_unit;

	// Get size of bbox in pixels
	y_max = mFTFace->bbox.yMax * pixels_per_unit;
	y_min = mFTFace->bbox.yMin * pixels_per_unit;
	x_max = mFTFace->bbox.xMax * pixels_per_unit;
	x_min = mFTFace->bbox.xMin * pixels_per_unit;
	mAscender = mFTFace->ascender * pixels_per_unit;
	mDescender = -mFTFace->descender * pixels_per_unit;
	mLineHeight = mFTFace->height * pixels_per_unit;

	S32 max_char_width = ll_roundp(0.5f + x_max - x_min);
	S32 max_char_height = ll_roundp(0.5f + y_max - y_min);

	mFontBitmapCachep->init(max_char_width, max_char_height);

	if (!mFTFace->charmap)
	{
		FT_Set_Charmap(mFTFace, mFTFace->charmaps[0]);
	}

	if (!mIsFallback)
	{
		// Add the default glyph
		addGlyphFromFont(this, 0, 0);
	}

	mName = filename;
	mPointSize = point_size;

	mStyle = LLFontGL::NORMAL;
	if (mFTFace->style_flags & FT_STYLE_FLAG_BOLD)
	{
		mStyle |= LLFontGL::BOLD;
	}

	if (mFTFace->style_flags & FT_STYLE_FLAG_ITALIC)
	{
		mStyle |= LLFontGL::ITALIC;
	}

	return true;
}

void LLFontFreetype::addFallbackFont(const LLPointer<LLFontFreetype>& fallback_font,
									 const char_functor_t& functor)
{
	mFallbackFonts.emplace_back(fallback_font, functor);
}

F32 LLFontFreetype::getXAdvance(llwchar wch) const
{
	if (!mFTFace)
	{
		return 0.f;
	}

	// Return existing info only if it is current
	LLFontGlyphInfo* gi = getGlyphInfo(wch);
	if (gi)
	{
		return gi->mXAdvance;
	}

	glyph_info_map_t::iterator it = mCharGlyphInfoMap.find((llwchar)0);
	if (it != mCharGlyphInfoMap.end())
	{
		return it->second->mXAdvance;
	}

	// Last ditch fallback - no glyphs defined at all.
	return (F32)mFontBitmapCachep->getMaxCharWidth();
}

F32 LLFontFreetype::getXKerning(const LLFontGlyphInfo* left_glyph_infop,
								const LLFontGlyphInfo* right_glyph_infop) const
{
	if (!mFTFace)
	{
		return 0.f;
	}

	U32 left_glyph = left_glyph_infop ? left_glyph_infop->mGlyphIndex : 0;
	U32 right_glyph = right_glyph_infop ? right_glyph_infop->mGlyphIndex : 0;
	U64 kerning_key = right_glyph;
	kerning_key |= U64(left_glyph) << 32;
	kerning_cache_map_t::iterator it = mKerningCache.find(kerning_key);
	if (it != mKerningCache.end())
	{
		return it->second;
	}

	F32 delta_correction = 0.f;
	FT_Vector delta;
	delta.x = delta.y = 0;
	if (FT_HAS_KERNING(mFTFace))
	{
		FT_Get_Kerning(mFTFace, left_glyph, right_glyph, FT_KERNING_UNFITTED,
					   &delta);
		// Apply the auto-hinter subpixel side-bearing correction between
		// adjacent glyphs. When the hinter has shifted the right side of the
		// left glyph or the left side of the right glyph, rsb_delta-lsb_delta
		// is the sub-pixel nudge that keeps spacing visually even.
		if (left_glyph_infop && right_glyph_infop)
		{
			// According to FreeType documentation, these delta values should
			// only trigger discrete +/-1 pixel adjustments when they cross
			// certain thresholds. Substructing delta_diff from delta.x does
			// not work as well as treating it as a thresholds
			S32 diff = left_glyph_infop->mRsbDelta -
					   right_glyph_infop->mLsbDelta;
			if (diff > 32)
			{
				delta_correction = -1.f;
			}
			else if (diff < -31)
			{
				delta_correction = 1.f;
			}
		}
	}

	constexpr F32 k = 1.f / 64.f;
	F32 ret = delta.x * k + delta_correction;
	mKerningCache[kerning_key] = ret;
	return ret;
}

bool LLFontFreetype::hasGlyph(const llwchar wch) const
{
	return !mIsFallback && mCharGlyphInfoMap.count(wch) != 0;
}

LLFontGlyphInfo* LLFontFreetype::addGlyph(llwchar wch, U32 glyph_type) const
{
	if (!mFTFace || mIsFallback)
	{
		return NULL;
	}

	// Initialize char to glyph map
	FT_UInt glyph_index = FT_Get_Char_Index(mFTFace, wch);
	if (glyph_index)
	{
		LL_DEBUGS("Freetype") << "Glyph index for 0x" << std::hex << (U32)wch
							  << std::dec << " in " << mName << ": "
							  << glyph_index << LL_ENDL;
	}
	else
	{
		LL_DEBUGS("Freetype") << "No glyph found for 0x" << std::hex
							  << (U32)wch << std::dec << " in " << mName
							  << ". Searching in fallback fonts..." << LL_ENDL;
		// No corresponding glyph in this font: look for a glyph in fallback
		// fonts.
		size_t count = mFallbackFonts.size();
		if (LLStringOps::isEmoji(wch))
		{
			// This is a "genuine" emoji (in the range 0x1f000-0x20000): print
			// it using the emoji font(s) if possible. HB
			for (size_t i = 0; i < count; ++i)
			{
				const font_t& pair = mFallbackFonts[i];
				if (!pair.second || !pair.second(wch))
				{
					// If this font does not have a functor, or the character
					// does not pass the functor, reject it. Note: we keep the
					// functor test (despite the fact we already tested for
					// LLStringOps::isEmoji(wch) above), in case we would use
					// different, more restrictive or partitionned functors in
					// the future with several different emoji fonts. HB
					continue;
				}
				glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
				if (glyph_index)
				{
					LL_DEBUGS("Freetype") << "Glyph index for 0x" << std::hex
										  << (U32)wch << std::dec
										  << " in " << pair.first->mName
										  << ": " << glyph_index << LL_ENDL;
					return addGlyphFromFont(pair.first, wch, glyph_index,
											glyph_type);
				}
			}
		}
		// Then try and find a monochrome fallback font that could print this
		// glyph: such fonts do *not* have a functor. We give priority to
		// monochrome fonts for non-genuine emojis so that UI elements which
		// used to render with them before the emojis font introduction (e.g.
		// check marks in menus, or LSL dialogs text and buttons) do render the
		// same way as they always did. HB
		std::vector<size_t> emoji_fonts_idx;
		for (size_t i = 0; i < count; ++i)
		{
			const font_t& pair = mFallbackFonts[i];
			if (pair.second)
			{
				// If this font got a functor, remember the index for later and
				// try the next fallback font. HB
				emoji_fonts_idx.push_back(i);
				continue;
			}
			glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
			if (glyph_index)
			{
				LL_DEBUGS("Freetype") << "Glyph index for 0x" << std::hex
									  << (U32)wch << std::dec
									  << " in " << pair.first->mName << ": "
									  << glyph_index << LL_ENDL;
				return addGlyphFromFont(pair.first, wch, glyph_index,
										glyph_type);
			}
		}
		// Everything failed so far: this character is not a genuine emoji,
		// neither a special character known from our monochrome fallback
		// fonts: make a last try, using the emoji font(s), but ignoring the
		// functor to render using whatever (colorful) glyph that might be
		// available in such fonts for this character. HB
		for (size_t j = 0, count2 = emoji_fonts_idx.size(); j < count2; ++j)
		{
			const font_t& pair = mFallbackFonts[emoji_fonts_idx[j]];
			glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
			if (glyph_index)
			{
				LL_DEBUGS("Freetype") << "Glyph index for 0x" << std::hex
									  << (U32)wch << std::dec
									  << " in " << pair.first->mName << ": "
									  << glyph_index << LL_ENDL;
				return addGlyphFromFont(pair.first, wch, glyph_index,
										glyph_type);
			}
		}
#if 0	// For some reason, this causes crashes, excepted under Linux x86_64:
		// Something to do with jemalloc usage that would prevent the
		// crash ?...  HB
		// No corresponding glyph to be found anywhere: replace it with the
		// UTF-8 "replacement character". HB
		constexpr llwchar rch = (llwchar)0x0fffdUL;
		for (size_t i = 0; i < count; ++i)
		{
			const font_t& pair = mFallbackFonts[i];
			if (pair.second)
			{
				continue;	// Do not bother with emoji fonts. HB
			}
			glyph_index = FT_Get_Char_Index(pair.first->mFTFace, rch);
			if (glyph_index)
			{
				LL_DEBUGS("Freetype") << "Replacement character found in "
									  << pair.first->mName << " at index: "
									  << glyph_index << LL_ENDL;
				return addGlyphFromFont(pair.first, wch, glyph_index,
										glyph_type);
			}
		}
#endif
		// Ultimate fallback: use a question mark if none of our fallback
		// fonts got a glyph for the UTF-8 "replacement character". HB
		glyph_index = FT_Get_Char_Index(mFTFace, (FT_ULong)'?');
	}

	auto range_it = mCharGlyphInfoMap.equal_range(wch);
	glyph_info_map_t::iterator it =
		std::find_if(range_it.first, range_it.second,
					 [&glyph_type](const glyph_info_map_t::value_type& entry)
					 {
						return entry.second->mGlyphType == glyph_type;
					 });
	if (it == range_it.second)
	{
		return addGlyphFromFont(this, wch, glyph_index, glyph_type);
	}

	return NULL;
}

LLFontGlyphInfo* LLFontFreetype::addGlyphFromFont(const LLFontFreetype* fontp,
												  llwchar wch,
												  U32 glyph_index,
												  U32 glyph_type) const
{
	if (!mFTFace || mIsFallback)
	{
		return NULL;
	}

	fontp->renderGlyph(glyph_type, glyph_index);

	U32 b_type = EFontGlyphType::Unspecified;
	switch (fontp->mFTFace->glyph->bitmap.pixel_mode)
	{
		case FT_PIXEL_MODE_MONO:
		case FT_PIXEL_MODE_GRAY:
			b_type = EFontGlyphType::Grayscale;
			break;

		case FT_PIXEL_MODE_BGRA:
			b_type = EFontGlyphType::Color;
			break;

		default:
			llerrs << "Unknown glyph type: "
				   << fontp->mFTFace->glyph->bitmap.pixel_mode << llendl;
	}

	S32 width = fontp->mFTFace->glyph->bitmap.width;
	S32 height = fontp->mFTFace->glyph->bitmap.rows;

	S32 pos_x, pos_y;
	U32 bitmap_num;
	mFontBitmapCachep->nextOpenPos(width, pos_x, pos_y, b_type, bitmap_num);

	// Convert these from 26.6 units to float pixels.
	constexpr F32 k = 1.f / 64.f;
	F32 x_advance = fontp->mFTFace->glyph->advance.x * k;
	F32 y_advance = fontp->mFTFace->glyph->advance.y * k;
	LLFontGlyphInfo* gip =
		new LLFontGlyphInfo(glyph_index, glyph_type, b_type, bitmap_num,
							pos_x, pos_y, width, height,
							fontp->mFTFace->glyph->bitmap_left,
							fontp->mFTFace->glyph->bitmap_top,
							x_advance, y_advance,
							// FreeType fills these when the glyph has been
							// auto-hinted; they describe how much the hinter
							// nudged the left/right side bearings (in
							// 26.6 pixels). Keep them so inter-glyph spacing
							// can be corrected in getXKerning().
							(S32)fontp->mFTFace->glyph->lsb_delta,
							(S32)fontp->mFTFace->glyph->rsb_delta);
	insertGlyphInfo(wch, gip);

	if (glyph_type != b_type)
	{
		LLFontGlyphInfo* tempgip = new LLFontGlyphInfo(*gip);
		tempgip->mGlyphType = b_type;
		insertGlyphInfo(wch, tempgip);
	}

	if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ||
		fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
	{
		U8* bufferp = fontp->mFTFace->glyph->bitmap.buffer;
		S32 buffer_row_stride = fontp->mFTFace->glyph->bitmap.pitch;
		U8* graydatap = NULL;

		if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
		{
			// Need to expand 1-bit bitmap to 8-bit graymap.
			graydatap = new U8[width * height];
			S32 xpos, ypos;
			for (ypos = 0; ypos < height; ++ypos)
			{
				S32 bm_row_offset = buffer_row_stride * ypos;
				for (xpos = 0; xpos < width; ++xpos)
				{
					U32 bm_col_offsetbyte = xpos / 8;
					U32 bm_col_offsetbit = 7 - (xpos % 8);
					U32 bit = !!(bufferp[bm_row_offset + bm_col_offsetbyte] &
								 (1 << bm_col_offsetbit));
					graydatap[width * ypos + xpos] = 255 * bit;
				}
			}
			// Use newly-built graymap.
			bufferp = graydatap;
			buffer_row_stride = width;
		}

		setSubImageLuminanceAlpha(pos_x, pos_y, bitmap_num, width, height,
								  bufferp, buffer_row_stride);

		if (graydatap)
		{
			delete[] graydatap;
		}
	}
	else if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA)
	{
		setSubImageBGRA(pos_x, pos_y, bitmap_num,
						fontp->mFTFace->glyph->bitmap.width,
						fontp->mFTFace->glyph->bitmap.rows,
						fontp->mFTFace->glyph->bitmap.buffer,
						abs(fontp->mFTFace->glyph->bitmap.pitch));
	}
	else
	{
		// We do not know how to handle this pixel format from Freetype; omit
		// it from the font-image.
		llwarns_once << "Unknown pixel format for font: "
					 << fontp->mName << ". Will not render..." << llendl;
	}

	LLImageGL* glimagep = mFontBitmapCachep->getImageGL(b_type, bitmap_num);
	LLImageRaw* rawimagep = mFontBitmapCachep->getImageRaw(b_type, bitmap_num);
	if (glimagep && rawimagep)
	{
		glimagep->setSubImage(rawimagep, 0, 0, glimagep->getWidth(),
							  glimagep->getHeight());
	}
	else
	{
		llwarns << "Failed to add glyph image for character: " << std::hex
				<< (S32)wch << std::dec << " !  Out of memory ?" << llendl;
	}

	return gip;
}

LLFontGlyphInfo* LLFontFreetype::getGlyphInfo(llwchar wch, U32 g_type) const
{
	auto range_it = mCharGlyphInfoMap.equal_range(wch);
	glyph_info_map_t::iterator it;
	if (g_type == EFontGlyphType::Unspecified)
	{
		it = range_it.first;
		g_type = EFontGlyphType::Grayscale;
	}
	else
	{
		it = std::find_if(range_it.first, range_it.second,
						  [&g_type](const glyph_info_map_t::value_type& entry)
						  {
							return entry.second->mGlyphType == g_type;
						  });
	}

	if (it != range_it.second)
	{
		return it->second;
	}

	// This glyph does not yet exist, so render it and return the result
	return addGlyph(wch, g_type);
}

void LLFontFreetype::insertGlyphInfo(llwchar wch, LLFontGlyphInfo* gip) const
{
	auto range_it = mCharGlyphInfoMap.equal_range(wch);
	glyph_info_map_t::iterator it =
		std::find_if(range_it.first, range_it.second,
					 [&gip](const glyph_info_map_t::value_type& entry)
					 {
						return entry.second->mGlyphType == gip->mGlyphType;
					 });
	if (it != range_it.second)
	{
		delete it->second;
		it->second = gip;
	}
	else
	{
		mCharGlyphInfoMap.insert(std::make_pair(wch, gip));
	}
}

void LLFontFreetype::renderGlyph(U32 bitmap_type, U32 glyph_index) const
{
	if (!mFTFace)
	{
		return;
	}
#if 1	// Do *not* force auto-hinting. HB
	FT_Int32 load_flags = FT_LOAD_DEFAULT;
#else
	FT_Int32 load_flags = FT_LOAD_FORCE_AUTOHINT;
#endif
	if (bitmap_type == EFontGlyphType::Color)
	{
		// We may not actually get a color render so our caller should always
		// examine mFTFace->glyph->bitmap.pixel_mode
		load_flags |= FT_LOAD_COLOR;
	}
	FT_Error error = FT_Load_Glyph(mFTFace, glyph_index, load_flags);
	if (error)
	{
			LL_DEBUGS("Freetype") << "Error loading glyph, index: "
								  << glyph_index << LL_ENDL;
			glyph_index = FT_Get_Char_Index(mFTFace, (FT_ULong)'?');
			FT_Load_Glyph(mFTFace, glyph_index, FT_LOAD_DEFAULT);
	}

	error = FT_Render_Glyph(mFTFace->glyph, gFontRenderMode);
	if (error)
	{
		LL_DEBUGS("Freetype") << "Error rendering glyph, index: "
							  << glyph_index << LL_ENDL;
		llassert(false);
	}

	++mRenderGlyphCount;
}

void LLFontFreetype::reset(F32 vert_dpi, F32 horz_dpi)
{
	resetBitmapCache();

	loadFace(mName, mPointSize, vert_dpi, horz_dpi, mIsFallback);
	if (mIsFallback)
	{
		return;
	}

	if (mFallbackFonts.empty())
	{
		llwarns << "No fallback fonts present" << llendl;
		return;
	}

	// This is the head of the list, we need to rebuild ourself and all
	// fallbacks.
	for (U32 i = 0, count = mFallbackFonts.size(); i < count; ++i)
	{
		mFallbackFonts[i].first->reset(vert_dpi, horz_dpi);
	}
}

void LLFontFreetype::resetBitmapCache()
{
	for (glyph_info_map_t::iterator it = mCharGlyphInfoMap.begin(),
									end = mCharGlyphInfoMap.end();
		 it != end; ++it)
	{
		delete it->second;
	}

	mCharGlyphInfoMap.clear();
	mFontBitmapCachep->reset();

	if (!mIsFallback)
	{
		// Add the empty glyph
		addGlyphFromFont(this, 0, 0);
	}
}

void LLFontFreetype::destroyGL()
{
	mFontBitmapCachep->destroyGL();
}

void LLFontFreetype::setSubImageLuminanceAlpha(U32 x, U32 y, U32 bitmap_num,
											   U32 width, U32 height,
											   U8* datap, S32 stride) const
{
	if (mIsFallback || !datap)
	{
		return;
	}

	LLImageRaw* imagep =
		mFontBitmapCachep->getImageRaw(EFontGlyphType::Grayscale, bitmap_num);
	if (!imagep || imagep->getComponents() != 2)
	{
		return;
	}

	U8* targetp = imagep->getData();
	if (!targetp)
	{
		return;
	}

	if (stride == 0)
	{
		stride = width;
	}

	U32 target_width = imagep->getWidth();
	for (U32 i = 0; i < height; ++i)
	{
		U32 to_offset = (y + i) * target_width + x;
		U32 from_offset = (height - 1 - i) * stride;
		for (U32 j = 0; j < width; ++j)
		{
			*(targetp + to_offset++ * 2 + 1) = *(datap + from_offset++);
		}
	}
}

void LLFontFreetype::setSubImageBGRA(U32 x, U32 y, U32 bitmap_num,
									 U32 width, U32 height,
									 U8* datap, S32 stride) const
{
	if (mIsFallback)
	{
		return;
	}

	LLImageRaw* imagep = mFontBitmapCachep->getImageRaw(EFontGlyphType::Color,
														bitmap_num);
	if (!imagep || imagep->getComponents() != 4)
	{
		return;
	}
	
	U32* targetp = (U32*)imagep->getData();
	if (!targetp)
	{
		return;
	}

	const U32 image_width = imagep->getWidth();
	for (U32 row = 0; row < height; ++row)
	{
		U32 src_offset = (height - row - 1) * width * 4;
		U32 dst_offset = (y + row) * image_width + x;
		for (U32 col = 0; col < width; ++col)
		{
			U32 pixel = src_offset + col * 4;
			targetp[dst_offset + col] = datap[pixel + 3] << 24 |
										datap[pixel] << 16 |
										datap[pixel + 1] << 8 |
										datap[pixel + 2];
		}
	}
}
