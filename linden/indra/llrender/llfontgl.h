/**
 * @file llfontgl.h
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

#pragma once

#include "llcoord.h"
#include "hbfastmap.h"
#include "llfontregistry.h"
#include "llimagegl.h"
#include "llvertexbuffer.h"

class LLColor4;
class LLFontDescriptor;
class LLFontFreetype;
class LLGLTexture;
class LLVector4a;

// Structure used to store previously requested fonts.
class LLFontRegistry;

// IMPORTANT: if you change this, also change LLFontGL::getFont() accordingly !
enum LLFONT_ID
{
	LLFONT_SANSSERIF,
	LLFONT_SANSSERIF_SMALL,
	LLFONT_SANSSERIF_LARGE,
	LLFONT_SMALL,
	LLFONT_EMOJI,
};

class LLFontGL
{
	friend class LLFontRegistry;
	friend class LLFontVertexBuffer;
	friend class LLHUDText;

protected:
	LOG_CLASS(LLFontGL);

public:
	enum HAlign : U32
	{
		// Horizontal location of x, y coord to render.
		LEFT = 0,		// Left align
		RIGHT = 1,		// Right align
		HCENTER = 2,	// Center
	};

	enum VAlign : U32
	{
		// Vertical location of x, y coord to render.
		TOP = 3,		// Top align
		VCENTER = 4,	// Center
		BASELINE = 5,	// Baseline
		BOTTOM = 6		// Bottom
	};

	enum StyleFlags : U32
	{
		// Text style to render. May be combined (these are bit flags)
		NORMAL = 0,
		BOLD = 1,
		ITALIC = 2,
		UNDERLINE = 4,
		DROP_SHADOW = 8,
		DROP_SHADOW_SOFT = 16
	};

	// Takes a string with potentially several flags, i.e. "NORMAL|BOLD|ITALIC"
	static U8 getStyleFromString(const std::string& style);

	LLFontGL() = default;
	~LLFontGL();

	LLFontGL(const LLFontGL&) = delete;
	LLFontGL& operator=(const LLFontGL&) = delete;

	void init(); // Internal init, or reinitialization

	// Reset a font after GL cleanup. ONLY works on an already loaded font.
	void reset();

	void destroyGL();

	bool loadFace(const std::string& filename, F32 point_size,
				  F32 vert_dpi, F32 horz_dpi, bool is_fallback);

	S32 render(const LLWString& text, S32 begin_offset, F32 x, F32 y,
			   const LLColor4& color,
			   HAlign halign = LEFT, VAlign valign = BASELINE,
			   U8 style = NORMAL,
			   S32 max_chars = S32_MAX, S32 max_pixels = S32_MAX,
			   F32* right_x = NULL,
			   bool use_embedded = false, bool use_ellipses = false,
			   bool use_color = true) const;

	S32 render(const LLWString& text, S32 begin_offset, F32 x, F32 y,
			   const LLColor4& color) const;

	// The renderUTF8() methods perform a conversion, so they are slower...
	S32 renderUTF8(const std::string& text, S32 begin_offset, F32 x, F32 y,
				   const LLColor4& color, HAlign halign, VAlign valign,
				   U8 style, S32 max_chars, S32 max_pixels, F32* right_x,
				   bool use_ellipses) const;

	S32 renderUTF8(const std::string& text, S32 begin_offset, S32 x, S32 y,
				   const LLColor4& color) const;

	S32 renderUTF8(const std::string& text, S32 begin_offset, S32 x, S32 y,
				   const LLColor4& color,
				   HAlign halign, VAlign valign, U8 style = NORMAL) const;

	// Font metrics - override for LLFontFreetype that returns units of virtual
	// pixels
	F32 getAscenderHeight() const;
	F32 getDescenderHeight() const;
	F32 getLineHeight() const;

	S32 getWidth(const std::string& utf8text) const;
	S32 getWidth(const llwchar* wchars) const;
	S32 getWidth(const std::string& utf8text, S32 offset, S32 max_chars) const;
	S32 getWidth(const llwchar* wchars, S32 offset, S32 max_chars,
				 bool use_embedded = false) const;

	F32 getWidthF32(const std::string& utf8text) const;
	F32 getWidthF32(const llwchar* wchars) const;
	F32 getWidthF32(const std::string& text, S32 offset, S32 max_chars) const;
	F32 getWidthF32(const llwchar* wchars, S32 offset, S32 max_chars,
					bool use_embedded = false) const;

	// The following are called often, frequently with large buffers, so do not
	// use a string interface

	// Returns the max number of complete characters from text (up to
	// max_chars) that can be drawn in max_pixels
	S32	maxDrawableChars(const llwchar* wchars, F32 max_pixels,
						 S32 max_chars = S32_MAX,
						 bool end_on_word_boundary = false,
						 bool use_embedded = false,
						 F32* drawn_pixels = NULL) const;

	// Returns the index of the first complete characters from text that can be
	// drawn in max_pixels given that the character at start_pos should be the
	// last character (or as close to last as possible).
	S32	firstDrawableChar(const llwchar* wchars, F32 max_pixels,
						  S32 text_len, S32 start_pos = S32_MAX,
						  S32 max_chars = S32_MAX) const;

	// Returns the index of the character closest to pixel position x (ignoring
	// text to the right of max_pixels and max_chars)
	S32 charFromPixelOffset(const llwchar* wchars, S32 char_offset, F32 x,
							F32 max_pixels = F32_MAX, S32 max_chars = S32_MAX,
							bool round = true, bool embedded = false) const;

	LL_INLINE const LLFontDescriptor& getFontDesc() const
	{
		return mFontDescriptor;
	}

	void generateASCIIglyphs();

	static void initClass(F32 screen_dpi, F32 x_scale, F32 y_scale,
						  const std::vector<std::string>& xui_paths,
						  bool create_gl_textures = true);
	static void destroyAllGL();

	LLImageGL* getImageGL() const;

	void addEmbeddedChar(llwchar wc, LLGLTexture* image,
						 const std::string& label) const;
	void addEmbeddedChar(llwchar wc, LLGLTexture* image,
						 const LLWString& label) const;
	void removeEmbeddedChar(llwchar wc) const;

	U32 getCacheGeneration() const;

	static std::string nameFromFont(const LLFontGL* fontp);

	static const std::string& nameFromHAlign(LLFontGL::HAlign align);
	static LLFontGL::HAlign hAlignFromName(const std::string& name);

	static const std::string& nameFromVAlign(LLFontGL::VAlign align);
	static LLFontGL::VAlign vAlignFromName(const std::string& name);

	static void setFontDisplay(bool flag)				{ sDisplayFont = flag; }

	static LLFontGL* getFontMonospace();
	static LLFontGL* getFontSansSerifSmall();
	static LLFontGL* getFontSansSerif();
	static LLFontGL* getFontSansSerifLarge();
	static LLFontGL* getFontSansSerifHuge();
	static LLFontGL* getFontSansSerifBold();
	static LLFontGL* getFontEmoji();
	static LLFontGL* getFont(const LLFontDescriptor& desc,
							 bool normalize = true);
	// Only to try and use other fonts than the default ones. HB
	static LLFontGL* getFont(const char* name, const char* size = NULL,
							 U8 style = 0);
	// Use with names like "SANSSERIF_SMALL"
	static LLFontGL* getFont(const std::string& name);
	// Use with font ids like LLFONT_SANSSERIF_SMALL
	static LLFontGL* getFont(S32 font_id);

	// Fallback to sans serif as default font
	LL_INLINE static LLFontGL* getFontDefault()			{ return getFontSansSerif(); }

	LL_INLINE static void setColorUse(bool allow)		{ sAllowColorUse = allow; }

	LL_INLINE static void newDPI()						{ ++sResGeneration; }

private:
	struct embedded_data_t
	{
		embedded_data_t(LLImageGL* image, const LLWString& label)
		:	mImage(image),
			mLabel(label)
		{
		}

		LLPointer<LLImageGL> mImage;
		LLWString			 mLabel;
	};

	// New, optimized routines for texts without embedded data:
	S32 newrender(const LLWString& wstr, S32 begin_offset, F32 x, F32 y,
				  const LLColor4& color, HAlign halign, VAlign valign,
				  U8 style, S32 max_chars, S32 max_pixels, F32* right_x,
				  bool use_ellipses, bool use_color,
				  bool* has_color_characters = NULL) const;

	void renderQuad(LLVector4a* vertex_out, LLVector2* uv_out,
					LLColor4U* colors_out, const LLRectf& screen_rect,
					const LLRectf& uv_rect, const LLColor4U& color,
					F32 slant_amt) const;
	void drawGlyph(S32& glyph_count, LLVector4a* vertex_out, LLVector2* uv_out,
				   LLColor4U* colors_out, const LLRectf& screen_rect,
				   const LLRectf& uv_rect, const LLColor4U& color, U8 style,
				   F32 drop_shadow_fade) const;

	// Old, slower routines for texts with embedded data:
	// *TODO: change the UI code to allow getting fully rid of these
	S32 oldrender(const LLWString& wstr, S32 begin_offset, F32 x, F32 y,
				  const LLColor4& color, HAlign halign, VAlign valign,
				  U8 style, S32 max_chars, S32 max_pixels, F32* right_x,
				  bool use_ellipses) const;

	void renderQuad(const LLRectf& screen_rect, const LLRectf& uv_rect,
					F32 slant_amt) const;
	void drawGlyph(const LLRectf& screen_rect, const LLRectf& uv_rect,
				   const LLColor4& color, U8 style,
				   F32 drop_shadow_fade) const;
	const embedded_data_t* getEmbeddedCharData(llwchar wch) const;
	F32 getEmbeddedCharAdvance(const embedded_data_t* ext_data) const;

public:
	static LLColor4				sShadowColor;
	// Converted value of sShadowColor, for speed
	static LLColor4U			sShadowColorU;

	static LLCoordGL			sCurOrigin;
	static F32					sCurDepth;
	static F32					sVertDPI;
	static F32					sHorizDPI;
	static F32					sScaleX;
	static F32					sScaleY;
	static bool					sDisplayFont;

	static std::vector<std::pair<LLCoordGL, F32> > sOriginStack;

private:
	LLFontDescriptor			mFontDescriptor;
	LLPointer<LLFontFreetype>	mFontFreetype;

	typedef fast_hmap<llwchar, embedded_data_t> embedded_map_t;
	mutable embedded_map_t		mEmbeddedChars;

	// Registry holds all instantiated fonts:
	static LLFontRegistry*		sFontRegistry;

	static U32					sResGeneration;

	static bool					sAllowColorUse;
};

// Buffer storage for optimized font rendering. Author: Andrii Kleshchev.
class LLFontVertexBuffer
{
	friend class LLFontGL;

protected:
	LOG_CLASS(LLFontVertexBuffer);

public:
	LLFontVertexBuffer();
	~LLFontVertexBuffer();

	// Call each time you need to render new text using the same instance of
	// this class.
	void reset();

	// IMPORTANT NOTE: the render() methods below do not check for changed
	// 'text' since last call, so you *must* ensure that reset() got called
	// before, in case the text did change.

	S32 render(const LLFontGL* fontp, const LLWString& text, S32 begin_offset,
			   LLRect rect, const LLColor4& color,
			   LLFontGL::HAlign halign = LLFontGL::LEFT,
			   LLFontGL::VAlign valign = LLFontGL::BASELINE,
			   U8 style = LLFontGL::NORMAL,
			   S32 max_chars = S32_MAX, F32* right_x = NULL,
			   bool use_ellipses = false, bool use_color = true);

	S32 render(const LLFontGL* fontp, const LLWString& text, S32 begin_offset,
			   LLRectf rect, const LLColor4& color,
			   LLFontGL::HAlign halign = LLFontGL::LEFT,
			   LLFontGL::VAlign valign = LLFontGL::BASELINE,
			   U8 style = LLFontGL::NORMAL,
			   S32 max_chars = S32_MAX, F32* right_x = NULL,
			   bool use_ellipses = false, bool use_color = true);

	S32 render(const LLFontGL* fontp, const LLWString& text, S32 begin_offset,
			   F32 x, F32 y, const LLColor4& color,
			   LLFontGL::HAlign halign = LLFontGL::LEFT,
			   LLFontGL::VAlign valign = LLFontGL::BASELINE,
			   U8 style = LLFontGL::NORMAL,
			   S32 max_chars = S32_MAX, S32 max_pixels = S32_MAX,
			   F32* right_x = NULL, bool use_ellipses = false,
			   bool use_color = true);

	// Let's allow to disable this feature, just in case (and for performances
	// comparisons). HB
	LL_INLINE static void enable(bool b)		{ sEnabled = b; }
	LL_INLINE static bool enabled()				{ return sEnabled; }

	// Logs some stats (ratio of text printed with vertex buffers against the
	// total amount of printed text in the session). HB
	static void reportHitRate();

private:
	// Allows to cleanup GL properly on shutdown, and to restart GL. Only
	// called, where and when appropriate, by LLFontGL. HB
	static void destroyGL();

	void genBuffers(const LLFontGL* fontp, const LLWString& text,
					S32 begin_offset, F32 x, F32 y, const LLColor4& color,
					LLFontGL::HAlign halign, LLFontGL::VAlign valign, U8 style,
					S32 max_chars, S32 max_pixels, F32* right_x,
					bool use_ellipses, bool use_color);

	void renderBuffers();

private:
	LLVertexBufferData::list_t		mBufferList;
	LLCoordGL						mLastOrigin;
	LLColor4						mLastColor;
	const LLFontGL*					mLastFont;
	U32								mLastResGeneration;
	U32								mLastFontCacheGen;
	S32								mChars;
	S32								mLastOffset;
	S32								mLastMaxChars;
	S32								mLastMaxPixels;
	F32								mLastRightX;
	F32								mLastX;
	F32								mLastY;
	LLFontGL::HAlign				mLastHalign;
	LLFontGL::VAlign				mLastValign;
	U8								mLastStyle;
	bool							mFirstRender;

	static U64						sTotalHits;
	static U64						sTotalMisses;
	static bool						sEnabled;
	static bool						sStopped;
};
