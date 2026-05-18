/**
 * @file llkeyboardlinux.h
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

class LLKeyboardLinux final : public LLKeyboard
{
public:
	LLKeyboardLinux();
	~LLKeyboardLinux() override	{}

	bool handleKeyUp(U32 key, MASK mask) override;
	bool handleKeyDown(U32 key, MASK mask) override;
	void resetMaskKeys() override;
	MASK currentMask(bool for_mouse_event) override;
	void scanKeyboard() override;

protected:
	MASK updateModifiers(U32 mask);
#if 0
	void setModifierKeyLevel(KEY key, bool new_state);
#endif
	bool translateNumpadKey(U32 os_key, KEY* translated_key, MASK mask);
	U32	inverseTranslateNumpadKey(KEY translated_key);

private:
	// Special map for translating OS keys to numpad keys
	std::map<U32, KEY> mTranslateNumpadMap;
	// Inverse of the above
	std::map<KEY, U32> mInvTranslateNumpadMap;
};
