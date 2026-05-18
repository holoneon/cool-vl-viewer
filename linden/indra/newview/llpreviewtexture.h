/**
 * @file llpreviewtexture.h
 * @brief LLPreviewTexture class definition
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

#include "llbutton.h"
#include "hbfileselector.h"
#include "llframetimer.h"

#include "llpreview.h"
#include "llviewertexture.h"

class LLImageRaw;

class LLPreviewTexture final : public LLPreview
{
public:
	LLPreviewTexture(const std::string& name,
					 const LLRect& rect,
					 const std::string& title,
					 const LLUUID& item_uuid,
					 const LLUUID& object_id,
					 bool show_keep_discard = false);
	LLPreviewTexture(const std::string& name,
					 const LLRect& rect,
					 const std::string& title,
					 const LLUUID& asset_id,
					 bool copy_to_inv = false);
	~LLPreviewTexture();

	void draw() override;

	bool canSaveAs() const override;
	void saveAs() override;

	void loadAsset() override;
	EAssetStatus getAssetStatus() override;

	LL_INLINE void setNotCopyable()						{ mIsCopyable = false; }

	static void saveAsCallback(HBFileSelector::ESaveFilter type,
							   std::string& filename, void* user_data);

	static void onFileLoadedForSave(bool success,
									LLViewerFetchedTexture* src_vi,
									LLImageRaw* src,
									LLImageRaw* aux_src,
									S32 discard_level,
									bool is_final,
									void* userdata);

	LL_INLINE static S32 getPreviewCount()				{ return sList.size(); }

protected:
	void init();
	bool setAspectRatio(F32 width, F32 height);
	static void onAspectRatioCommit(LLUICtrl*, void* userdata);
	static void onRefreshBtn(void* data);

	LL_INLINE const char* getTitleName() const override	{ return "Texture"; }

private:
	void updateDimensions();

private:
	LLPointer<LLViewerFetchedTexture>	mImage;

	LLFrameTimer						mSavedFileTimer;

	std::string							mSaveFileName;

	LLUUID								mImageID;

	uuid_list_t							mCallbackTextureList;

	S32                 				mImageOldBoostLevel;

	S32									mLastHeight;
	S32									mLastWidth;
	F32									mAspectRatio;	// 0 = Unconstrained

	bool								mShowKeepDiscard;
	bool								mCopyToInv;
	bool								mLoadingFullImage;

	// This is stored off in a member variable, because the save-as
	// button and drag and drop functionality need to know.
	bool								mIsCopyable;

	static std::set<LLPreviewTexture*>	sList;
};
