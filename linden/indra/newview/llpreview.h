/**
 * @file llpreview.h
 * @brief LLPreview class definition
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

#include <map>					// For multimap

#include "llfloater.h"
#include "llresizehandle.h"
#include "lltabcontainer.h"

#include "llinventorymodel.h"
#include "llviewerinventory.h"

class LLLineEditor;
class LLRadioGroup;
class LLPreview;

class LLMultiPreview : public LLMultiFloater
{
public:
	LLMultiPreview(const LLRect& rect);

	void open() override;
	void tabOpen(LLFloater* opened_floater, bool from_click) override;
	void userSetShape(const LLRect& new_rect) override;

	static LLMultiPreview* getAutoOpenInstance(const LLUUID& id);
	static void setAutoOpenInstance(LLMultiPreview* previewp,
									const LLUUID& id);

protected:
	typedef fast_hmap<LLUUID, LLHandle<LLFloater> > handle_map_t;
	static handle_map_t sAutoOpenPreviewHandles;
};

class LLPreview : public LLFloater, LLInventoryObserver
{
protected:
	LOG_CLASS(LLPreview);

public:
	typedef enum e_asset_status
	{
		PREVIEW_ASSET_ERROR,
		PREVIEW_ASSET_UNLOADED,
		PREVIEW_ASSET_LOADING,
		PREVIEW_ASSET_LOADED
	} EAssetStatus;

	// Used for XML-based construction.
	LLPreview(const std::string& name);
	LLPreview(const std::string& name, const LLRect& rect,
			  const std::string& title, const LLUUID& item_uuid,
			  const LLUUID& object_uuid, bool allow_resize = false,
			  S32 min_width = 0, S32 min_height = 0,
			  LLPointer<LLViewerInventoryItem> inv_item = NULL);
	~LLPreview() override;

	LL_INLINE virtual void setObjectID(const LLUUID& object_id)
	{
		mObjectUUID = object_id;
	}

	virtual void setItemID(const LLUUID& item_id);
	void setAssetId(const LLUUID& asset_id);
	// Searches if not constructed with it
	const LLViewerInventoryItem* getItem() const;

	static LLPreview* find(const LLUUID& item_uuid);
	static LLPreview* show(const LLUUID& item_uuid, bool take_focus = true);
	static void	hide(const LLUUID& item_uuid, bool no_saving = false);
	static void	rename(const LLUUID& item_uuid, const std::string& new_name);
	static bool	save(const LLUUID& item_uuid,
					 LLPointer<LLInventoryItem>* itemptr);

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;
	void open() override;
	virtual bool saveItem(LLPointer<LLInventoryItem>* itemptr);

	virtual void setAuxItem(const LLInventoryItem* itemp);

	static void onBtnCopyToInv(void* userdata);

	void addKeepDiscardButtons();
	static void onKeepBtn(void* data);
	static void onDiscardBtn(void* data);
	void userSetShape(const LLRect& new_rect) override;

	void userResized()							{ mUserResized = true; };

	virtual void loadAsset()					{ mAssetStatus = PREVIEW_ASSET_LOADED; }
	virtual EAssetStatus getAssetStatus()		{ return mAssetStatus;}

	void setNotecardInfo(const LLUUID& notecard_inv_id,
						 const LLUUID& object_id);

	void draw() override;

	virtual void refreshFromItem();

	// We cannot modify item or description in preview if either in-world object
	// or item itself is unmodifiable.
	static bool canModify(const LLUUID& task_id, const LLInventoryItem* itemp);

protected:
	void onCommit() override;

	void addDescriptionUI();

	static void onText(LLUICtrl*, void* userdata);
	static void onRadio(LLUICtrl*, void* userdata);

	// LLInventoryObserver override
	void changed(U32 mask) override;

	virtual const char* getTitleName() const	{ return "Preview"; }

protected:
	LLUUID								mItemUUID;

	// mObjectID will have a value if it is associated with a rezzed object
	// (task), and will be LLUUID::null if it is in the agent inventory.
	LLUUID								mObjectUUID;

	LLUUID								mObjectID;
	LLUUID								mNotecardInventoryID;

	LLPointer<LLViewerInventoryItem>	mItem;
	LLPointer<LLInventoryItem>			mAuxItem;  // HACK!

	LLButton*							mCopyToInvBtn;

	LLRect								mClientRect;

	EAssetStatus						mAssetStatus;

	// Close without saving changes
	bool								mForceClose;

	bool								mDirty;

	bool								mUserResized;

	// When closing springs a "Want to save ?" dialog, we want to keep the
	// preview open until the save completes.
	bool								mCloseAfterSave;

	// True if the save changes confirmation dialog was already shown
	bool								mSaveDialogShown;

	typedef fast_hmap<LLUUID, LLPreview*> preview_map_t;
	static preview_map_t				sInstances;
};

constexpr S32 PREVIEW_BORDER = 4;
constexpr S32 PREVIEW_PAD = 5;
constexpr S32 PREVIEW_BUTTON_WIDTH = 100;

constexpr S32 PREVIEW_LINE_HEIGHT = 19;
constexpr S32 PREVIEW_CLOSE_BOX_SIZE = 16;
constexpr S32 PREVIEW_BORDER_WIDTH = 2;
constexpr S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) +
										   PREVIEW_BORDER_WIDTH;
constexpr S32 PREVIEW_VPAD = 2;
constexpr S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
constexpr S32 PREVIEW_HEADER_SIZE = 2 * PREVIEW_LINE_HEIGHT + 2 * PREVIEW_VPAD;
