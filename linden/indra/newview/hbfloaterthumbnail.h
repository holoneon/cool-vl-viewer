/**
 * @file hbfloaterthumbnail.h
 * @author Henri Beauchamp
 * @brief HBFloaterThumbnail class declaration
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 *
 * Copyright (c) 2023-2024, Henri Beauchamp.
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

#include "hbfastmap.h"
#include "llfloater.h"

class LLButton;
class LLFetchedGLTFMaterial;
class LLIconCtrl;
class LLImageRaw;
class LLInventoryObject;
class LLTextBox;
class LLScrollListItem;
class HBThumbnailDropTarget;
class LLView;
class LLViewerFetchedTexture;
class LLViewerInventoryItem;
class LLViewerTexture;

class HBFloaterThumbnail final : public LLFloater
{
	friend class HBThumbnailDropTarget;

protected:
	LOG_CLASS(HBFloaterThumbnail);

public:
	LL_INLINE bool isForViewOnly() const		{ return mOwner != NULL; }

	// Note: here the 'id' is either the agent's inventory item Id, or the
	// inventory item Id XORed with its container object (task) Id.
	static HBFloaterThumbnail* findInstance(const LLUUID& id);

	// When 'ownerp' is not NULL, show the (unique) temporary floater without
	// controls, and parent it to the floater owning 'ownerp'.
	static void showInstance(const LLUUID& inv_obj_id,
							 const LLUUID& task_id = LLUUID::null,
							 LLView* ownerp = NULL);
	// Omitting 'id' (or passing a null UUID) causes this call to close the
	// (unique) temporary thumbnail view floater. If the floater is not the
	// temporary one and got unsaved changes, it is not closed. Note that 'id'
	// is either the agent's inventory object Id, or the task_id XORed with
	// the Id of the item it contains and with which the thumbnail is
	// associated.
	static void hideInstance(const LLUUID& id = LLUUID::null);

	// Note: the raw image may be modified (scaled down) by this method.
	static void uploadThumbnail(const LLUUID& inv_obj_id,
								LLPointer<LLImageRaw> rawp);

private:
	// Use showInstance() only
	HBFloaterThumbnail(const LLUUID& inv_obj_id, const LLUUID& task_id,
					   LLFloater* ownerp);
	~HBFloaterThumbnail() override;

	void unregister();

	// LLFloater overrides
	bool postBuild() override;
	void draw() override;

	void updateDropTarget();

	void setInventoryObjectId(const LLUUID& inv_obj_id);
	LLInventoryObject* getInventoryObject();

	void setThumbTexture();
	void setThumbnail();

	void uploadFailure(const std::string& reason);

	static void uploadThumbnailCoro(std::string url, LLSD data, LLUUID id);

	void onChoosenTexture(LLViewerInventoryItem* itemp, bool final_choice);

	static void onBtnChange(LLUICtrl* ctrlp, void* userdata);
	static void onBtnCancel(void* userdata);
	static void onBtnClose(void* userdata);

private:
	LLUUID								mTaskId;
	LLUUID								mInventoryObjectId;
	LLUUID								mInitialThumbnailId;
	LLUUID								mThumbnailId;
	LLUUID								mTempThumbId;
	HBThumbnailDropTarget*				mDropTarget;
	LLFloater*							mOwner;
	LLIconCtrl*							mIcon;
	LLTextBox*							mInventoryObjectName;
	LLButton*							mCancelButton;
	LLScrollListItem*					mCopyThumbnail;
	LLScrollListItem*					mPasteThumbnail;
	LLScrollListItem*					mClearThumbnail;
	LLScrollListItem*					mUndoThumbnail;
	LLPointer<LLFetchedGLTFMaterial>	mMaterialp;
	LLPointer<LLViewerTexture>			mMatTexturep;
	LLPointer<LLViewerFetchedTexture>	mTexturep;
	LLRect								mThumbnailRect;
	std::string							mTempFilename;
	bool								mMustClose;
	bool								mIsCategory;
	bool								mIsMaterialPreview;

	typedef fast_hmap<LLUUID, HBFloaterThumbnail*> instances_map_t;
	static instances_map_t				sInstances;
};
