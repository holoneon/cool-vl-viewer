/**
 * @file llwindowlinux.h
 * @brief Linux implementation of LLWindow class
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

// Note: although SDL2 now (starting with v2.26) implements copy/paste buffers
// methods, those do *not* work properly with all genuine X11 programs, such as
// ones using (Open)Motif (e.g. Nedit). On the other hand, my (old) code, based
// on native X11 calls done via Xlib, works for all programs, so let's keep
// using it !  HB
#define HB_USE_NATIVE_X11_CLIPBOARD 1

#include <X11/Xlib.h>
#if HB_USE_NATIVE_X11_CLIPBOARD
# include <X11/Xatom.h>
#endif

#include "SDL2/SDL.h"

#include "lltimer.h"
#include "llwindow.h"

class LLWindowLinux final : public LLWindow
{
	friend class LLWindow;

protected:
	LOG_CLASS(LLWindowLinux);

public:
	void setWindowTitle(const std::string& title) override;
	LL_INLINE void show() override						{}
	LL_INLINE void hide() override						{}
	void close() override;
	void minimize() override;
	void restore() override;
	bool getVisible() override;
	bool getMinimized() override;

	// *TODO
	LL_INLINE bool getMaximized() override				{ return false; }
	LL_INLINE bool maximize() override					{ return false; }

	LL_INLINE bool getFullscreen() override				{ return mFullscreen; }
	bool getPosition(LLCoordScreen* pos) override;
	bool getSize(LLCoordScreen* size) override;
	bool getSize(LLCoordWindow* size) override;

	// SDL-specific method needed to force-redraw the screen after a graphics
	// benchmark or a GL shared context creation. HB.
	void refresh() override;

	// *TODO
	LL_INLINE bool setPosition(LLCoordScreen pos) override
	{
		return true;
	}

	bool setSize(LLCoordScreen size) override;
	bool switchContext(bool fullscreen, const LLCoordScreen& size,
					   bool disable_vsync,
					   const LLCoordScreen* const posp = NULL) override;
	void* createSharedContext() override;
	void makeContextCurrent(void* ctxp) override;
	void destroySharedContext(void* ctxp) override;
	bool setCursorPosition(const LLCoordWindow& position) override;
	bool getCursorPosition(LLCoordWindow* position) override;
	void showCursor() override;
	void hideCursor() override;
	void showCursorFromMouseMove() override;
	void hideCursorUntilMouseMove() override;
	LL_INLINE bool isCursorHidden() override			{ return mCursorHidden; }
	void setCursor(ECursorType cursor) override;
	void captureMouse() override;
	void releaseMouse() override;
	void setMouseClipping(bool b) override;

	bool isClipboardTextAvailable() override;
	bool pasteTextFromClipboard(LLWString& text) override;
	bool copyTextToClipboard(const LLWString& text) override;

	bool isPrimaryTextAvailable() override;
	bool pasteTextFromPrimary(LLWString& text) override;
	bool copyTextToPrimary(const LLWString& text) override;

	void flashIcon(F32 seconds) override;

	// Sets the gamma
	bool setGamma(F32 gamma) override;

	LL_INLINE U32 getFSAASamples() override				{ return mFSAASamples; }

	LL_INLINE void setFSAASamples(U32 n) override
	{
		// Make sure it is a multiple of 2. HB
		mFSAASamples = n & (U32_MAX - 1);
	}

	// Restore original gamma table (before updating gamma):
	bool restoreGamma() override;

	ESwapMethod getSwapMethod() override				{ return mSwapMethod; }
	void gatherInput() override;
	void swapBuffers() override;

	LL_INLINE void delayInputProcessing() override		{}

	// Handy coordinate space conversion routines
	bool convertCoords(LLCoordScreen from, LLCoordWindow* to) override;
	bool convertCoords(LLCoordWindow from, LLCoordScreen* to) override;
	bool convertCoords(LLCoordWindow from, LLCoordGL* to) override;
	bool convertCoords(LLCoordGL from, LLCoordWindow* to) override;
	bool convertCoords(LLCoordScreen from, LLCoordGL* to) override;
	bool convertCoords(LLCoordGL from, LLCoordScreen* to) override;

	LLWindowResolution* getSupportedResolutions(S32& num_res) override;
	F32	getNativeAspectRatio() override;
	F32 getPixelAspectRatio() override;

	void beforeDialog() override;
	void afterDialog() override;

	void* getPlatformWindow() override;
	void bringToFront() override;

	void spawnWebBrowser(const std::string& escaped_url, bool async) override;

	// *HACK: to compute window borders offsets
	void calculateBordersOffsets() override;

#if HB_USE_NATIVE_X11_CLIPBOARD
	bool getSelectionText(Atom selection, LLWString& text);
	bool setSelectionText(Atom selection, const LLWString& text);
	LL_INLINE LLWString& getPrimaryText()				{ return mPrimaryClipboard; }
	LL_INLINE LLWString& getSecondaryText()				{ return mSecondaryClipboard; }
	LL_INLINE void clearPrimaryText()					{ mPrimaryClipboard.clear(); }
	LL_INLINE void clearSecondaryText()					{ mSecondaryClipboard.clear(); }
#endif

	static void initXlibThreads();

	static std::vector<std::string> getDynamicFallbackFontList();

protected:
	LLWindowLinux(const std::string& title, S32 x, S32 y, U32 width,
				  U32 height, U32 flags, bool fullscreen, bool disable_vsync,
				  U32 fsaa_samples);
	~LLWindowLinux() override;

	LL_INLINE bool isValid() override					{ return mWindow != NULL; }
	LLSD getNativeKeyData() override;

	void initCursors();
	void quitCursors();
	void moveWindow(const LLCoordScreen& position, const LLCoordScreen& size);

	//
	// Platform specific methods
	//

	// Creates or re-creates the GL context/window. Called from the constructor
	// and switchContext():
	bool createContext(S32 x, S32 y, S32 width, S32 height, S32 bits,
					   bool fullscreen, bool disable_vsync);
	void destroyContext();
	void setupFailure(const std::string& text);
	void fixWindowSize();
	U32 SDLCheckGrabbyKeys(U32 keysym, bool gain);
	bool SDLReallyCaptureInput(bool capture);

	void x11_set_urgent(bool urgent);
#if HB_USE_NATIVE_X11_CLIPBOARD
	void initialiseX11Clipboard();
#endif

private:
	void setWindowIcon();
#if HB_USE_NATIVE_X11_CLIPBOARD
	void processX11ClipboardEvent(SDL_Event& evt);
	bool convertX11Selection(Atom selection, Atom target);
#endif

	// Returns true when successful, false otherwise.
	bool getFullScreenSize(S32& width, S32& height);

public:
	// Not great that these are public, but they have to be accessible
	// by non-class code and it is better than making them global.
	Window			mSDL_XWindowID;
	Display*		mSDL_Display;
	SDL_Window*		mWindow;

protected:
	//
	// Platform specific variables
	//
	SDL_GLContext	mContext;

	S32				mInitialPosX;
	S32				mInitialPosY;
	S32				mPosOffsetX;
	S32				mPosOffsetY;

#if HB_USE_NATIVE_X11_CLIPBOARD
	LLWString		mSecondaryClipboard;
#endif

	std::string		mWindowTitle;
	F32				mOriginalAspectRatio;
	U32				mFSAASamples;

	S32				mSDLFlags;

	SDL_Cursor*		mSDLCursors[UI_CURSOR_COUNT];

	U16				mPrevGammaRamp[3][256];
	U16				mCurrentGammaRamp[256];

	U32				mKeyModifiers;
	U32				mKeyVirtualKey;

	U32				mGrabbyKeyFlags;
	bool			mCaptured;

	LLTimer			mFlashTimer;
	bool			mFlashing;

	bool			mCustomGammaSet;
};

class HBSplashScreenSDLImpl;

class LLSplashScreenLinux final : public LLSplashScreen
{
public:
	LLSplashScreenLinux();
	~LLSplashScreenLinux() override;

	void showImpl() override;
	void updateImpl(const std::string& mesg) override;
	void hideImpl() override;

private:
	HBSplashScreenSDLImpl*	mImpl;
};

S32 OSMessageBoxLinux(const std::string& text, const std::string& caption,
					  U32 type);

extern bool gXlibThreadSafe;
extern bool gXWayland;
extern bool gUseFullDesktop;
