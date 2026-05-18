/**
 * @file hbfloateruploadasset.cpp
 * @brief HBFloaterUploadAsset class implementation
 *        This is a full rewrite of LL's LLFloaterNameDesc class.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 *
 * Copyright (c) 2023, Henri Beauchamp.
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

#include "hbfloateruploadasset.h"

#include "llbutton.h"
#include "lldir.h"
#include "lleconomy.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"

#include "llfloaterimagepreview.h"
#include "llfloaterperms.h"
#include "llviewerassetupload.h"
#include "llviewercontrol.h"

HBFloaterUploadAsset::HBFloaterUploadAsset(const std::string& filename,
										   S32 inventory_type)
:	LLFloater("asset upload"),
	mFilenameAndPath(filename),
	mFilename(LLDir::getBaseFileName(filename, false)),
	mInventoryType(inventory_type),
	mTempAsset(false)
{
}

bool HBFloaterUploadAsset::postBuild()
{
	setTitle(mFilename);

	std::string asset_name = mFilename;
	LLStringUtil::replaceNonstandardASCII(asset_name, '?');
	LLStringUtil::replaceChar(asset_name, '|', '?');
	LLStringUtil::stripNonprintable(asset_name);
	LLStringUtil::trim(asset_name);
	mNameEditor = getChild<LLLineEditor>("name_form");
	mNameEditor->setText(LLDir::getBaseFileName(asset_name, true));
	mNameEditor->setMaxTextLength(DB_INV_ITEM_NAME_STR_LEN);
	mNameEditor->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);

	mDescEditor = getChild<LLLineEditor>("description_form");
	mDescEditor->setMaxTextLength(DB_INV_ITEM_DESC_STR_LEN);
	mDescEditor->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);

	mCost = getExpectedUploadCost();

	// OK button
	mUploadButton = getChild<LLButton>("ok_btn");
	mUploadButton->setClickedCallback(onBtnOK, this);
	mUploadButton->setLabelArg("[AMOUNT]", llformat("%d", mCost));
	setDefaultBtn(mUploadButton);

	// Cancel button
	childSetAction("cancel_btn", onBtnCancel, this);

	center();

	return true;
}

//virtual
S32 HBFloaterUploadAsset::getExpectedUploadCost() const
{
	switch (mInventoryType)
	{
		case LLInventoryType::IT_TEXTURE:
			return LLEconomy::getInstance()->getTextureUploadCost();

		case LLInventoryType::IT_SOUND:
			return LLEconomy::getInstance()->getSoundUploadCost();

		case LLInventoryType::IT_ANIMATION:
			return LLEconomy::getInstance()->getAnimationUploadCost();

		default:
			break;
	}
	return 0;
}

//virtual
void HBFloaterUploadAsset::uploadAsset()
{
	// Upload a chargeable asset.
	LLResourceUploadInfo::ptr_t
		info(new LLNewFileResourceUploadInfo(mFilenameAndPath,
											 mNameEditor->getText(),
											 mDescEditor->getText(), 0,
											 LLFolderType::FT_NONE,
											 LLInventoryType::IT_NONE,
											 LLFloaterPerms::getNextOwnerPerms(),
											 LLFloaterPerms::getGroupPerms(),
											 LLFloaterPerms::getEveryonePerms(),
											 mCost));
	upload_new_resource(info, NULL, NULL, mTempAsset);
}

//static
void HBFloaterUploadAsset::onBtnOK(void* userdata)
{
	HBFloaterUploadAsset* self = (HBFloaterUploadAsset*)userdata;
	if (self)
	{
		// Do not allow inadvertent duplicate uploads
		self->mUploadButton->setEnabled(false);
		// This is potentially overridden. HB
		self->uploadAsset();
		// Whatever the result, we are done: close the floater.
		self->close();
	}
}

//static
void HBFloaterUploadAsset::onBtnCancel(void* userdata)
{
	HBFloaterUploadAsset* self = (HBFloaterUploadAsset*)userdata;
	if (self)
	{
		self->close();
	}
}

///////////////////////////////////////////////////////////////////////////////
// HBFloaterUploadSound class
///////////////////////////////////////////////////////////////////////////////

HBFloaterUploadSound::HBFloaterUploadSound(const std::string& filename)
:	HBFloaterUploadAsset(filename, LLInventoryType::IT_SOUND)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_sound_preview.xml");
}
