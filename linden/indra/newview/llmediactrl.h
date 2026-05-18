/**
 * @file llmediactrl.h
 * @brief Web browser UI control
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llframetimer.h"
#include "llmemberlistener.h"
#include "lluictrl.h"

#include "llviewermedia.h"

class LLViewBorder;
class LLUICtrlFactory;

class LLMediaCtrl : public LLUICtrl, public LLViewerMediaObserver,
					public LLViewerMediaEventEmitter
{
protected:
	LOG_CLASS(LLMediaCtrl);

public:
	LLMediaCtrl(const std::string& name, const LLRect& rect);
	~LLMediaCtrl() override;

	void setBorderVisible(bool border_visible);

	const std::string& getTag() const override;

	LLXMLNodePtr getXML(bool save_children = true) const override;
	static LLView* fromXML(LLXMLNodePtr node, LLView* parent,
						   LLUICtrlFactory* factory);

	void reshape(S32 width, S32 height, bool from_parent = true) override;
	void draw() override;

	// Focus overrides
	void onFocusLost() override;
	void onFocusReceived() override;

	// Input overrides
	bool handleKeyHere(KEY key, MASK mask) override;
	bool handleKeyUpHere(KEY key, MASK mask) override;
	bool handleUnicodeCharHere(llwchar uni_char) override;

	// The browser window wants keyup and keydown events. Overridden from
	// LLFocusableElement to return true.
	LL_INLINE bool wantsKeyUpKeyDown() const override	{ return true; }
	LL_INLINE bool wantsReturnKey() const override		{ return true; }

	LL_INLINE bool acceptsTextInput() const override	{ return true; }

	// Mouse handling related methods
	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleRightMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleDoubleClick(S32 x, S32 y, MASK mask) override;
	bool handleScrollWheel(S32 x, S32 y, S32 clicks) override;

	// Incoming media event dispatcher. Inherited from LLViewerMediaObserver.
	void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent evt) override;

	// Navigation
	void navigateTo(const std::string& url_in,
					const std::string& mime_type = LLStringUtil::null);
	void navigateBack();
	void navigateHome();
	void navigateForward();
	void navigateToLocalPage(const std::string& subdir,
								 const std::string& filename_in);
	bool canNavigateBack();
	bool canNavigateForward();
	LL_INLINE std::string getCurrentNavUrl()			{ return mCurrentNavUrl; }

	// By default, we do not handle "secondlife:///app/" SLURLs, because those
	// can cause teleports, open windows, etc. We cannot be sure that each
	// "click" is actually due to a user action, versus Javascript or some
	// other mechanism. However, we need the search floater and login page to
	// handle these URLs. Those are safe because we control the page content.
	// See DEV-9530.  JC.
	void setTrusted(bool trusted);
	LL_INLINE bool isTrusted()							{ return mTrusted; }

	void setHomePageUrl(const std::string urlIn,
						const std::string& mime_type = LLStringUtil::null);
	std::string getHomePageUrl()						{ return mHomePageUrl; }

	void setTarget(const std::string& target);

	// Set URL to visit when a 404 page is reached
	LL_INLINE void setErrorPageURL(std::string url)		{ mErrorPageURL = url; }
	LL_INLINE const std::string& getErrorPageURL()		{ return mErrorPageURL; }

	// Accessor/mutator for flag that indicates if frequent updates to texture
	// happen:
	LL_INLINE bool getFrequentUpdates()					{ return mFrequentUpdates; }
	LL_INLINE void setFrequentUpdates(bool b)			{ mFrequentUpdates = b; }

	LL_INLINE void setAlwaysRefresh(bool b)				{ mAlwaysRefresh = b; }
	LL_INLINE bool getAlwaysRefresh()					{ return mAlwaysRefresh; }

	LL_INLINE void setForceUpdate(bool b)				{ mForceUpdate = b; }
	LL_INLINE bool getForceUpdate()						{ return mForceUpdate; }

	LL_INLINE void setDecoupleTextureSize(bool b)		{ mDecoupleTextureSize = b; }
	LL_INLINE bool getDecoupleTextureSize()				{ return mDecoupleTextureSize; }

	void setTextureSize(S32 width, S32 height);
	LL_INLINE S32 getTextureWidth()						{ return mTextureWidth; }
	LL_INLINE S32 getTextureHeight()					{ return mTextureHeight; }

	bool ensureMediaSourceExists();
	LL_INLINE void unloadMediaSource()					{ mMediaSource = NULL; }

	LL_INLINE LLViewerMediaImpl* getMediaSource()
	{
		return mMediaSource.isNull() ? NULL : (LLViewerMediaImpl*)mMediaSource;
	}

	LLPluginClassMedia* getMediaPlugin();

#if 0	// Not used
	virtual void handleVisibilityChange(bool new_visibility);
#endif

	static void setOpenIdCookie(const std::string& url,
								const std::string& cookie_host,
								const std::string& cookie);

protected:
	void convertInputCoords(S32& x, S32& y);

	class LLMenuCut final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD&) override;
	};

	class LLMenuCanCut final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuCopy final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD&) override;
	};

	class LLMenuCanCopy final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuPaste final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD&) override;
	};

	class LLMenuCanPaste final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuUndo final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuCanUndo final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuRedo final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuCanRedo final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuDelete final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuCanDelete final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuSelectAll final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuCanSelectAll final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuShowSource final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuCanShowSource final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

	class LLMenuReloadPage final : public LLMemberListener<LLMediaCtrl>
	{
	public:
		bool handleEvent(LLPointer<LLOldEvents::LLEvent>,
						 const LLSD& userdata) override;
	};

private:
	void onVisibilityChange(bool new_visibility) override;

	static bool onClickLinkExternalTarget(const LLSD&, const LLSD&);

	static bool parseRawCookie(const std::string& raw_cookie,
							   std::string& name, std::string& value,
							   std::string& path);

private:
	LLViewBorder*		mBorder;

	std::string			mHomePageUrl;
	std::string			mHomePageMimeType;
	std::string			mCurrentNavUrl;
	std::string			mErrorPageURL;
	std::string			mTarget;

	viewer_media_t		mMediaSource;
	LLUUID				mMediaTextureID;

	LLHandle<LLView>	mPopupMenuHandle;

	const S32			mTextureDepthBytes;
	S32					mTextureWidth;
	S32					mTextureHeight;

	bool				mFrequentUpdates;
	bool				mForceUpdate;
	bool				mTrusted;
	bool				mAlwaysRefresh;
	bool				mStretchToFill;
	bool				mMaintainAspectRatio;
	bool				mHidingInitialLoad;
	bool				mDecoupleTextureSize;

	typedef fast_hset<LLMediaCtrl*> instances_list_t;
	static instances_list_t sMediaCtrlInstances;
};
