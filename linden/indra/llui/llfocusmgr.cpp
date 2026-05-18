/**
 * @file llfocusmgr.cpp
 * @brief LLFocusMgr base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "lluictrl.h"		// Also includes llfocusmgr.h
#include "llcolor4.h"

constexpr F32 FOCUS_FADE_TIME = 0.3f;

LLFocusMgr gFocusMgr;

///////////////////////////////////////////////////////////////////////////////
// LLFocusableElement class
///////////////////////////////////////////////////////////////////////////////

LLFocusableElement::LLFocusableElement()
:	mFocusLostCallback(NULL),
	mFocusReceivedCallback(NULL),
	mFocusChangedCallback(NULL),
	mFocusCallbackUserData(NULL)
{
}

void LLFocusableElement::setFocusLostCallback(void (*cb)(LLFocusableElement*,
														 void*),
											  void* user_data)
{
	mFocusLostCallback = cb;
	mFocusCallbackUserData = user_data;
}

void LLFocusableElement::setFocusReceivedCallback(void (*cb)(LLFocusableElement*,
															 void*),
												  void* user_data)
{
	mFocusReceivedCallback = cb;
	mFocusCallbackUserData = user_data;
}

void LLFocusableElement::setFocusChangedCallback(void (*cb)(LLFocusableElement*,
															void*),
												 void* user_data)
{
	mFocusChangedCallback = cb;
	mFocusCallbackUserData = user_data;
}

void LLFocusableElement::onFocusReceived()
{
	if (mFocusReceivedCallback)
	{
		mFocusReceivedCallback(this, mFocusCallbackUserData);
	}
	if (mFocusChangedCallback)
	{
		mFocusChangedCallback(this, mFocusCallbackUserData);
	}
}

void LLFocusableElement::onFocusLost()
{
	if (mFocusLostCallback)
	{
		mFocusLostCallback(this, mFocusCallbackUserData);
	}

	if (mFocusChangedCallback)
	{
		mFocusChangedCallback(this, mFocusCallbackUserData);
	}
}

bool LLFocusableElement::hasFocus() const
{
	return gFocusMgr.getKeyboardFocus() == this;
}

///////////////////////////////////////////////////////////////////////////////
// LLFocusMgr class
///////////////////////////////////////////////////////////////////////////////

LLFocusMgr::LLFocusMgr()
:	mLockedView(NULL),
	mMouseCaptor(NULL),
	mKeyboardFocus(NULL),
	mLastKeyboardFocus(NULL),
	mDefaultKeyboardFocus(NULL),
	mKeystrokesOnly(false),
	mTopCtrl(NULL),
	mFocusWeight(0.f),
	mFocusColor(LLColor4::white),
#if LL_DEBUG
	mMouseCaptorName("none"),
	mKeyboardFocusName("none"),
	mTopCtrlName("none"),
#endif
	mAppHasFocus(true)
{
}

LLFocusMgr::~LLFocusMgr()
{
	mFocusHistory.clear();
}

void LLFocusMgr::releaseFocusIfNeeded(const LLView* viewp)
{
	if (childHasMouseCapture(viewp))
	{
		setMouseCapture(NULL);
	}

	if (childHasKeyboardFocus(viewp))
	{
		if (viewp == mLockedView)
		{
			mLockedView = NULL;
			setKeyboardFocus(NULL);
		}
		else
		{
			setKeyboardFocus(mLockedView, false, mKeystrokesOnly);
		}
	}

	if (childIsTopCtrl(viewp))
	{
		setTopCtrl(NULL);
	}
}

LLUICtrl* LLFocusMgr::getKeyboardFocusUICtrl()
{
	if (mKeyboardFocus && mKeyboardFocus->isFocusableCtrl())
	{
		return (LLUICtrl*)mKeyboardFocus;
	}
	return NULL;
}

LLUICtrl* LLFocusMgr::getLastKeyboardFocusUICtrl()
{
	if (mLastKeyboardFocus && mLastKeyboardFocus->isFocusableCtrl())
	{
		return (LLUICtrl*)mLastKeyboardFocus;
	}
	return NULL;
}

void LLFocusMgr::setKeyboardFocus(LLFocusableElement* new_focusp,
								  bool lock, bool keystrokes_only)
{
	// When locked, do not allow focus to go to anything that is not the locked
	// focus or one of its descendants
	if (mLockedView)
	{
		if (!new_focusp)
		{
			return;
		}
		if (new_focusp != mLockedView)
		{
			LLView* viewp = dynamic_cast<LLView*>(new_focusp);
			if (!viewp || !viewp->hasAncestor(mLockedView))
			{
				return;
			}
		}
	}

	mKeystrokesOnly = keystrokes_only;
	if (LLView::sDebugKeys)
	{
		llinfos << "mKeystrokesOnly = " << mKeystrokesOnly << llendl;
	}

	if (new_focusp != mKeyboardFocus)
	{
		mLastKeyboardFocus = mKeyboardFocus;
		mKeyboardFocus = new_focusp;

		if (mLastKeyboardFocus)
		{
			mLastKeyboardFocus->onFocusLost();
		}

		// Clear out any existing flash
		if (new_focusp)
		{
			mFocusWeight = 0.f;
			new_focusp->onFocusReceived();
		}
		mFocusTimer.reset();

#if LL_DEBUG
		LLUICtrl* ctrlp = NULL;
		if (new_focusp && new_focusp->isFocusableCtrl())
		{
			ctrlp = (LLUICtrl*)new_focusp;
		}
		mKeyboardFocusName = ctrlp ? ctrlp->getName() : std::string("none");
#endif

		// If we have got a default keyboard focus, and the caller is releasing
		// keyboard focus, move to the default.
		if (mDefaultKeyboardFocus != NULL && mKeyboardFocus == NULL)
		{
			mDefaultKeyboardFocus->setFocus(true);
		}

		LLView* focused_viewp = dynamic_cast<LLView*>(mKeyboardFocus);
		LLView* focus_subtreep = focused_viewp;
		LLView* viewp = focus_subtreep;
		// Find root-most focus root
		while (viewp)
		{
			if (viewp->isFocusRoot())
			{
				focus_subtreep = viewp;
			}
			viewp = viewp->getParent();
		}
		if (focus_subtreep)
		{
			mFocusHistory[focus_subtreep->getHandle()] =
				focused_viewp ? focused_viewp->getHandle()
							  : LLHandle<LLView>();
		}
	}

	if (lock)
	{
		lockFocus();
	}
}

// Returns true is parent or any descedent of parent has keyboard focus.
bool LLFocusMgr::childHasKeyboardFocus(const LLView* parentp) const
{
	LLView* focus_viewp = dynamic_cast<LLView*>(mKeyboardFocus);
	while (focus_viewp)
	{
		if (focus_viewp == parentp)
		{
			return true;
		}
		focus_viewp = focus_viewp->getParent();
	}
	return false;
}

// Returns true is parent or any descedent of parent is the mouse captor.
bool LLFocusMgr::childHasMouseCapture(const LLView* parentp) const
{
	if (mMouseCaptor && mMouseCaptor->isView())
	{
		LLView* captor_viewp = (LLView*)mMouseCaptor;
		while (captor_viewp)
		{
			if (captor_viewp == parentp)
			{
				return true;
			}
			captor_viewp = captor_viewp->getParent();
		}
	}
	return false;
}

void LLFocusMgr::removeKeyboardFocusWithoutCallback(const LLFocusableElement* focusp)
{
	// should be ok to unlock here, as you have to know the locked view
	// in order to unlock it
	if (mLockedView == focusp)
	{
		mLockedView = NULL;
	}

	if (mKeyboardFocus == focusp)
	{
		mKeyboardFocus = NULL;
#if LL_DEBUG
		mKeyboardFocusName = "none";
#endif
	}
}

void LLFocusMgr::setMouseCapture(LLMouseHandler* new_captorp)
{
#if 0
	if (mFocusLocked)
	{
		return;
	}
#endif

	if (new_captorp != mMouseCaptor)
	{
		LLMouseHandler* old_captorp = mMouseCaptor;
		mMouseCaptor = new_captorp;
		if (old_captorp)
		{
			old_captorp->onMouseCaptureLost();
		}

#if LL_DEBUG
		mMouseCaptorName = new_captorp ? new_captorp->getName()
									   : std::string("none");
#endif
	}
}

void LLFocusMgr::removeMouseCaptureWithoutCallback(const LLMouseHandler* captorp)
{
#if 0
	if (mFocusLocked)
	{
		return;
	}
#endif
	if (mMouseCaptor == captorp)
	{
		mMouseCaptor = NULL;
#if LL_DEBUG
		mMouseCaptorName = "none";
#endif
	}
}

bool LLFocusMgr::childIsTopCtrl(const LLView* parentp) const
{
	LLView* top_viewp = (LLView*)mTopCtrl;
	while (top_viewp)
	{
		if (top_viewp == parentp)
		{
			return true;
		}
		top_viewp = top_viewp->getParent();
	}
	return false;
}

// Set ctrlp = NULL to release top_view.
void LLFocusMgr::setTopCtrl(LLUICtrl* ctrlp)
{
	LLUICtrl* old_topp = mTopCtrl;
	if (ctrlp != old_topp)
	{
		mTopCtrl = ctrlp;
#if LL_DEBUG
		mTopCtrlName = ctrlp ? ctrlp->getName() : std::string("none");
#endif
		if (old_topp)
		{
			old_topp->onLostTop();
		}
	}
}

void LLFocusMgr::removeTopCtrlWithoutCallback(const LLUICtrl* ctrlp)
{
	if (mTopCtrl == ctrlp)
	{
		mTopCtrl = NULL;
#if LL_DEBUG
		mTopCtrlName = "none";
#endif
	}
}

void LLFocusMgr::lockFocus()
{
	if (mKeyboardFocus && mKeyboardFocus->isFocusableCtrl())
	{
		mLockedView = (LLUICtrl*)mKeyboardFocus;
	}
	else
	{
		mLockedView = NULL;
	}
}

F32 LLFocusMgr::getFocusFlashAmt() const
{
	return clamp_rescale(getFocusTime(), 0.f, FOCUS_FADE_TIME, mFocusWeight,
						 0.f);
}

LLColor4 LLFocusMgr::getFocusColor() const
{
	LLColor4 focus_color = lerp(mFocusColor, LLColor4::white,
								getFocusFlashAmt());
	// De-emphasize keyboard focus when app has lost focus (to avoid typing
	// into wrong window problem)
	if (!mAppHasFocus)
	{
		focus_color.mV[VALPHA] *= 0.4f;
	}
	return focus_color;
}

void LLFocusMgr::triggerFocusFlash()
{
	mFocusTimer.reset();
	mFocusWeight = 1.f;
}

void LLFocusMgr::setAppHasFocus(bool focus)
{
	if (!mAppHasFocus && focus)
	{
		triggerFocusFlash();
	}

	// Release focus from "top ctrl"s, which generally hides them
	if (!focus && mTopCtrl)
	{
		setTopCtrl(NULL);
	}
	mAppHasFocus = focus;
}

LLUICtrl* LLFocusMgr::getLastFocusForGroup(LLView* viewp) const
{
	if (viewp)
	{
		focus_history_map_t::const_iterator it =
			mFocusHistory.find(viewp->getHandle());
		if (it != mFocusHistory.end())
		{
			// Found last focus for this subtree
			return (LLUICtrl*)it->second.get();
		}
	}
	return NULL;
}

void LLFocusMgr::clearLastFocusForGroup(LLView* viewp)
{
	if (viewp)
	{
		mFocusHistory.erase(viewp->getHandle());
	}
}
