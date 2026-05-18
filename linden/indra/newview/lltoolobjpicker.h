/**
 * @file lltoolobjpicker.h
 * @brief LLToolObjPicker class header file
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

#include "lluuid.h"

#include "lltool.h"

class LLPickInfo;

class LLToolObjPicker final : public LLTool
{
protected:
	LOG_CLASS(LLToolObjPicker);

public:
	LLToolObjPicker();

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;

	void handleSelect() override;
	void handleDeselect() override;

	void onMouseCaptureLost() override;

	LL_INLINE void setExitCallback(void (*callback)(void*), void* user_data)
	{
		mExitCallback = callback;
		mExitCallbackData = user_data;
	}

	LL_INLINE const LLUUID& getObjectID() const		{ return mHitObjectID; }

	static void pickCallback(const LLPickInfo& pick_info);

protected:
	LLUUID	mHitObjectID;
	void 	(*mExitCallback)(void *callback_data);
	void*	mExitCallbackData;
	bool	mPicked;
};

extern LLToolObjPicker gToolObjPicker;
