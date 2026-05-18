/**
 * @file llkeyboardwindows.h
 * @brief Handler for assignable key bindings
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llkeyboard.h"

// This mask distinguishes extended keys, which include non-numpad arrow keys
// (and, curiously, the num lock and numpad '/')
const MASK MASK_EXTENDED =  0x0100;

class LLKeyboardWindows final : public LLKeyboard
{
public:
	LLKeyboardWindows();
	~LLKeyboardWindows() override		{}

	bool handleKeyUp(U32 key, MASK mask) override;
	bool handleKeyDown(U32 key, MASK mask) override;
	void resetMaskKeys() override;
	MASK currentMask(bool for_mouse_event) override;
	void scanKeyboard() override;

protected:
	bool translateExtendedKey(U32 os_key, MASK mask, KEY* translated_key,
							  MASK translated_mask);
	U32 inverseTranslateExtendedKey(KEY translated_key);

	MASK updateModifiers();

#if 0
	void setModifierKeyLevel(KEY key, bool new_state);
#endif

private:
	std::map<U32, KEY> mTranslateNumpadMap;
	std::map<KEY, U32> mInvTranslateNumpadMap;
};
