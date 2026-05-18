/**
 * @file llpanelavatar.h
 * @brief LLPanelAvatar and related class definitions
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#pragma once

#include "llavatarnamecache.h"
#include "llpanel.h"
#include "llvector3d.h"

#include "llavatarproperties.h"
#include "llmediactrl.h"

class LLButton;
class LLCheckBoxCtrl;
class LLDropTarget;
class LLFlyoutButton;
class LLLineEditor;
class LLMessageSystem;
class LLNameEditor;
class LLPanelAvatar;
class LLScrollListCtrl;
class LLTabContainer;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLUICtrl;

enum EOnlineStatus
{
	ONLINE_STATUS_NO	= 0,
	ONLINE_STATUS_YES	= 1
};

// Base class for all sub-tabs inside the avatar profile. Many of these panels
// need to keep track of the parent panel (to get the avatar id) and only
// request data from the database when they are first drawn. JC
class LLPanelAvatarTab : public LLPanel
{
public:
	LLPanelAvatarTab(const std::string& name, const LLRect& rect,
					 LLPanelAvatar* panel_avatar);

	// Calls refresh() once per frame when panel is visible
	void draw() override;

	LL_INLINE LLPanelAvatar* getPanelAvatar() const	{ return mPanelAvatar; }

	LL_INLINE void resetDataRequested()				{ mDataRequested = false; }

	// If the data for this tab has not yet been requested, send the request.
	// Used by tabs that are filled in only when they are first displayed.
	// 'method' is one of "avatarnotesrequest", "avatarpicksrequest",
	// or "avatarclassifiedsrequest"
	void sendAvatarProfileRequestIfNeeded(S32 type);

private:
	LLPanelAvatar*	mPanelAvatar;
	bool			mDataRequested;
};

class LLPanelAvatarFirstLife final : public LLPanelAvatarTab
{
public:
	LLPanelAvatarFirstLife(const std::string& name,
						   const LLRect& rect,
						   LLPanelAvatar* panel_avatar);

	bool postBuild() override;

	void enableControls(bool own_avatar);

public:
	LLTextureCtrl*	m1stLifePicture;
	LLTextEditor*	mAbout1stLifeText;
	LLTextBox*		m1stLifeCharLimitText;
};

class LLPanelAvatarSecondLife final : public LLPanelAvatarTab
{
public:
	LLPanelAvatarSecondLife(const std::string& name,
							const LLRect& rect,
							LLPanelAvatar* panel_avatar);

	bool postBuild() override;
	void refresh() override;

	// Clear out the controls anticipating new network data.
	void clearControls();
	void enableControls(bool own_avatar);
	void updateOnlineText(bool online, bool have_calling_card);
	void updatePartnerName();

	LL_INLINE void setPartnerID(const LLUUID& id)
	{
		mPartnerID = id;
		mPartnerNamePending = true;
	}

private:
	static void onDoubleClickGroup(void* datap);
	static void onClickShowInSearchHelp(void* datap);
	static void onClickPartnerHelp(void* datap);
	static bool onClickPartnerHelpLoadURL(const LLSD& notification,
										  const LLSD& response);
	static void onClickPartnerInfo(void* datap);
	static void onCommitDisplayRatioCheck(LLUICtrl* ctrlp, void* datap);

public:
	LLNameEditor*		mLegacyName;
	LLNameEditor*		mCompleteName;

	LLTextureCtrl*		m2ndLifePicture;

	LLLineEditor*		mBornText;
	LLTextBox*			mOnlineText;
	LLTextBox*			mAccountInfoText;
	LLTextBox*			mAboutCharLimitText;

	LLScrollListCtrl*	mGroupsListCtrl;

	LLTextEditor*		mAbout2ndLifeText;
	LLCheckBoxCtrl*		mShowInSearchCheck;
	LLButton*			mShowInSearchHelpButton;

	LLButton*			mFindOnMapButton;
	LLButton*			mOfferTPButton;
	LLButton*			mRequestTPButton;
	LLButton*			mAddFriendButton;
	LLButton*			mPayButton;
	LLButton*			mIMButton;
	LLButton*			mMuteButton;

private:
	LLButton*			mPartnerInfoButton;

	LLUUID				mPartnerID;
	bool				mPartnerNamePending;
};

// WARNING !  The order of the inheritance here matters !  Do not change. - KLW
class LLPanelAvatarWeb final : public LLPanelAvatarTab,
							   public LLViewerMediaObserver
{
public:
	LLPanelAvatarWeb(const std::string& name,
					 const LLRect& rect,
					 LLPanelAvatar* panel_avatar);

	bool postBuild() override;

	void refresh() override;

	void enableControls(bool own_avatar);

	void setWebURL(std::string url);
	LL_INLINE const std::string& getWebURL() const	{ return mHome; }

	void load(const std::string& url);

	static void onURLKeystroke(LLLineEditor* editorp, void* datap);
	static void onCommitLoad(LLUICtrl* ctrlp, void* datap);
	static void onCommitSLWebProfile(LLUICtrl* ctrlp, void* datap);
	static void onCommitURL(LLUICtrl*, void* datap);
	static void onClickWebProfileHelp(void*);

	// Inherited from LLViewerMediaObserver
	void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent evt) override;

private:
	bool			mCanEditURL;
	std::string		mHome;
	std::string		mNavigateTo;
	LLFlyoutButton* mWebProfileBtn;
	LLMediaCtrl*	mWebBrowser;
};

class LLPanelAvatarAdvanced final : public LLPanelAvatarTab
{
public:
	LLPanelAvatarAdvanced(const std::string& name,
						  const LLRect& rect,
						  LLPanelAvatar* panel_avatar);

	bool postBuild() override;

	void enableControls(bool own_avatar);
	void setWantSkills(U32 want_to_mask, const std::string& want_to_text,
					   U32 skills_mask, const std::string& skills_text,
					   const std::string& languages_text);
	void getWantSkills(U32& want_to_mask, std::string& want_to_text,
					   U32& skills_mask, std::string& skills_text,
					   std::string& languages_text);

private:
	LLCheckBoxCtrl*	mWantToCheck[8];
	LLLineEditor*	mWantToEdit;
	LLCheckBoxCtrl*	mSkillsCheck[8];
	LLLineEditor*	mSkillsEdit;
	S32				mWantToCount;
	S32				mSkillsCount;
};

class LLPanelAvatarNotes final : public LLPanelAvatarTab
{
public:
	LLPanelAvatarNotes(const std::string& name, const LLRect& rect,
					   LLPanelAvatar* panel_avatar);

	bool postBuild() override;

	void refresh() override;

	void clearControls();

	static void onCommitNotes(LLUICtrl* ctrlp, void* datap);

public:
	LLTextEditor*	mNotesText;
};

class LLPanelAvatarClassified final : public LLPanelAvatarTab
{
public:
	LLPanelAvatarClassified(const std::string& name,
							const LLRect& rect,
							LLPanelAvatar* panel_avatar);

	bool postBuild() override;

	void refresh() override;

	// If can close, return true. If cannot close, pop save/discard dialog
	// and return false.
	bool canClose();

	void apply();

	bool titleIsValid();

	// Delete all the classified sub-panels from the tab container
	void deleteClassifiedPanels();

	// Unpack the outline of classified for this avatar (count, names, but not
	// actual data).
	void processAvatarClassifiedReply(LLAvatarClassifieds* datap);

private:
	static void onClickNew(void* datap);
	static void onClickDelete(void* datap);

	bool callbackDelete(const LLSD& notification, const LLSD& response);
	bool callbackNew(const LLSD& notification, const LLSD& response);

private:
	LLTabContainer*	mClassifiedTab;
	LLButton*		mButtonNew;
	LLButton*		mButtonDelete;
	LLTextBox*		mLoadingText;
};

class LLPanelAvatarPicks final : public LLPanelAvatarTab
{
public:
	LLPanelAvatarPicks(const std::string& name,
					   const LLRect& rect,
					   LLPanelAvatar* panel_avatar);

	bool postBuild() override;

	void refresh() override;

	// Delete all the pick sub-panels from the tab container
	void deletePickPanels();

	// Unpack the outline of picks and classifieds for this avatar (count,
	// names, but not actual data).
	void processAvatarPicksReply(LLAvatarPicks* datap);
	void processAvatarClassifiedReply(LLAvatarClassifieds* datap);

private:
	static void onClickNew(void* datap);
	static void onClickDelete(void* datap);

	bool callbackDelete(const LLSD& notification, const LLSD& response);

private:
	LLTabContainer*	mPicksTab;
	LLButton*		mButtonNew;
	LLButton*		mButtonDelete;
	LLTextBox*		mLoadingText;
};

class LLPanelAvatar final : public LLPanel, LLAvatarPropertiesObserver
{
public:
	LLPanelAvatar(const std::string& name, const LLRect& rect,
				  bool allow_edit);
	~LLPanelAvatar() override;

	bool postBuild() override;

	// If can close, return true. If cannot close, pop save/discard dialog
	// and return false.
	bool canClose();

	// LLAvatarPropertiesObserver override
	void processProperties(S32 type, void* datap) override;

	// Fill in the avatar ID and handle some field fill-in, as well as
	// button enablement.
	// Pass one of the ONLINE_STATUS_foo constants above.
	void setAvatarID(const LLUUID& avatar_id, const std::string& name,
					 EOnlineStatus online_status);

	void setOnlineStatus(EOnlineStatus online_status);

	LL_INLINE const LLUUID& getAvatarID() const		{ return mAvatarID; }

	LL_INLINE const std::string& getAvatarUserName() const
	{
		return mAvatarUserName;
	}

	// Lists the agents groups, based on the info held in LLAgent, and flagging
	// hidden groups as such, via the use of an Italics font and an explanatory
	// tool tip. HB
	void listAgentGroups();

	void sendAvatarPropertiesUpdate();
	void sendAvatarNotesUpdate();

	void selectTab(S32 tabnum);
	void selectTabByName(std::string tab_name);

	LL_INLINE bool haveData() const					{ return mHaveProperties; }
	LL_INLINE bool isEditable() const				{ return mAllowEdit; }

	static void onClickTrack(void* datap);
	static void onClickIM(void* datap);
	static void onClickOfferTeleport(void* datap);
	static void onClickRequestTeleport(void* datap);
	static void onClickPay(void* datap);
	static void onClickAddFriend(void* datap);
	static void onClickOK(void* datap);
	static void onClickCancel(void* datap);
	static void onClickKick(void* datap);
	static void onClickFreeze(void* datap);
	static void onClickUnfreeze(void* datap);
	static void onClickCSR(void* datap);
	static void onClickMute(void* datap);

private:
	void enableOKIfReady();

	static void showProfileCallback(S32 option, void* datap);
	static void completeNameCallback(const LLUUID& agent_id,
									 const LLAvatarName& avatar_name,
									 LLHandle<LLPanel> handle);

	static void* createPanelAvatar(void* datap);
	static void* createFloaterAvatarInfo(void* datap);
	static void* createPanelAvatarSecondLife(void* datap);
	static void* createPanelAvatarWeb(void* datap);
	static void* createPanelAvatarInterests(void* datap);
	static void* createPanelAvatarPicks(void* datap);
	static void* createPanelAvatarClassified(void* datap);
	static void* createPanelAvatarFirstLife(void* datap);
	static void* createPanelAvatarNotes(void* datap);

public:
	LLPanelAvatarSecondLife*	mPanelSecondLife;
	LLPanelAvatarAdvanced*		mPanelAdvanced;
	LLPanelAvatarClassified*	mPanelClassified;
	LLPanelAvatarPicks*			mPanelPicks;
	LLPanelAvatarNotes*			mPanelNotes;
	LLPanelAvatarFirstLife*		mPanelFirstLife;
	LLPanelAvatarWeb*			mPanelWeb;

	LLDropTarget* 				mDropTarget;

	// Teen users are not allowed to see or enter data into the first life
	// page, or their own about/interests text entry fields.
	static bool					sAllowFirstLife;

	// Tool tip and text strings, defined in panel_avatar.xml
	static std::string			sLoading;			// public

private:
	static std::string			sClickToEnlarge;
	static std::string			sShowOnMapNonFriend;
	static std::string			sShowOnMapFriendOffline;
	static std::string			sShowOnMapFriendOnline;
	static std::string			sTeleportGod;
	static std::string			sTeleportPrelude;
	static std::string			sTeleportNormal;

	// Which avatar's profile is this ?
	LLUUID						mAvatarID;
	std::string					mAvatarUserName;	// Known avatar user name
	LLButton*					mOKButton;
	LLButton*					mCancelButton;
	LLButton*					mKickButton;
	LLButton*					mFreezeButton;
	LLButton*					mUnfreezeButton;
	LLButton*					mCSRButton;
	LLTabContainer*				mTab;

	// Only update note if data received from database and note is changed from
	// database version
	std::string					mLastNotes;
	bool						mHaveNotes;

	bool						mHaveProperties;
	bool						mHaveInterests;
	bool						mIsFriend;			// Are we friends ?

	bool						mAllowEdit;
};
