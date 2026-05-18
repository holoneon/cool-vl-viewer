/**
 * @file lltoolpipette.h
 * @brief LLToolPipette class header file
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

// A tool to pick texture entry infro from objects in world (color/texture)
// This tool assumes it is transient in the codebase and must be used
// accordingly. We should probably restructure the way tools are
// managed so that this is handled automatically.

#pragma once

#include "lltextureentry.h"

#include "lltool.h"

class LLViewerObject;
class LLPickInfo;

class LLToolPipette final : public LLTool
{
protected:
	LOG_CLASS(LLToolPipette);

public:
	LLToolPipette();

	// This is an object edit tool
	LL_INLINE bool isObjectEditTool() const override		{ return true; }

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleToolTip(S32 x, S32 y, std::string& msg, LLRect* rect) override;

	typedef void (*select_callback)(const LLTextureEntry& te, void *data);
	void setSelectCallback(select_callback callback, void* user_data);
	void setResult(bool success, const std::string& msg);

	static void pickCallback(const LLPickInfo& pick_info);

protected:
	LLTextureEntry	mTextureEntry;
	select_callback mSelectCallback;
	std::string		mTooltipMsg;
	void*			mUserData;
	bool			mSuccess;
};

extern LLToolPipette gToolPipette;
