/**
 * @file lltextureview.h
 * @brief LLTextureView class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
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

#include <vector>

#include "llcontainerview.h"

class LLViewerFetchedTexture;
class LLTextureBar;
class LLTexViewStats;

class LLTextureView final : public LLContainerView
{
	friend class LLTextureBar;
	friend class LLTexViewStats;

protected:
	LOG_CLASS(LLTextureView);

public:
	LLTextureView(const std::string& name);
	~LLTextureView() override;

	void setVisible(bool visible) override;
	void draw() override;

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleKey(KEY key, MASK mask, bool called_from_parent) override;

private:
	void addBar(LLViewerFetchedTexture* imagep, S32 hilight);
	void removeAllBars();

private:
	LLTexViewStats*				mTexViewStats;
	std::vector<LLTextureBar*>	mTextureBars;
	std::vector<LLTextureBar*>	mUsedTextureBars;
    bool                        mFreezeView;
    bool                        mOrderFetch;
    bool                        mPrintList;
};

extern LLTextureView* gTextureViewp;
