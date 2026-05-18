/**
 * @file llpreviewlandmark.h
 * @brief LLPreviewLandmark class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include <deque>

#include "lllandmark.h"
#include "llvector3d.h"

#include "llpreview.h"

class LLPreviewLandmark;
class LLPanelPlace;

typedef std::deque<LLPreviewLandmark*> LLPreviewLandmarkList;

class LLPreviewLandmark final : public LLPreview
{
public:
	LLPreviewLandmark(const std::string& name,
					  const LLRect& rect,
					  const std::string& title,
					  const LLUUID& item_uuid,
					  bool show_keep_discard = false,
					  LLViewerInventoryItem* inv_item = NULL);
	~LLPreviewLandmark() override;

	void draw() override;

	std::string getName() const override;
	LLVector3d getPositionGlobal() const;

	static void* createPlaceDetail(void* userdata);

	void loadAsset() override;
	EAssetStatus getAssetStatus() override;

protected:
	void getDegreesAndDist(F32* degrees, F64* horiz_dist,
						   F64* vert_dist) const;

	const char* getTitleName() const override	{ return "Landmark"; }

private:
	LLPanelPlace*	mPlacePanel;
	LLLandmark*		mLandmark;

	static LLPreviewLandmarkList sOrderedInstances;
};
