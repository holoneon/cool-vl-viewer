/**
 * @file llwindow.cpp
 * @brief Basic graphical window class
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

#if LL_LINUX
# include "llwindowlinux.h"
#elif LL_WINDOWS
# include "llwindowwindows.h"
#endif

#include "llkeyboard.h"

// Globals

LLWindow* gWindowp = NULL;
LLSplashScreen* gSplashScreenp = NULL;

bool gDebugClicks = false;
bool gDebugWindowProc = false;
bool gHiDPISupport = false;

const std::string gURLProtocolWhitelist[] =
{
	"file:",
	"http:",
	"https:",
	"ftp:",
	"data:"
};
const S32 gURLProtocolWhitelistCount = LL_ARRAY_SIZE(gURLProtocolWhitelist);

// Static instance for default callbacks
LLWindowCallbacks LLWindow::sDefaultCallbacks;

// Helper function
S32 OSMessageBox(const std::string& text, const std::string& caption, U32 type)
{
	// Properly hide the splash screen when displaying the message box
	bool was_visible = LLSplashScreen::isVisible();
	if (was_visible)
	{
		LLSplashScreen::hide();
	}

	S32 result = 0;
#if LL_LINUX
	result = OSMessageBoxLinux(text, caption, type);
#elif LL_WINDOWS
	result = OSMessageBoxWindows(text, caption, type);
#else
# error("OSMessageBox not implemented for this platform !")
#endif

	if (was_visible)
	{
		LLSplashScreen::show();
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// LLWindow class
///////////////////////////////////////////////////////////////////////////////

//static
void LLWindow::createWindow(const std::string& title, S32 x, S32 y,
							U32 width, U32 height, U32 flags, bool fullscreen,
							bool disable_vsync, U32 fsaa_samples)
{
	llassert_always(gWindowp == NULL);

#if LL_LINUX
	gWindowp = new LLWindowLinux(title, x, y, width, height, flags,
								 fullscreen, disable_vsync, fsaa_samples);
#elif LL_WINDOWS
	gWindowp = new LLWindowWindows(title, x, y, width, height, flags, fullscreen,
								   disable_vsync, fsaa_samples);
#endif

	if (!gWindowp->isValid())
	{
		llwarns << "Invalid window. Destroying it." << llendl;
		delete gWindowp;
		gWindowp = NULL;
	}
}

//static
void LLWindow::destroyWindow()
{
	if (gWindowp)
	{
		gWindowp->close();
		delete gWindowp;
		gWindowp = NULL;
	}
}

LLWindow::LLWindow(bool fullscreen, U32 flags)
:	mCallbacks(&sDefaultCallbacks),
	mPostQuit(true),
	mFullscreen(fullscreen),
	mFullscreenWidth(0),
	mFullscreenHeight(0),
	mFullscreenRefresh(0),
	mOverrideAspectRatio(0.f),
	mCurrentGamma(1.f),
	mSupportedResolutions(NULL),
	mNumSupportedResolutions(0),
	mCurrentCursor(UI_CURSOR_ARROW),
	mCursorFrozen(false),
	mCursorHidden(false),
	mBusyCount(0),
	mIsMouseClipping(false),
	mSwapMethod(SWAP_METHOD_UNDEFINED),
	mHideCursorPermanent(false),
	mFlags(flags),
	mHighSurrogate(0)
{
}

void LLWindow::decBusyCount()
{
	if (mBusyCount > 0)
	{
		--mBusyCount;
	}
}

void LLWindow::setCallbacks(LLWindowCallbacks* callbacks)
{
	mCallbacks = callbacks;
	if (gKeyboardp)
	{
		gKeyboardp->setCallbacks(callbacks);
	}
}

//static
std::vector<std::string> LLWindow::getDynamicFallbackFontList()
{
#if LL_WINDOWS
	return LLWindowWindows::getDynamicFallbackFontList();
#elif LL_LINUX
	return LLWindowLinux::getDynamicFallbackFontList();
#else
	return std::vector<std::string>();
#endif
}

#define UTF16_IS_HIGH_SURROGATE(U) ((U16)((U) - 0xD800) < 0x0400)
#define UTF16_IS_LOW_SURROGATE(U)  ((U16)((U) - 0xDC00) < 0x0400)
#define UTF16_SURROGATE_PAIR_TO_UTF32(H,L) (((H) << 10) + (L) - (0xD800 << 10) - 0xDC00 + 0x00010000)

void LLWindow::handleUnicodeUTF16(U16 utf16, MASK mask)
{
	LL_DEBUGS("Window") << "UTF16 key = " << std::hex << (U32)utf16 << std::dec
						<< " - mask = " << mask << LL_ENDL;

	// Note that we could discard unpaired surrogates, but I am following the
	// Unicode Consortium's recommendation here, that is, to preserve those
	// unpaired surrogates in UTF-32 values. _To_preserve_ means to pass to the
	// callback in our context.

	if (mHighSurrogate == 0)
	{
		if (UTF16_IS_HIGH_SURROGATE(utf16))
		{
			mHighSurrogate = utf16;
		}
		else
		{
			mCallbacks->handleUnicodeChar(utf16, mask);
		}
	}
	else if (UTF16_IS_LOW_SURROGATE(utf16))
	{
		// A legal surrogate pair.
		mCallbacks->handleUnicodeChar(UTF16_SURROGATE_PAIR_TO_UTF32(mHighSurrogate,
																	utf16),
									  mask);
		mHighSurrogate = 0;
	}
	else if (UTF16_IS_HIGH_SURROGATE(utf16))
	{
		// Two consecutive high surrogates.
		mCallbacks->handleUnicodeChar(mHighSurrogate, mask);
		mHighSurrogate = utf16;
	}
	else
	{
		// A non-low-surrogate preceeded by a high surrogate.
		mCallbacks->handleUnicodeChar(mHighSurrogate, mask);
		mHighSurrogate = 0;
		mCallbacks->handleUnicodeChar(utf16, mask);
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLSplashScreen class
///////////////////////////////////////////////////////////////////////////////

//static
bool LLSplashScreen::isVisible()
{
	return gSplashScreenp != NULL;
}

//static
void LLSplashScreen::show()
{
	if (!gSplashScreenp)
	{
#if LL_LINUX
		gSplashScreenp = new LLSplashScreenLinux;
#elif LL_WINDOWS
		gSplashScreenp = new LLSplashScreenWindows;
#endif
		gSplashScreenp->showImpl();
	}
}

//static
void LLSplashScreen::update(const std::string& str)
{
	LLSplashScreen::show();
	if (gSplashScreenp)
	{
		gSplashScreenp->updateImpl(str);
	}
}

//static
void LLSplashScreen::hide()
{
	if (gSplashScreenp)
	{
		gSplashScreenp->hideImpl();
	}
	delete gSplashScreenp;
	gSplashScreenp = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// LLWindowCallbacks class
///////////////////////////////////////////////////////////////////////////////

//virtual
bool LLWindowCallbacks::handleTranslatedKeyDown(KEY, MASK, bool)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleTranslatedKeyUp(KEY, MASK)
{
	return false;
}

//virtual
void LLWindowCallbacks::handleScanKey(KEY, bool, bool, bool)
{
}

//virtual
bool LLWindowCallbacks::handleUnicodeChar(llwchar, MASK)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleMouseDown(LLWindow*, const LLCoordGL, MASK)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleMouseUp(LLWindow*, const LLCoordGL, MASK)
{
	return false;
}

//virtual
void LLWindowCallbacks::handleMouseLeave(LLWindow*)
{
	return;
}

//virtual
bool LLWindowCallbacks::handleCloseRequest(LLWindow*, bool)
{
	// Allow the window to close
	return true;
}

//virtual
bool LLWindowCallbacks::handleSessionExit(LLWindow*)
{
	return true;
}

//virtual
void LLWindowCallbacks::handleQuit(LLWindow* window)
{
	if (window == gWindowp)
	{
		LLWindow::destroyWindow();
	}
	else
	{
		llerrs << "Invalid window !" << llendl;
	}
}

//virtual
bool LLWindowCallbacks::handleRightMouseDown(LLWindow*, const LLCoordGL, MASK)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleRightMouseUp(LLWindow*, const LLCoordGL, MASK)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleMiddleMouseDown(LLWindow*, const LLCoordGL, MASK)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleMiddleMouseUp(LLWindow*, const LLCoordGL, MASK)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleActivate(LLWindow*, bool)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleActivateApp(LLWindow*, bool)
{
	return false;
}

//virtual
void LLWindowCallbacks::handleMouseMove(LLWindow*, const LLCoordGL, MASK)
{
}

//virtual
void LLWindowCallbacks::handleScrollWheel(LLWindow*, S32)
{
}

//virtual
void LLWindowCallbacks::handleResize(LLWindow*, S32, S32)
{
}

//virtual
void LLWindowCallbacks::handleFocus(LLWindow*)
{
}

//virtual
void LLWindowCallbacks::handleFocusLost(LLWindow*)
{
}

//virtual
void LLWindowCallbacks::handleMenuSelect(LLWindow*, S32)
{
}

//virtual
bool LLWindowCallbacks::handlePaint(LLWindow*, S32, S32, S32, S32)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleDoubleClick(LLWindow*, const LLCoordGL, MASK)
{
	return false;
}

//virtual
void LLWindowCallbacks::handleWindowBlock(LLWindow*)
{
}

//virtual
void LLWindowCallbacks::handleWindowUnblock(LLWindow*)
{
}

//virtual
void LLWindowCallbacks::handleDataCopy(LLWindow*, S32, void*)
{
}

//virtual
bool LLWindowCallbacks::handleTimerEvent(LLWindow*)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleDeviceChange(LLWindow*)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleDPIChanged(LLWindow*, F32, S32, S32)
{
	return false;
}

//virtual
bool LLWindowCallbacks::handleWindowDidChangeScreen(LLWindow*)
{
	return false;
}
