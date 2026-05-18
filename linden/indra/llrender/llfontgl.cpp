/**
 * @file llfontgl.cpp
 * @brief Wrapper around FreeType
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

#include "linden_common.h"

#include <utility>

#include "boost/tokenizer.hpp"

#include "llfontgl.h"

#include "hbfastset.h"
#include "llfasttimer.h"
#include "llfontbitmapcache.h"
#include "llfontfreetype.h"
#include "llgl.h"
#include "llglslshader.h"
#include "llgltexture.h"
#include "llrender.h"
#include "llvertexbuffer.h"

// This defines the max number of glyphs per batch (8 chars, 6 glyphs per char)
constexpr S32 GLYPH_BATCH_SIZE = 48;

// If you change drawGlyph() or renderQuad(), you may have to change this
// number:
constexpr S32 MAX_VERT_PER_GLYPH = 36;	// 6 vertices * 6 passes max

constexpr S32 BOLD_OFFSET = 1;

constexpr U8 HAS_SHADOW = LLFontGL::DROP_SHADOW | LLFontGL::DROP_SHADOW_SOFT;
constexpr U8 NO_SHADOW = ~HAS_SHADOW;

// Static class members
F32 LLFontGL::sVertDPI = 96.f;
F32 LLFontGL::sHorizDPI = 96.f;
F32 LLFontGL::sScaleX = 1.f;
F32 LLFontGL::sScaleY = 1.f;
bool LLFontGL::sDisplayFont = true;
bool LLFontGL::sAllowColorUse = true;
U32 LLFontGL::sResGeneration = 0;

LLColor4 LLFontGL::sShadowColor(0.f, 0.f, 0.f, 1.f);
LLColor4U LLFontGL::sShadowColorU(0, 0, 0, 255);
LLFontRegistry* LLFontGL::sFontRegistry = NULL;

LLCoordGL LLFontGL::sCurOrigin;
F32 LLFontGL::sCurDepth;
std::vector<std::pair<LLCoordGL, F32> > LLFontGL::sOriginStack;

constexpr F32 EXT_X_BEARING = 1.f;
constexpr F32 EXT_Y_BEARING = 0.f;
constexpr F32 EXT_KERNING = 1.f;
 // Half of vertical padding between glyphs in the glyph texture:
constexpr F32 PAD_UVY = 0.5f;
constexpr F32 DROP_SHADOW_SOFT_STRENGTH = 0.3f;

LLFontGL::~LLFontGL()
{
	mEmbeddedChars.clear();
}

void LLFontGL::reset()
{
	mFontFreetype->reset(sVertDPI, sHorizDPI);
}

void LLFontGL::destroyGL()
{
	mFontFreetype->destroyGL();
}

bool LLFontGL::loadFace(const std::string& filename, F32 point_size,
						F32 vert_dpi, F32 horz_dpi, bool is_fallback)
{
	if (!mFontFreetype)
	{
		mFontFreetype = new LLFontFreetype;
	}
	return mFontFreetype->loadFace(filename, point_size, vert_dpi, horz_dpi,
								   is_fallback);
}

U32 LLFontGL::getCacheGeneration() const
{
	if (!mFontFreetype)
	{
		return 0;
	}
	const LLFontBitmapCache* bcachep = mFontFreetype->getFontBitmapCache();
	return bcachep->getCacheGeneration();
}

S32 LLFontGL::render(const LLWString& text, S32 begin_offset, F32 x, F32 y,
					 const LLColor4& color, HAlign halign, VAlign valign,
					 U8 style, S32 max_chars, S32 max_pixels, F32* right_x,
					 bool use_embedded, bool use_ellipses,
					 bool use_color) const
{
	if (!sDisplayFont || text.empty())
	{
		return text.length();
	}
	gGL.flush();
	// We dispatch to either the legacy, glyph by glyph renderer or to the new,
	// batched glyphs renderer depending whether we need support for embedded
	// items or not (i.e. only the notecards and the text editors allowing
	// embedded items still use the legacy renderer to render the said embedded
	// items, and only them). HB
	if (use_embedded)
	{
		return oldrender(text, begin_offset, x, y, color, halign, valign,
						 style, max_chars, max_pixels, right_x, use_ellipses);
	}
	return newrender(text, begin_offset, x, y, color, halign, valign, style,
					 max_chars, max_pixels, right_x, use_ellipses, use_color);
}

S32 LLFontGL::newrender(const LLWString& wstr, S32 begin_offset, F32 x, F32 y,
						const LLColor4& color, HAlign halign, VAlign valign,
						U8 style, S32 max_chars, S32 max_pixels, F32* right_x,
						bool use_ellipses, bool use_color,
						bool* has_color_characters) const
{
	LL_FAST_TIMER(FTM_RENDER_FONTS_BATCHED);

	if (!sAllowColorUse)
	{
		use_color = false;
	}

	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->enable(LLTexUnit::TT_TEXTURE);

	S32 scaled_max_pixels =
		max_pixels == S32_MAX ? S32_MAX : llceil((F32)max_pixels * sScaleX);

	// Strip off any style bits that are already accounted for by the font.
	style = (style | mFontDescriptor.getStyle()) & ~mFontFreetype->getStyle();

	F32 drop_shadow_strength = 0.f;
	if (style & HAS_SHADOW)
	{
		F32 luminance;
		color.calcHSL(NULL, NULL, &luminance);
		drop_shadow_strength = clamp_rescale(luminance, 0.35f, 0.6f, 0.f, 1.f);
		if (luminance < 0.35f)
		{
			style = style & NO_SHADOW;
		}
	}

	gGL.pushUIMatrix();
	gGL.loadUIIdentity();

	// Depth translation, so that floating text appears 'in-world' and is
	// correctly occluded.
	gGL.translatef(0.f, 0.f, sCurDepth);

	S32 length = (S32)wstr.length() - begin_offset;
	if (max_chars != -1 && length > max_chars)
	{
		length = max_chars;
	}

 	// Not guaranteed to be set correctly
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	F32 origin_x = floorf(sCurOrigin.mX * sScaleX);
	F32 origin_y = floorf(sCurOrigin.mY * sScaleY);
	F32 cur_x = (F32)x * sScaleX + origin_x;
	F32 cur_y = (F32)y * sScaleY + origin_y;

	// Offset y by vertical alignment; use unscaled font metrics here
	switch (valign)
	{
		case BASELINE:	// Baseline, do nothing.
			break;

		case TOP:
			cur_y -= llceil(mFontFreetype->getAscenderHeight());
			break;

		case BOTTOM:
			cur_y += llceil(mFontFreetype->getDescenderHeight());
			break;

		case VCENTER:
			cur_y -= llceil((llceil(mFontFreetype->getAscenderHeight()) -
							 llceil(mFontFreetype->getDescenderHeight())) *
							0.5f);
			break;

		default:
			break;
	}

	switch (halign)
	{
		case LEFT:
			break;

		case RIGHT:
	  		cur_x -= llmin(scaled_max_pixels,
						   ll_roundp(getWidthF32(wstr.c_str(), 0, length) *
									 sScaleX));
			break;

		case HCENTER:
	    	cur_x -= llmin(scaled_max_pixels,
						   ll_roundp(getWidthF32(wstr.c_str(), 0, length) *
									 sScaleX)) / 2;
			break;

		default:
			break;
	}

	F32 cur_render_y = cur_y;
	F32 cur_render_x = cur_x;

	F32 start_x = ll_round(cur_x);

	const LLFontBitmapCache* bcachep = mFontFreetype->getFontBitmapCache();

	F32 inv_width = 1.f / bcachep->getBitmapWidth();
	F32 inv_height = 1.f / bcachep->getBitmapHeight();

	constexpr S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;

	bool draw_ellipses = false;
	if (use_ellipses && halign == LEFT)
	{
		// Check for too long of a string
		if (getWidthF32(wstr.c_str(), 0, max_chars) * sScaleX > scaled_max_pixels)
		{
			// Use four dots for ellipsis width to generate padding
			static const LLWString dots(utf8str_to_wstring(std::string("....")));
			scaled_max_pixels = scaled_max_pixels -
								ll_roundp(getWidthF32(dots.c_str()));
			if (scaled_max_pixels < 0)
			{
				scaled_max_pixels = 0;
			}
			draw_ellipses = true;
		}
	}

	const LLFontGlyphInfo* next_glyph = NULL;

	constexpr S32 BUFFER_SIZE = GLYPH_BATCH_SIZE * MAX_VERT_PER_GLYPH;
	alignas(16) static LLVector4a vertices[BUFFER_SIZE];
	static LLVector2 uvs[BUFFER_SIZE];
	static LLColor4U colors[BUFFER_SIZE];

	LLColor4U text_color = LLColor4U(color);
	// Preserve the transparency to render fading emojis in fading text (e.g.
	// for the chat console)... HB
	LLColor4U emoji_color(255, 255, 255, text_color.mV[VW]);

	static const std::pair<U32, S32> no_bitmap_entry =
		std::make_pair(EFontGlyphType::Grayscale, -1);
	const std::pair<U32, S32>* bitmap_entry = &no_bitmap_entry;
	U32 b_type = use_color ? EFontGlyphType::Color : EFontGlyphType::Grayscale;
	S32 glyph_count = 0;
	S32 chars_drawn = 0;
	bool use_colors = false;
	for (S32 i = begin_offset; i < begin_offset + length; ++i)
	{
		llwchar wch = wstr[i];

		const LLFontGlyphInfo* fgi = next_glyph;
		next_glyph = NULL;
		if (!fgi)
		{
			fgi = mFontFreetype->getGlyphInfo(wch, b_type);
		}
		if (!fgi)
		{
			llerrs << "Missing Glyph Info" << llendl;
			break;
		}
		// Per-glyph bitmap texture.
		if (fgi->mBitmapEntry != *bitmap_entry)
		{
			// Actually draw the queued glyphs before switching their texture;
			// otherwise the queued glyphs will be taken from wrong textures.
			if (glyph_count > 0)
			{
				gGL.begin(LLRender::TRIANGLES);
				gGL.vertexBatchPreTransformed(vertices, uvs, colors,
											  glyph_count * 6);
				gGL.end();
				glyph_count = 0;
			}

			bitmap_entry = &fgi->mBitmapEntry;
			LLImageGL* impagep =
				bcachep->getImageGL(bitmap_entry->first,
									bitmap_entry->second);
			unit0->bind(impagep);
		}

		if (start_x + scaled_max_pixels < cur_x + fgi->mXBearing + fgi->mWidth)
		{
			// Not enough room for this character.
			break;
		}

		// Draw the text at the appropriate location
		//Specify vertices and texture coordinates
		LLRectf uv_rect((fgi->mXBitmapOffset) * inv_width,
						(fgi->mYBitmapOffset + fgi->mHeight + PAD_UVY) *
						inv_height,
						(fgi->mXBitmapOffset + fgi->mWidth) * inv_width,
						(fgi->mYBitmapOffset - PAD_UVY) * inv_height);
		// Snap glyph origin to whole screen pixel
		LLRectf screen_rect(ll_round(cur_render_x + (F32)fgi->mXBearing),
						    ll_round(cur_render_y + (F32)fgi->mYBearing),
				 		    ll_round(cur_render_x + (F32)fgi->mXBearing) +
							fgi->mWidth,
						    ll_round(cur_render_y + (F32)fgi->mYBearing) -
							fgi->mHeight);

		// Note: 6 is the max number of glyphs printed at each loop. HB
		if (glyph_count > GLYPH_BATCH_SIZE - 6)
		{
			gGL.begin(LLRender::TRIANGLES);
			gGL.vertexBatchPreTransformed(vertices, uvs, colors,
										  glyph_count * 6);
			gGL.end();
			glyph_count = 0;
		}

		bool is_grey_scale = bitmap_entry->first == EFontGlyphType::Grayscale;
		if (!is_grey_scale)
		{
			use_colors = true;
		}
		const LLColor4U& col = is_grey_scale ? text_color : emoji_color;
		drawGlyph(glyph_count, vertices, uvs, colors, screen_rect, uv_rect,
				  col, style, drop_shadow_strength);

		++chars_drawn;
		cur_x += fgi->mXAdvance;
		cur_y += fgi->mYAdvance;

		llwchar next_char = wstr[i + 1];
		if (next_char && next_char < LAST_CHARACTER)
		{
			// Kern this puppy.
			next_glyph = mFontFreetype->getGlyphInfo(next_char, b_type);
			cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
		}

		// Round after kerning. Must do this to cur_x, not just to
		// cur_render_x, otherwise you will squish sub-pixel kerned characters
		// too close together. For example, "CCCCC" looks bad.
		cur_x = ll_round(cur_x);
#if 0
		cur_y = ll_round(cur_y);
#endif

		cur_render_x = cur_x;
		cur_render_y = cur_y;
	}

	if (glyph_count > 0)
	{
		gGL.begin(LLRender::TRIANGLES);
		gGL.vertexBatchPreTransformed(vertices, uvs, colors, glyph_count * 6);
		gGL.end();
	}

	if (right_x)
	{
		*right_x = (cur_x - origin_x) / sScaleX;
	}

	// FIXME: add underline as glyph ?
	if (style & UNDERLINE)
	{
		F32 descender = mFontFreetype->getDescenderHeight();
		unit0->unbind(LLTexUnit::TT_TEXTURE);
		gGL.begin(LLRender::LINES);
		gGL.vertex2f(start_x, cur_y - descender);
		gGL.vertex2f(cur_x, cur_y - descender);
		gGL.end();
	}

	if (draw_ellipses)
	{
		// Recursively render ellipses at end of string; we have already
		// reserved enough room
		static const LLWString elipse = utf8str_to_wstring(std::string("..."));
		newrender(elipse, 0, (cur_x - origin_x) / sScaleX, (F32)y, color, LEFT,
				  valign, style, S32_MAX, max_pixels, right_x, false, false);
	}

	gGL.popUIMatrix();
	gGL.flush();

	++LLFontVertexBuffer::sTotalMisses;

	if (has_color_characters)
	{
		*has_color_characters = use_colors;
	}

	return chars_drawn;
}

S32 LLFontGL::oldrender(const LLWString& wstr, S32 begin_offset, F32 x, F32 y,
						const LLColor4& color, HAlign halign, VAlign valign,
						U8 style, S32 max_chars, S32 max_pixels, F32* right_x,
						bool use_ellipses) const
{
	LL_FAST_TIMER(FTM_RENDER_FONTS_SERIALIZED);

	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->enable(LLTexUnit::TT_TEXTURE);

	S32 scaled_max_pixels =
		max_pixels == S32_MAX ? S32_MAX : llceil((F32)max_pixels * sScaleX);

	// Strip off any style bits that are already accounted for by the font.
	style = (style | mFontDescriptor.getStyle()) & ~mFontFreetype->getStyle();

	F32 drop_shadow_strength = 0.f;
	if (style & HAS_SHADOW)
	{
		F32 luminance;
		color.calcHSL(NULL, NULL, &luminance);
		drop_shadow_strength = clamp_rescale(luminance, 0.35f, 0.6f, 0.f, 1.f);
		if (luminance < 0.35f)
		{
			style = style & NO_SHADOW;
		}
	}

	gGL.pushUIMatrix();
	gGL.loadUIIdentity();

	// Depth translation, so that floating text appears 'in-world' and is
	// correctly occluded.
	gGL.translatef(0.f, 0.f, sCurDepth);

	gGL.color4fv(color.mV);

	S32 length = (S32)wstr.length() - begin_offset;
	if (max_chars != -1 && length > max_chars)
	{
		length = max_chars;
	}

	// Not guaranteed to be set correctly
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	F32 origin_x = floorf(sCurOrigin.mX * sScaleX);
	F32 origin_y = floorf(sCurOrigin.mY * sScaleY);

	F32 cur_x = (F32)x * sScaleX + origin_x;
	// Offset x by horizontal alignment.
	if (halign == RIGHT)
	{
	 	cur_x -= llmin(scaled_max_pixels,
					   ll_roundp(getWidthF32(wstr.c_str(), 0, length) *
								 sScaleX));
	}
	else if (halign == HCENTER)
	{
    	cur_x -= llmin(scaled_max_pixels,
					   ll_roundp(getWidthF32(wstr.c_str(), 0, length) *
								 sScaleX)) / 2;
	}

	F32 cur_y = (F32)y * sScaleY + origin_y;
	// Offset y by vertical alignment.
	if (valign == TOP)
	{
		cur_y -= llceil(mFontFreetype->getAscenderHeight());
	}
	else if (valign == BOTTOM)
	{
		cur_y += llceil(mFontFreetype->getDescenderHeight());
	}
	else if (valign == VCENTER)
	{
		cur_y -= llceil((llceil(mFontFreetype->getAscenderHeight()) -
						 llceil(mFontFreetype->getDescenderHeight())) * 0.5f);
	}

	F32 cur_render_y = cur_y;
	F32 cur_render_x = cur_x;

	F32 start_x = ll_round(cur_x);

	const LLFontBitmapCache* bcachep = mFontFreetype->getFontBitmapCache();

	F32 inv_width = 1.f / bcachep->getBitmapWidth();
	F32 inv_height = 1.f / bcachep->getBitmapHeight();

	constexpr S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;

	bool draw_ellipses = false;
	if (use_ellipses && halign == LEFT)
	{
		// Check for too long of a string
		if (getWidthF32(wstr.c_str(), 0,
						max_chars) * sScaleX > scaled_max_pixels)
		{
			// Use four dots for ellipsis width to generate padding
			static const LLWString dots(utf8str_to_wstring(std::string("....")));
			scaled_max_pixels = llmax(0, scaled_max_pixels -
								ll_roundp(getWidthF32(dots.c_str())));
			draw_ellipses = true;
		}
	}

	// Remember last-used texture to avoid unnecesssary bind calls.
	LLImageGL* last_bound_texp = NULL;

	static LLFontGL* label_fontp = getFontSansSerif();
	S32 chars_drawn = 0;
	for (S32 i = begin_offset; i < begin_offset + length; ++i)
	{
		llwchar wch = wstr[i];

		// Handle embedded characters first, if they are enabled. Embedded
		// characters are a hack for notecards
		const embedded_data_t* ext_data = getEmbeddedCharData(wch);
		if (ext_data)
		{
			LLImageGL* ext_image = ext_data->mImage;
			const LLWString& label = ext_data->mLabel;

			F32 ext_height = (F32)ext_image->getHeight() * sScaleY;

			F32 image_width = ext_image->getWidth();
			F32 ext_width = image_width * sScaleX;
			F32 ext_advance = (EXT_X_BEARING * sScaleX) + ext_width;

			if (!label.empty())
			{
				ext_advance += (EXT_X_BEARING +
								label_fontp->getWidthF32(label.c_str())) *
							   sScaleX;
			}

			if (start_x + scaled_max_pixels < cur_x + ext_advance)
			{
				// Not enough room for this character.
				break;
			}

			if (last_bound_texp != ext_image)
			{
				unit0->bind(ext_image);
				last_bound_texp = ext_image;
			}

			// Snap origin to whole screen pixel
			const F32 ext_x = ll_round(cur_render_x + EXT_X_BEARING * sScaleX);
			const F32 ext_y = ll_round(cur_render_y + EXT_Y_BEARING * sScaleY +
									   mFontFreetype->getAscenderHeight() -
									   mFontFreetype->getLineHeight());

			LLRectf uv_rect(0.f, 1.f, 1.f, 0.f);
			LLRectf screen_rect(ext_x, ext_y + ext_height,
								ext_x + ext_width, ext_y);
			drawGlyph(screen_rect, uv_rect, LLColor4::white, style,
					  drop_shadow_strength);

			if (!label.empty())
			{
				gGL.pushMatrix();
				label_fontp->render(label, 0,
									ext_x / sScaleX + image_width +
									EXT_X_BEARING - sCurOrigin.mX,
									cur_render_y / sScaleY - sCurOrigin.mY,
									color, halign, BASELINE, NORMAL, S32_MAX,
									S32_MAX, NULL, true);
				gGL.popMatrix();
			}

			gGL.color4fv(color.mV);

			++chars_drawn;
			cur_x += ext_advance;
			if (i + 1 < length && wstr[i + 1])
			{
				cur_x += EXT_KERNING * sScaleX;
			}
			cur_render_x = cur_x;
		}
		else
		{
			const LLFontGlyphInfo* fgi = mFontFreetype->getGlyphInfo(wch);
			if (!fgi)
			{
				llerrs << "Missing glyph info" << llendl;
				break;
			}
			// Per-glyph bitmap texture.
			LLImageGL* glimagep =
				bcachep->getImageGL(fgi->mBitmapEntry.first,
									fgi->mBitmapEntry.second);
			if (last_bound_texp != glimagep)
			{
				unit0->bind(glimagep);
				last_bound_texp = glimagep;
			}

			if (start_x + scaled_max_pixels <
					cur_x + fgi->mXBearing + fgi->mWidth)
			{
				// Not enough room for this character.
				break;
			}

			// Draw the text at the appropriate location
			// Specify vertices and texture coordinates
			LLRectf uv_rect((fgi->mXBitmapOffset) * inv_width,
							(fgi->mYBitmapOffset + fgi->mHeight + PAD_UVY) *
							inv_height,
							(fgi->mXBitmapOffset + fgi->mWidth) * inv_width,
							(fgi->mYBitmapOffset - PAD_UVY) * inv_height);
			// Snap glyph origin to whole screen pixel
			LLRectf screen_rect(ll_round(cur_render_x + (F32)fgi->mXBearing),
								ll_round(cur_render_y + (F32)fgi->mYBearing),
								ll_round(cur_render_x + (F32)fgi->mXBearing) +
								(F32)fgi->mWidth,
								ll_round(cur_render_y + (F32)fgi->mYBearing) -
								(F32)fgi->mHeight);

			drawGlyph(screen_rect, uv_rect, color, style,
					  drop_shadow_strength);

			++chars_drawn;
			cur_x += fgi->mXAdvance;
			cur_y += fgi->mYAdvance;

			llwchar next_char = wstr[i + 1];
			if (next_char && next_char < LAST_CHARACTER)
			{
				// Kern this puppy.
				const LLFontGlyphInfo* next_glyph =
					mFontFreetype->getGlyphInfo(next_char);
				cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
			}

			// Round after kerning. Must do this to cur_x, not just to
			// cur_render_x, otherwise you will squish sub-pixel kerned
			// characters too close together. For example, "CCCCC" looks bad.
			cur_x = ll_round(cur_x);
#if 0
			cur_y = ll_round(cur_y);
#endif

			cur_render_x = cur_x;
			cur_render_y = cur_y;
		}
	}

	if (right_x)
	{
		*right_x = (cur_x - origin_x) / sScaleX;
	}

	if (style & UNDERLINE)
	{
		unit0->unbind(LLTexUnit::TT_TEXTURE);
		gGL.begin(LLRender::LINES);
		F32 descender = mFontFreetype->getDescenderHeight();
		gGL.vertex2f(start_x, cur_y - descender);
		gGL.vertex2f(cur_x, cur_y - descender);
		gGL.end();
	}

	// *FIXME: get this working in all alignment cases, etc.
	if (draw_ellipses)
	{
		// Recursively render ellipses at end of string; we have already
		// reserved enough room
		static const LLWString elipse = utf8str_to_wstring(std::string("..."));
		oldrender(elipse, 0, (cur_x - origin_x) / sScaleX, (F32)y, color, LEFT,
				  valign, style, S32_MAX, max_pixels, right_x, false);
	}

	gGL.popUIMatrix();
	gGL.flush();

	return chars_drawn;
}

S32 LLFontGL::render(const LLWString& text, S32 begin_offset, F32 x, F32 y,
					 const LLColor4& color) const
{
	if (!sDisplayFont || text.empty())
	{
		return text.length();
	}
	gGL.flush();
	return newrender(text, begin_offset, x, y, color, LEFT, BASELINE,
					 NORMAL, S32_MAX, S32_MAX, NULL, false, true);
}

S32 LLFontGL::renderUTF8(const std::string& text, S32 offset, F32 x, F32 y,
						 const LLColor4& color, const HAlign halign,
						 const VAlign valign, U8 style, S32 max_chars,
						 S32 max_pixels, F32* right_x, bool use_ellipses) const
{

	if (!sDisplayFont || text.empty())
	{
		return text.length();
	}
	return newrender(utf8str_to_wstring(text), offset, x, y, color, halign,
					 valign, style, max_chars, max_pixels, right_x,
					 use_ellipses, true);
}

S32 LLFontGL::renderUTF8(const std::string& text, S32 begin_offset, S32 x,
						 S32 y, const LLColor4& color) const
{

	if (!sDisplayFont || text.empty())
	{
		return text.length();
	}
	return newrender(utf8str_to_wstring(text), begin_offset, (F32)x,
					 (F32)y, color, LEFT, BASELINE, NORMAL, S32_MAX,
					 S32_MAX, NULL, false, true);
}

S32 LLFontGL::renderUTF8(const std::string& text, S32 begin_offset, S32 x,
						 S32 y, const LLColor4& color, HAlign halign,
						 VAlign valign, U8 style) const
{

	if (!sDisplayFont || text.empty())
	{
		return text.length();
	}

	return newrender(utf8str_to_wstring(text), begin_offset, (F32)x, (F32)y,
					 color, halign, valign, style, S32_MAX, S32_MAX, NULL,
					 false, true);
}

// font metrics - override for LLFontFreetype that returns units of virtual pixels
F32 LLFontGL::getAscenderHeight() const
{
	return mFontFreetype->getAscenderHeight() / sScaleY;
}

F32 LLFontGL::getDescenderHeight() const
{
	return mFontFreetype->getDescenderHeight() / sScaleY;
}

F32 LLFontGL::getLineHeight() const
{
	return llceil((mFontFreetype->getAscenderHeight() +
				   mFontFreetype->getDescenderHeight()) / sScaleY);
}

S32 LLFontGL::getWidth(const std::string& utf8text) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidth(wtext.c_str(), 0, S32_MAX);
}

S32 LLFontGL::getWidth(const llwchar* wchars) const
{
	return getWidth(wchars, 0, S32_MAX);
}

S32 LLFontGL::getWidth(const std::string& utf8text, S32 begin_offset,
					   S32 max_chars) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidth(wtext.c_str(), begin_offset, max_chars);
}

S32 LLFontGL::getWidth(const llwchar* wchars, S32 begin_offset,
					   S32 max_chars, bool use_embedded) const
{
	F32 width = getWidthF32(wchars, begin_offset, max_chars, use_embedded);
	return ll_roundp(width);
}

F32 LLFontGL::getWidthF32(const std::string& utf8text) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidthF32(wtext.c_str(), 0, S32_MAX);
}

F32 LLFontGL::getWidthF32(const llwchar* wchars) const
{
	return getWidthF32(wchars, 0, S32_MAX);
}

F32 LLFontGL::getWidthF32(const std::string& utf8text, S32 begin_offset,
						  S32 max_chars) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidthF32(wtext.c_str(), begin_offset, max_chars);
}

F32 LLFontGL::getWidthF32(const llwchar* wchars, S32 begin_offset,
						  S32 max_chars, bool use_embedded) const
{
	constexpr S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;

	F32 cur_x = 0;
	const S32 max_index = begin_offset + max_chars;

	if (use_embedded)
	{
		for (S32 i = begin_offset; i < max_index; ++i)
		{
			const llwchar wch = wchars[i];
			if (!wch)
			{
				break; // Done
			}
			const embedded_data_t* ext_data = getEmbeddedCharData(wch);
			if (ext_data)
			{
				// Handle crappy embedded hack
				cur_x += getEmbeddedCharAdvance(ext_data);
				if (i + 1 < max_chars && i + 1 < max_index)
				{
					cur_x += EXT_KERNING * sScaleX;
				}
			}
			else
			{
				cur_x += mFontFreetype->getXAdvance(wch);
				llwchar next_char = wchars[i + 1];
				if (i + 1 < max_chars && next_char && next_char < LAST_CHARACTER)
				{
					// Kern this puppy.
					cur_x += mFontFreetype->getXKerning(wch, next_char);
				}
			}
			// Round after kerning.
			cur_x = (F32)llfloor(cur_x + 0.5f);
		}
		return cur_x / sScaleX;
	}

	for (S32 i = begin_offset; i < max_index; ++i)
	{
		const llwchar wch = wchars[i];
		if (!wch)
		{
			break; // Done
		}
		cur_x += mFontFreetype->getXAdvance(wch);
		llwchar next_char = wchars[i + 1];
		if (i + 1 < max_chars && next_char && next_char < LAST_CHARACTER)
		{
			// Kern this puppy.
			cur_x += mFontFreetype->getXKerning(wch, next_char);
		}
		// Round after kerning.
		cur_x = (F32)llfloor(cur_x + 0.5f);
	}
	return cur_x / sScaleX;
}

void LLFontGL::generateASCIIglyphs()
{
	for (U32 i = 32; i < 127; ++i)
	{
		mFontFreetype->getGlyphInfo(i);
	}
}

// Returns the max number of complete characters from text (up to max_chars)
// that can be drawn in max_pixels
S32 LLFontGL::maxDrawableChars(const llwchar* wchars, F32 max_pixels,
							   S32 max_chars, bool end_on_word_boundary,
							   bool use_embedded, F32* drawn_pixels) const
{
	if (!wchars || !wchars[0] || max_chars <= 0)
	{
		return 0;
	}

	llassert(max_pixels >= 0.f);
	llassert(max_chars >= 0);

	bool clip = false;
	F32 cur_x = 0;
	F32 drawn_x = 0;

	S32 start_of_last_word = 0;
	bool in_word = false;

	F32 scaled_max_pixels =	(F32)llceil(max_pixels * sScaleX);

	S32 i = 0;
	if (use_embedded)
	{
		for (i = 0; i < max_chars; ++i)
		{
			llwchar wch = wchars[i];
			if (!wch)
			{
				break; // Done
			}

			const embedded_data_t* ext_data = getEmbeddedCharData(wch);
			if (ext_data)
			{
				if (in_word)
				{
					in_word = false;
				}
				else
				{
					start_of_last_word = i;
				}
				cur_x += getEmbeddedCharAdvance(ext_data);

				if (scaled_max_pixels < cur_x)
				{
					clip = true;
					break;
				}

				if (i + 1 < max_chars && wchars[i + 1])
				{
					cur_x += EXT_KERNING * sScaleX;
				}

				if (scaled_max_pixels < cur_x)
				{
					clip = true;
					break;
				}
			}
			else
			{
				if (in_word)
				{
					if (iswspace(wch))
					{
						in_word = false;
					}
				}
				else
				{
					start_of_last_word = i;
					if (!iswspace(wch))
					{
						in_word = true;
					}
				}

				cur_x += mFontFreetype->getXAdvance(wch);

				if (scaled_max_pixels < cur_x)
				{
					clip = true;
					break;
				}

				if (i + 1 < max_chars && wchars[i + 1])
				{
					// Kern this puppy.
					cur_x += mFontFreetype->getXKerning(wch, wchars[i + 1]);
				}
			}
			// Round after kerning.
			cur_x = (F32)llfloor(cur_x + 0.5f);
			drawn_x = cur_x;
		}
	}
	else
	{
		for (i = 0; i < max_chars; ++i)
		{
			llwchar wch = wchars[i];
			if (!wch)
			{
				break;	// Done
			}

			if (in_word)
			{
				if (iswspace(wch))
				{
					in_word = false;
				}
			}
			else
			{
				start_of_last_word = i;
				if (!iswspace(wch))
				{
					in_word = true;
				}
			}

			cur_x += mFontFreetype->getXAdvance(wch);

			if (scaled_max_pixels < cur_x)
			{
				clip = true;
				break;
			}

			if (i + 1 < max_chars && wchars[i + 1])
			{
				// Kern this puppy.
				cur_x += mFontFreetype->getXKerning(wch, wchars[i + 1]);
			}
		}
		// Round after kerning.
		cur_x = (F32)llfloor(cur_x + 0.5f);
		drawn_x = cur_x;
	}

	if (clip && end_on_word_boundary && start_of_last_word != 0)
	{
		i = start_of_last_word;
	}
	if (drawn_pixels)
	{
		*drawn_pixels = drawn_x;
	}
	return i;
}

S32	LLFontGL::firstDrawableChar(const llwchar* wchars, F32 max_pixels,
								S32 text_len, S32 start_pos,
								S32 max_chars) const
{
	if (!wchars || !wchars[0] || max_chars <= 0)
	{
		return 0;
	}

	F32 total_width = 0.0;
	S32 drawable_chars = 0;

	F32 scaled_max_pixels =	max_pixels * sScaleX;

	S32 start = llmin(start_pos, text_len - 1);
	for (S32 i = start; i >= 0; --i)
	{
		llwchar wch = wchars[i];

		const embedded_data_t* ext_data = getEmbeddedCharData(wch);
		F32 char_width = ext_data ? getEmbeddedCharAdvance(ext_data)
								  : mFontFreetype->getXAdvance(wch);

		if (scaled_max_pixels < total_width + char_width)
		{
			break;
		}

		total_width += char_width;
		++drawable_chars;

		if (max_chars >= 0 && drawable_chars >= max_chars)
		{
			break;
		}

		if (i > 0)
		{
			// Kerning
			total_width += ext_data ? EXT_KERNING * sScaleX
									: mFontFreetype->getXKerning(wchars[i - 1], wch);
		}

		// Round after kerning.
		total_width = ll_roundp(total_width);
	}

	return start_pos - drawable_chars;
}

S32 LLFontGL::charFromPixelOffset(const llwchar* wchars, S32 begin_offset,
								  F32 target_x, F32 max_pixels, S32 max_chars,
								  bool round, bool use_embedded) const
{
	if (!wchars || !wchars[0] || max_chars == 0)
	{
		return 0;
	}

	F32 cur_x = 0;
	S32 pos = 0;

	target_x *= sScaleX;

	// max_chars is S32_MAX by default, so make sure we do not get overflow
	const S32 max_index = begin_offset +
						  llmin(S32_MAX - begin_offset, max_chars);

	F32 scaled_max_pixels =	max_pixels * sScaleX;

	if (use_embedded)
	{
		for (S32 i = begin_offset; i < max_index; ++i)
		{
			llwchar wch = wchars[i];
			if (!wch)
			{
				break; // Done
			}
			const embedded_data_t* ext_data = getEmbeddedCharData(wch);
			if (ext_data)
			{
				F32 ext_advance = getEmbeddedCharAdvance(ext_data);

				if (round)
				{
					// Note: if the mouse is on the left half of the character,
					// the pick is to the character's left. If it is on the
					// right half, the pick is to the right.
					if (target_x  < cur_x + ext_advance * 0.5f)
					{
						break;
					}
				}
				else if (target_x  < cur_x + ext_advance)
				{
					break;
				}

				if (scaled_max_pixels < cur_x + ext_advance)
				{
					break;
				}

				++pos;
				cur_x += ext_advance;

				if (i + 1 < max_index && wchars[i + 1])
				{
					cur_x += EXT_KERNING * sScaleX;
				}
				// Round after kerning.
				cur_x = (F32)llfloor(cur_x + 0.5f);
			}
			else
			{
				F32 char_width = mFontFreetype->getXAdvance(wch);

				if (round)
				{
					// Note: if the mouse is on the left half of the character,
					// the pick is to the character's left. If it is on the
					// right half, the pick is to the right.
					if (target_x  < cur_x + char_width * 0.5f)
					{
						break;
					}
				}
				else if (target_x  < cur_x + char_width)
				{
					break;
				}

				if (scaled_max_pixels < cur_x + char_width)
				{
					break;
				}

				++pos;
				cur_x += char_width;

				if (i + 1 < max_index && wchars[i + 1])
				{
					llwchar next_char = wchars[i + 1];
					// Kern this puppy.
					cur_x += mFontFreetype->getXKerning(wch, next_char);
				}

				// Round after kerning.
				cur_x = (F32)llfloor(cur_x + 0.5f);
			}
		}
		return pos;
	}

	for (S32 i = begin_offset; i < max_index; ++i)
	{
		llwchar wch = wchars[i];
		if (!wch)
		{
			break; // Done
		}

		F32 char_width = mFontFreetype->getXAdvance(wch);

		if (round)
		{
			// Note: if the mouse is on the left half of the character, the
			// pick is to the character's left. If it is on the right half,
			// the pick is to the right.
			if (target_x  < cur_x + char_width * 0.5f)
			{
				break;
			}
		}
		else if (target_x  < cur_x + char_width)
		{
			break;
		}

		if (scaled_max_pixels < cur_x + char_width)
		{
			break;
		}

		++pos;
		cur_x += char_width;

		if (i + 1 < max_index && wchars[i + 1])
		{
			llwchar next_char = wchars[i + 1];
			// Kern this puppy.
			cur_x += mFontFreetype->getXKerning(wch, next_char);
		}

		// Round after kerning.
		cur_x = (F32)llfloor(cur_x + 0.5f);
	}
	return pos;
}

//static
void LLFontGL::initClass(F32 screen_dpi, F32 x_scale, F32 y_scale,
						 const std::vector<std::string>& xui_paths,
						 bool create_gl_textures)
{
	// This must be incremented each time the DPI or scales change. HB
	++sResGeneration;
	sVertDPI = (F32)llfloor(screen_dpi * y_scale);
	sHorizDPI = (F32)llfloor(screen_dpi * x_scale);
	sScaleX = x_scale;
	sScaleY = y_scale;

	// Font registry init
	if (sFontRegistry)
	{
		sFontRegistry->reset();
	}
	else
	{
		sFontRegistry = new LLFontRegistry(xui_paths, create_gl_textures);
		sFontRegistry->parseFontInfo("fonts.xml");
	}

	// Allow font vertex buffer usage.
	LLFontVertexBuffer::sStopped = false;

	// Force-load all default fonts (can take time, so best done on init).
	if (!getFontSansSerifSmall())
	{
		llwarns << "Failed to load font: SansSerifSmall" << llendl;
	}
	if (!getFontSansSerif())
	{
		llwarns << "Failed to load font: SansSerif" << llendl;
	}
	if (!getFontSansSerifLarge())
	{
		llwarns << "Failed to load font: SansSerifLarge" << llendl;
	}
	if (!getFontSansSerifHuge())
	{
		llwarns << "Failed to load font: SansSerifHuge" << llendl;
	}
	if (!getFontSansSerifBold())
	{
		llwarns << "Failed to load font: SansSerifBold" << llendl;
	}
	if (!getFontMonospace())
	{
		llwarns << "Failed to load font: Monospace" << llendl;
	}
	if (!getFontEmoji())
	{
		llwarns << "Failed to load font: Emoji" << llendl;
	}
}

//static
void LLFontGL::destroyAllGL()
{
	LLFontVertexBuffer::destroyGL();
	if (sFontRegistry)
	{
		sFontRegistry->destroyGL();
	}
}

//static
U8 LLFontGL::getStyleFromString(const std::string& style)
{
	S32 ret = 0;
	if (style.find("BOLD") != std::string::npos)
	{
		ret |= BOLD;
	}
	if (style.find("ITALIC") != std::string::npos)
	{
		ret |= ITALIC;
	}
	if (style.find("UNDERLINE") != std::string::npos)
	{
		ret |= UNDERLINE;
	}
	if (style.find("SHADOW") != std::string::npos)
	{
		ret |= DROP_SHADOW;
	}
	if (style.find("SOFT_SHADOW") != std::string::npos)
	{
		ret |= DROP_SHADOW_SOFT;
	}
	return ret;
}

//static
std::string LLFontGL::nameFromFont(const LLFontGL* fontp)
{
	return fontp->getFontDesc().getName();
}

//static
const std::string& LLFontGL::nameFromHAlign(HAlign align)
{
	static const std::string& hleft("left");
	static const std::string& hright("right");
	static const std::string& hcenter("center");

	switch (align)
	{
		case LEFT:
			return hleft;

		case RIGHT:
			return hright;

		case HCENTER:
			return hcenter;

		default:
			return LLStringUtil::null;
	}
}

//static
LLFontGL::HAlign LLFontGL::hAlignFromName(const std::string& name)
{
	if (name == "right")
	{
		return LLFontGL::RIGHT;
	}
	if (name == "center")
	{
		return LLFontGL::HCENTER;
	}
	return LLFontGL::LEFT;
}

//static
const std::string& LLFontGL::nameFromVAlign(LLFontGL::VAlign align)
{
	static const std::string& vtop("top");
	static const std::string& vcenter("center");
	static const std::string& vbaseline("baseline");
	static const std::string& vbottom("bottom");

	switch (align)
	{
		case TOP:
			return vtop;

		case VCENTER:
			return vcenter;

		case BASELINE:
			return vbaseline;

		case BOTTOM:
			return vbottom;

		default:
			return LLStringUtil::null;
	}
}

//static
LLFontGL::VAlign LLFontGL::vAlignFromName(const std::string& name)
{
	if (name == "top")
	{
		return LLFontGL::TOP;
	}
	if (name == "center")
	{
		return LLFontGL::VCENTER;
	}
	if (name == "bottom")
	{
		return LLFontGL::BOTTOM;
	}
	return LLFontGL::BASELINE;
}

//static
LLFontGL* LLFontGL::getFontMonospace()
{
	static const LLFontDescriptor desc("Monospace", "Monospace");
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFontSansSerifSmall()
{
	static const LLFontDescriptor desc("SansSerif", "Small");
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFontSansSerif()
{
	static const LLFontDescriptor desc("SansSerif", "Medium");
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFontSansSerifLarge()
{
	static const LLFontDescriptor desc("SansSerif", "Large");
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFontSansSerifHuge()
{
	static const LLFontDescriptor desc("SansSerif", "Huge");
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFontSansSerifBold()
{
	static const LLFontDescriptor desc("SansSerif", "Medium", BOLD);
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFontEmoji()
{
	static const LLFontDescriptor desc("Emoji", "Large");
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFont(const LLFontDescriptor& desc, bool normalize)
{
	return sFontRegistry->getFont(desc, normalize);
}

//static
LLFontGL* LLFontGL::getFont(const char* name, const char* size, U8 style)
{
	if (!name || !*name)
	{
		return NULL;
	}
	const LLFontDescriptor desc(name, size ? size : "Medium", style);
	LLFontGL* fontp = getFont(desc, true);
	if (fontp)
	{
		return fontp;
	}
	return getFont(desc, false);
}

//static
LLFontGL* LLFontGL::getFont(const std::string& name)
{
	// Check for most common fonts first
	if (name.empty())
	{
		return NULL;
	}
	if (name == "SANSSERIF")
	{
		return getFontSansSerif();
	}
	if (name == "SANSSERIF_SMALL")
	{
		return getFontSansSerifSmall();
	}
	if (name == "SMALL" || name == "MONOSPACE")
	{
		return getFontMonospace();
	}
	if (name == "SANSSERIF_LARGE" || name == "SANSSERIF_BIG")
	{
		return getFontSansSerifLarge();
	}
	if (name == "EMOJI")
	{
		return getFontEmoji();
	}

	llwarns << "Unknown font specification: " << name << llendl;
	return NULL;
}

//static
LLFontGL* LLFontGL::getFont(S32 font_id)
{
	static std::vector<LLFontGL*> fonts_cache;
	if (fonts_cache.empty())
	{
		// IMPORTANT: must be pushed in LLFONT_ID order !
		fonts_cache.push_back(getFontSansSerif());
		fonts_cache.push_back(getFontSansSerifSmall());
		fonts_cache.push_back(getFontSansSerifLarge());
		fonts_cache.push_back(getFontMonospace());
		fonts_cache.push_back(getFontEmoji());
	}

	if (font_id >= 0 && (size_t)font_id < fonts_cache.size())
	{
		return fonts_cache[font_id];
	}

	llwarns << "Unknown font Id: " << font_id << ". Expect a crash !"
			<< llendl;
	llassert(false);

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// These are the new, optimized routines to use for texts without embbeded
// items.
///////////////////////////////////////////////////////////////////////////////

void LLFontGL::renderQuad(LLVector4a* vertex_out, LLVector2* uv_out,
						  LLColor4U* colors_out,
						  const LLRectf& screen_rect, const LLRectf& uv_rect,
						  const LLColor4U& color, F32 slant_amt) const
{
	S32 index = 0;

	vertex_out[index].set(screen_rect.mLeft, screen_rect.mTop, 0.f);
	uv_out[index].set(uv_rect.mLeft, uv_rect.mTop);
	colors_out[index++] = color;

	vertex_out[index].set(screen_rect.mLeft + slant_amt, screen_rect.mBottom,
						  0.f);
	uv_out[index].set(uv_rect.mLeft, uv_rect.mBottom);
	colors_out[index++] = color;

	vertex_out[index].set(screen_rect.mRight, screen_rect.mTop, 0.f);
	uv_out[index].set(uv_rect.mRight, uv_rect.mTop);
	colors_out[index++] = color;

	vertex_out[index].set(screen_rect.mRight, screen_rect.mTop, 0.f);
	uv_out[index].set(uv_rect.mRight, uv_rect.mTop);
	colors_out[index++] = color;

	vertex_out[index].set(screen_rect.mLeft + slant_amt, screen_rect.mBottom,
						  0.f);
	uv_out[index].set(uv_rect.mLeft, uv_rect.mBottom);
	colors_out[index++] = color;

	vertex_out[index].set(screen_rect.mRight + slant_amt, screen_rect.mBottom,
						  0.f);
	uv_out[index].set(uv_rect.mRight, uv_rect.mBottom);
	colors_out[index] = color;
}

void LLFontGL::drawGlyph(S32& glyph_count, LLVector4a* vertex_out,
						 LLVector2* uv_out, LLColor4U* colors_out,
						 const LLRectf& screen_rect, const LLRectf& uv_rect,
						 const LLColor4U& color, U8 style,
						 F32 drop_shadow_strength) const
{
	F32 slant_offset = 0.f;
	if (style & ITALIC)
	{
		slant_offset = -mFontFreetype->getAscenderHeight() * 0.2f;
	}

	// *FIXME: bold and drop shadow are mutually exclusive only for
	// convenience. Allow both when we need them.
	if (style & BOLD)
	{
		for (S32 pass = 0; pass < 2; ++pass)
		{
			LLRectf screen_rect_offset = screen_rect;
			const U32 idx = glyph_count * 6;
			screen_rect_offset.translate((F32)(pass * BOLD_OFFSET), 0.f);
			renderQuad(&vertex_out[idx], &uv_out[idx], &colors_out[idx],
					   screen_rect_offset, uv_rect, color, slant_offset);
			++glyph_count;
		}
	}
	else if (style & DROP_SHADOW_SOFT)
	{
		LLColor4U shadow_color = sShadowColorU;
		shadow_color.mV[VALPHA] = U8(color.mV[VALPHA] * drop_shadow_strength *
									 DROP_SHADOW_SOFT_STRENGTH);
		for (S32 pass = 0; pass < 5; ++pass)
		{
			LLRectf screen_rect_offset = screen_rect;

			switch (pass)
			{
				case 0:
					screen_rect_offset.translate(-1.f, -1.f);
					break;

				case 1:
					screen_rect_offset.translate(1.f, -1.f);
					break;

				case 2:
					screen_rect_offset.translate(1.f, 1.f);
					break;

				case 3:
					screen_rect_offset.translate(-1.f, 1.f);
					break;

				case 4:
					screen_rect_offset.translate(0, -2.f);
			}

			const U32 idx = glyph_count * 6;
			renderQuad(&vertex_out[idx], &uv_out[idx], &colors_out[idx],
					   screen_rect_offset, uv_rect, shadow_color,
					   slant_offset);
			++glyph_count;
		}
		const U32 idx = glyph_count * 6;
		renderQuad(&vertex_out[idx], &uv_out[idx], &colors_out[idx],
				   screen_rect, uv_rect, color, slant_offset);
		++glyph_count;
	}
	else if (style & DROP_SHADOW)
	{
		LLColor4U shadow_color = sShadowColorU;
		shadow_color.mV[VALPHA] = U8(color.mV[VALPHA] * drop_shadow_strength);
		LLRectf screen_rect_shadow = screen_rect;
		screen_rect_shadow.translate(1.f, -1.f);
		U32 idx = glyph_count * 6;
		renderQuad(&vertex_out[idx], &uv_out[idx], &colors_out[idx],
				   screen_rect_shadow, uv_rect, shadow_color, slant_offset);
		idx = ++glyph_count * 6;
		renderQuad(&vertex_out[idx], &uv_out[idx], &colors_out[idx],
				   screen_rect, uv_rect, color, slant_offset);
		++glyph_count;
	}
	else // Normal rendering
	{
		const U32 idx = glyph_count * 6;
		renderQuad(&vertex_out[idx], &uv_out[idx], &colors_out[idx],
				   screen_rect, uv_rect, color, slant_offset);
		++glyph_count;
	}
}

///////////////////////////////////////////////////////////////////////////////
// These are the old, slower routines to use for texts with embbeded items.
///////////////////////////////////////////////////////////////////////////////

void LLFontGL::renderQuad(const LLRectf& screen_rect, const LLRectf& uv_rect,
						  F32 slant_amt) const
{
	gGL.texCoord2f(uv_rect.mLeft, uv_rect.mTop);
	gGL.vertex2f(screen_rect.mLeft, screen_rect.mTop);

	gGL.texCoord2f(uv_rect.mLeft, uv_rect.mBottom);
	gGL.vertex2f(screen_rect.mLeft + slant_amt, screen_rect.mBottom);

	gGL.texCoord2f(uv_rect.mRight, uv_rect.mTop);
	gGL.vertex2f(screen_rect.mRight,  screen_rect.mTop);

	gGL.texCoord2f(uv_rect.mRight, uv_rect.mTop);
	gGL.vertex2f(screen_rect.mRight,  screen_rect.mTop);

	gGL.texCoord2f(uv_rect.mLeft, uv_rect.mBottom);
	gGL.vertex2f(screen_rect.mLeft + slant_amt, screen_rect.mBottom);

	gGL.texCoord2f(uv_rect.mRight, uv_rect.mBottom);
	gGL.vertex2f(screen_rect.mRight + slant_amt, screen_rect.mBottom);
}

void LLFontGL::drawGlyph(const LLRectf& screen_rect, const LLRectf& uv_rect,
						 const LLColor4& color, U8 style,
						 F32 drop_shadow_strength) const
{
	F32 slant_offset = 0.f;
	if (style & ITALIC)
	{
		slant_offset = -mFontFreetype->getAscenderHeight() * 0.2f;
	}

	gGL.begin(LLRender::TRIANGLES);

	// *FIXME: bold and drop shadow are mutually exclusive only for
	// convenience. Allow both when we need them.
	if (style & BOLD)
	{
		gGL.color4fv(color.mV);
		for (S32 pass = 0; pass < 2; ++pass)
		{
			LLRectf screen_rect_offset = screen_rect;
			screen_rect_offset.translate((F32)(pass * BOLD_OFFSET), 0.f);
			renderQuad(screen_rect_offset, uv_rect, slant_offset);
		}
	}
	else if (style & DROP_SHADOW_SOFT)
	{
		LLColor4 shadow_color = sShadowColor;
		shadow_color.mV[VALPHA] = color.mV[VALPHA] * drop_shadow_strength *
								  DROP_SHADOW_SOFT_STRENGTH;
		gGL.color4fv(shadow_color.mV);
		for (S32 pass = 0; pass < 5; ++pass)
		{
			LLRectf screen_rect_offset = screen_rect;

			switch (pass)
			{
				case 0:
					screen_rect_offset.translate(-1.f, -1.f);
					break;

				case 1:
					screen_rect_offset.translate(1.f, -1.f);
					break;

				case 2:
					screen_rect_offset.translate(1.f, 1.f);
					break;

				case 3:
					screen_rect_offset.translate(-1.f, 1.f);
					break;

				case 4:
					screen_rect_offset.translate(0, -2.f);
			}

			renderQuad(screen_rect_offset, uv_rect, slant_offset);
		}
		gGL.color4fv(color.mV);
		renderQuad(screen_rect, uv_rect, slant_offset);
	}
	else if (style & DROP_SHADOW)
	{
		LLColor4 shadow_color = sShadowColor;
		shadow_color.mV[VALPHA] = color.mV[VALPHA] * drop_shadow_strength;
		gGL.color4fv(shadow_color.mV);
		LLRectf screen_rect_shadow = screen_rect;
		screen_rect_shadow.translate(1.f, -1.f);
		renderQuad(screen_rect_shadow, uv_rect, slant_offset);
		gGL.color4fv(color.mV);
		renderQuad(screen_rect, uv_rect, slant_offset);
	}
	else	// Normal rendering
	{
		gGL.color4fv(color.mV);
		renderQuad(screen_rect, uv_rect, slant_offset);
	}
	gGL.end();
}

const LLFontGL::embedded_data_t* LLFontGL::getEmbeddedCharData(llwchar wch) const
{
	// Handle crappy embedded hack
	embedded_map_t::const_iterator iter = mEmbeddedChars.find(wch);
	return iter != mEmbeddedChars.end() ? &iter->second : NULL;
}

F32 LLFontGL::getEmbeddedCharAdvance(const embedded_data_t* ext_data) const
{
	const LLWString& label = ext_data->mLabel;
	LLImageGL* ext_image = ext_data->mImage;

	F32 ext_width = (F32)ext_image->getWidth();
	if (!label.empty())
	{
		ext_width += (EXT_X_BEARING +
					  getFontSansSerif()->getWidthF32(label.c_str())) *
					 sScaleX;
	}

	return EXT_X_BEARING * sScaleX + ext_width;
}

void LLFontGL::addEmbeddedChar(llwchar wc, LLGLTexture* image,
							   const std::string& label) const
{
	mEmbeddedChars.emplace(std::piecewise_construct,
						   std::forward_as_tuple(wc),
						   std::forward_as_tuple(image->getGLImage(),
												 utf8str_to_wstring(label)));
}

void LLFontGL::addEmbeddedChar(llwchar wc, LLGLTexture* image,
							   const LLWString& wlabel) const
{
	mEmbeddedChars.emplace(std::piecewise_construct,
						   std::forward_as_tuple(wc),
						   std::forward_as_tuple(image->getGLImage(), wlabel));
}

void LLFontGL::removeEmbeddedChar(llwchar wc) const
{
	mEmbeddedChars.erase(wc);
}

///////////////////////////////////////////////////////////////////////////////
// LLFontVertexBuffer class
///////////////////////////////////////////////////////////////////////////////

//static
U64 LLFontVertexBuffer::sTotalHits = 0;
U64 LLFontVertexBuffer::sTotalMisses = 0;
bool LLFontVertexBuffer::sEnabled = false;
bool LLFontVertexBuffer::sStopped = true;

static fast_hset<LLFontVertexBuffer*> sFontVBInstances;

LLFontVertexBuffer::LLFontVertexBuffer()
:	mLastFont(NULL),
	mLastResGeneration(0),
	mLastFontCacheGen(0),
	mChars(0),
	mLastOffset(0),
	mLastMaxChars(0),
	mLastMaxPixels(0),
	mLastRightX(0),
	mLastX(0.f),
	mLastY(0.f),
	mLastHalign(LLFontGL::LEFT),
	mLastValign(LLFontGL::BASELINE),
	mLastStyle(LLFontGL::NORMAL),
	mFirstRender(true)
{
	sFontVBInstances.insert(this);
}

LLFontVertexBuffer::~LLFontVertexBuffer()
{
	sFontVBInstances.erase(this);
}

void LLFontVertexBuffer::reset()
{
	mBufferList.clear();
}

S32 LLFontVertexBuffer::render(const LLFontGL* fontp, const LLWString& text,
							   S32 begin_offset, LLRect rect,
							   const LLColor4& color, LLFontGL::HAlign halign,
							   LLFontGL::VAlign valign, U8 style,
							   S32 max_chars, F32* right_x, bool use_ellipses,
							   bool use_color)
{
	LLRectf rectf((F32)rect.mLeft, (F32)rect.mTop, (F32)rect.mRight,
				  (F32)rect.mBottom);
	return render(fontp, text, begin_offset, rectf, color, halign, valign,
				  style, max_chars, right_x, use_ellipses, use_color);
}

S32 LLFontVertexBuffer::render(const LLFontGL* fontp, const LLWString& text,
							   S32 begin_offset, LLRectf rect,
							   const LLColor4& color, LLFontGL::HAlign halign,
							   LLFontGL::VAlign valign, U8 style,
							   S32 max_chars, F32* right_x, bool use_ellipses,
							   bool use_color)
{
	F32 x = rect.mLeft;
	F32 y;
	switch (valign)
	{
		case LLFontGL::TOP:		y = rect.mTop;			break;
		case LLFontGL::VCENTER:	y = rect.getCenterY();	break;
		default:				y = rect.mBottom;
	}
	return render(fontp, text, begin_offset, x, y, color, halign, valign,
				  style, max_chars, (S32)rect.getWidth(), right_x,
				  use_ellipses, use_color);
}

S32 LLFontVertexBuffer::render(const LLFontGL* fontp, const LLWString& text,
							   S32 begin_offset, F32 x, F32 y,
							   const LLColor4& color, LLFontGL::HAlign halign,
							   LLFontGL::VAlign valign, U8 style,
							   S32 max_chars, S32 max_pixels, F32* right_x,
							   bool use_ellipses, bool use_color)
{
	if (!fontp || text.empty())
	{
		llassert(fontp);
		return 0;
	}
	if (!LLFontGL::sDisplayFont || sStopped)
	{
		return text.length();
	}

	// See if we can actually use vertex buffer based rendering. If not, fall
	// back to normal font rendering. HB
	if (!sEnabled || !LLGLSLShader::sCurBoundShaderPtr ||
		(!LLRender::sUseBufferCache && !gUsePBRShaders))
	{
		return fontp->newrender(text, begin_offset, x, y, color, halign,
								valign, style, max_chars, max_pixels,
								right_x, use_ellipses, use_color);
	}

	bool needs_render = mBufferList.empty() || x != mLastX || y != mLastY ||
						begin_offset != mLastOffset || color != mLastColor ||
						style != mLastStyle || fontp != mLastFont ||
						// Adding new characters to bitmap cache can alter
						// value from getBitmapWidth() which alters whole
						// string. So rerender when new characters were added
						// to the cache.
						fontp->getCacheGeneration() != mLastFontCacheGen ||
						halign != mLastHalign || valign != mLastValign ||
						max_chars != mLastMaxChars ||
						max_pixels != mLastMaxPixels ||
						LLFontGL::sResGeneration != mLastResGeneration ||
						LLFontGL::sCurOrigin != mLastOrigin;
	// Note: to work around issues with bad rendering at the first pass when
	// mixed fonts (e.g. SansSerif + Emoji fonts) are used in the same line of
	// text, or even sometimes when a *single* emoji is used, we do two render
	// passes when emojis are present. mFirstRender is used for this; it is
	// reset to false by genBuffers() if no emoji has been printed.
	// *TODO: find the root cause for this render glitch instead... HB
	if (needs_render || mFirstRender)
	{
		mFirstRender = needs_render;
		genBuffers(fontp, text, begin_offset, x, y, color, halign, valign,
				   style, max_chars, max_pixels, right_x, use_ellipses,
				   use_color);
	}
	else
	{
		renderBuffers();
		if (right_x)
		{
			*right_x = mLastRightX;
		}
	}
	return mChars;
}

void LLFontVertexBuffer::genBuffers(const LLFontGL* fontp,
									const LLWString& text, S32 begin_offset,
									F32 x, F32 y, const LLColor4& color,
									LLFontGL::HAlign halign,
									LLFontGL::VAlign valign, U8 style,
									S32 max_chars, S32 max_pixels,
									F32* right_x, bool use_ellipses,
									bool use_color)
{
	mBufferList.clear();
	// Save before rendreing, it can change mid-render, so will need to re-
	// render previous characters.
	mLastFontCacheGen = fontp->getCacheGeneration();
	gGL.beginList(mBufferList);
	bool has_color_characters = false;
	mChars = fontp->newrender(text, begin_offset, x, y, color, halign, valign,
							  style, max_chars, max_pixels, right_x,
							  use_ellipses, use_color, &has_color_characters);
	gGL.endList();
	// If the text does not use color fonts (i.e. does not have emojis), we do
	// not need the double-render workaround. HB
	if (!has_color_characters)
	{
		mFirstRender = false;
	}
	mLastFont = fontp;
	mLastOffset = begin_offset;
	mLastMaxChars = max_chars;
	mLastMaxPixels = max_pixels;
	mLastX = x;
	mLastY = y;
	mLastColor = color;
	mLastHalign = halign;
	mLastValign = valign;
	mLastStyle = style;
	mLastResGeneration = LLFontGL::sResGeneration;
	mLastOrigin = LLFontGL::sCurOrigin;
	if (right_x)
	{
		 mLastRightX = *right_x;
	}
}

void LLFontVertexBuffer::renderBuffers()
{
	gGL.flush();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
	gGL.pushUIMatrix();
	gGL.loadUIIdentity();

	// Depth translation, so that floating text can be used 'in-world', with
	// proper occlusion.
	gGL.translatef(0.f, 0.f, LLFontGL::sCurDepth);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	// Note: ellipses should technically be covered by push/load/translate of
	// their own but it adds more complexity, values do not change, skipping
	// does not appear to break anything, so we can skip that until it proves
	// to cause issues.

	for (LLVertexBufferData& buffer : mBufferList)
	{
		buffer.draw();
	}

	gGL.popUIMatrix();

	++sTotalHits;
}

//static
void LLFontVertexBuffer::destroyGL()
{
	sStopped = true;
	for (auto instp : sFontVBInstances)
	{
		instp->reset();
	}
}

//static
void LLFontVertexBuffer::reportHitRate()
{
	if (!sTotalMisses)
	{
		return;
	}
	F64 rate = F64(sTotalHits) / F64(sTotalHits + sTotalMisses) * 100.0;
	llinfos << "Font vertex buffer cache rendering hit rate: " << (S32)rate
			<< "%" << llendl;
}
