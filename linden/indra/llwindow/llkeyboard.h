/**
 * @file llkeyboard.h
 * @brief Handler for assignable key bindings
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

#include <functional>
#include <map>

#include "indra_constants.h"
#include "llstringtable.h"
#include "lltimer.h"

enum EKeystate
{
	KEYSTATE_DOWN,
	KEYSTATE_LEVEL,
	KEYSTATE_UP
};

typedef void (*LLKeyFunc)(EKeystate keystate);

enum EKeyboardInsertMode
{
	LL_KIM_INSERT,
	LL_KIM_OVERWRITE
};

class LLKeyBinding
{
public:
	KEY			mKey;
	MASK		mMask;
#if 0	// unused
	const char*	mName;
#endif
	LLKeyFunc	mFunction;
};

class LLWindowCallbacks;

class LLKeyboard
{
protected:
	LOG_CLASS(LLKeyboard);

public:
	typedef enum e_numpad_distinct
	{
		ND_NEVER,
		ND_NUMLOCK_OFF,
		ND_NUMLOCK_ON
	} ENumpadDistinct;

public:
	LLKeyboard();
	virtual ~LLKeyboard()							{}

	void resetKeys();

	LL_INLINE F32  getCurKeyElapsedTime()			{ return getKeyDown(mCurScanKey) ? getKeyElapsedTime(mCurScanKey) : 0.f; }
	LL_INLINE F32  getCurKeyElapsedFrameCount()		{ return getKeyDown(mCurScanKey) ? (F32)getKeyElapsedFrameCount(mCurScanKey) : 0.f; }
	LL_INLINE bool getKeyDown(KEY key)				{ return mKeyLevel[key]; }
	LL_INLINE bool getKeyRepeated(KEY key)			{ return mKeyRepeated[key]; }

	bool translateKey(U32 os_key, KEY* translated_key, MASK mask);
	U32 inverseTranslateKey(KEY translated_key);
	// Translated into "Linden" keycodes
	bool handleTranslatedKeyUp(KEY translated_key, U32 translated_mask);
	// Translated into "Linden" keycodes
	bool handleTranslatedKeyDown(KEY translated_key, U32 translated_mask);

	virtual bool handleKeyUp(U32 key, MASK mask) = 0;
	virtual bool handleKeyDown(U32 key, MASK mask) = 0;

	// Asynchronously poll the control, alt, and shift keys and set the
	// appropriate internal key masks.

	virtual void resetMaskKeys() = 0;
	// scans keyboard, calls functions as necessary:
	virtual void scanKeyboard() = 0;
	// Mac must differentiate between Command = Control for keyboard events
	// and Command != Control for mouse events.
	virtual MASK currentMask(bool for_mouse_event) = 0;
	LL_INLINE virtual KEY currentKey()				{ return mCurTranslatedKey; }

	LL_INLINE EKeyboardInsertMode getInsertMode()	{ return mInsertMode; }
	void toggleInsertMode();

	// false on failure
	static bool maskFromString(const char* str, MASK* mask);
	static bool keyFromString(const char* str, KEY* key);

	static std::string stringFromKey(KEY key);

	e_numpad_distinct getNumpadDistinct()			{ return mNumpadDistinct; }
	void setNumpadDistinct(e_numpad_distinct val)	{ mNumpadDistinct = val; }

	void setCallbacks(LLWindowCallbacks* cbs)		{ mCallbacks = cbs; }

	// Returns time in seconds since key was pressed:
	F32 getKeyElapsedTime(KEY key);
	// Returns time in frames since key was pressed:
	S32 getKeyElapsedFrameCount(KEY key);

protected:
	void addKeyName(KEY key, const std::string& name);

protected:
	// Map of translations from OS keys to Linden KEYs
	std::map<U32, KEY>		mTranslateKeyMap;
	// Map of translations from Linden KEYs to OS keys
	std::map<KEY, U32>		mInvTranslateKeyMap;
	LLWindowCallbacks*		mCallbacks;
	// Time since level was set
	LLTimer					mKeyLevelTimer[KEY_COUNT];
	// Frames since level was set
	S32						mKeyLevelFrameCount[KEY_COUNT];
	KEY						mCurTranslatedKey;
	// Used during the scanKeyboard()
	KEY						mCurScanKey;

	e_numpad_distinct		mNumpadDistinct;
	EKeyboardInsertMode		mInsertMode;

	bool					mKeyLevel[KEY_COUNT];		// Levels
	bool					mKeyRepeated[KEY_COUNT];	// Key was repeated
	bool					mKeyUp[KEY_COUNT];			// Up edge
	bool					mKeyDown[KEY_COUNT];		// Down edge

	typedef std::map<KEY, std::string> key2name_map_t;
	static key2name_map_t	sKeysToNames;
	typedef std::map<std::string, KEY, std::less<> > name2key_map_t;
	static name2key_map_t	sNamesToKeys;
};

extern LLKeyboard* gKeyboardp;
