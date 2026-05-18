/**
 * @file hbfloaterthumbnail.cpp
 * @author Henri Beauchamp
 * @brief HBFloaterThumbnail class implementation
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

#include "llviewerprecompiledheaders.h"

#include "hbfloaterthumbnail.h"

#include "llbutton.h"
#include "llcombobox.h"
#include "llcorehttputil.h"
#include "lldir.h"
#include "hbfileselector.h"
#include "llfontgl.h"
#include "lliconctrl.h"
#include "llimagej2c.h"
#include "llnotifications.h"
#include "llrenderutils.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llappviewer.h"			// For gDisconnected & LLApp::isExiting()
#include "llfloaterimagepreview.h"
#include "hbfloaterinvitemspicker.h"
#include "llfloatersnapshot.h"
#include "llgltfmateriallist.h"
#include "llgltfmaterialpreview.h"
#include "hbinventoryclipboard.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llpipeline.h"				// For LLPipeline::sReflectionProbesEnabled
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewertexturelist.h"
#include "llvoinventorylistener.h"

constexpr LLInventoryType::EType TEXTYPE = LLInventoryType::IT_TEXTURE;

static LLTimer sAutoCloseTimer;

// Helper functions

static bool validate_item_permissions(const LLViewerInventoryItem* itemp)
{
	const LLPermissions& perms = itemp->getPermissions();
	return perms.allowCopyBy(gAgentID) &&
		   perms.allowTransferBy(gAgentID);
}

static bool validate_asset_perms(const LLUUID& asset_id)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLAssetIDMatches asset_id_matches(asset_id);
	gInventory.collectDescendentsIf(LLUUID::null, cats, items,
									LLInventoryModel::INCLUDE_TRASH,
									asset_id_matches);
	if (items.empty())
	{
		// No inventory item bears any such asset, so it is most likely another
		// thumbnail Id, and thus allowed to copy/transfer already.
		return true;
	}
	for (U32 i = 0, count = items.size(); i < count; ++i)
	{
		if (items[i] && validate_item_permissions(items[i]))
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// HBThumbnailDropTarget class - UI element for texture drop targets. It also
// handles automatically click-selection via the inventory items picker.
// Finally, it acts as an agent's inventory or object's inventory observer for
// its owner (this avoids having to #include the corresponding headers from the
// hbfloaterthumbnail.h header).
//-----------------------------------------------------------------------------

class HBThumbnailDropTarget final : public LLView, public LLInventoryObserver,
									public LLVOInventoryListener
{
protected:
	LOG_CLASS(HBThumbnailDropTarget);

public:
	HBThumbnailDropTarget(HBFloaterThumbnail* ownerp, LLView* parentp)
	:	LLView(parentp->getName() + "_drop", false), // Not mouse-opaque
		mParentFloater(ownerp),
		mObserveAgentInventory(false),
		mObserveObjectInventory(false)
	{
		setFollows(FOLLOWS_LEFT | FOLLOWS_TOP);

		// Set our rect to the parent view (usually a view border) rect.
		LLRect rect = parentp->getRect();
		setRect(rect);
		// Create a text box associated with our drop target view (we will not
		// use this as an actual text box, but this allows to set a clicked
		// callback for it, that a simple LLView would not have).
		LLTextBox* textp = new LLTextBox(parentp->getName() + "_click", rect,
										 "", LLFontGL::getFontSansSerif(),
										 true); // Opaque text box
		// Add as a child of our owner floater
		ownerp->addChild(textp);

		// Add ourselves as a child of the floater: this must be done *after*
		// the text box was added, so that the drop target view is on top (note
		// that it is however not opaque to mouse: tool tip hovers and clicks
		// do get to the underlying text box).
		ownerp->addChild(this);

		// Add an adequate tool tip
		textp->setToolTip(ownerp->getString("thumbnail_tool_tip"));

		// Setup click-action on the text of the drop target (inventory picker
		// or edit thumbmail floater opening)
		textp->setClickedCallback(onTextClicked, this);

		// Observe inventory changes on behalf of our owner.
		updateObservers();
	}

	void updateObservers(bool remove_only = false)
	{
		// Remove any old observer
		if (mObserveAgentInventory)
		{
			gInventory.removeObserver(this);
			mObserveAgentInventory = false;
		}
		if (mObserveObjectInventory)
		{
			removeVOInventoryListeners();
			mObserveObjectInventory = false;
		}
		if (!remove_only)
		{
			// Add an appropriate observer for the new item
			if (mParentFloater->mTaskId.isNull())
			{
				mObserveAgentInventory = true;
				gInventory.addObserver(this);
			}
			else
			{
				LLViewerObject* objectp =
					gObjectList.findObject(mParentFloater->mTaskId);
				if (objectp)
				{
					mObserveObjectInventory = true;
					registerVOInventoryListener(objectp, NULL);
				}
			}
		}
	}

	~HBThumbnailDropTarget() override
	{
		updateObservers(true);	// Remove observers
	}

	bool handleDragAndDrop(S32 x, S32 y, MASK, bool drop,
						   EDragAndDropType cargo_type,
						   void* cargo_data, EAcceptance* accept,
						   std::string&) override
	{
		// Careful: pointInView() gets f*cked up whenever the panel is embedded
		// inside a layout stack.
		if (mParentFloater->mOwner || !getEnabled() || !pointInView(x, y))
		{
			return false;
		}

		*accept = ACCEPT_NO;
		if (cargo_type == DAD_TEXTURE)
		{
			LLViewerInventoryItem* itemp = (LLViewerInventoryItem*)cargo_data;
			if (itemp && gInventory.getItem(itemp->getUUID()) &&
				validate_item_permissions(itemp))
			{
				*accept = ACCEPT_YES_COPY_SINGLE;
				if (drop)
				{
					// Inform our owner about the user choice
					mParentFloater->onChoosenTexture(itemp, true);
				}
			}
		}
		return true;
	}

	// LLInventoryObserver override
	void changed(U32 mask) override
	{
		constexpr U32 WATCHED_CHANGES = LABEL | INTERNAL | REMOVE;
		if (mask & WATCHED_CHANGES)
		{
			// Passing a null UUID causes a simple refresh.
			mParentFloater->setInventoryObjectId(LLUUID::null);
		}
	}

	// LLVOInventoryListener override
	void inventoryChanged(LLViewerObject*, LLInventoryObject::object_list_t*,
						  S32, void*) override
	{
		// Passing a null UUID causes a simple refresh.
		mParentFloater->setInventoryObjectId(LLUUID::null);
	}

private:
	static void onTextClicked(void* userdata)
	{
		HBThumbnailDropTarget* self = (HBThumbnailDropTarget*)userdata;
		if (!self || !self->getEnabled())
		{
			return;
		}

		HBFloaterThumbnail*	floaterp = self->mParentFloater;

		if (floaterp->mOwner)
		{
			const LLUUID& item_id = floaterp->mInventoryObjectId;
			if (item_id.notNull())
			{
				// Show a thumbnail edit floater for our viewed item.
				HBFloaterThumbnail::showInstance(item_id, floaterp->mTaskId);
			}
			// Flag our parent floater for closing (do not close it ourselves,
			// since this could cause the clicked callback or focus underlying
			// code to possibly use destroyed UI elements pointers).
			floaterp->mMustClose = true;
			return;
		}

		HBFloaterInvItemsPicker* pickerp =
			new HBFloaterInvItemsPicker(self, invItemsPickerCallback, self);
		// We want an empty selection callback on picker closing by any other
		// mean than the "Select" button.
		pickerp->callBackOnClose();
		if (pickerp)
		{
			pickerp->setAssetType(LLAssetType::AT_TEXTURE);
		}
		pickerp->setApplyImmediatelyControl("ApplyThumbnailImmediately");
		// Thumbnails must be at least copy OK and transfer OK.
		pickerp->setFilterPermMask(PERM_COPY | PERM_TRANSFER);

		static LLCachedControl<bool> auto_pick(gSavedSettings,
											   "ThumbnailAutoPickTexture");
		if (!auto_pick || floaterp->mTaskId.notNull())
		{
			return;
		}

		// Search for a texture with the right permissions in the folder (or
		// parent folder for an item) we want to set the thumbnail for, and
		// select it by default. The rationale is that if a texture exists at
		// this level it is likely representative of the thumbnail we want for
		// this folder or item...

		LLInventoryObject* invobjp = floaterp->getInventoryObject();
		if (!invobjp)	// Paranoia
		{
			return;
		}
		const LLUUID& cat_id =
			floaterp->mIsCategory ? invobjp->getUUID()
								  : invobjp->getParentUUID();

		// First, search among direct descendents...
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(cat_id, cats, items);
		if (!items)	// Failed to collect descendents !
		{
			return;
		}
		for (LLInventoryModel::item_array_t::iterator it = items->begin(),
													  end = items->end();
			 it != end; ++it)
		{
			LLViewerInventoryItem* itemp = *it;
			if (itemp && itemp->getType() == LLAssetType::AT_TEXTURE &&
				validate_item_permissions(itemp))
			{
				// Select this texture by default.
				pickerp->setSelection(itemp->getUUID());
				return;
			}
		}
		// Finally, search deeper down in the whole folder tree (there is no
		// set order on the returned items, thus why we searched for direct
		// descendents first)...
		LLInventoryModel::cat_array_t all_cats;
		LLInventoryModel::item_array_t all_items;
		gInventory.collectDescendents(cat_id, all_cats, all_items, false);
		for (LLInventoryModel::item_array_t::iterator it = all_items.begin(),
													  end = all_items.end();
			 it != end; ++it)
		{
			LLViewerInventoryItem* itemp = *it;
			if (itemp && itemp->getType() == LLAssetType::AT_TEXTURE &&
				validate_item_permissions(itemp))
			{
				// Select this texture by default.
				pickerp->setSelection(itemp->getUUID());
				return;
			}
		}
	}

	static void invItemsPickerCallback(const std::vector<std::string>&,
									   const uuid_vec_t& ids, void* userdata,
									   bool on_close)
	{
		HBThumbnailDropTarget* self = (HBThumbnailDropTarget*)userdata;
		if (!self)	// Paranoia
		{
			return;
		}
		// Empty ids happen on close by any other mean than "Select".
		if (ids.empty())
		{
			if (on_close)
			{
				self->mParentFloater->onChoosenTexture(NULL, true);
			}
			return;
		}
		LLUUID inv_id = ids[0];
		// Make sure we are not trying to use a link and get the linked item
		// Id in that case.
		if (inv_id.notNull())
		{
			inv_id = gInventory.getLinkedItemID(inv_id);
		}
		LLViewerInventoryItem* itemp = gInventory.getItem(inv_id);
		if (itemp)
		{
			// Inform our owner about the user choice
			self->mParentFloater->onChoosenTexture(itemp, on_close);
		}
	}

private:
	HBFloaterThumbnail*		mParentFloater;
	bool					mObserveAgentInventory;
	bool					mObserveObjectInventory;
};

///////////////////////////////////////////////////////////////////////////////
// HBFloaterThumbnail class proper
///////////////////////////////////////////////////////////////////////////////

//static
HBFloaterThumbnail::instances_map_t HBFloaterThumbnail::sInstances;

//static
HBFloaterThumbnail* HBFloaterThumbnail::findInstance(const LLUUID& id)
{
	instances_map_t::iterator it = sInstances.find(id);
	return it != sInstances.end() ? it->second : NULL;
}

//static
void HBFloaterThumbnail::showInstance(const LLUUID& inv_obj_id,
									  const LLUUID& task_id, LLView* ownerp)
{
	// Search for our owner's parent floater.
	LLFloater* parentp = ownerp ? ownerp->getParentFloater() : NULL;
	if (parentp)
	{
		sAutoCloseTimer.reset();
	}	

	HBFloaterThumbnail* floaterp = findInstance(inv_obj_id ^ task_id);
	if (floaterp)	// A floater for this inventory object exists already.
	{
		if (!floaterp->getVisible())
		{
			floaterp->open();
		}
		// If it is an edit floater with matching item, we can close the
		// temporary view floater when it exists.
		if (parentp && !floaterp->mOwner)
		{
			// The null UUID is used for the unique and shared, temporary view
			// floater.
			floaterp = findInstance(LLUUID::null);
			if (floaterp)
			{
				floaterp->close();
			}
		}
		return;
	}

	if (parentp)	// Look for an existing thumbnail shared view floater
	{
		// The null UUID is used for the unique and shared, temporary view
		// floater.
		floaterp = findInstance(LLUUID::null);
		// If this floater exists and does not belong to our new owner's parent
		// floater, then close it.
		if (floaterp && floaterp->mOwner != parentp)
		{
			floaterp->close();
			floaterp = NULL;
		}
	}
	if (floaterp)
	{
		// Set the new inventory item for this shared, unique floater. Note
		// that this call may actually close the said floater, when there is
		// no thumbnail associated with this item.
		if (parentp)
		{
			floaterp->setInventoryObjectId(inv_obj_id);
			floaterp->mTaskId = task_id;
			// We need this in case the view floater switched from an agent's
			// inventory item to the item of an object's inventory, or vice
			// versa (observers need updating).
			floaterp->updateDropTarget();
		}
	}
	else
	{
		new HBFloaterThumbnail(inv_obj_id, task_id, parentp);
		if (!parentp)
		{
			// Check that the temporary floater is not open for this same item
			// and when it is, close it.
			floaterp = findInstance(LLUUID::null);
			if (floaterp && floaterp->mInventoryObjectId == inv_obj_id &&
				floaterp->mTaskId == task_id)
			{
				floaterp->close();
			}
		}
	}
}

//static
void HBFloaterThumbnail::hideInstance(const LLUUID& id)
{
	HBFloaterThumbnail* self = findInstance(id);
	if (self &&
		(self->mOwner ||
		 // Do not close an edit floater with unsaved changes.
		 self->mThumbnailId == self->mInitialThumbnailId))
	{
		self->close();
	}
}

HBFloaterThumbnail::HBFloaterThumbnail(const LLUUID& inv_obj_id,
									   const LLUUID& task_id,
									   LLFloater* ownerp)
:	mOwner(ownerp),
	mPasteThumbnail(NULL),
	mCopyThumbnail(NULL),
	mClearThumbnail(NULL),
	mUndoThumbnail(NULL),
	mCancelButton(NULL),
	mTaskId(task_id),
	mMustClose(false),
	mIsCategory(false),
	mIsMaterialPreview(false)
{
	// Note: the only floater with an owner is the preview one.
	std::string xml_file = ownerp ? "floater_thumbnail_view.xml"
								  : "floater_thumbnail.xml";
	LLUICtrlFactory::getInstance()->buildFloater(this, xml_file, NULL,
												 !mOwner);
	if (ownerp)
	{
		setIsChrome(true);
		setSoundFlags(SILENT);
		ownerp->addDependentFloater(this);
	}

	setInventoryObjectId(inv_obj_id);
}

//virtual
HBFloaterThumbnail::~HBFloaterThumbnail()
{
	unregister();
}

void HBFloaterThumbnail::unregister()
{
	// There shall be exactly one entry for each floater registered in the map.
	// Always use setInventoryObjectId() when changing the associated inventory
	// object !
	for (instances_map_t::iterator it = sInstances.begin(),
								   end = sInstances.end();
		 it != end; ++it)
	{
		if (it->second == this)
		{
			sInstances.erase(it);
			break;
		}
	}
}

//virtual
bool HBFloaterThumbnail::postBuild()
{
	mIcon = getChild<LLIconCtrl>("icon");

	mInventoryObjectName = getChild<LLTextBox>("item_name");

	mDropTarget = new HBThumbnailDropTarget(this,
											getChild<LLView>("thumbnail"));
	mThumbnailRect = mDropTarget->getRect();
	// Adjust to keep the view border showing while we will draw the thumbnail
	// inside it.
	++mThumbnailRect.mBottom;
	--mThumbnailRect.mTop;
	++mThumbnailRect.mLeft;
	--mThumbnailRect.mRight;

	if (mOwner)
	{
		return true;
	}

	LLFlyoutButton* change_buttonp = getChild<LLFlyoutButton>("change");
	change_buttonp->setCommitCallback(onBtnChange);
	change_buttonp->setCallbackUserData(this);
	std::string operation;
	for (U32 i = 0, count = change_buttonp->getItemCount(); i < count; ++i)
	{
		LLScrollListItem* itemp = change_buttonp->getItemByIndex(i);
		operation = itemp->getValue().asString();
		if (operation == "copy")
		{
			mCopyThumbnail = itemp;
		}
		else if (operation == "paste")
		{
			mPasteThumbnail = itemp;
		}
		else if (operation == "clear")
		{
			mClearThumbnail = itemp;
		}
		else if (operation == "undo")
		{
			mUndoThumbnail = itemp;
		}
	}

	mCancelButton = getChild<LLButton>("cancel_btn");
	mCancelButton->setClickedCallback(onBtnCancel, this);

	childSetAction("ok_btn", onBtnClose, this);

	return true;
}

//virtual
void HBFloaterThumbnail::draw()
{
	static LLCachedControl<U32> timeout(gSavedSettings,
										"ThumbnailViewTimeout");
	if (mMustClose ||
		(mOwner && timeout &&
		 sAutoCloseTimer.getElapsedTimeF32() > F32(timeout)))
	{
		close();
		return;
	}

	if (mCancelButton)
	{
		mCancelButton->setEnabled(mThumbnailId != mInitialThumbnailId);
	}
	if (mPasteThumbnail)
	{
		mPasteThumbnail->setEnabled(HBInventoryClipboard::hasAssets(TEXTYPE));
	}
	
	// Draw all UI elements before we would draw the texture.
	LLFloater::draw();

	if (isMinimized())
	{
		return;	// No need to draw the texture.
	}

	bool has_texture = mTexturep.notNull();
	bool has_preview = !has_texture && mMatTexturep.notNull();
	if (!has_texture && !has_preview && mMaterialp.isNull())
	{
		// No texture, draw a grey square...
		gl_rect_2d(mThumbnailRect, LLColor4::grey);
		// ... with a black X.
		gl_draw_x(mThumbnailRect, LLColor4::black);
		return;
	}

	F32 width = mThumbnailRect.getWidth();
	F32 height = mThumbnailRect.getHeight();

	if (has_texture)
	{
		// Update the texture the priority
		mTexturep->addTextureStats(width * height);
	}

	F32 left = mThumbnailRect.mLeft;
	F32 bottom = mThumbnailRect.mBottom;

	F32 tex_width = 0.f;
	F32 tex_height = 0.f;
	if (has_texture)
	{
		tex_width = mTexturep->getFullWidth();
		tex_height = mTexturep->getFullHeight();
	}
	else if (has_preview)
	{
		tex_width = mMatTexturep->getFullWidth();
		tex_height = mMatTexturep->getFullHeight();
	}
	if (tex_width && tex_height && tex_width != tex_height)
	{
		// If necessary, compute the offset in the display, to draw the texture
		// with its native aspect ratio.
		F32 proportion =  tex_height / tex_width;
		if (proportion > 1.f)
		{
			left += (width - width / proportion) * 0.5f;
			width /= proportion;
		}
		else
		{
			bottom += (height - height * proportion) * 0.5f;
			height *= proportion;
		}
	}
	// If one of the dimensions of the image is smaller than the display,
	// center it.
	if (tex_width && tex_height && (tex_width < width || tex_height < height))
	{
		if (tex_width < width)
		{
			left += (width - tex_width) * .5f;
			width = tex_width;
		}
		if (tex_height < height)
		{
			bottom += (height - tex_height) * .5f;
			height = tex_height;
		}
	}
	if (has_texture)
	{
		gl_draw_scaled_image(left, bottom, width, height, mTexturep);
	}
	else if (has_preview)
	{
		gl_draw_scaled_image(left, bottom, width, height, mMatTexturep);
	}
	else
	{
		// Preview not ready, draw a grey square...
		gl_rect_2d(mThumbnailRect, LLColor4::grey);
	}

	if ((has_texture && !mTexturep->isFullyLoaded()) ||
		(!has_texture && !has_preview))
	{
		if (mOwner && timeout)
		{
			sAutoCloseTimer.reset();
		}
		// Show "Loading..." string on the bottom left corner while the texture
		// is loading.
		static LLFontGL* fontp = LLFontGL::getFontSansSerif();
		static LLWString loading = LLTrans::getWString("texture_loading");
		fontp->render(loading, 0, mThumbnailRect.mLeft + 8,
					  mThumbnailRect.mBottom + 6, LLColor4::white,
					  LLFontGL::LEFT, LLFontGL::BASELINE,
					  LLFontGL::DROP_SHADOW);
	}
	// If we are waiting for the preview texture, retry to get it now.
	if (!has_texture && !has_preview && mMaterialp.notNull())
	{
		mMatTexturep = LLGLTFPreviewTexture::getPreview(mMaterialp);
	}
}

void HBFloaterThumbnail::updateDropTarget()
{
	if (mDropTarget)
	{
		mDropTarget->updateObservers();
	}
}

LLInventoryObject* HBFloaterThumbnail::getInventoryObject()
{
	mIsCategory = false;
	LLInventoryObject* invobjp = NULL;
	if (mInventoryObjectId.notNull())
	{
		if (mTaskId.notNull())
		{
			LLViewerObject* objectp = gObjectList.findObject(mTaskId);
			if (objectp)
			{
				invobjp = objectp->getInventoryObject(mInventoryObjectId);
			}
		}
		else
		{
			invobjp = gInventory.getCategory(mInventoryObjectId);
			if (invobjp)
			{
				mIsCategory = true;
			}
			else
			{
				invobjp = gInventory.getItem(mInventoryObjectId);
			}
		}
	}
	return invobjp;
}

void HBFloaterThumbnail::setThumbTexture()
{
	mTexturep = NULL;
	mMatTexturep = NULL;
	const LLUUID& id = mTempThumbId.notNull() ? mTempThumbId : mThumbnailId;
	if (id.notNull())
	{
		if (mIsMaterialPreview && id == mInitialThumbnailId)
		{
			mMaterialp = gGLTFMaterialList.getMaterial(id);
			mMatTexturep = LLGLTFPreviewTexture::getPreview(mMaterialp);
		}
		else
		{
			mMaterialp = NULL;
			mTexturep =
				LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT,
														  true,
														  LLGLTexture::BOOST_PREVIEW);
		}
	}

	bool has_texture = mTexturep.notNull();
	if (mCopyThumbnail)
	{
		mCopyThumbnail->setEnabled(has_texture && validate_asset_perms(id));
	}
	if (mClearThumbnail)
	{
		mClearThumbnail->setEnabled(has_texture);
	}
	if (mUndoThumbnail)
	{
		mUndoThumbnail->setEnabled(mThumbnailId != mInitialThumbnailId);
	}
}

void HBFloaterThumbnail::setInventoryObjectId(const LLUUID& inv_obj_id)
{
	// A null UUID is passed by the inventory observer when we only need a
	// refresh for the currently associated inventory object.
	if (inv_obj_id.notNull())
	{
		mInventoryObjectId = inv_obj_id;
		unregister();
		sInstances.emplace(mOwner ? LLUUID::null : inv_obj_id ^ mTaskId, this);
	}

	LLInventoryObject* invobjp = getInventoryObject();
	if (!invobjp)
	{
		close();	// No associated inventory object, so just commit suicide.
		return;
	}

	mIsMaterialPreview = false;
	LLUUID thumb_id = invobjp->getThumbnailUUID();
	if (thumb_id.isNull() && gUsePBRShaders &&
		// Note: for now, material preview does not render at all when
		// reflection probes are disabled. *TODO: find out why and fix it.
		LLPipeline::sReflectionProbesEnabled)
	{
		bool is_mat = invobjp->getType() == LLAssetType::AT_MATERIAL;
		if (is_mat)
		{
			thumb_id = ((LLInventoryItem*)invobjp)->getAssetUUID();
			mIsMaterialPreview = true;
		}
	}
	if (mOwner)
	{
		if (thumb_id.isNull())
		{
			// Nothing to display, close the temporary floater.
			close();
			return;
		}
		// If there is indeed something to display, we can open the temporary
		// floater.
		if (!getVisible())
		{
			open();
		}
	}
	if (thumb_id != mInitialThumbnailId)	// May not have changed on refresh.
	{
		mThumbnailId = mInitialThumbnailId = thumb_id;
		setThumbTexture();
	}
	else if (mCopyThumbnail)
	{
		mCopyThumbnail->setEnabled(mThumbnailId.notNull() &&
								   validate_asset_perms(mThumbnailId));
	}

	mInventoryObjectName->setText(invobjp->getName());

	// Set the corresponding inventory icon.
	LLInventoryItem* itemp = invobjp->asInventoryItem();
	if (itemp)
	{
		mIcon->setValue(LLInventoryIcon::getIconName(itemp->getType(),
													 itemp->getInventoryType(),
													 itemp->getFlags()));
	}
	else
	{
		static const std::string folder_icon = "inv_folder_plain_closed.tga";
		mIcon->setValue(folder_icon);
	}
}

void HBFloaterThumbnail::onChoosenTexture(LLViewerInventoryItem* itemp,
										  bool final_choice)
{
	if (!itemp)	// Happens on picker closing with "Close" instead of "Select".
	{
		if (final_choice)
		{
			mTempThumbId.setNull();
			setThumbTexture();
		}
		return;
	}

	if (!validate_item_permissions(itemp))
	{
		gNotifications.add("ThumbnailInsufficientPermissions");
		return;
	}

	if (final_choice)
	{
		mThumbnailId = itemp->getAssetUUID();
		mTempThumbId.setNull();
	}
	else
	{
		mTempThumbId = itemp->getAssetUUID();
	}
	setThumbTexture();
}

void HBFloaterThumbnail::setThumbnail()
{
	LLInventoryObject* invobjp = getInventoryObject();
	if (!invobjp || invobjp->getThumbnailUUID() == mThumbnailId ||
		(mIsMaterialPreview && mThumbnailId == mInitialThumbnailId))
	{
		return;	// Nothing to do.
	}
#if 1
	// Set the thumbnail locally
	invobjp->setThumbnailUUID(mThumbnailId);
#endif
	if (mTaskId.notNull())
	{
		return;
	}
	gInventory.addChangedMask(LLInventoryObserver::INTERNAL,
							  mInventoryObjectId);
	// Update the thumbnail on the server.
	LLSD updates;
	if (mThumbnailId.notNull())
	{
		updates["thumbnail"] = LLSD().with("asset_id",
										   mThumbnailId.asString());
	}
	else
	{
		updates["thumbnail"] = LLSD();
	}
	if (mIsCategory)
	{
		update_inventory_category(mInventoryObjectId, updates, NULL);
	}
	else
	{
		update_inventory_item(mInventoryObjectId, updates, NULL);
	}
}

//static
void HBFloaterThumbnail::onBtnCancel(void* userdata)
{
	HBFloaterThumbnail* self = (HBFloaterThumbnail*)userdata;
	if (self)
	{
		self->close();
	}
}

//static
void HBFloaterThumbnail::onBtnClose(void* userdata)
{
	HBFloaterThumbnail* self = (HBFloaterThumbnail*)userdata;
	if (self)
	{
		self->setThumbnail();
		self->close();
	}
}

static void file_selector_callback(HBFileSelector::ELoadFilter,
								   std::string& filename, void* datap)
{
	LLUUID id;
	if (datap)
	{
		LLUUID* idp = (LLUUID*)datap;
		id = *idp;
		delete idp;
	}
	if (filename.empty())	// Selection cancelled.
	{
		return;
	}
	if (!HBFloaterThumbnail::findInstance(id))
	{
		// Thumbnail floater already gone... Give up !
		return;
	}
	// Open the texture preview.
	new LLFloaterImagePreview(filename, id);
}

//static
void HBFloaterThumbnail::onBtnChange(LLUICtrl* ctrlp, void* userdata)
{
	HBFloaterThumbnail* self = (HBFloaterThumbnail*)userdata;
	if (!self || !ctrlp) return;

	std::string operation = ctrlp->getValue().asString();
	if (operation == "clear")
	{
		self->mThumbnailId.setNull();
		self->setThumbTexture();
		return;
	}
	if (operation == "undo")
	{
		self->mThumbnailId = self->mInitialThumbnailId;
		self->setThumbTexture();
		return;
	}

	if (operation == "upload")
	{
		LLUUID* idp = new LLUUID(self->mInventoryObjectId ^ self->mTaskId);
		HBFileSelector::loadFile(HBFileSelector::FFLOAD_IMAGE,
								 file_selector_callback, (void*)idp);
		return;
	}

	if (operation == "copy")
	{
		const LLUUID& asset_id = self->mThumbnailId;
		if (asset_id.notNull())
		{
			if (validate_asset_perms(asset_id))
			{
				HBInventoryClipboard::storeAsset(asset_id, TEXTYPE);
			}
			else
			{
				gNotifications.add("ThumbnailInsufficientPermissions");
			}
		}
		return;
	}

	if (operation == "paste")
	{
		uuid_vec_t asset_ids;
		HBInventoryClipboard::retrieveAssets(asset_ids, TEXTYPE);
		size_t count = asset_ids.size();
		if (count)
		{
			for (size_t i = 0; i < count; ++i)
			{
				// Use the first valid asset Id...
				if (validate_asset_perms(asset_ids[i]))
				{
					self->mThumbnailId = asset_ids[i];
					self->setThumbTexture();
					return;
				}
			}
			gNotifications.add("ThumbnailInsufficientPermissions");
		}
		return;
	}

	if (operation == "upload")
	{
		LLUUID* idp = new LLUUID(self->mInventoryObjectId ^ self->mTaskId);
		HBFileSelector::loadFile(HBFileSelector::FFLOAD_IMAGE,
								 file_selector_callback, (void*)idp);
		return;
	}

	// "snapshot" in pull-down list or direct click on the button
	LLFloaterSnapshot::show(NULL);
	LLFloaterSnapshot* snapshotp = LLFloaterSnapshot::getInstance();
	snapshotp->setupForInventoryThumbnail(self->mInventoryObjectId);
}

void HBFloaterThumbnail::uploadFailure(const std::string& reason)
{
	LLSD args;
	args["MESSAGE"] = reason;
	gNotifications.add("ThumbnailFailedUpload", args);
	mThumbnailId = mInitialThumbnailId;
	setThumbTexture();
}

//static
void HBFloaterThumbnail::uploadThumbnailCoro(std::string url, LLSD data,
											 LLUUID id)
{
	HBFloaterThumbnail* self = findInstance(id);
	if (!self)
	{
		return;	// Floater already gone...
	}

	// Copy this on stack, in case the floater gets closed before we get a
	// server reply, which would not prevent us to continue the upload...
	std::string filename = self->mTempFilename;

	LLCore::HttpOptions::ptr_t options(new LLCore::HttpOptions);
	options->setFollowRedirects(true);

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("uploadThumbnail");

	LLSD result = adapter.postAndSuspend(url, data, options);

	if (gDisconnected || LLApp::isExiting())
	{
		return;	// Too late, abort.
	}

	bool failed = false;
	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (!status)
	{
		llwarns << "Failed to get the uploader capability. Status: "
				<< status.toString() << llendl;
		failed = true;
	}
	else if (!result.has("uploader"))
	{
		llwarns << "Failed to get uploader cap, response contains no data."
				<< llendl;
		failed = true;
	}
	else
	{
		url = result["uploader"].asString();
		failed = url.empty();
	}
	if (failed)
	{
		self = findInstance(id);	// This could be NULL now...
		if (self)
		{
			self->uploadFailure(self->getString("upload_failure"));
		}
		LLFile::remove(filename);
		return;
	}

	S32 length = LLFile::getFileSize(filename);
	LLCore::HttpHeaders::ptr_t headers(new LLCore::HttpHeaders);
	headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, "application/jp2");
	headers->append(HTTP_OUT_HEADER_CONTENT_LENGTH, llformat("%d", length));

	result = adapter.postFileAndSuspend(url, filename, options, headers);
	LLFile::remove(filename);	// We are done with it, now.

	if (gDisconnected || LLApp::isExiting())
	{
		return;	// Too late, abort.
	}

	status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (!status)
	{
		llwarns << "Failed to upload image data. Status: " << status.toString()
				<< llendl;
		failed = true;
	}
	else if (!result.has("state") || !result.has("new_asset") ||
			 result["state"].asString() != "complete")
	{
		llwarns << "Failed to upload image data.";
		if (result.has("state"))
		{
			llcont << ". State: " << result["state"].asString();
		}
		if (result.has("message"))
		{
			llcont << ". Message: " << result["message"].asString();
		}
		if (!result.has("new_asset"))
		{
			llcont << ". No thumbnail UUID transmitted.";
		}
		llcont << llendl;
		failed = true;
	}
	if (failed)
	{
		self = findInstance(id);	// This could be NULL now...
		if (self)
		{
			self->uploadFailure(self->getString("upload_failure"));
		}
		return;
	}

	// Update inventory accordingly. This will also cause a refresh of the
	// corresponding thumbnail floater, if still open.
	LLInventoryObject* invobjp = NULL;
	if (data.has("task_id"))
	{
		LLUUID task_id = data["task_id"].asUUID();
		LLViewerObject* objectp = gObjectList.findObject(task_id);
		if (objectp)
		{
			LLUUID item_id = data["item_id"].asUUID();
			invobjp = objectp->getInventoryObject(item_id);
		}
	}
	else if (data.has("category_id"))
	{
		LLUUID cat_id = data["category_id"].asUUID();
		invobjp = gInventory.getCategory(cat_id);
	}
	else if (data.has("item_id"))
	{
		LLUUID item_id = data["item_id"].asUUID();
		invobjp = gInventory.getItem(item_id);
	}
	if (invobjp)
	{
		invobjp->setThumbnailUUID(result["new_asset"].asUUID());
		if (!data.has("task_id"))
		{
			gInventory.addChangedMask(LLInventoryObserver::INTERNAL,
									  invobjp->getUUID());
		}
	}
}

//static
void HBFloaterThumbnail::uploadThumbnail(const LLUUID& id,
										 LLPointer<LLImageRaw> rawp)
{
	HBFloaterThumbnail* self = findInstance(id);
	if (!self || rawp.isNull())
	{
		return;
	}

	constexpr S32 MAX_THUMBNAIL_SIZE = 256;
	rawp->biasedScaleToPowerOfTwo(MAX_THUMBNAIL_SIZE);

	LLPointer<LLImageJ2C> imagep =
		LLViewerTextureList::convertToUploadFile(rawp);
	if (imagep.isNull())
	{
		self->uploadFailure(self->getString("error_conversion"));
		return;
	}

	self->mTempFilename = gDirUtil.getTempFilename();
	if (!imagep->save(self->mTempFilename))
	{
		std::string error_msg = self->getString("error_file_write") + ":\n";
		self->uploadFailure(error_msg + self->mTempFilename);
		return;
	}

	const std::string& url =
		gAgent.getRegionCapability("InventoryThumbnailUpload");
	if (url.empty())
	{
		LLFile::remove(self->mTempFilename);
		self->uploadFailure(self->getString("missing_capability"));
		return;
	}

	LLSD data;
	bool is_cat = self->mIsCategory;
	if (self->mTaskId.notNull())
	{
		data["task_id"] = self->mTaskId;
		is_cat = false;
	}
	const char* type_id = is_cat ? "category_id" : "item_id";
	data[type_id] = self->mInventoryObjectId;

	gCoros.launch("HBFloaterThumbnail::uploadThumbnailCoro",
				  std::bind(&HBFloaterThumbnail::uploadThumbnailCoro, url,
							data, id));
}
