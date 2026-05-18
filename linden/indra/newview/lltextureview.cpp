/**
 * @file lltextureview.cpp
 * @brief LLTextureView class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
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

#include "llviewerprecompiledheaders.h"

#include "lltextureview.h"

#include "llconsole.h"				// For CONSOLE_PADDING_LEFT
#include "llimagedecodethread.h"
#include "llimagegl.h"
#include "llrendertarget.h"
#include "llvertexbuffer.h"

#include "llappviewer.h"
#include "llhoverview.h"
#include "llselectmgr.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvovolume.h"

LLTextureView* gTextureViewp = NULL;

#define HIGH_PRIORITY 100000000.f

///////////////////////////////////////////////////////////////////////////////

static const char* mem_format_str1 =
	"Mem (MB): GL tex: %d/%d  Bound: %d/%d  VB: %d  FB: %d  Free VRAM: %d%s  Free RAM: %d%s";
static const char* mem_format_str2 =
	"Mem (MB): GL tex: %d/%d  Bound: %d/%d  VB: %d  FB: %d  Free RAM: %d%s";
static const char* tex_format_str =
	"Tex(Raw): %d(%d)  Fetches: %d(%d)  HTTP: %d UDP BW: %.0f  Cache R/W: %d/%d  Decodes: %d  Bias: %.3f";
static const char* fetcher_format_str =
	"Fetch boost factor: %.1f - Upd/frame: %d - GL img created: immediate: %d / threaded: %d";
static const char* fetch1_format_str = "%s %7.0f %d(%d) 0x%08x(%8.0f)";
static const char* fetch2_format_str = "%s %7.0f %d(%d) %8.0f(0x%08x) %1.2f";
static const char* image_format_str1 = "%4dx%4d (%d) %8d";
static const char* image_format_str2 = "%4dx%4d (%d) %7d";
static const std::string title_string1a("Tex UUID  Area DDis(Req)  DecodePri(Fetch)      [download]");
static const std::string title_string1b("Tex UUID  Area DDis(Req)  Fetch(DecodePri)      [download]");
static const std::string title_string2("State");
static const std::string title_string3("Pkt Bnd");
static const std::string title_string4("  W  x  H (Dis)    Mem");

// State. *HACK: mirrored from lltexturefetch.cpp
static struct { const std::string desc; LLColor4* color; } fetch_state_desc[] = {
	{ "---", &LLColor4::red },		// INVALID
	{ "INI", &LLColor4::white },	// INIT
	{ "DSK", &LLColor4::cyan },		// LOAD_FROM_TEXTURE_CACHE
	{ "DSK", &LLColor4::blue },		// CACHE_POST
	{ "NET", &LLColor4::green },	// LOAD_FROM_NETWORK
	{ "SIM", &LLColor4::green },	// LOAD_FROM_SIMULATOR
	{ "HTW", &LLColor4::green },	// WAIT_HTTP_RESOURCE
	{ "HTW", &LLColor4::green },	// WAIT_HTTP_RESOURCE2
	{ "REQ", &LLColor4::yellow },	// SEND_HTTP_REQ
	{ "HTP", &LLColor4::green },	// WAIT_HTTP_REQ
	{ "DEC", &LLColor4::yellow },	// DECODE_IMAGE
	{ "DEC", &LLColor4::green },	// DECODE_IMAGE_UPDATE
	{ "WRT", &LLColor4::purple },	// WRITE_TO_CACHE
	{ "WRT", &LLColor4::orange },	// WAIT_ON_WRITE
	{ "END", &LLColor4::red },		// DONE
#define LAST_STATE 14
	{ "CRE", &LLColor4::magenta },	// LAST_STATE+1
	{ "FUL", &LLColor4::green },	// LAST_STATE+2
	{ "BAD", &LLColor4::red },		// LAST_STATE+3
	{ "MIS", &LLColor4::red },		// LAST_STATE+4
	{ "---", &LLColor4::white },	// LAST_STATE+5
};
constexpr S32 fetch_state_desc_size = (S32)LL_ARRAY_SIZE(fetch_state_desc);

constexpr S32 TEXTUREVIEW_WIDTH = 648;
constexpr S32 TEXTUREVIEW_TOP_DELTA = 50;

constexpr S32 TITLE_X1 = 0;
constexpr S32 BAR_LEFT = TITLE_X1 + 290;
constexpr S32 BAR_WIDTH = 100;
constexpr S32 BAR_HEIGHT = 8;
constexpr S32 TITLE_X2 = BAR_LEFT + BAR_WIDTH + 10;
constexpr S32 TITLE_X3 = TITLE_X2 + 40;
constexpr S32 TITLE_X4 = TITLE_X3 + 50;

///////////////////////////////////////////////////////////////////////////////

class LLTextureBar final : public LLView
{
protected:
	LOG_CLASS(LLTextureBar);

public:
	LLTextureBar(const std::string& name, const LLRect& r, LLTextureView* view)
	:	LLView(name, r, false),
		mHilite(0),
		mTextureView(view)
	{
	}

	void draw() override;

	// Returns the height of this object, given the set options.
	LL_INLINE LLRect getRequiredRect() override
	{
		LLRect rect;
		rect.mTop = BAR_HEIGHT;
		return rect;
	}

	// Used for sorting
	struct sort_decode
	{
		LL_INLINE bool operator()(const LLView* i1, const LLView* i2)
		{
			LLViewerFetchedTexture* i1p = ((LLTextureBar*)i1)->mTexturep.get();
			LLViewerFetchedTexture* i2p = ((LLTextureBar*)i2)->mTexturep.get();
			if (!i2p)
			{
				return true;
			}
			if (!i1p)
			{
				return false;
			}
			F32 pri1 = i1p->getDecodePriority();
			F32 pri2 = i2p->getDecodePriority();
			if (pri1 > pri2)
			{
				return true;
			}
			if (pri2 > pri1)
			{
				return false;
			}
			return i1p->getID() < i2p->getID();
		}
	};

	struct sort_fetch
	{
		LL_INLINE bool operator()(const LLView* i1, const LLView* i2)
		{
			LLViewerFetchedTexture* i1p = ((LLTextureBar*)i1)->mTexturep.get();
			LLViewerFetchedTexture* i2p = ((LLTextureBar*)i2)->mTexturep.get();
			if (!i2p)
			{
				return true;
			}
			if (!i1p)
			{
				return false;
			}
			U32 pri1 = i1p->getFetchPriority();
			U32 pri2 = i2p->getFetchPriority();
			if (pri1 > pri2)
			{
				return true;
			}
			if (pri2 > pri1)
			{
				return false;
			}
			return i1p->getID() < i2p->getID();
		}
	};

private:
	LLTextureView*						mTextureView;

public:
	LLPointer<LLViewerFetchedTexture>	mTexturep;
	S32									mHilite;
};

//virtual
void LLTextureBar::draw()
{
	static LLFontGL* fontp = LLFontGL::getFontMonospace();
	if (!fontp)
	{
		llwarns_sparse << "No monospace font !" << llendl;
		return;
	}
	if (!mTexturep || !gTextureFetchp)
	{
		return;
	}

	LLColor4 color;
	if (mHilite)
	{
		S32 idx = llclamp(mHilite, 1, 3);
		if (idx == 1)
		{
			color = LLColor4::orange;
		}
		else if (idx == 2)
		{
			color = LLColor4::yellow;
		}
		else
		{
			color = LLColor4::pink2;
		}
	}
	else if (mTexturep->mDontDiscard)
	{
		color = LLColor4::green4;
	}
	else if (mTexturep->getBoostLevel() > LLGLTexture::BOOST_ALM)
	{
		color = LLColor4::magenta;
	}
	else if (mTexturep->getDecodePriority() <= 0.f)
	{
		color = LLColor4::grey;
		color[VALPHA] = 0.7f;
	}
	else
	{
		color = LLColor4::white;
		color[VALPHA] = 0.7f;
	}

	// We need to draw the texture UUID or name, the progress bar for the
	// texture (highlighted if it is being downloaded) and various numerical
	// stats.

	static char uuid_str[UUID_STR_LENGTH];
	mTexturep->mID.toCString(uuid_str);
	uuid_str[7] = '\0';	// Keep only the first six digits

	std::string tex_str;
	if (mTextureView->mOrderFetch)
	{
		tex_str = llformat(fetch1_format_str, uuid_str,
						   mTexturep->mMaxVirtualSize,
						   mTexturep->mDesiredDiscardLevel,
						   mTexturep->mRequestedDiscardLevel,
						   mTexturep->mFetchPriority,
						   mTexturep->getDecodePriority());
	}
	else
	{
		tex_str = llformat(fetch2_format_str, uuid_str,
						   mTexturep->mMaxVirtualSize,
						   mTexturep->mDesiredDiscardLevel,
						   mTexturep->mRequestedDiscardLevel,
						   mTexturep->getDecodePriority(),
						   mTexturep->mFetchPriority,
						   mTexturep->mDownloadProgress);
	}

	fontp->renderUTF8(tex_str, 0, TITLE_X1, getRect().getHeight(), color,
					  LLFontGL::LEFT, LLFontGL::TOP);

	S32 state = mTexturep->mNeedsCreateTexture ? LAST_STATE + 1 :
					mTexturep->mFullyLoaded ? LAST_STATE + 2 :
						mTexturep->mMinDiscardLevel > 0 ? LAST_STATE + 3 :
							mTexturep->mIsMissingAsset ? LAST_STATE + 4 :
								!mTexturep->mIsFetching ? LAST_STATE + 5 :
									mTexturep->mFetchState;
	state = llclamp(state, 0, fetch_state_desc_size - 1);

	fontp->renderUTF8(fetch_state_desc[state].desc, 0, TITLE_X2,
					  getRect().getHeight(),
					  *(fetch_state_desc[state].color),
					  LLFontGL::LEFT, LLFontGL::TOP);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	// Draw the progress bar.
	S32 left = BAR_LEFT;
	S32 right = left + BAR_WIDTH;
	S32 top = 0;
	S32 bottom = top + 6;
	gGL.color4f(0.f, 0.f, 0.f, 0.75f);
	gl_rect_2d(left, top, right, bottom);

	F32 data_progress = mTexturep->mDownloadProgress;

	if (data_progress > 0.f && data_progress <= 1.f)
	{
		// Downloaded bytes
		right = left + llfloor(data_progress * (F32)BAR_WIDTH);
		if (right > left)
		{
			gGL.color4f(0.f, 0.f, 1.f, 0.75f);
			gl_rect_2d(left, top, right, bottom);
		}
	}
	else if (data_progress > 1.f)
	{
		// Small cached textures generate this oddity. SNOW-168
		right = left + BAR_WIDTH;
		gGL.color4f(0.f, 0.33f, 0.f, 0.75f);
		gl_rect_2d(left, top, right, bottom);
	}

	S32 pip_width = 6;
	S32 pip_space = 14;
	S32 pip_x = TITLE_X3 + pip_space / 2;

	// Draw the packet pip
	LLColor4 clr;
	constexpr F32 pip_max_time = 5.f;
	F32 last_event = gFrameTimeSeconds - mTexturep->mLastPacketTime;
	if (last_event < pip_max_time)
	{
		clr = LLColor4::white;
	}
	else
	{
		last_event = mTexturep->mRequestDeltaTime;
		if (last_event < pip_max_time)
		{
			clr = LLColor4::green;
		}
		else
		{
			last_event = mTexturep->mFetchDeltaTime;
			if (last_event < pip_max_time)
			{
				clr = LLColor4::yellow;
			}
		}
	}
	if (last_event < pip_max_time)
	{
		clr.setAlpha(1.f - last_event / pip_max_time);
		gGL.color4fv(clr.mV);
		gl_rect_2d(pip_x, top, pip_x + pip_width, bottom);
	}
	pip_x += pip_width + pip_space;

	// We do not want to show bind/resident pips for textures using the default
	// texture
	if (mTexturep->hasGLTexture())
	{
		// Draw the bound pip
		last_event = mTexturep->getTimePassedSinceLastBound();
		if (last_event < 1.f)
		{
			clr = LLColor4::magenta1;
			clr.setAlpha(1.f - last_event);
			gGL.color4fv(clr.mV);
			gl_rect_2d(pip_x, top, pip_x + pip_width, bottom);
		}
	}
	pip_x += pip_width + pip_space;

	// Draw the image size at the end
	S32 discard = mTexturep->getDiscardLevel();
	std::string num_str = llformat(discard >= 0 ? image_format_str1
												: image_format_str2,
								   mTexturep->getWidth(),
								   mTexturep->getHeight(), discard,
								   mTexturep->hasGLTexture() ?
									mTexturep->getTextureMemory() : 0);
	fontp->renderUTF8(num_str, 0, TITLE_X4, getRect().getHeight(), color,
					  LLFontGL::LEFT, LLFontGL::TOP);
}

////////////////////////////////////////////////////////////////////////////

class LLTexViewStats final : public LLView
{
protected:
	LOG_CLASS(LLTexViewStats);

public:
	LLTexViewStats(const std::string& name, LLTextureView* texviewp)
	:	LLView(name, false),
		mTextureView(texviewp)
		// Note: we do not bother initializing the last stats values since they
		// will all be updated at first draw(). HB
	{
		S32 h = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
		setRect(LLRect(0, 0, 100, h * 5));
	}

	void draw() override;

	LL_INLINE bool handleMouseDown(S32, S32, MASK) override
	{
		return false;
	}

	// Returns the height of this object, given the set options.
	LL_INLINE LLRect getRequiredRect() override
	{
		LLRect rect;
		// Room for four lines of text
		rect.mTop = (7 * BAR_HEIGHT) / 2;
		return rect;
	}

private:
	LLTextureView*		mTextureView;
	// Cached UTF-8 text strings. Rebuilt only when needed. HB
	LLWString			mLine1Text;
	LLWString			mLine2Text;
	LLWString			mLine3Text;
	LLWString			mLine4Seg1Text;
	// Font vertex buffers, to cache rendered text. HB
	LLFontVertexBuffer	mLine1VB;
	LLFontVertexBuffer	mLine2VB;
	LLFontVertexBuffer	mLine3VB;
	LLFontVertexBuffer	mLine4Segment1VB;
	LLFontVertexBuffer	mLine4Segment2VB;
	LLFontVertexBuffer	mLine4Segment3VB;
	LLFontVertexBuffer	mLine4Segment4VB;
	// We keep the last stats in memory to avoid rebuilding each line of text
	// each time while it does not need to change. HB
	S32					mLastGLTexMemoryMB;
	S32					mLastUsableGLTexMemMB;
	S32					mLastBoundTexMemoryMB;
	S32					mLastMaxBoundTexMemMB;
	S32					mLastVBMegabytes;
	S32					mLastFBMegabytes;
	S32					mLastFreeVRAMMegabytes;
	S32					mLastRAMUsage;
	S32					mLastNumImages;
	S32					mLastRawCount;
	U32					mLastApproxNumRequests;
	S32					mLastNumDeletes;
	U32					mLastNumHTTPRequests;
	F32					mTextureBandwidth;
	U32					mLastNumReads;
	U32					mLastNumWrites;
	U32					mLastpending;
	F32					mLastDesiredDiscardBias;
	S32					mLastNumUpdatesStat;
	F32					mLastFetchingBoostFactor;
	U32					mLastMainThreadCreations;
	U32					mLastImageThreadCreations;
	U32					mLastImageThreadQueueSize;
	bool				mLastThreadCreationsCapped;
	bool				mLastDebugPause;
	bool				mLastFreezeView;
	bool				mLastOrderFetch;
};

//virtual
void LLTexViewStats::draw()
{
	static LLFontGL* fontp = LLFontGL::getFontMonospace();
	if (!fontp)
	{
		llwarns_sparse << "No monospace font !" << llendl;
		return;
	}
	static S32 line_height = S32(fontp->getLineHeight() + 0.5f);
	static LLColor4 text_color(1.f, 1.f, 1.f, 0.75f);
	static std::string text;

	if (!gTextureFetchp || !gTextureCachep)
	{
		return;
	}

	static bool can_do_vram = LLGLManager::sHasATIMemInfo ||
							  LLGLManager::sHasNVXMemInfo;
	static LLCachedControl<bool> no_vram(gSavedSettings, "DisableVRAMCheck");
	S32 vram_free_mb = -1;
	if (can_do_vram && !no_vram)
	{
		vram_free_mb = LLImageGLThread::getFreeVRAMMegabytes();
	}
	S32 vb_mb = LLVertexBuffer::getVRAMMegabytes();
	S32 fb_mb = BYTES2MEGABYTES(LLRenderTarget::sBytesAllocated);
	S32 ram_usage = LLMemory::getAvailablePhysicalMemKB() >> 10;
	if (mLine1Text.empty() || mLastVBMegabytes != vb_mb ||
		mLastFBMegabytes != fb_mb || mLastFreeVRAMMegabytes != vram_free_mb ||
		mLastRAMUsage != ram_usage ||
		mLastGLTexMemoryMB != LLViewerTexture::sGLTexMemoryMB ||
		mLastUsableGLTexMemMB != LLViewerTexture::sUsableGLTexMemMB ||
		mLastBoundTexMemoryMB != LLViewerTexture::sBoundTexMemoryMB ||
		mLastMaxBoundTexMemMB != LLViewerTexture::sMaxBoundTexMemMB)
	{
		mLastVBMegabytes = vb_mb;
		mLastFBMegabytes = fb_mb;
		mLastFreeVRAMMegabytes = vram_free_mb;
		mLastRAMUsage = ram_usage;
		mLastGLTexMemoryMB = LLViewerTexture::sGLTexMemoryMB;
		mLastUsableGLTexMemMB = LLViewerTexture::sUsableGLTexMemMB;
		mLastBoundTexMemoryMB = LLViewerTexture::sBoundTexMemoryMB;
		mLastMaxBoundTexMemMB = LLViewerTexture::sMaxBoundTexMemMB;
		mLine1VB.reset();
		if (can_do_vram && !no_vram)
		{
			text = llformat(mem_format_str1, mLastGLTexMemoryMB,
							mLastUsableGLTexMemMB, mLastBoundTexMemoryMB,
							mLastMaxBoundTexMemMB, vb_mb, fb_mb, vram_free_mb,
							LLViewerTexture::sLowFreeVRAM ? "!" : "",
							ram_usage, gLowMemory ? "!" : "");
		}
		else
		{
			text = llformat(mem_format_str2, mLastGLTexMemoryMB,
							mLastUsableGLTexMemMB, mLastBoundTexMemoryMB,
							mLastMaxBoundTexMemMB, vb_mb, fb_mb, ram_usage,
							gLowMemory ? "!" : "");
		}
		mLine1Text = utf8str_to_wstring(text);
	}
	mLine1VB.render(fontp, mLine1Text, 0, 0, line_height * 4, text_color,
					LLFontGL::LEFT, LLFontGL::TOP);

	S32 num_images = gTextureList.getNumImages();
	S32 raw_count = LLImageRaw::sRawImageCount;
	U32 num_requests = gTextureFetchp->getApproxNumRequests();
	S32 num_deletes = gTextureFetchp->getNumDeletes();
	U32 num_http_reqs = gTextureFetchp->getNumHTTPRequests();
	F32 bandwidth = gTextureFetchp->getTextureBandwidth();
	U32 num_reads = gTextureCachep->getNumReads();
	U32 num_writes = gTextureCachep->getNumWrites();
	U32 pending = (U32)gImageDecodeThreadp->getPending();
	F32 discard_bias = LLViewerTexture::sDesiredDiscardBias;
	if (mLine2Text.empty() || mLastNumImages != num_images ||
		mLastRawCount != raw_count || mLastApproxNumRequests != num_requests ||
		mLastNumDeletes != num_deletes ||
		mLastNumHTTPRequests != num_http_reqs ||
		mTextureBandwidth != bandwidth || mLastNumReads != num_reads ||
		mLastNumWrites != num_writes || mLastpending != pending ||
		mLastDesiredDiscardBias != discard_bias)
	{
		mLastNumImages = num_images;
		mLastRawCount = raw_count;
		mLastApproxNumRequests = num_requests;
		mLastNumDeletes = num_deletes;
		mLastNumHTTPRequests = num_http_reqs;
		mTextureBandwidth = bandwidth;
		mLastNumReads = num_reads;
		mLastNumWrites = num_writes;
		mLastpending = pending;
		mLastDesiredDiscardBias = discard_bias;
		mLine2VB.reset();
		text = llformat(tex_format_str, num_images, raw_count, num_requests,
						num_deletes, num_http_reqs, bandwidth, num_reads,
						num_writes, pending, discard_bias);
		mLine2Text = utf8str_to_wstring(text);
	}
	mLine2VB.render(fontp, mLine2Text, 0, 0, line_height * 3, text_color, LLFontGL::LEFT,
					LLFontGL::TOP);

	S32 num_updates = (S32)LLViewerTextureList::sNumUpdatesStat.getMean();
	if (mLine3Text.empty() || mLastNumUpdatesStat != num_updates ||
		mLastFetchingBoostFactor !=
			LLViewerTextureList::sFetchingBoostFactor ||
		mLastMainThreadCreations !=
			LLViewerFetchedTexture::sMainThreadCreations ||
		mLastImageThreadCreations !=
			LLViewerFetchedTexture::sImageThreadCreations ||
		mLastThreadCreationsCapped !=
			LLViewerFetchedTexture::sImageThreadCreationsCapped ||
		mLastImageThreadQueueSize !=
			LLViewerFetchedTexture::sImageThreadQueueSize)
	{
		mLastFetchingBoostFactor = LLViewerTextureList::sFetchingBoostFactor;
		mLastNumUpdatesStat = num_updates;
		mLastMainThreadCreations =
			LLViewerFetchedTexture::sMainThreadCreations;
		mLastImageThreadCreations =
			LLViewerFetchedTexture::sImageThreadCreations;
		mLastThreadCreationsCapped =
			LLViewerFetchedTexture::sImageThreadCreationsCapped;
		mLastImageThreadQueueSize =
			LLViewerFetchedTexture::sImageThreadQueueSize;
		mLine3VB.reset();
		text = llformat(fetcher_format_str, mLastFetchingBoostFactor,
						num_updates, mLastMainThreadCreations,
						mLastImageThreadCreations);
		if (mLastThreadCreationsCapped)
		{
			text += " (queue full)";
		}
		else
		{
			text += llformat(" (%d)", mLastImageThreadQueueSize);
		}
		mLine3Text = utf8str_to_wstring(text);
	}
	mLine3VB.render(fontp, mLine3Text, 0, 0, line_height * 2, text_color,
					LLFontGL::LEFT, LLFontGL::TOP);

	if (mLine4Seg1Text.empty() ||
		mLastDebugPause != gTextureFetchp->mDebugPause ||
		mLastFreezeView != mTextureView->mFreezeView ||
		mLastOrderFetch != mTextureView->mOrderFetch)
	{
		mLastDebugPause = gTextureFetchp->mDebugPause;
		mLastFreezeView = mTextureView->mFreezeView;
		mLastOrderFetch = mTextureView->mOrderFetch;
		mLine4Segment1VB.reset();
		text.clear();
		if (mLastDebugPause)
		{
			text = "!";
		}
		else if (mLastFreezeView)
		{
			text = "*";
		}
		if (mLastOrderFetch)
		{
			text += title_string1b;
		}
		else
		{
			text += title_string1a;
		}
		mLine4Seg1Text = utf8str_to_wstring(text);
	}
	mLine4Segment1VB.render(fontp, mLine4Seg1Text, 0, TITLE_X1, line_height,
							text_color, LLFontGL::LEFT, LLFontGL::TOP);

	static const LLWString utf8_string2 = utf8str_to_wstring(title_string2);
	mLine4Segment2VB.render(fontp, utf8_string2, 0, TITLE_X2, line_height,
							text_color, LLFontGL::LEFT, LLFontGL::TOP);

	static const LLWString utf8_string3 = utf8str_to_wstring(title_string3);
	mLine4Segment3VB.render(fontp, utf8_string3, 0, TITLE_X3, line_height,
							text_color, LLFontGL::LEFT, LLFontGL::TOP);

	static const LLWString utf8_string4 = utf8str_to_wstring(title_string4);
	mLine4Segment4VB.render(fontp, utf8_string4, 0, TITLE_X4, line_height,
							text_color, LLFontGL::LEFT, LLFontGL::TOP);
}

////////////////////////////////////////////////////////////////////////////

constexpr U32 MAX_BARS = 50;

typedef std::pair<F32, LLViewerFetchedTexture*> decode_pair_t;
struct compare_decode_pair
{
	LL_INLINE bool operator()(const decode_pair_t& a,
							  const decode_pair_t& b) const
	{
		return a.first > b.first;
	}
};

LLTextureView::LLTextureView(const std::string& name)
:	LLContainerView(name, LLRect()),
	mTexViewStats(new LLTexViewStats("gl texmem bar", this)),
	mFreezeView(false),
	mOrderFetch(false),
	mPrintList(false)
{
	llassert(gTextureViewp == NULL);
	gTextureViewp = this;

	mTextureBars.reserve(MAX_BARS);
	mUsedTextureBars.reserve(MAX_BARS);

	setVisible(false);
	setFollowsTop();
	setFollowsLeft();

	addChild(mTexViewStats);

	// NOTE: we must ensure the initial rect got a valid (non-zero) size, else
	// draw() is never called, and the rect is never resized (and stays
	// invisible)... HB
	S32 cur_height = gViewerWindowp->getVirtualWindowRect().getHeight();
	LLRect rect;
	rect.setLeftTopAndSize(CONSOLE_PADDING_LEFT,
						   cur_height - TEXTUREVIEW_TOP_DELTA,
						   TEXTUREVIEW_WIDTH, cur_height / 2);
	setRect(rect);
	reshape(rect.getWidth(), rect.getHeight(), false);
}

//virtual
LLTextureView::~LLTextureView()
{
	removeChild(mTexViewStats);
	delete mTexViewStats;
	removeAllBars();
	for (size_t i = 0, count = mTextureBars.size(); i < count; ++i)
	{
		delete mTextureBars[i];
	}
	gTextureViewp = NULL;
}

void LLTextureView::addBar(LLViewerFetchedTexture* texp, S32 hilite)
{
	LLTextureBar* barp;
	size_t used = mUsedTextureBars.size();
	if (++used > mTextureBars.size())
	{
		LLRect r;
		barp = new LLTextureBar("texture bar", r, this);
		mTextureBars.push_back(barp);
	}
	else
	{
		barp = mTextureBars[used - 1];
	}
	mUsedTextureBars.push_back(barp);
	barp->mTexturep = texp;
	barp->mHilite = hilite;
	addChild(barp);
}

void LLTextureView::removeAllBars()
{
	for (size_t i = 0, count = mUsedTextureBars.size(); i < count; ++i)
	{
		LLTextureBar* barp = mUsedTextureBars[i];
		removeChild(barp);
		// Do not keep the LLPointer on the fetched texture. HB
		barp->mTexturep = NULL;
	}
	mUsedTextureBars.clear();
}

//virtual
void LLTextureView::setVisible(bool visible)
{
	if (!visible)
	{
		// Do not keep LLPointers on fetched textures, else these textures
		// would not get removed from memory once no more in use until we
		// show again the texture view. HB
		removeAllBars();
	}
	LLContainerView::setVisible(visible);
}

//virtual
void LLTextureView::draw()
{
	static S32 window_height = -1;
	S32 cur_height = gViewerWindowp->getVirtualWindowRect().getHeight();
	if (cur_height != window_height)
	{
		window_height = cur_height;
		LLRect rect;
		rect.setLeftTopAndSize(CONSOLE_PADDING_LEFT,
							   cur_height - TEXTUREVIEW_TOP_DELTA,
							   TEXTUREVIEW_WIDTH, cur_height / 2);
		setRect(rect);
		reshape(rect.getWidth(), rect.getHeight(), false);
	}

	if (mFreezeView)
	{
		LLContainerView::draw();
		return;
	}

	// Remove all children.
	removeChild(mTexViewStats);
	removeAllBars();

	typedef std::multiset<decode_pair_t, compare_decode_pair> display_list_t;
	static display_list_t display_tex_list;

	if (mPrintList)
	{
		llinfos << "ID\tMEM\tBOOST\tPRI\tWIDTH\tHEIGHT\tDISCARD" << llendl;
	}

	for (LLViewerTextureList::priority_list_t::iterator
			it = gTextureList.mImageList.begin(),
			end = gTextureList.mImageList.end();
		 it != end; ++it)
	{
		LLPointer<LLViewerFetchedTexture> texp = *it;
		if (!texp->hasFetcher())
		{
			continue;
		}

		S32 cur_discard = texp->getDiscardLevel();
		S32 desired_discard = texp->mDesiredDiscardLevel;

		if (mPrintList)
		{
			S32 bytes = texp->hasGLTexture() ? texp->getTextureMemory() : 0;
			llinfos << texp->getID() << "\t" << bytes
					<< "\t" << texp->getBoostLevel()
					<< "\t" << texp->getDecodePriority()
					<< "\t" << texp->getWidth()
					<< "\t" << texp->getHeight()
					<< "\t" << cur_discard << llendl;
		}
#if 0
		if (texp->getDontDiscard() || texp->isMissingAsset())
		{
			continue;
		}
#endif
		F32 pri;
		if (mOrderFetch)
		{
			constexpr F32 ONE256TH = 1.f / 256.f;
			pri = (F32)texp->mFetchPriority * ONE256TH;
		}
		else
		{
			pri = texp->getDecodePriority();
		}
		pri = llclamp(pri, 0.f, HIGH_PRIORITY - 1.f);

		if (!mOrderFetch)
		{
			if (pri < HIGH_PRIORITY)
			{
				struct f final : public LLSelectedTEFunctor
				{
					LLViewerFetchedTexture* mImage;

					f(LLViewerFetchedTexture* image) : mImage(image)
					{
					}

					bool apply(LLViewerObject* object, S32 te) override
					{
						return (mImage == object->getTEImage(te));
					}
				} func(texp);
				bool match = gSelectMgr.getSelection()->applyToTEs(&func,
																   true);
				if (match)
				{
					pri += 3 * HIGH_PRIORITY;
				}
			}

			if (gHoverViewp && pri < HIGH_PRIORITY &&
				(cur_discard< 0 || desired_discard < cur_discard))
			{
				LLViewerObject* objectp =
					gHoverViewp->getLastHoverObject();
				if (objectp)
				{
					S32 tex_count = objectp->getNumTEs();
					for (S32 i = 0; i < tex_count; ++i)
					{
						if (texp == objectp->getTEImage(i))
						{
							pri += 2 * HIGH_PRIORITY;
							break;
						}
					}
				}
			}

			if (pri > 0.f && pri < HIGH_PRIORITY)
			{
				if (gFrameTimeSeconds - texp->mLastPacketTime < 1.f ||
					texp->mFetchDeltaTime < 0.25f)
				{
					pri += HIGH_PRIORITY;
				}
			}
		}

 		if (pri > 0.f)
		{
			display_tex_list.emplace(pri, texp);
		}
	}
	mPrintList = false;

	U32 count = 0;
	for (display_list_t::iterator it = display_tex_list.begin(),
								  end = display_tex_list.end();
		 it != end; ++it)
	{
		LLViewerFetchedTexture* texp = it->second;
		S32 hilite = 0;
		F32 pri = it->first;
		if (pri >= 1 * HIGH_PRIORITY)
		{
			hilite = (S32)((pri + 1) / HIGH_PRIORITY) - 1;
		}
		if ((hilite || count < MAX_BARS - 10) && count < MAX_BARS)
		{
			addBar(texp, hilite);
			++count;
		}
	}
	display_tex_list.clear();

	if (mOrderFetch)
	{
		sortChildren(LLTextureBar::sort_fetch());
	}
	else
	{
		sortChildren(LLTextureBar::sort_decode());
	}

	// Re-add the header lines, now that the other children got sorted.
	addChild(mTexViewStats);

	// Note: at this point, it would look logical to call LLUI::pushMatrix()
	// and then LLUI::popMatrix() just after LLUI::translate(), but doing so
	// would cause up/down "jumps" of the texture view each time a line is
	// added to it or removed from it... HB
	reshape(getRect().getWidth(), getRect().getHeight(), true);
	LLUI::popMatrix();	// Popping debug view matrix ?... Yes, weird ! HB
	LLUI::pushMatrix();	// Pushing our own matrix in its place...
	LLUI::translate((F32)getRect().mLeft, (F32)getRect().mBottom);

	for (child_list_const_iter_t it = getChildList()->begin(),
								 end = getChildList()->end();
		 it != end; ++it)
	{
		LLView* viewp = *it;
		if (viewp && viewp->getRect().mBottom < 0)
		{
			viewp->setVisible(false);
		}
	}

	LLContainerView::draw();
}

//virtual
bool LLTextureView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if ((mask & (MASK_CONTROL | MASK_SHIFT | MASK_ALT)) ==
			(MASK_ALT | MASK_SHIFT))
	{
		mPrintList = true;
		return true;
	}
	if ((mask & (MASK_CONTROL | MASK_SHIFT | MASK_ALT)) ==
			(MASK_CONTROL | MASK_SHIFT))
	{
		if (gTextureFetchp)
		{
			gTextureFetchp->mDebugPause = !gTextureFetchp->mDebugPause;
		}
		return true;
	}
	if (mask & MASK_SHIFT)
	{
		mFreezeView = !mFreezeView;
		return true;
	}
	if (mask & MASK_CONTROL)
	{
		mOrderFetch = !mOrderFetch;
		return true;
	}
	return LLView::handleMouseDown(x, y, mask);
}

//virtual
bool LLTextureView::handleMouseUp(S32, S32, MASK)
{
	return false;
}

//virtual
bool LLTextureView::handleKey(KEY, MASK, bool)
{
	return false;
}
