/**
 * @file llpreviewgesture.h
 * @brief Editing UI for inventory-based gestures.
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
#include "llmultigesture.h"

class LLMultiGesture;
class LLLineEditor;
class LLTextBox;
class LLCheckBoxCtrl;
class LLComboBox;
class LLScrollListCtrl;
class LLScrollListItem;
class LLButton;
class LLGestureStep;
class LLRadioGroup;

class LLPreviewGesture final : public LLPreview
{
protected:
	LOG_CLASS(LLPreviewGesture);

public:
	// Pass an object_id if this gesture is inside an object in the world,
	// otherwise use LLUUID::null.
	static LLPreviewGesture* show(const std::string& title,
								  const LLUUID& item_id,
								  const LLUUID& object_id,
								  bool take_focus = true);

	// LLView overrides
	bool handleKeyHere(KEY key, MASK mask) override;
	bool handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
						   EDragAndDropType cargo_type, void* cargo_data,
						   EAcceptance* accept, std::string& tooltip) override;

	// LLPanel override
	bool postBuild() override;

	// LLFloater overrides
	bool canClose() override;
	void setMinimized(bool minimize) override;
	void onClose(bool app_quitting) override;

protected:
	LLPreviewGesture();
	~LLPreviewGesture() override;

	void init(const LLUUID& item_id, const LLUUID& object_id);

	void onUpdateSucceeded();

	// Populate various comboboxes
	void addModifiers();
	void addKeys();
	void addAnimations();
	void addSounds();

	void refresh() override;

	void initDefaultGesture();

	void loadAsset() override;

	static void onLoadComplete(const LLUUID& asset_uuid,
							   LLAssetType::EType type, void* user_data,
							   S32 status, LLExtStat);

	void loadUIFromGesture(LLMultiGesture* gesture);

	void saveIfNeeded();

	static void finishInventoryUpload(LLUUID item_id, LLUUID new_asset_id);

	static void onSaveComplete(const LLUUID& asset_uuid, void* user_data,
							   S32 status, LLExtStat);

	bool handleSaveChangesDialog(const LLSD& notification, const LLSD& response);

	// Write UI back into gesture
	LLMultiGesture* createGesture();

	// Add a step.  Pass the name of the step, like "Animation",
	// "Sound", "Chat", or "Wait"
	LLScrollListItem* addStep(const enum EStepType step_type);

	static void updateLabel(LLScrollListItem* item);

	static void onCommitSetDirty(LLUICtrl* ctrl, void* data);
	static void onCommitLibrary(LLUICtrl* ctrl, void* data);
	static void onCommitStep(LLUICtrl* ctrl, void* data);
	static void onCommitAnimation(LLUICtrl* ctrl, void* data);
	static void onCommitSound(LLUICtrl* ctrl, void* data);
	static void onCommitChat(LLUICtrl* ctrl, void* data);
	static void onCommitWait(LLUICtrl* ctrl, void* data);
	static void onCommitWaitTime(LLUICtrl* ctrl, void* data);

	static void onCommitAnimationTrigger(LLUICtrl* ctrl, void *data);

	// Handy function to commit each keystroke
	static void onKeystrokeCommit(LLLineEditor* caller, void* data);

	static void onClickAdd(void* data);
	static void onClickUp(void* data);
	static void onClickDown(void* data);
	static void onClickDelete(void* data);

	static void onCommitActive(LLUICtrl* ctrl, void* data);
	static void onClickSave(void* data);
	static void onClickPreview(void* data);

	static void onDonePreview(LLMultiGesture* gesture, void* data);

	LL_INLINE const char* getTitleName() const override	{ return "Gesture"; }

protected:
	// LLPreview contains mDescEditor
	LLLineEditor*	mTriggerEditor;
	LLTextBox*		mReplaceText;
	LLLineEditor*	mReplaceEditor;
	LLComboBox*		mModifierCombo;
	LLComboBox*		mKeyCombo;

	LLScrollListCtrl*	mLibraryList;
	LLButton*			mAddBtn;
	LLButton*			mUpBtn;
	LLButton*			mDownBtn;
	LLButton*			mDeleteBtn;
	LLScrollListCtrl*	mStepList;

	// Options panels for items in gesture list
	LLTextBox*			mOptionsText;
	LLRadioGroup*		mAnimationRadio;
	LLComboBox*			mAnimationCombo;
	LLComboBox*			mSoundCombo;
	LLLineEditor*		mChatEditor;
	LLCheckBoxCtrl*		mWaitAnimCheck;
	LLCheckBoxCtrl*		mWaitTimeCheck;
	LLLineEditor*		mWaitTimeEditor;

	LLCheckBoxCtrl*		mActiveCheck;
	LLButton*			mSaveBtn;
	LLButton*			mPreviewBtn;

	LLMultiGesture*		mPreviewGesture;
	bool mDirty;
};
