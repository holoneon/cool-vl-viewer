/**
 * @file llpreviewanim.h
 * @brief LLPreviewAnim class definition
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

#include "llpreview.h"
#include "llcharacter.h"

class LLPreviewAnim final : public LLPreview
{
public:
	LLPreviewAnim(const std::string& name, const LLRect& rect,
				  const std::string& title, const LLUUID& item_uuid,
				  S32 activate, const LLUUID& object_uuid = LLUUID::null);

	void refreshFromItem() override;

	static void playAnim(void* userdata);
	static void auditionAnim(void* userdata);
	static void endAnimCallback(void* userdata);

protected:
	void onClose(bool app_quitting) override;
	LL_INLINE const char* getTitleName() const override	{ return "Animation"; }

protected:
	LLButton*			mPlayBtn;
	LLButton*			mAuditionBtn;
	std::string			mTitle;
	LLAnimPauseRequest	mPauseRequest;
	LLUUID				mItemID;
	LLUUID				mObjectID;
};
