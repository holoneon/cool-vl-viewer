/**
 * @file hbfloatermakenewoutfit.h
 * @brief The "Make new outfit" floater - header file
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 *
 * Copyright (c) 2011-2015 Henri Beauchamp
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

#include "llfloater.h"

class LLButton;
class LLCheckBoxCtrl;
class LLScrollListCtrl;

class HBFloaterMakeNewOutfit final
:	public LLFloater, public LLFloaterSingleton<HBFloaterMakeNewOutfit>
{
	friend class LLUISingleton<HBFloaterMakeNewOutfit,
							   VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBFloaterMakeNewOutfit);

public:
	~HBFloaterMakeNewOutfit() override;

	void getIncludedItems(uuid_vec_t& wearables_to_include,
						  uuid_vec_t& attachments_to_include);

	static void setDirty();

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBFloaterMakeNewOutfit(const LLSD&);

	bool postBuild() override;
	void draw() override;

	bool hasCheckedItems();

	static void onCommitWearableList(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBox(LLUICtrl*, void* user_data);
	static void onCommitCheckBoxLinkAll(LLUICtrl* ctrl, void* user_data);
	static void onButtonSave(void* user_data);
	static void onButtonCancel(void* user_data);

private:
	LLButton*			mSaveButton;
	LLCheckBoxCtrl*		mShapeCheck;
	LLCheckBoxCtrl*		mSkinCheck;
	LLCheckBoxCtrl*		mHairCheck;
	LLCheckBoxCtrl*		mEyesCheck;
	LLCheckBoxCtrl*		mUseAllLinksCheck;
	LLCheckBoxCtrl*		mUseClothesLinksCheck;
	LLCheckBoxCtrl*		mUseNoCopyLinksCheck;
	LLCheckBoxCtrl*		mRenameCheck;
	LLScrollListCtrl*	mAttachmentsList;
	LLScrollListCtrl*	mWearablesList;

	bool				mIsDirty;
	bool				mSaveStatusDirty;

	static uuid_list_t	sFetchingRequests;
	static uuid_list_t	sUnderpants;
	static uuid_list_t	sUndershirts;
};
