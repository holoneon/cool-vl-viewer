/**
 * @file llviewermediafocus.cpp
 * @brief Governs focus on media prims and implements the media HUD panel
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llviewermediafocus.h"

#include "llbutton.h"
#include "lleditmenuhandler.h"
#include "llkeyboard.h"
#include "llmediaentry.h"
#include "llpanel.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "llrender.h"
#include "llsliderctrl.h"
#include "lltextureentry.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llappviewer.h"			// For gFrameTimeSeconds
#include "llchatbar.h"				// For gChatBarp and CHAT_BAR_HEIGHT
#include "lldrawable.h"
#include "llface.h"
#include "llfloatertools.h"			// For LLFloaterTools::isVisible()
#include "llhudview.h"
#include "llselectmgr.h"
#include "lltoolbar.h"				// For LLToolBar::isVisible()
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerdisplay.h"		// For get_hud_matrices()
#include "llviewermedia.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "llweb.h"

///////////////////////////////////////////////////////////////////////////////
// LLPanelMediaHUD class
// Note: it used to be in a separate llpanelmediahud.h/cpp module, but is only
// consumed by LLViewerMediaFocus, so it is best kept here. HB
///////////////////////////////////////////////////////////////////////////////

constexpr F32 ZOOM_MEDIUM_PADDING = 1.2f;

class LLPanelMediaHUD final : public LLPanel
{
protected:
	LOG_CLASS(LLPanelMediaHUD);

public:
	LLPanelMediaHUD();

	bool postBuild() override;
	void draw() override;
	void setAlpha(F32 alpha) override;
	bool handleScrollWheel(S32 x, S32 y, S32 clicks) override;

	void updateShape();

	bool isMouseOver();
	LL_INLINE void setMediaFocus(bool b)		{ mMediaFocus = b; }

	void nextZoomLevel();
	LL_INLINE void resetZoomLevel()				{ mCurrentZoom = ZOOM_NONE; }

	LL_INLINE bool isZoomed() const
	{
		return mCurrentZoom == ZOOM_MEDIUM;
	}

	LL_INLINE LLHandle<LLPanelMediaHUD> getHandle() const
	{
		return mPanelHandle;
	}

	enum EZoomLevel
	{
		ZOOM_NONE = 0,
		ZOOM_MEDIUM = 1,
		ZOOM_END
	};

	enum EScrollDir
	{
		SCROLL_UP = 0,
		SCROLL_DOWN,
		SCROLL_LEFT,
		SCROLL_RIGHT,
		SCROLL_NONE
	};

	void setMediaFace(LLPointer<LLViewerObject> objectp, S32 face = 0,
					  viewer_media_t mediap = NULL,
					  LLVector3 pick_normal = LLVector3::zero);

private:
	static void onClickClose(void* datap);
	static void onClickBack(void* datap);
	static void onClickForward(void* datap);
	static void onClickHome(void* datap);
	static void onClickOpen(void* datap);
	static void onClickReload(void* datap);
	static void onClickPlay(void* datap);
	static void onClickPause(void* datap);
	static void onClickStop(void* datap);
	static void onClickMediaStop(void* datap);
	static void onClickZoom(void* datap);
	static void onClickVolume(void* datap);
	static void onScrollUp(void* datap);
	static void onScrollUpHeld(void* datap);
	static void onScrollLeft(void* datap);
	static void onScrollLeftHeld(void* datap);
	static void onScrollRight(void* datap);
	static void onScrollRightHeld(void* datap);
	static void onScrollDown(void* datap);
	static void onScrollDownHeld(void* datap);
	static void onScrollStop(void* datap);
	static void onHoverVolume(void* datap);
	static void onHoverSlider(LLUICtrl* ctrl, void* datap);
	static void onVolumeChange(LLUICtrl* ctrl, void* datap);

	LLViewerMediaImpl* getTargetMediaImpl();
	LLViewerObject* getTargetObject();
	LLPluginClassMedia* getTargetMediaPlugin();

private:
	LLUUID							mTargetImplID;
	LLUUID							mTargetObjectID;
	LLRootHandle<LLPanelMediaHUD>	mPanelHandle;

	LLPanel*						mFocusedControls;
	LLPanel*						mHoverControls;

	LLButton*						mCloseButton;
	LLButton*						mBackButton;
	LLButton*						mForwardButton;
	LLButton*						mHomeButton;
	LLButton*						mOpenButton;
	LLButton*						mOpenButton2;
	LLButton*						mReloadButton;
	LLButton*						mPlayButton;
	LLButton*						mPauseButton;
	LLButton*						mStopButton;
	LLButton*						mMediaStopButton;
	LLButton*						mMediaVolumeButton;
	LLButton*						mMediaMutedButton;
	LLButton*						mZoomButton;
	LLButton*						mUnzoomButton;
	LLButton*						mZoomButton2;

	LLSliderCtrl*					mVolumeSlider;

	LLView*							mMediaFullView;

	std::string						mOpenButtonTooltip;

	LLCoordWindow					mLastCursorPos;

	LLFrameTimer					mMouseMoveTimer;
	LLFrameTimer					mFadeTimer;
	LLFrameTimer					mVolumeSliderTimer;

	LLVector3						mTargetObjectNormal;
	S32								mTargetObjectFace;

	F32								mLastVolume;
	F32								mControlFadeTime;

	EZoomLevel						mCurrentZoom;
	EScrollDir						mScrollState;

	bool							mTargetIsHUDObject;

	bool							mMediaFocus;
	bool							mLargeControls;
	bool							mHasTimeControl;

	static LLUUID					sLastTargetImplID;
	static EZoomLevel				sLastMediaZoom;
};

//static
LLUUID LLPanelMediaHUD::sLastTargetImplID;
LLPanelMediaHUD::EZoomLevel LLPanelMediaHUD::sLastMediaZoom =
	LLPanelMediaHUD::ZOOM_NONE;

LLPanelMediaHUD::LLPanelMediaHUD()
:	mTargetObjectFace(0),
	mTargetIsHUDObject(false),
	mControlFadeTime(3.f),
	mLastVolume(0.f),
	mMediaFocus(false),
	mLargeControls(false),
	mHasTimeControl(false),
	mCurrentZoom(ZOOM_NONE),
	mScrollState(SCROLL_NONE)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_media_hud.xml");
	mMouseMoveTimer.reset();
	LL_DEBUGS("MediaHUD") << "Stopping the fading timer." << LL_ENDL;
	mFadeTimer.stop();
	mPanelHandle.bind(this);
}

//virtual
bool LLPanelMediaHUD::postBuild()
{
	mMediaFullView = getChild<LLView>("media_full_view");

	mFocusedControls = getChild<LLPanel>("media_focused_controls");
	mHoverControls = getChild<LLPanel>("media_hover_controls");

	mCloseButton = getChild<LLButton>("close");
	mCloseButton->setClickedCallback(onClickClose, this);

	mBackButton = getChild<LLButton>("back");
	mBackButton->setClickedCallback(onClickBack, this);

	mForwardButton = getChild<LLButton>("fwd");
	mForwardButton->setClickedCallback(onClickForward, this);

	mHomeButton = getChild<LLButton>("home");
	mHomeButton->setClickedCallback(onClickHome, this);

	mStopButton = getChild<LLButton>("stop");
	mStopButton->setClickedCallback(onClickStop, this);

	mMediaStopButton = getChild<LLButton>("media_stop");
	mMediaStopButton->setClickedCallback(onClickMediaStop, this);

	mReloadButton = getChild<LLButton>("reload");
	mReloadButton->setClickedCallback(onClickReload, this);

	mPlayButton = getChild<LLButton>("play");
	mPlayButton->setClickedCallback(onClickPlay, this);

	mPauseButton = getChild<LLButton>("pause");
	mPauseButton->setClickedCallback(onClickPause, this);

	mOpenButton = getChild<LLButton>("new_window");
	mOpenButton->setClickedCallback(onClickOpen, this);
	mOpenButtonTooltip = mOpenButton->getToolTip();

	mMediaVolumeButton = getChild<LLButton>("volume");
	mMediaVolumeButton->setClickedCallback(onClickVolume, this);
	mMediaVolumeButton->setMouseHoverCallback(onHoverVolume);

	mMediaMutedButton = getChild<LLButton>("muted");
	mMediaMutedButton->setClickedCallback(onClickVolume, this);
	mMediaMutedButton->setMouseHoverCallback(onHoverVolume);

	mVolumeSlider = getChild<LLSliderCtrl>("volume_slider");
	mVolumeSlider->setCommitCallback(onVolumeChange);
	mVolumeSlider->setCallbackUserData(this);
	mVolumeSlider->setMouseHoverCallback(onHoverSlider);

	mZoomButton = getChild<LLButton>("zoom_frame");
	mZoomButton->setClickedCallback(onClickZoom, this);

	mUnzoomButton = getChild<LLButton>("unzoom_frame");
	mUnzoomButton->setClickedCallback(onClickZoom, this);

	mOpenButton2 = getChild<LLButton>("new_window_hover");
	mOpenButton2->setClickedCallback(onClickOpen, this);

	mZoomButton2 = getChild<LLButton>("zoom_frame_hover");
	mZoomButton2->setClickedCallback(onClickZoom, this);

	LLButton* scroll_up_btn = getChild<LLButton>("scrollup");
	scroll_up_btn->setClickedCallback(onScrollUp, this);
	scroll_up_btn->setHeldDownCallback(onScrollUpHeld);
	scroll_up_btn->setMouseUpCallback(onScrollStop);
	LLButton* scroll_left_btn = getChild<LLButton>("scrollleft");
	scroll_left_btn->setClickedCallback(onScrollLeft, this);
	scroll_left_btn->setHeldDownCallback(onScrollLeftHeld);
	scroll_left_btn->setMouseUpCallback(onScrollStop);
	LLButton* scroll_right_btn = getChild<LLButton>("scrollright");
	scroll_right_btn->setClickedCallback(onScrollRight, this);
	scroll_right_btn->setHeldDownCallback(onScrollLeftHeld);
	scroll_right_btn->setMouseUpCallback(onScrollStop);
	LLButton* scroll_down_btn = getChild<LLButton>("scrolldown");
	scroll_down_btn->setClickedCallback(onScrollDown, this);
	scroll_down_btn->setHeldDownCallback(onScrollDownHeld);
	scroll_down_btn->setMouseUpCallback(onScrollStop);

	// Clicks on HUD buttons do not remove keyboard focus from media
	setIsChrome(true);

	return true;
}

void LLPanelMediaHUD::setMediaFace(LLPointer<LLViewerObject> objectp,
								   S32 face, viewer_media_t mediap,
								   LLVector3 pick_normal)
{
	if (mediap.notNull() && objectp.notNull())
	{
		mTargetImplID = mediap->getMediaTextureID();
		mTargetObjectID = objectp->getID();
		mTargetObjectFace = face;
		mTargetObjectNormal = pick_normal;

		if (sLastTargetImplID != mTargetImplID)
		{
			sLastTargetImplID = mTargetImplID;
			sLastMediaZoom = mCurrentZoom;
			mVolumeSlider->setValue(mediap->getVolume());
		}
		else
		{
			mCurrentZoom = sLastMediaZoom;
		}

		updateShape();
		if (mTargetIsHUDObject)
		{
			// Set the zoom to none
			sLastMediaZoom = mCurrentZoom = ZOOM_NONE;
		}
	}
	else
	{
		mTargetImplID.setNull();
		mTargetObjectID.setNull();
		mTargetObjectFace = 0;
	}
}

LLViewerMediaImpl* LLPanelMediaHUD::getTargetMediaImpl()
{
	return LLViewerMedia::getMediaImplFromTextureID(mTargetImplID);
}

LLViewerObject* LLPanelMediaHUD::getTargetObject()
{
	return gObjectList.findObject(mTargetObjectID);
}

LLPluginClassMedia* LLPanelMediaHUD::getTargetMediaPlugin()
{
	LLViewerMediaImpl* mediap = getTargetMediaImpl();
	return mediap && mediap->hasMedia() ? mediap->getMediaPlugin() : NULL;
}

void LLPanelMediaHUD::updateShape()
{
	constexpr S32 MIN_HUD_WIDTH = 235;
	constexpr S32 MIN_HUD_HEIGHT = 120;

	LLParcel* parcel = gViewerParcelMgr.getAgentParcel();
	LLViewerMediaImpl* mediap = getTargetMediaImpl();
	LLViewerObject* objectp = getTargetObject();
	if (!parcel || !mediap || !objectp || LLFloaterTools::isVisible())
	{
		setVisible(false);
		return;
	}

	mTargetIsHUDObject = objectp->isHUDAttachment();
	if (mTargetIsHUDObject)
	{
		// Make sure the "used on HUD" flag is set for this impl
		mediap->setUsedOnHUD(true);
	}

	bool can_navigate = parcel->getMediaAllowNavigate();

	LLPluginClassMedia* pluginp = NULL;
	if (mediap->hasMedia())
	{
		pluginp = getTargetMediaPlugin();
	}

	LLVOVolume* vobjp = objectp->asVolume();

	mLargeControls = false;
	// Do not show the media HUD if we do not have permissions
	LLTextureEntry* tep = objectp->getTE(mTargetObjectFace);
	LLMediaEntry* entryp = tep ? tep->getMediaData() : NULL;
	if (entryp)
	{
		mLargeControls = entryp->getControls() == LLMediaEntry::STANDARD;

		if (vobjp &&
			!vobjp->hasMediaPermission(entryp, LLVOVolume::MEDIA_PERM_CONTROL))
		{
			setVisible(false);
			return;
		}
	}
	mLargeControls = mLargeControls || mMediaFocus;

	// Set the state of the buttons
	mBackButton->setVisible(true);
	mForwardButton->setVisible(true);
	mReloadButton->setVisible(true);
	mStopButton->setVisible(false);
	mHomeButton->setVisible(true);
	mCloseButton->setVisible(true);
	mZoomButton->setVisible(!isZoomed());
	mUnzoomButton->setVisible(isZoomed());
	mZoomButton->setEnabled(!mTargetIsHUDObject);
	mUnzoomButton->setEnabled(!mTargetIsHUDObject);
	mZoomButton2->setEnabled(!mTargetIsHUDObject);

	std::string tooltip = mediap->getMediaURL();
	if (tooltip.empty())
	{
		tooltip = mOpenButtonTooltip + ".";
	}
	else
	{
		tooltip = mOpenButtonTooltip + ": " + tooltip;
	}
	mOpenButton->setToolTip(tooltip);
	mOpenButton2->setToolTip(tooltip);

	if (mLargeControls)
	{
		mBackButton->setEnabled(can_navigate && mediap->canNavigateBack());
		mForwardButton->setEnabled(can_navigate &&
								   mediap->canNavigateForward());
		mStopButton->setEnabled(can_navigate);
		mHomeButton->setEnabled(can_navigate);

		F32 media_volume = mediap->getVolume();
		bool muted = media_volume <= 0.f;
		mMediaVolumeButton->setVisible(!muted);
		mMediaMutedButton->setVisible(muted);
		mVolumeSlider->setValue((F32)media_volume);

		LLPluginClassMediaOwner::EMediaStatus result =
			pluginp ? pluginp->getStatus()
					: LLPluginClassMediaOwner::MEDIA_NONE;
		mHasTimeControl = pluginp && pluginp->pluginSupportsMediaTime();
		if (mHasTimeControl)
		{
			mReloadButton->setEnabled(false);
			mReloadButton->setVisible(false);
			mMediaStopButton->setVisible(true);
			mHomeButton->setVisible(false);
			mBackButton->setEnabled(true);
			mForwardButton->setEnabled(true);
			if (result == LLPluginClassMediaOwner::MEDIA_PLAYING)
			{
				mPlayButton->setEnabled(false);
				mPlayButton->setVisible(false);
				mPauseButton->setEnabled(true);
				mPauseButton->setVisible(true);
				mMediaStopButton->setEnabled(true);
			}
			else
			{
				mPlayButton->setEnabled(true);
				mPlayButton->setVisible(true);
				mPauseButton->setEnabled(false);
				mPauseButton->setVisible(false);
				mMediaStopButton->setEnabled(false);
			}
		}
		else
		{
			mPlayButton->setVisible(false);
			mPauseButton->setVisible(false);
			mMediaStopButton->setVisible(false);
			if (result == LLPluginClassMediaOwner::MEDIA_LOADING)
			{
				mReloadButton->setEnabled(false);
				mReloadButton->setVisible(false);
				mStopButton->setEnabled(true);
				mStopButton->setVisible(true);
			}
			else
			{
				mReloadButton->setEnabled(true);
				mReloadButton->setVisible(true);
				mStopButton->setEnabled(false);
				mStopButton->setVisible(false);
			}
		}
	}

	mVolumeSlider->setVisible(mLargeControls &&
							  !mVolumeSliderTimer.hasExpired());

	mFocusedControls->setVisible(mLargeControls);
	mHoverControls->setVisible(!mLargeControls);

	// Handle Scrolling
	switch (mScrollState)
	{
		case SCROLL_UP:
			mediap->scrollWheel(0, 0, 0, -1, MASK_NONE);
			break;
		case SCROLL_DOWN:
			mediap->scrollWheel(0, 0, 0, 1, MASK_NONE);
			break;
		case SCROLL_LEFT:
			mediap->scrollWheel(0, 0, 1, 0, MASK_NONE);
			//mediap->handleKeyHere(KEY_LEFT, MASK_NONE);
			break;
		case SCROLL_RIGHT:
			mediap->scrollWheel(0, 0, -1, 0, MASK_NONE);
			//mediap->handleKeyHere(KEY_RIGHT, MASK_NONE);
			break;
		case SCROLL_NONE:
		default:
			break;
	}

	LLBBox screen_bbox;
	LLMatrix4a mat;
	if (mTargetIsHUDObject)
	{
		LLMatrix4a proj, modelview;
		if (get_hud_matrices(proj, modelview))
		{
			mat.setMul(proj, modelview);
		}
		else
		{
			llwarns_sparse << "Cannot get HUD matrices" << llendl;
			setVisible(false);
			return;
		}
	}
	else
	{
		mat.setMul(gGLProjection, gGLModelView);
	}

	std::vector<LLVector3> vect_face;
	LLVolume* volume = objectp->getVolume();
	if (volume)
	{
		const LLVolumeFace& vf = volume->getVolumeFace(mTargetObjectFace);

		LLVector3 ext[2];
		ext[0].set(vf.mExtents[0].getF32ptr());
		ext[1].set(vf.mExtents[1].getF32ptr());

		LLVector3 center = (ext[0] + ext[1]) * 0.5f;
		LLVector3 size = (ext[1] - ext[0]) * 0.5f;
		LLVector3 vert[] = {
			center + size.scaledVec(LLVector3(1.f, 1.f, 1.f)),
			center + size.scaledVec(LLVector3(-1.f, 1.f, 1.f)),
			center + size.scaledVec(LLVector3(1.f, -1.f, 1.f)),
			center + size.scaledVec(LLVector3(-1.f, -1.f, 1.f)),
			center + size.scaledVec(LLVector3(1.f, 1.f, -1.f)),
			center + size.scaledVec(LLVector3(-1.f, 1.f, -1.f)),
			center + size.scaledVec(LLVector3(1.f, -1.f, -1.f)),
			center + size.scaledVec(LLVector3(-1.f, -1.f, -1.f)),
		};

		for (U32 i = 0; i < 8; ++i)
		{
			vect_face.emplace_back(vobjp->volumePositionToAgent(vert[i]));
		}
	}

	LLVector4a min, max, screen_vert;;
	min.splat(1.f);
	max.splat(-1.f);
	for (std::vector<LLVector3>::iterator vert_it = vect_face.begin(),
										  vert_end = vect_face.end();
		 vert_it != vert_end; ++vert_it)
	{
		// Project silhouette vertices into screen space
		screen_vert.load3(vert_it->mV, 1.f);
		mat.perspectiveTransform(screen_vert, screen_vert);

		// Add to screenspace bounding box
		min.setMin(screen_vert, min);
		max.setMax(screen_vert, max);
	}

	LLCoordGL screen_min;
	screen_min.mX = ll_round((F32)gViewerWindowp->getWindowWidth() *
							 (min.getF32ptr()[VX] + 1.f) * 0.5f);
	screen_min.mY = ll_round((F32)gViewerWindowp->getWindowHeight() *
							 (min.getF32ptr()[VY] + 1.f) * 0.5f);

	LLCoordGL screen_max;
	screen_max.mX = ll_round((F32)gViewerWindowp->getWindowWidth() *
							 (max.getF32ptr()[VX] + 1.f) * 0.5f);
	screen_max.mY = ll_round((F32)gViewerWindowp->getWindowHeight() *
							 (max.getF32ptr()[VY] + 1.f) * 0.5f);

	// Grow panel so that screenspace bounding box fits inside the
	// "media_full_view" element of the HUD
	LLRect media_hud_rect;
	getParent()->screenRectToLocal(LLRect(screen_min.mX, screen_max.mY,
										  screen_max.mX, screen_min.mY),
								   &media_hud_rect);
	media_hud_rect.mLeft -= mMediaFullView->getRect().mLeft;
	media_hud_rect.mBottom -= mMediaFullView->getRect().mBottom;
	media_hud_rect.mTop += getRect().getHeight() -
						   mMediaFullView->getRect().mTop;
	media_hud_rect.mRight += getRect().getWidth() -
							 mMediaFullView->getRect().mRight;

	// Keep all parts of HUD on-screen
	LLRect parent_rect = getParent()->getLocalRect();
	// Account for the chat bar height when both the tool and chat bars are
	// visible. *TODO: make this accounted for in the HUD view. HB
	if (gChatBarp && gChatBarp->getVisible() && LLToolBar::isVisible())
	{
		parent_rect.mBottom += CHAT_BAR_HEIGHT;
	}
	media_hud_rect.intersectWith(parent_rect);

	// Clamp to minimum size, keeping centered
	media_hud_rect.setCenterAndSize(media_hud_rect.getCenterX(),
									media_hud_rect.getCenterY(),
									llmax(MIN_HUD_WIDTH,
										  media_hud_rect.getWidth()),
									llmax(MIN_HUD_HEIGHT,
										  media_hud_rect.getHeight()));

	userSetShape(media_hud_rect);

	setVisible(true);

	if (!mMediaFocus)
	{
		// Test mouse position to see if the cursor is stationary
		LLCoordWindow cursor_pos_window;
		gWindowp->getCursorPosition(&cursor_pos_window);

		// If last pos is not equal to current pos, the mouse has moved
		// We need to reset the timer, and make sure the panel is visible
		if (cursor_pos_window.mX != mLastCursorPos.mX ||
			cursor_pos_window.mY != mLastCursorPos.mY ||
			mScrollState != SCROLL_NONE)
		{
			mMouseMoveTimer.start();
			mLastCursorPos = cursor_pos_window;
		}

		// Mouse has been stationary, but not for long enough to fade the UI
		static LLCachedControl<F32> control_timeout(gSavedSettings,
													"MediaControlTimeout");
		static LLCachedControl<F32> fade_time(gSavedSettings,
											  "MediaControlFadeTime");
		mControlFadeTime = llmax(0.5f, (F32)fade_time);
		if (mMouseMoveTimer.getElapsedTimeF32() < control_timeout ||
			(mLargeControls && !mVolumeSliderTimer.hasExpired()))
		{
			// If we have started fading, stop and reset the alpha values
			if (mFadeTimer.getStarted())
			{
				LL_DEBUGS("MediaHUD") << "Stopping the fading timer (mouse moved, media scrolled or volume slider shown)."
									  << LL_ENDL;
				mFadeTimer.stop();
				setAlpha(1.f);
			}
		}
		// If we need to start fading the UI (and we have not already started)
		else if (!mFadeTimer.getStarted())
		{
			LL_DEBUGS("MediaHUD") << "Starting the fading timer." << LL_ENDL;
			mFadeTimer.reset();
			mFadeTimer.start();
		}
		else if (mFadeTimer.getElapsedTimeF32() >= mControlFadeTime)
		{
			setVisible(false);
		}
	}
	else if (mFadeTimer.getStarted())
	{
		LL_DEBUGS("MediaHUD") << "Focused: stopping the fading timer."
							  << LL_ENDL;
		mFadeTimer.stop();
		setAlpha(1.f);
	}
}

//virtual
void LLPanelMediaHUD::draw()
{
	if (mFadeTimer.getStarted())
	{
		if (mFadeTimer.getElapsedTimeF32() >= mControlFadeTime)
		{
			setVisible(false);
		}
		else
		{
			F32 time = mFadeTimer.getElapsedTimeF32();
			F32 alpha = llmax(lerp(1.f, 0.f, time / mControlFadeTime), 0.f);
			setAlpha(alpha);
			setVisible(true);
		}
	}

	LLPanel::draw();
}

void LLPanelMediaHUD::setAlpha(F32 alpha)
{
	LLViewQuery query;

	LLView* query_view = mLargeControls ? mFocusedControls : mHoverControls;
	child_list_t children = query(query_view);
	for (child_list_iter_t it = children.begin(), end = children.end();
		 it != end; ++it)
	{
		LLView* viewp = *it;
		if (viewp && viewp->isFocusableCtrl())
		{
			((LLUICtrl*)viewp)->setAlpha(alpha);
		}
	}

	LLPanel::setAlpha(alpha);
}

//virtual
bool LLPanelMediaHUD::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return LLViewerMediaFocus::getInstance()->handleScrollWheel(x, y, clicks);
}

bool LLPanelMediaHUD::isMouseOver()
{
	if (!getVisible())
	{
		return false;
	}

	LLCoordWindow cursor_pos_window;
	gWindowp->getCursorPosition(&cursor_pos_window);

	LLRect screen_rect;
	localRectToScreen(getLocalRect(), &screen_rect);

	return screen_rect.pointInRect(cursor_pos_window.mX, cursor_pos_window.mY);
}

//static
void LLPanelMediaHUD::onClickClose(void* datap)
{
	LLViewerMediaFocus::getInstance()->setFocusFace(false, NULL, 0, NULL);
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		if (self->mCurrentZoom != ZOOM_NONE)
		{
#if 0
			gAgent.setFocusOnAvatar();
#endif
			sLastMediaZoom = self->mCurrentZoom = ZOOM_NONE;
		}
		self->setVisible(false);
	}
}

//static
void LLPanelMediaHUD::onClickBack(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			if (self->mHasTimeControl)
			{
				mediap->skipBack(0.2f);
			}
			else
			{
				mediap->navigateForward();
			}
		}
	}
}

//static
void LLPanelMediaHUD::onClickForward(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			if (self->mHasTimeControl)
			{
				mediap->skipForward(0.2f);
			}
			else
			{
				mediap->navigateForward();
			}
		}
	}
}

//static
void LLPanelMediaHUD::onClickHome(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->navigateHome();
		}
	}
}

//static
void LLPanelMediaHUD::onClickOpen(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			LLWeb::loadURL(mediap->getCurrentMediaURL());
		}
	}
}

//static
void LLPanelMediaHUD::onClickReload(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->navigateReload();
		}
	}
}

//static
void LLPanelMediaHUD::onClickPlay(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			// First enable it
			mediap->setDisabled(false);
			// Then play it
			mediap->play();
		}
	}
}

//static
void LLPanelMediaHUD::onClickPause(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->pause();
		}
	}
}

//static
void LLPanelMediaHUD::onClickStop(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->navigateStop();
		}
	}
}

//static
void LLPanelMediaHUD::onClickMediaStop(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->stop();
		}
	}
}

//static
void LLPanelMediaHUD::onClickVolume(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			F32 volume = mediap->getVolume();
			if (volume > 0.f)
			{
				self->mLastVolume = volume;
				mediap->setVolume(0.f);
			}
			else
			{
				volume = self->mLastVolume;
				if (volume <= 0.f)
				{
					volume = gSavedSettings.getF32("AudioLevelMedia");
				}
				mediap->setVolume(volume);
			}
			self->mVolumeSlider->setValue(volume);
		}
	}
}

//static
void LLPanelMediaHUD::onClickZoom(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		self->nextZoomLevel();
	}
}

void LLPanelMediaHUD::nextZoomLevel()
{
	if (mTargetIsHUDObject)
	{
		// Do not try to zoom on HUD objects...
		sLastMediaZoom = mCurrentZoom = ZOOM_NONE;
		return;
	}

	F32 zoom_padding = 0.f;
	S32 last_zoom_level = (S32)mCurrentZoom;
	sLastMediaZoom = mCurrentZoom = (EZoomLevel)((last_zoom_level + 1) %
												 (S32)ZOOM_END);

	switch (mCurrentZoom)
	{
		case ZOOM_NONE:
		{
			gAgent.setFocusOnAvatar();
			break;
		}
		case ZOOM_MEDIUM:
		{
			zoom_padding = ZOOM_MEDIUM_PADDING;
			break;
		}
		default:
		{
			gAgent.setFocusOnAvatar();
			break;
		}
	}

	if (zoom_padding > 0.f)
	{
		LLViewerMediaFocus::getInstance()->setCameraZoom(getTargetObject(),
														 mTargetObjectNormal,
														 zoom_padding, true);
	}
}

//static
void LLPanelMediaHUD::onScrollUp(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->scrollWheel(0, 0, 0, -1, MASK_NONE);
		}
	}
}

//static
void LLPanelMediaHUD::onScrollUpHeld(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		self->mScrollState = SCROLL_UP;
	}
}

//static
void LLPanelMediaHUD::onScrollRight(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->scrollWheel(0, 0, -1, 0, MASK_NONE);
		}
	}
}

//static
void LLPanelMediaHUD::onScrollRightHeld(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		self->mScrollState = SCROLL_RIGHT;
	}
}

//static
void LLPanelMediaHUD::onScrollLeft(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->scrollWheel(0, 0, 1, 0, MASK_NONE);
		}
	}
}

//static
void LLPanelMediaHUD::onScrollLeftHeld(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		self->mScrollState = SCROLL_LEFT;
	}
}

//static
void LLPanelMediaHUD::onScrollDown(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->scrollWheel(0, 0, 0, 1, MASK_NONE);
		}
	}
}

//static
void LLPanelMediaHUD::onScrollDownHeld(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		self->mScrollState = SCROLL_DOWN;
	}
}

//static
void LLPanelMediaHUD::onScrollStop(void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		self->mScrollState = SCROLL_NONE;
	}
}

//static
void LLPanelMediaHUD::onVolumeChange(LLUICtrl* ctrl, void* datap)
{
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self && self->mVolumeSlider)
	{
		LLViewerMediaImpl* mediap = self->getTargetMediaImpl();
		if (mediap)
		{
			mediap->setVolume(self->mVolumeSlider->getValueF32());
		}
	}
}

//static
void LLPanelMediaHUD::onHoverSlider(LLUICtrl* ctrl, void* datap)
{
	onHoverVolume(datap);
}

//static
void LLPanelMediaHUD::onHoverVolume(void* datap)
{
	static LLCachedControl<F32> control_timeout(gSavedSettings,
												"MediaControlTimeout");
	LLPanelMediaHUD* self = (LLPanelMediaHUD*)datap;
	if (self)
	{
		self->mVolumeSliderTimer.reset();
		self->mVolumeSliderTimer.setTimerExpirySec(control_timeout);
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLViewerMediaFocus class proper
///////////////////////////////////////////////////////////////////////////////

LLViewerMediaFocus::LLViewerMediaFocus()
:	mFocusedObjectFace(0),
	mFocusedIsHUDObject(false),
	mHoverObjectFace(0)
{
}

LLViewerMediaFocus::~LLViewerMediaFocus()
{
	// The destructor for LLSingletons happens at atexit() time, which is too
	// late to do much. Clean up in cleanupClass() instead.
}

void LLViewerMediaFocus::cleanupClass()
{
	LLViewerMediaFocus* self = LLViewerMediaFocus::getInstance();
	if (self)
	{
		if (self->mMediaHUD.get())
		{
			// Paranoia: the media HUD is normally already deleted at this
			// point.
			self->mMediaHUD.get()->resetZoomLevel();
			self->mMediaHUD.get()->setMediaFace(NULL);
		}
		self->mFocusedObjectID.setNull();
		self->mFocusedImplID.setNull();
	}
}

void LLViewerMediaFocus::setFocusFace(bool face_auto_zoom,
									  LLPointer<LLViewerObject> objectp,
									  S32 face, viewer_media_t mediap,
									  LLVector3 pick_normal)
{
	LLParcel* parcel = gViewerParcelMgr.getAgentParcel();
	if (!parcel) return;
	bool allow_media_zoom = !parcel->getMediaPreventCameraZoom();

	static LLCachedControl<bool> media_ui(gSavedSettings, "MediaOnAPrimUI");

	LLViewerMediaImpl* old_mediap = getFocusedMediaImpl();
	if (old_mediap)
	{
		old_mediap->focus(false);
	}

	// Always clear the stored selection.
	mSelection = NULL;

	if (mediap.notNull() && objectp.notNull())
	{
		// Clear the current selection. If we are setting focus on a face,
		// we will reselect the correct object below, when and if appropriate.
		gSelectMgr.deselectAll();

		mPrevFocusedImplID.setNull();
		mFocusedImplID = mediap->getMediaTextureID();
		mFocusedObjectID = objectp->getID();
		mFocusedObjectFace = face;
		mFocusedObjectNormal = pick_normal;
		mFocusedIsHUDObject = objectp->isHUDAttachment();
		if (mFocusedIsHUDObject)
		{
			// Make sure the "used on HUD" flag is set for this impl
			mediap->setUsedOnHUD(true);
		}

		LL_DEBUGS("Media") << "Focusing on object: " << mFocusedObjectID
						   << ", face #" << mFocusedObjectFace << LL_ENDL;

		// Focusing on a media face clears its disable flag.
		mediap->setDisabled(false);

		if (!mediap->isParcelMedia())
		{
			LLTextureEntry* tep = objectp->getTE(face);
			if (tep && tep->hasMedia())
			{
				LLMediaEntry* mep = tep->getMediaData();
				allow_media_zoom = mep && mep->getAutoZoom();
				if (!mep)
				{
					// This should never happen.
					llwarns << "Cannot find media implement for focused face"
							<< llendl;
				}
				else if (!mediap->hasMedia())
				{
					std::string url =
						mep->getCurrentURL().empty() ? mep->getHomeURL()
													 : mep->getCurrentURL();
					mediap->navigateTo(url, "", true);
				}
			}
			else
			{
				// This should never happen.
				llwarns << "Can't find media entry for focused face" << llendl;
			}
		}

		if (!mFocusedIsHUDObject)
		{
			// Set the selection in the selection manager so we can draw the
			// focus ring.
			mSelection = gSelectMgr.selectObjectOnly(objectp, face);
		}

		mediap->focus(true);
		gFocusMgr.setKeyboardFocus(this);
		gEditMenuHandlerp = mediap;

		gFocusMgr.setKeyboardFocus(this);

		// We must do this before  processing the media HUD zoom, or it may
		// zoom to the wrong face. 
		update();

		// Zoom if necessary and possible
		if (media_ui && !mFocusedIsHUDObject && mMediaHUD.get())
		{
			static LLCachedControl<U32> auto_zoom(gSavedSettings,
												  "MediaAutoZoom");
			if (auto_zoom == 1)
			{
				allow_media_zoom = false;
			}
			else if (auto_zoom == 2)
			{
				allow_media_zoom = true;
			}
			if (face_auto_zoom && allow_media_zoom)
			{
				mMediaHUD.get()->resetZoomLevel();
				mMediaHUD.get()->nextZoomLevel();
			}
		}
	}
	else
	{
		LL_DEBUGS("Media") << "Focus lost (no object)." << LL_ENDL;

		if (hasFocus())
		{
			gFocusMgr.setKeyboardFocus(NULL);
		}

		LLViewerMediaImpl* mediap = getFocusedMediaImpl();
		if (gEditMenuHandlerp == mediap)
		{
			gEditMenuHandlerp = NULL;
		}

		mFocusedImplID.setNull();
		// Null out the media hud media pointer
		if (mMediaHUD.get())
		{
			mMediaHUD.get()->resetZoomLevel();
			mMediaHUD.get()->setMediaFace(NULL);
		}

		if (objectp)
		{
			// Still record the focused object... it may mean we need to load
			// media data. This will aid us in determining this object is
			// "important enough"
			mFocusedObjectID = objectp->getID();
			mFocusedObjectFace = face;
			mFocusedIsHUDObject = objectp->isHUDAttachment();
		}
		else
		{
			mFocusedObjectID.setNull();
			mFocusedObjectFace = 0;
			mFocusedIsHUDObject = false;
		}
	}
	if (media_ui && mMediaHUD.get())
	{
		mMediaHUD.get()->setMediaFocus(mFocusedObjectID.notNull());
	}
}

void LLViewerMediaFocus::clearFocus()
{
	mPrevFocusedImplID = mFocusedImplID;
	setFocusFace(false, NULL, 0, NULL);
}

void LLViewerMediaFocus::setHoverFace(LLPointer<LLViewerObject> objectp,
									  S32 face, viewer_media_t mediap,
									  LLVector3 pick_normal)
{
	if (mediap)
	{
		mHoverImplID = mediap->getMediaTextureID();
		mHoverObjectID = objectp->getID();
		mHoverObjectFace = face;
		mHoverObjectNormal = pick_normal;
	}
	else
	{
		mHoverObjectID.setNull();
		mHoverObjectFace = 0;
		mHoverImplID.setNull();
	}
}

void LLViewerMediaFocus::clearHover()
{
	setHoverFace(NULL, 0, NULL);
}

bool LLViewerMediaFocus::getFocus()
{
	return gFocusMgr.getKeyboardFocus() == this;
}

// This function selects an ideal viewing distance given a selection bounding
// box, normal, and padding value
void LLViewerMediaFocus::setCameraZoom(LLViewerObject* objectp,
									   LLVector3 normal, F32 padding_factor,
									   bool zoom_in_only)
{
	if (mFocusedIsHUDObject)
	{
		// Do not try to zoom on HUD objects...
		return;
	}
	if (!objectp)
	{
		// If we have no object, focus back on the avatar.
		gAgent.setFocusOnAvatar();
		return;
	}

	gAgent.setFocusOnAvatar(false);

	LLBBox bbox = objectp->getBoundingBoxAgent();
	LLVector3d center = gAgent.getPosGlobalFromAgent(bbox.getCenterAgent());
	F32 height, width, depth, angle_of_view, distance;
	// We need the aspect ratio, and the 3 components of the bbox as
	// height, width, and depth.
	F32 aspect_ratio = getBBoxAspectRatio(bbox, normal, &height, &width,
										  &depth);
	F32 camera_aspect = gViewerCamera.getAspect();

	// We will normally use the side of the volume aligned with the short side
	// of the screen (i.e. the height for a screen in a landscape aspect
	// ratio), however there is an edge case where the aspect ratio of the
	// object is more extreme than the screen. In this case we invert the
	// logic, using the longer component of both the object and the screen.
	bool invert = (camera_aspect > 1.f && aspect_ratio > camera_aspect) ||
				  (camera_aspect < 1.f && aspect_ratio < camera_aspect);

	// To calculate the optimum viewing distance we will need the angle of the
	// shorter side of the view rectangle. In portrait mode this is the width,
	// and in landscape it is the height. We then calculate the distance based
	// on the corresponding side of the object bbox (width for portrait, height
	// for landscape). We will add half the depth of the bounding box, as the
	// distance projection uses the center point of the bbox.
	if (camera_aspect < 1.f || invert)
	{
		angle_of_view =
			llmax(0.1f, gViewerCamera.getView() * gViewerCamera.getAspect());
		distance = width * 0.5f * padding_factor / tanf(angle_of_view * 0.5f);
	}
	else
	{
		angle_of_view = llmax(0.1f, gViewerCamera.getView());
		distance = height * 0.5f * padding_factor / tanf(angle_of_view * 0.5f);
	}
	distance += depth * 0.5;

	// Finally animate the camera to this new position and focal point
	LLVector3d camera_pos, target_pos;
	// The target lookat position is the center of the selection (in global
	// coords)
	target_pos = center;
	// Target look-from (camera) position is "distance" away from the target
	// along the normal 
	LLVector3d pick_norm = LLVector3d(normal);
	pick_norm.normalize();
	camera_pos = target_pos + pick_norm * distance;
	if (pick_norm == LLVector3d::z_axis || pick_norm == LLVector3d::z_axis_neg)
	{
		// If the normal points directly up, the camera will "flip" around. We
		// try to avoid this by adjusting the target camera position a  smidge
		// towards current camera position
		// NOTE: this solution is not perfect. All it attempts to solve is the
		// "looking down" problem where the camera flips around when it
		// animates to that position. You still are not guaranteed to be
		// looking at the media in the correct orientation. What this solution
		// does is it will put the camera into position keeping as best it can
		// the current orientation with respect to the face. In other words, if
		// before zoom the media appears "upside down" from the camera, after
		// zooming it will still be upside down, but at least it will not flip.
		LLVector3d cur_cam_pos = LLVector3d(gAgent.getCameraPositionGlobal());
		LLVector3d delta = cur_cam_pos - camera_pos;
		F64 len = delta.length();
		delta.normalize();
		// Move 1% of the distance towards original camera location
		camera_pos += 0.01 * len * delta;
	}

	// If we are not allowing zooming out and the old camera position is closer
	// to the center then the new intended camera position, do not move camera
	// and return.
	if (zoom_in_only &&
		dist_vec_squared(gAgent.getCameraPositionGlobal(),
						 target_pos) < dist_vec_squared(camera_pos,
														target_pos))
	{
		return;
	}

	gAgent.setCameraPosAndFocusGlobal(camera_pos, target_pos,
									  objectp->getID());
}

void LLViewerMediaFocus::focusZoomOnMedia(const LLUUID& media_id)
{
	LLViewerMediaImpl* mediap =
		LLViewerMedia::getMediaImplFromTextureID(media_id);
	if (!mediap)
	{
		return;
	}

	// Get the first object from the media implement's object list. This is
	// completely arbitrary, but suffices when the object got only one media
	// implement.
	LLVOVolume* objectp = mediap->getSomeObject();
	if (!objectp)
	{
		return;
	}

	// This media is attached to at least one object. Figure out which face it
	// is on.
	S32 face = objectp->getFaceIndexWithMediaImpl(mediap, -1);

	// We do not have a proper pick normal here, and finding a face's real
	// normal is... complicated.
	LLVector3 normal = objectp->getApproximateFaceNormal(face);
	if (normal.isNull())
	{
		// If that did not work, use the inverse of the camera "look at" axis,
		// which should keep the camera pointed in the same direction.
		normal = gViewerCamera.getAtAxis();
		normal *= -1.f;
	}

	// Focus on that face.
	setFocusFace(false, objectp, face, mediap, normal);
	// Attempt to zoom on that face.
	if (mMediaHUD.get() && !objectp->isHUDAttachment())
	{
		mMediaHUD.get()->resetZoomLevel();
		mMediaHUD.get()->nextZoomLevel();
	}
}

void LLViewerMediaFocus::unZoom()
{
	if (mMediaHUD.get() && mMediaHUD.get()->isZoomed())
	{
		mMediaHUD.get()->nextZoomLevel();
	}
}

bool LLViewerMediaFocus::isZoomed()
{
	return mMediaHUD.get() && mMediaHUD.get()->isZoomed();
}

bool LLViewerMediaFocus::isZoomedOnMedia(const LLUUID& media_id)
{
	return isZoomed() &&
		   (mFocusedImplID == media_id || mPrevFocusedImplID == media_id);
}

void LLViewerMediaFocus::onFocusReceived()
{
	LLViewerMediaImpl* mediap = getFocusedMediaImpl();
	if (mediap)
	{
		mediap->focus(true);
	}
	LLFocusableElement::onFocusReceived();
}

void LLViewerMediaFocus::onFocusLost()
{
	LLViewerMediaImpl* mediap = getFocusedMediaImpl();
	if (mediap)
	{
		mediap->focus(false);
	}
	gViewerWindowp->focusClient();
	LLFocusableElement::onFocusLost();
}

bool LLViewerMediaFocus::handleKey(KEY key, MASK mask, bool called_from_parent)
{
	LLViewerMediaImpl* mediap = getFocusedMediaImpl();
	if (mediap)
	{
		mediap->handleKeyHere(key, mask);
	}
	return true;
}

bool LLViewerMediaFocus::handleKeyUp(KEY key, MASK mask,
									 bool called_from_parent)
{
	LLViewerMediaImpl* mediap = getFocusedMediaImpl();
	if (mediap)
	{
		mediap->handleKeyUpHere(key, mask);
	}
	return true;
}

bool LLViewerMediaFocus::handleUnicodeChar(llwchar uni_char,
										   bool called_from_parent)
{
	LLViewerMediaImpl* mediap = getFocusedMediaImpl();
	if (mediap)
	{
		mediap->handleUnicodeCharHere(uni_char);
	}
	return true;
}

bool LLViewerMediaFocus::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	LLViewerMediaImpl* mediap = getFocusedMediaImpl();
	if (mediap && mediap->hasMedia() && gKeyboardp)
	{
		mediap->scrollWheel(x, y, 0, clicks, gKeyboardp->currentMask(true));
		return true;
	}
	return false;
}

void LLViewerMediaFocus::update()
{
	static LLCachedControl<bool> media_ui(gSavedSettings, "MediaOnAPrimUI");
	if (media_ui && mMediaHUD.get())
	{
		if (mFocusedImplID.notNull() || mMediaHUD.get()->isMouseOver())
		{
			//mMediaHUD.get()->setVisible(true);
			mMediaHUD.get()->updateShape();
		}
		else
		{
			mMediaHUD.get()->setVisible(false);
		}
	}

	LLViewerMediaImpl* mediap = getFocusedMediaImpl();
	LLViewerObject* objectp = getFocusedObject();
	S32 face = mFocusedObjectFace;
	LLVector3 normal = mFocusedObjectNormal;

	if (!mediap || !objectp)
	{
		mediap = getHoverMediaImpl();
		objectp = getHoverObject();
		face = mHoverObjectFace;
		normal = mHoverObjectNormal;
	}

	if (media_ui && mediap && objectp)
	{
		static F32 last_update = 0.f;
		// We have an object and implement to point at. Make sure the media HUD
		// object exists.
		if (!mMediaHUD.get())
		{
			LLPanelMediaHUD* media_hud = new LLPanelMediaHUD();
			mMediaHUD = media_hud->getHandle();
			if (gHUDViewp)
			{
				gHUDViewp->addChild(media_hud);
			}
			mMediaHUD.get()->setMediaFace(objectp, face, mediap, normal);
		}
		// Do not update every frame: that would be insane !
		else if (gFrameTimeSeconds > last_update + 0.5f)
		{
			last_update = gFrameTimeSeconds;
			mMediaHUD.get()->setMediaFace(objectp, face, mediap, normal);
		}
		mPrevFocusedImplID.setNull();
		mFocusedImplID = mediap->getMediaTextureID();
	}
	else if (mMediaHUD.get())
	{
		// The media HUD is no longer needed.
		mMediaHUD.get()->resetZoomLevel();
		mMediaHUD.get()->setMediaFace(NULL);
	}
}

// This function calculates the aspect ratio and the world aligned components
// of a selection bounding box.
F32 LLViewerMediaFocus::getBBoxAspectRatio(const LLBBox& bbox,
										   const LLVector3& normal,
										   F32* height, F32* width, F32* depth)
{
	// Convert the selection normal and an up vector to local coordinate space
	// of the bbox
	LLVector3 local_normal = bbox.agentToLocalBasis(normal);
	LLVector3 z_vec = bbox.agentToLocalBasis(LLVector3::z_axis);

	LLVector3 comp1, comp2;
	LLVector3 bbox_max = bbox.getExtentLocal();
	F32 dot1 = 0.f;
	F32 dot2 = 0.f;

	// The largest component of the localized normal vector is the depth
	// component meaning that the other two are the legs of the rectangle.
	local_normal.abs();
	if (local_normal.mV[VX] > local_normal.mV[VY])
	{
		if (local_normal.mV[VX] > local_normal.mV[VZ])
		{
			// Use the y and z comps
			comp1.mV[VY] = bbox_max.mV[VY];
			comp2.mV[VZ] = bbox_max.mV[VZ];
			*depth = bbox_max.mV[VX];
		}
		else
		{
			// Use the x and y comps
			comp1.mV[VY] = bbox_max.mV[VY];
			comp2.mV[VZ] = bbox_max.mV[VZ];
			*depth = bbox_max.mV[VZ];
		}
	}
	else if (local_normal.mV[VY] > local_normal.mV[VZ])
	{
		// Use the x and z comps
		comp1.mV[VX] = bbox_max.mV[VX];
		comp2.mV[VZ] = bbox_max.mV[VZ];
		*depth = bbox_max.mV[VY];
	}
	else
	{
		// Use the x and y comps
		comp1.mV[VY] = bbox_max.mV[VY];
		comp2.mV[VZ] = bbox_max.mV[VZ];
		*depth = bbox_max.mV[VX];
	}

	// The height is the vector closest to vertical in the bbox coordinate
	// space (highest dot product value)
	dot1 = comp1 * z_vec;
	dot2 = comp2 * z_vec;
	if (fabs(dot1) > fabs(dot2))
	{
		*height = comp1.length();
		*width = comp2.length();
	}
	else
	{
		*height = comp2.length();
		*width = comp1.length();
	}

	// Return the aspect ratio.
	return *width / *height;
}

bool LLViewerMediaFocus::isFocusedOnFace(LLPointer<LLViewerObject> objectp,
										 S32 face)
{
	return objectp->getID() == mFocusedObjectID && face == mFocusedObjectFace;
}

bool LLViewerMediaFocus::isHoveringOverFace(LLPointer<LLViewerObject> objectp,
											S32 face)
{
	return objectp->getID() == mHoverObjectID && face == mHoverObjectFace;
}

LLViewerMediaImpl* LLViewerMediaFocus::getFocusedMediaImpl()
{
	return LLViewerMedia::getMediaImplFromTextureID(mFocusedImplID);
}

LLViewerObject* LLViewerMediaFocus::getFocusedObject()
{
	return gObjectList.findObject(mFocusedObjectID);
}

LLViewerMediaImpl* LLViewerMediaFocus::getHoverMediaImpl()
{
	return LLViewerMedia::getMediaImplFromTextureID(mHoverImplID);
}

LLViewerObject* LLViewerMediaFocus::getHoverObject()
{
	return gObjectList.findObject(mHoverObjectID);
}
