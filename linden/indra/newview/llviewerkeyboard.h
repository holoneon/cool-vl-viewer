/**
 * @file llviewerkeyboard.h
 * @brief LLViewerKeyboard class header file
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 *
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llkeyboard.h" // For EKeystate

constexpr S32 MAX_NAMED_FUNCTIONS = 100;
constexpr S32 MAX_KEY_BINDINGS = 128; // was 60

class LLNamedFunction
{
public:
	LLNamedFunction() : mFunction(NULL)	{}
	~LLNamedFunction()					{}

	std::string	mName;
	LLKeyFunc	mFunction;
};

typedef enum e_keyboard_mode
{
	MODE_FIRST_PERSON,
	MODE_THIRD_PERSON,
	MODE_EDIT,
	MODE_EDIT_AVATAR,
	MODE_SITTING,
	MODE_COUNT
} EKeyboardMode;

void bind_keyboard_functions();

class LLViewerKeyboard
{
protected:
	LOG_CLASS(LLViewerKeyboard);

public:
	LLViewerKeyboard();

	bool handleKey(KEY key, MASK mask, bool repeated);
	bool handleKeyUp(KEY key, MASK mask);

	void bindNamedFunction(const std::string& name, LLKeyFunc func);

	// Returns number bound, 0 on error
	S32 loadBindings(const std::string& filename);

	EKeyboardMode getMode();

	// false on failure
	bool modeFromString(const std::string& string, S32* mode);

	void scanKey(KEY key, bool key_down, bool key_up, bool key_level);

protected:
	bool bindKey(S32 mode, KEY key, MASK mask, const std::string& func_name);

protected:
	S32				mNamedFunctionCount;
	LLNamedFunction	mNamedFunctions[MAX_NAMED_FUNCTIONS];

	// Hold all the ugly stuff torn out to make LLKeyboard non-viewer-specific
	// here
	S32				mBindingCount[MODE_COUNT];
	LLKeyBinding	mBindings[MODE_COUNT][MAX_KEY_BINDINGS];

	typedef std::map<U32, U32> key_remap_t;
	key_remap_t		mRemapKeys[MODE_COUNT];
	std::set<KEY>	mKeysSkippedByUI;
	// Key processed successfully by UI
	bool			mKeyHandledByUI[KEY_COUNT];
};

extern LLViewerKeyboard gViewerKeyboard;
