/**
 * @file lltoolface.h
 * @brief A tool to select object faces
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

#include "lltool.h"

class LLPickInfo;

class LLToolFace final : public LLTool
{
protected:
	LOG_CLASS(LLToolFace);

public:
	LLToolFace();

	// This is an object edit tool
	LL_INLINE bool isObjectEditTool() const override		{ return true; }

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleDoubleClick(S32 x, S32 y, MASK mask) override;
	void handleSelect() override;
	void handleDeselect() override;
	void render() override;		// Draws face highlights

	static void pickCallback(const LLPickInfo& pick_info);
};

extern LLToolFace gToolFace;
