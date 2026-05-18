/**
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
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

#include "llviewerprecompiledheaders.h"

#include "llpanelavatar.h"

#include "llcachename.h"
#include "llcheckboxctrl.h"
#include "llclassifiedflags.h"
#include "llcombobox.h"
#include "lleconomy.h"
#include "llfloater.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llnameeditor.h"
#include "llpluginclassmedia.h"
#include "llscrolllistctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lluictrlfactory.h"
#include "roles_constants.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatartracker.h"
#include "llfloateravatarinfo.h"
#include "llfloaterfriends.h"
#include "llfloatergroupinfo.h"
#include "llfloatermediabrowser.h"
#include "llfloatermute.h"
#include "llfloaterworldmap.h"
#include "llgridmanager.h"			// For gIsInSecondLife
#include "llinventorymodel.h"
#include "llmutelist.h"
#include "llpanelclassified.h"
#include "llpanelpick.h"
//MK
#include "mkrlinterface.h"
//mk
#include "llslurl.h"
#include "llstatusbar.h"
#include "lltooldraganddrop.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"		// For send_generic_message()
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexteditor.h"
#include "llvoavatar.h"
#include "llweb.h"

using namespace std::placeholders;

//static
bool LLPanelAvatar::sAllowFirstLife = false;

std::string LLPanelAvatar::sLoading;
std::string LLPanelAvatar::sClickToEnlarge;
std::string LLPanelAvatar::sShowOnMapNonFriend;
std::string LLPanelAvatar::sShowOnMapFriendOffline;
std::string LLPanelAvatar::sShowOnMapFriendOnline;
std::string LLPanelAvatar::sTeleportGod;
std::string LLPanelAvatar::sTeleportPrelude;
std::string LLPanelAvatar::sTeleportNormal;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLDropTarget : public LLView
{
protected:
	LOG_CLASS(LLDropTarget);

public:
	LLDropTarget(const std::string& name, const LLRect& rect,
				 const LLUUID& agent_id);

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	// LLView functionality
	bool handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
						   EDragAndDropType cargo_type, void* cargo_data,
						   EAcceptance* accept, std::string& tooltip) override;

	void setAgentID(const LLUUID& agent_id)		{ mAgentID = agent_id; }

protected:
	LLUUID mAgentID;
};

LLDropTarget::LLDropTarget(const std::string& name, const LLRect& rect,
						   const LLUUID& agent_id)
:	LLView(name, rect, false, FOLLOWS_ALL),
	mAgentID(agent_id)
{
}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "No operation." << llendl;
}

bool LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	if (getParent())
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop,
												 cargo_type, cargo_data, accept);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// LLPanelAvatarTab class
//-----------------------------------------------------------------------------

LLPanelAvatarTab::LLPanelAvatarTab(const std::string& name, const LLRect& rect,
								   LLPanelAvatar* panel_avatar)
:	LLPanel(name, rect),
	mPanelAvatar(panel_avatar),
	mDataRequested(false)
{
}

//virtual
void LLPanelAvatarTab::draw()
{
	refresh();
	LLPanel::draw();
}

void LLPanelAvatarTab::sendAvatarProfileRequestIfNeeded(S32 type)
{
	if (!mDataRequested)
	{
		mDataRequested = true;
		LLAvatarProperties::sendGenericRequest(mPanelAvatar->getAvatarID(),
											   type);
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarSecondLife class
//-----------------------------------------------------------------------------

LLPanelAvatarSecondLife::LLPanelAvatarSecondLife(const std::string& name,
												 const LLRect& rect,
												 LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mPartnerNamePending(false)
{
}

bool LLPanelAvatarSecondLife::postBuild()
{
	mLegacyName = getChild<LLNameEditor>("name");
	mCompleteName = getChild<LLNameEditor>("complete_name");

	mBornText = getChild<LLLineEditor>("born");
	mBornText->setEnabled(false);

	mAccountInfoText = getChild<LLTextBox>("acct");

	childSetEnabled("partner_edit", false);
	childSetAction("partner_help", onClickPartnerHelp, this);

	mPartnerInfoButton = getChild<LLButton>("partner_info");
	mPartnerInfoButton->setClickedCallback(onClickPartnerInfo, this);
	mPartnerInfoButton->setEnabled(mPartnerID.notNull());

	mAboutCharLimitText = getChild<LLTextBox>("sl_about_limit");
	bool limited = LLPanelAvatar::sAllowFirstLife &&
				   !(gSavedSettings.getBool("UseAgentProfileCap") &&
					 gAgent.hasRegionCapability("AgentProfile"));
	mAboutCharLimitText->setVisible(limited);
									  
	mAbout2ndLifeText = getChild<LLTextEditor>("about");
	mAbout2ndLifeText->setVisible(LLPanelAvatar::sAllowFirstLife);

	mShowInSearchCheck = getChild<LLCheckBoxCtrl>("show_in_search_chk");
	mShowInSearchCheck->setVisible(LLPanelAvatar::sAllowFirstLife);

	mShowInSearchHelpButton = getChild<LLButton>("show_in_search_help_btn");
	mShowInSearchHelpButton->setClickedCallback(onClickShowInSearchHelp, this);
	mShowInSearchHelpButton->setVisible(LLPanelAvatar::sAllowFirstLife);

	mOnlineText = getChild<LLTextBox>("online_yes");
	mOnlineText->setVisible(false);

	mFindOnMapButton = getChild<LLButton>("find_on_map_btn");
	mFindOnMapButton->setClickedCallback(LLPanelAvatar::onClickTrack,
										 getPanelAvatar());

	mOfferTPButton = getChild<LLButton>("offer_tp_btn");
	mOfferTPButton->setClickedCallback(LLPanelAvatar::onClickOfferTeleport,
									   getPanelAvatar());

	mRequestTPButton = getChild<LLButton>("request_tp_btn");
	mRequestTPButton->setClickedCallback(LLPanelAvatar::onClickRequestTeleport,
										 getPanelAvatar());

	mAddFriendButton = getChild<LLButton>("add_friend_btn");
	mAddFriendButton->setClickedCallback(LLPanelAvatar::onClickAddFriend,
										 getPanelAvatar());

	mPayButton = getChild<LLButton>("pay_btn");
	mPayButton->setClickedCallback(LLPanelAvatar::onClickPay,
								   getPanelAvatar());

	mIMButton = getChild<LLButton>("im_btn");
	mIMButton->setClickedCallback(LLPanelAvatar::onClickIM, getPanelAvatar());

	mMuteButton = getChild<LLButton>("mute_btn");
	mMuteButton->setClickedCallback(LLPanelAvatar::onClickMute,
									getPanelAvatar());

	mGroupsListCtrl = getChild<LLScrollListCtrl>("groups");
	mGroupsListCtrl->setDoubleClickCallback(onDoubleClickGroup);
	mGroupsListCtrl->setCallbackUserData(this);

	bool square = gSavedSettings.getBool("ProfilePictureSquare");
	m2ndLifePicture = getChild<LLTextureCtrl>("img");
	m2ndLifePicture->setFallbackImageName("default_profile_picture.j2c");
	m2ndLifePicture->setDisplayRatio(square ? 1.f : 0.f);

	LLCheckBoxCtrl* ratiocheckp = getChild<LLCheckBoxCtrl>("ratio_chk");
	ratiocheckp->setCommitCallback(onCommitDisplayRatioCheck);
	ratiocheckp->setCallbackUserData(this);
	ratiocheckp->set(square);

	enableControls(getPanelAvatar()->getAvatarID() == gAgentID);

	return true;
}

void LLPanelAvatarSecondLife::refresh()
{
	updatePartnerName();

	static LLCachedControl<bool> use_cap(gSavedSettings, "UseAgentProfileCap");
	bool limited = LLPanelAvatar::sAllowFirstLife &&
				   !(use_cap && gAgent.hasRegionCapability("AgentProfile"));
	mAboutCharLimitText->setVisible(limited);
}

void LLPanelAvatarSecondLife::updatePartnerName()
{
	bool has_partner = mPartnerID.notNull();
	mPartnerInfoButton->setEnabled(has_partner);

	if (has_partner && mPartnerNamePending)
	{
		std::string first, last;
		if (gCacheNamep && gCacheNamep->getName(mPartnerID, first, last))
		{
			childSetTextArg("partner_edit", "[FIRST]", first);
			childSetTextArg("partner_edit", "[LAST]", last);
			mPartnerNamePending = false;
		}
	}
}

// Empty the data out of the controls, since we have to wait for new data off
// the network.
void LLPanelAvatarSecondLife::clearControls()
{
	m2ndLifePicture->setImageAssetID(LLUUID::null);
	mAbout2ndLifeText->setValue("");
	mBornText->setValue("");
	mAccountInfoText->setValue("");

	childSetTextArg("partner_edit", "[FIRST]", LLStringUtil::null);
	childSetTextArg("partner_edit", "[LAST]", LLStringUtil::null);

	mPartnerID.setNull();

	mGroupsListCtrl->deleteAllItems();
}

void LLPanelAvatarSecondLife::enableControls(bool own_avatar)
{
	m2ndLifePicture->setEnabled(own_avatar);
	mAbout2ndLifeText->setEnabled(own_avatar);
	mShowInSearchCheck->setVisible(own_avatar);
	mShowInSearchCheck->setEnabled(own_avatar);
	mShowInSearchHelpButton->setVisible(own_avatar);
	mShowInSearchHelpButton->setEnabled(own_avatar);
}

//static
void LLPanelAvatarSecondLife::onCommitDisplayRatioCheck(LLUICtrl* ctrlp,
														void* datap)
{
	LLPanelAvatarSecondLife* self = (LLPanelAvatarSecondLife*)datap;
	LLCheckBoxCtrl* checkp = (LLCheckBoxCtrl*)ctrlp;
	if (self && checkp)
	{
		bool checked = checkp->get();
		self->m2ndLifePicture->setDisplayRatio(checked ? 1.f : 0.f);
		gSavedSettings.setBool("ProfilePictureSquare", checked);
	}
}

//static
void LLPanelAvatarSecondLife::onDoubleClickGroup(void* datap)
{
	LLPanelAvatarSecondLife* self = (LLPanelAvatarSecondLife*)datap;
	if (!self) return;

	LLScrollListItem* item = self->mGroupsListCtrl->getFirstSelected();
	if (item && item->getUUID().notNull())
	{
		llinfos << "Show group info " << item->getUUID() << llendl;
		LLFloaterGroupInfo::showFromUUID(item->getUUID());
	}
}

//static
void LLPanelAvatarSecondLife::onClickShowInSearchHelp(void*)
{
	gNotifications.add("ClickPublishHelpAvatar");
}

//static
void LLPanelAvatarSecondLife::onClickPartnerHelp(void*)
{
	gNotifications.add("ClickPartnerHelpAvatar", LLSD(), LLSD(),
					   onClickPartnerHelpLoadURL);
}

//static
bool LLPanelAvatarSecondLife::onClickPartnerHelpLoadURL(const LLSD& notification,
														const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		LLWeb::loadURL("https://secondlife.com/partner");
	}
	return false;
}

//static
void LLPanelAvatarSecondLife::onClickPartnerInfo(void* datap)
{
	LLPanelAvatarSecondLife* self = (LLPanelAvatarSecondLife*)datap;
	if (self && self->mPartnerID.notNull())
	{
		LLFloaterAvatarInfo::showFromProfile(self->mPartnerID,
											 self->getScreenRect());
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarFirstLife class
//-----------------------------------------------------------------------------

LLPanelAvatarFirstLife::LLPanelAvatarFirstLife(const std::string& name,
											   const LLRect& rect,
											   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

bool LLPanelAvatarFirstLife::postBuild()
{
	m1stLifePicture = getChild<LLTextureCtrl>("img");
	m1stLifePicture->setFallbackImageName("default_profile_picture.j2c");

	mAbout1stLifeText = getChild<LLTextEditor>("about");
	m1stLifeCharLimitText = getChild<LLTextBox>("fl_text_limit");

	enableControls(getPanelAvatar()->getAvatarID() == gAgentID);

	return true;
}

void LLPanelAvatarFirstLife::enableControls(bool own_avatar)
{
	m1stLifePicture->setEnabled(own_avatar);
	mAbout1stLifeText->setEnabled(own_avatar);
	bool limited = !(gSavedSettings.getBool("UseAgentProfileCap") &&
					 gAgent.hasRegionCapability("AgentProfile"));
	m1stLifeCharLimitText->setVisible(limited);
}

//-----------------------------------------------------------------------------
// LLPanelAvatarWeb class
//-----------------------------------------------------------------------------

LLPanelAvatarWeb::LLPanelAvatarWeb(const std::string& name, const LLRect& rect,
								   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

bool LLPanelAvatarWeb::postBuild()
{
	childSetKeystrokeCallback("url_edit", onURLKeystroke, this);
	childSetCommitCallback("load", onCommitLoad, this);

	mWebProfileBtn = getChild<LLFlyoutButton>("sl_web_profile");
	mWebProfileBtn->setCommitCallback(onCommitSLWebProfile);
	mWebProfileBtn->setCallbackUserData(this);
	bool enabled = !LLFloaterAvatarInfo::getProfileURL("").empty();
	mWebProfileBtn->setEnabled(enabled);

	childSetAction("web_profile_help", onClickWebProfileHelp, this);

	childSetCommitCallback("url_edit", onCommitURL, this);

	childSetControlName("auto_load", "AutoLoadWebProfiles");

	mWebBrowser = getChild<LLMediaCtrl>("profile_html");
	mWebBrowser->addObserver(this);

	return true;
}

void LLPanelAvatarWeb::refresh()
{
	if (!mNavigateTo.empty())
	{
		llinfos << "Loading " << mNavigateTo << llendl;
		mWebBrowser->navigateTo(mNavigateTo);
		mNavigateTo.clear();
	}
	std::string user_name = getPanelAvatar()->getAvatarUserName();
	bool enabled = !LLFloaterAvatarInfo::getProfileURL(user_name).empty();
	mWebProfileBtn->setEnabled(enabled);
}

void LLPanelAvatarWeb::enableControls(bool own_avatar)
{
	mCanEditURL = own_avatar;
	childSetEnabled("url_edit", own_avatar);
}

void LLPanelAvatarWeb::setWebURL(std::string url)
{
	bool changed_url = mHome != url;

	mHome = url;
	bool have_url = !mHome.empty();

	childSetText("url_edit", mHome);
	childSetEnabled("load", have_url);

	if (have_url && gSavedSettings.getBool("AutoLoadWebProfiles"))
	{
		if (changed_url)
		{
			load(mHome);
		}
	}
	else
	{
		childSetVisible("profile_html", false);
		childSetVisible("status_text", false);
	}
}

//static
void LLPanelAvatarWeb::onCommitURL(LLUICtrl*, void* datap)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)datap;
	if (self)
	{
		self->mHome = self->childGetText("url_edit");
		self->load(self->childGetText("url_edit"));
	}
}

//static
void LLPanelAvatarWeb::onClickWebProfileHelp(void*)
{
	gNotifications.add("ClickWebProfileHelpAvatar");
}

void LLPanelAvatarWeb::load(const std::string& url)
{
	bool have_url = !url.empty();

	childSetVisible("profile_html", have_url);
	childSetVisible("status_text", have_url);
	childSetText("status_text", LLStringUtil::null);

	if (have_url)
	{
		if (mCanEditURL)
		{
			childSetEnabled("url_edit", false);
		}
		childSetText("url_edit", LLPanelAvatar::sLoading);

		mNavigateTo = url;
	}
}

//static
void LLPanelAvatarWeb::onURLKeystroke(LLLineEditor* editorp, void* datap)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)datap;
	if (self && editorp)
	{
		const std::string& url = editorp->getText();
		self->childSetEnabled("load", !url.empty());
	}
}

//static
void LLPanelAvatarWeb::onCommitLoad(LLUICtrl* ctrlp, void* datap)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)datap;
	if (!self || !ctrlp) return;

	std::string valstr = ctrlp->getValue().asString();
	std::string urlstr = self->childGetText("url_edit");
	if (valstr == "builtin")
	{
		 // Open in built-in browser
		if (!self->mHome.empty())
		{
			LLFloaterMediaBrowser::showInstance(urlstr);
		}
	}
	else if (valstr == "open")
	{
		 // Open in user's external browser
		if (!urlstr.empty())
		{
			LLWeb::loadURLExternal(urlstr);
		}
	}
	else if (valstr == "home")
	{
		 // Reload profile owner's home page
		if (!self->mHome.empty())
		{
			self->mWebBrowser->setTrusted(false);
			self->load(self->mHome);
		}
	}
	else if (!urlstr.empty())
	{
		// Load url string into browser panel
		self->mWebBrowser->setTrusted(false);
		self->load(urlstr);
	}
}

//static
void LLPanelAvatarWeb::onCommitSLWebProfile(LLUICtrl* ctrlp, void* datap)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)datap;
	if (!self || !ctrlp) return;

	std::string user_name = self->getPanelAvatar()->getAvatarUserName();
	if (user_name.empty()) return;

	std::string urlstr = LLFloaterAvatarInfo::getProfileURL(user_name);
	if (urlstr.empty()) return;

	std::string valstr = ctrlp->getValue().asString();
	if (valstr == "sl_builtin")
	{
		 // Open in a trusted built-in browser
		LLFloaterMediaBrowser::showInstance(urlstr, true);
	}
	else if (valstr == "sl_open")
	{
		 // Open in user's external browser
		LLWeb::loadURLExternal(urlstr);
	}
	else
	{
		// Load SL's web-based avatar profile (trusted)
		self->mWebBrowser->setTrusted(true);
		self->load(urlstr);
	}
}

void LLPanelAvatarWeb::handleMediaEvent(LLPluginClassMedia* self,
										EMediaEvent event)
{
	switch (event)
	{
		case MEDIA_EVENT_STATUS_TEXT_CHANGED:
			childSetText("status_text", self->getStatusText());
			break;

		case MEDIA_EVENT_LOCATION_CHANGED:
			childSetText("url_edit", self->getLocation());
			enableControls(mCanEditURL);

		default:
			// Having a default case makes the compiler happy.
			break;
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarAdvanced class
//-----------------------------------------------------------------------------

LLPanelAvatarAdvanced::LLPanelAvatarAdvanced(const std::string& name,
											 const LLRect& rect,
											 LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mWantToCount(0),
	mSkillsCount(0)
{
}

bool LLPanelAvatarAdvanced::postBuild()
{
	mWantToEdit = getChild<LLLineEditor>("want_to_edit");
	mWantToEdit->setVisible(LLPanelAvatar::sAllowFirstLife);

	mSkillsEdit = getChild<LLLineEditor>("skills_edit");
	mSkillsEdit->setVisible(LLPanelAvatar::sAllowFirstLife);

	const size_t want_to_check_size = LL_ARRAY_SIZE(mWantToCheck);
	for (size_t i = 0; i < want_to_check_size; ++i)
	{
		mWantToCheck[i] = NULL;
	}

	const size_t skills_check_size = LL_ARRAY_SIZE(mSkillsCheck);
	for (size_t i = 0; i < skills_check_size; ++i)
	{
		mSkillsCheck[i] = NULL;
	}

	mWantToCount = want_to_check_size < 8 ? want_to_check_size : 8;
	for (S32 i = 0; i < mWantToCount; ++i)
	{
		std::string ctlname = llformat("chk%d", i);
		mWantToCheck[i] = getChild<LLCheckBoxCtrl>(ctlname.c_str());
	}

	mSkillsCount = skills_check_size < 6 ? skills_check_size : 6;
	for (S32 i = 0; i < mSkillsCount; ++i)
	{
		// Find the Skills checkboxes and save off their controls
		std::string ctlname = llformat("schk%d", i);
		mSkillsCheck[i] = getChild<LLCheckBoxCtrl>(ctlname.c_str());
	}

	return true;
}

void LLPanelAvatarAdvanced::enableControls(bool own_avatar)
{
	for (S32 i = 0; i < mWantToCount; ++i)
	{
		if (mWantToCheck[i])
		{
			mWantToCheck[i]->setEnabled(own_avatar);
		}
	}
	for (S32 i = 0; i < mSkillsCount; ++i)
	{
		if (mSkillsCheck[i])
		{
			mSkillsCheck[i]->setEnabled(own_avatar);
		}
	}

	if (mWantToEdit && mSkillsEdit)
	{
		mWantToEdit->setEnabled(own_avatar);
		mSkillsEdit->setEnabled(own_avatar);
	}
	childSetEnabled("languages_edit", own_avatar);
}

void LLPanelAvatarAdvanced::setWantSkills(U32 want_to_mask,
										  const std::string& want_to_text,
										  U32 skills_mask,
										  const std::string& skills_text,
										  const std::string& languages_text)
{
	static LLCachedControl<LLColor4U> color_off(gColors, "LabelDisabledColor");
	static LLCachedControl<LLColor4U> color_on(gColors, "LabelTextColor");
	LLColor4 enabled_color(color_on);
	LLColor4 disabled_color(color_off);
	for (S32 i = 0; i < mWantToCount; ++i)
	{
		bool enabled = want_to_mask & (1 << i);
		mWantToCheck[i]->set(enabled);
		mWantToCheck[i]->setDisabledColor(enabled ? enabled_color
												  : disabled_color);
	}
	for (S32 i = 0; i < mSkillsCount; ++i)
	{
		bool enabled = skills_mask & (1 << i);
		mSkillsCheck[i]->set(enabled);
		mSkillsCheck[i]->setDisabledColor(enabled ? enabled_color
												  : disabled_color);
	}
	if (mWantToEdit && mSkillsEdit)
	{
		mWantToEdit->setText(want_to_text);
		mSkillsEdit->setText(skills_text);
	}
	childSetText("languages_edit", languages_text);
}

void LLPanelAvatarAdvanced::getWantSkills(U32& want_to_mask,
										  std::string& want_to_text,
										  U32& skills_mask,
										  std::string& skills_text,
										  std::string& languages_text)
{
	want_to_mask = 0;
	for (S32 t = 0; t < mWantToCount; ++t)
	{
		if (mWantToCheck[t]->get())
		{
			want_to_mask |= 1 << t;
		}
	}

	skills_mask = 0;
	for (S32 t = 0; t < mSkillsCount; ++t)
	{
		if (mSkillsCheck[t]->get())
		{
			skills_mask |= 1 << t;
		}
	}

	if (mWantToEdit)
	{
		want_to_text = mWantToEdit->getText();
	}
	else
	{
		want_to_text.clear();
	}

	if (mSkillsEdit)
	{
		skills_text = mSkillsEdit->getText();
	}
	else
	{
		skills_text.clear();
	}

	languages_text = childGetText("languages_edit");
}

//-----------------------------------------------------------------------------
// LLPanelAvatarNotes class
//-----------------------------------------------------------------------------

LLPanelAvatarNotes::LLPanelAvatarNotes(const std::string& name,
									   const LLRect& rect,
									   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

bool LLPanelAvatarNotes::postBuild()
{
	mNotesText = getChild<LLTextEditor>("notes edit");
	mNotesText->setCommitCallback(onCommitNotes);
	mNotesText->setCallbackUserData(this);
	mNotesText->setCommitOnFocusLost(true);

	return true;
}

void LLPanelAvatarNotes::refresh()
{
	sendAvatarProfileRequestIfNeeded(APT_NOTES);
}

void LLPanelAvatarNotes::clearControls()
{
	mNotesText->setEnabled(false);
	mNotesText->setText(LLPanelAvatar::sLoading);
}

//static
void LLPanelAvatarNotes::onCommitNotes(LLUICtrl*, void* datap)
{
	LLPanelAvatarNotes* self = (LLPanelAvatarNotes*)datap;
	if (self)
	{
		self->getPanelAvatar()->sendAvatarNotesUpdate();
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarClassified class
//-----------------------------------------------------------------------------

LLPanelAvatarClassified::LLPanelAvatarClassified(const std::string& name,
												 const LLRect& rect,
												 LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mClassifiedTab(NULL),
	mButtonNew(NULL),
	mButtonDelete(NULL),
	mLoadingText(NULL)
{
}

bool LLPanelAvatarClassified::postBuild()
{
	mClassifiedTab = getChild<LLTabContainer>("classified tab");

	mButtonNew = getChild<LLButton>("New...");
	mButtonNew->setClickedCallback(onClickNew, this);

	mButtonDelete = getChild<LLButton>("Delete...");
	mButtonDelete->setClickedCallback(onClickDelete, this);

	mLoadingText = getChild<LLTextBox>("loading_text");

	return true;
}

void LLPanelAvatarClassified::refresh()
{
	bool is_self = getPanelAvatar()->getAvatarID() == gAgentID;

	S32 tab_count = mClassifiedTab ? mClassifiedTab->getTabCount() : 0;

	bool allow_new = tab_count < MAX_CLASSIFIEDS;
	bool allow_delete = tab_count > 0;
	bool show_help = tab_count == 0;

	// *HACK: Do not allow making new classifieds from inside the directory.
	// The logic for save/don't save when closing is too hairy, and the
	// directory is conceptually read-only. JC
	bool in_directory = false;
	LLView* view = this;
	while (view)
	{
		if (view->getName() == "directory")
		{
			in_directory = true;
			break;
		}
		view = view->getParent();
	}
	if (mButtonNew)
	{
		mButtonNew->setEnabled(is_self && !in_directory && allow_new);
		mButtonNew->setVisible(!in_directory);
	}
	if (mButtonDelete)
	{
		mButtonDelete->setEnabled(is_self && !in_directory && allow_delete);
		mButtonDelete->setVisible(!in_directory);
	}
	if (mClassifiedTab)
	{
		mClassifiedTab->setVisible(!show_help);
	}

	sendAvatarProfileRequestIfNeeded(APT_CLASSIFIEDS);
}

bool LLPanelAvatarClassified::canClose()
{
	if (!mClassifiedTab) return true;

	for (S32 i = 0; i < mClassifiedTab->getTabCount(); ++i)
	{
		LLPanelClassified* panel =
			(LLPanelClassified*)mClassifiedTab->getPanelByIndex(i);
		if (panel && !panel->canClose())
		{
			return false;
		}
	}
	return true;
}

bool LLPanelAvatarClassified::titleIsValid()
{
	if (mClassifiedTab)
	{
		LLPanelClassified* panel =
			(LLPanelClassified*)mClassifiedTab->getCurrentPanel();
		if (panel && !panel->titleIsValid())
		{
			return false;
		}
	}

	return true;
}

void LLPanelAvatarClassified::apply()
{
	if (!mClassifiedTab) return;

	for (S32 i = 0; i < mClassifiedTab->getTabCount(); ++i)
	{
		LLPanelClassified* panel =
			(LLPanelClassified*)mClassifiedTab->getPanelByIndex(i);
		if (panel)
		{
			panel->apply();
		}
	}
}

void LLPanelAvatarClassified::deleteClassifiedPanels()
{
	if (mClassifiedTab)
	{
		mClassifiedTab->deleteAllTabs();
	}
	if (mButtonNew)
	{
		mButtonNew->setVisible(false);
	}
	if (mButtonDelete)
	{
		mButtonDelete->setVisible(false);
	}
	if (mLoadingText)
	{
		mLoadingText->setVisible(true);
	}
}

void LLPanelAvatarClassified::processAvatarClassifiedReply(LLAvatarClassifieds* datap)
{
	// Note: we do not remove old panels. We need to be able to process
	// multiple packets for people who have lots of classifieds. JC

	LLUUID id;
	std::string classified_name;
	for (LLAvatarClassifieds::map_t::iterator it = datap->mMap.begin(),
											  end = datap->mMap.end();
		 it != end; ++it)
	{
		LLPanelClassified* panelp = new LLPanelClassified(false, false);
		panelp->setClassifiedID(it->first);

		// This will request data from the server when the classified is first
		// drawn
		panelp->markForServerRequest();

		// The button should automatically truncate long names for us
		if (mClassifiedTab)
		{
			mClassifiedTab->addTabPanel(panelp, it->second);
		}
	}

	// Make sure somebody is highlighted. This works even if there are no tabs
	// in the container.
	if (mClassifiedTab)
	{
		mClassifiedTab->selectFirstTab();
	}

	if (mButtonNew)
	{
		mButtonNew->setVisible(true);
	}
	if (mButtonDelete)
	{
		mButtonDelete->setVisible(true);
	}
	if (mLoadingText)
	{
		mLoadingText->setVisible(false);
	}
}

// Create a new classified panel. It will automatically handle generating its
// own id when it is time to save.
//static
void LLPanelAvatarClassified::onClickNew(void* datap)
{
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)datap;
	if (!self) return;

//MK
	if (gRLenabled && gRLInterface.mContainsShowloc)
	{
		return;
	}
//mk

	gNotifications.add("AddClassified", LLSD(), LLSD(),
					   std::bind(&LLPanelAvatarClassified::callbackNew, self,
								 _1, _2));
}

bool LLPanelAvatarClassified::callbackNew(const LLSD& notification,
										  const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		LLPanelClassified* panelp = new LLPanelClassified(false, false);
		panelp->initNewClassified();
		if (mClassifiedTab)
		{
			mClassifiedTab->addTabPanel(panelp, panelp->getClassifiedName());
			mClassifiedTab->selectLastTab();
		}
	}

	return false;
}

//static
void LLPanelAvatarClassified::onClickDelete(void* datap)
{
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)datap;
	if (!self) return;

	LLPanelClassified* panelp = NULL;
	if (self->mClassifiedTab)
	{
		panelp = (LLPanelClassified*)self->mClassifiedTab->getCurrentPanel();
	}
	if (!panelp) return;

	LLSD args;
	args["NAME"] = panelp->getClassifiedName();
	gNotifications.add("DeleteClassified", args, LLSD(),
					   std::bind(&LLPanelAvatarClassified::callbackDelete,
								 self, _1, _2));
}

bool LLPanelAvatarClassified::callbackDelete(const LLSD& notification,
											 const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) != 0 ||
		!mClassifiedTab)
	{
		return false;
	}

	LLPanelClassified* panelp =
		(LLPanelClassified*)mClassifiedTab->getCurrentPanel();
	if (!panelp)
	{
		return false;
	}

	LLAvatarProperties::sendClassifiedDelete(panelp->getClassifiedID());

	if (mClassifiedTab)
	{
		mClassifiedTab->removeTabPanel(panelp);
	}

	delete panelp;

	return false;
}

//-----------------------------------------------------------------------------
// LLPanelAvatarPicks class
//-----------------------------------------------------------------------------

LLPanelAvatarPicks::LLPanelAvatarPicks(const std::string& name,
									   const LLRect& rect,
									   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mPicksTab(NULL),
	mButtonNew(NULL),
	mButtonDelete(NULL),
	mLoadingText(NULL)
{
}

bool LLPanelAvatarPicks::postBuild()
{
	mPicksTab = getChild<LLTabContainer>("picks tab");

	mButtonNew = getChild<LLButton>("New...");
	mButtonNew->setClickedCallback(onClickNew, this);

	mButtonDelete = getChild<LLButton>("Delete...");
	mButtonDelete->setClickedCallback(onClickDelete, this);

	mLoadingText = getChild<LLTextBox>("loading_text");

	return true;
}

void LLPanelAvatarPicks::refresh()
{
	bool is_self = getPanelAvatar()->getAvatarID() == gAgentID;
	bool editable = getPanelAvatar()->isEditable();
	S32 tab_count = mPicksTab ? mPicksTab->getTabCount() : 0;
	if (mButtonNew)
	{
		S32 max_picks = LLEconomy::getInstance()->getPicksLimit();
		mButtonNew->setEnabled(is_self && tab_count < max_picks);
		mButtonNew->setVisible(is_self && editable);
	}
	if (mButtonDelete)
	{
		mButtonDelete->setEnabled(is_self && tab_count > 0);
		mButtonDelete->setVisible(is_self && editable);
	}

	sendAvatarProfileRequestIfNeeded(APT_PICKS);
}

void LLPanelAvatarPicks::deletePickPanels()
{
	if (mPicksTab)
	{
		mPicksTab->deleteAllTabs();
	}
	if (mButtonNew)
	{
		mButtonNew->setVisible(false);
	}
	if (mButtonDelete)
	{
		mButtonDelete->setVisible(false);
	}
	if (mLoadingText)
	{
		mLoadingText->setVisible(true);
	}
}

void LLPanelAvatarPicks::processAvatarPicksReply(LLAvatarPicks* datap)
{
	// Clear out all the old panels. We will replace them with the correct
	// number of new panels.
	deletePickPanels();

	// The database needs to know for which user to look up picks.
	LLUUID avatar_id = datap->mAvatarId;

	// We may want to sort picks by name in the tab container... HB
	typedef std::pair<LLPanelPick*, std::string> panel_name_pair_t;
	typedef std::vector<panel_name_pair_t> pick_tabs_vec_t;
	pick_tabs_vec_t pick_tabs;
	pick_tabs.reserve(datap->mMap.size());

	for (LLAvatarPicks::map_t::iterator it = datap->mMap.begin(),
										end = datap->mMap.end();
		 it != end; ++it)
	{
		LLPanelPick* panelp = new LLPanelPick(false);
		panelp->setPickID(it->first, avatar_id);

		// This will request data from the server when the pick is first drawn
		panelp->markForServerRequest();

		// Store the tab data data in our vector so that we may sort the latter
		// if needed. HB
		if (mPicksTab)
		{
			pick_tabs.emplace_back(panelp, it->second);
		}
	}
	// Now, add picks (if any) in the tab container. HB
	size_t picks_count = pick_tabs.size();
	if (picks_count)
	{
		// Sort by name if configured to do so. Note: with the legacy UDP
		// messaging protocol, the server used to send the picks in the order
		// in which they were created. This is however no more true with the
		// new HTTP/capability protocol. HB
		if (gSavedSettings.getBool("SortPicksInAvatarProfile"))
		{
			std::sort(pick_tabs.begin(), pick_tabs.end(),
					  [](const panel_name_pair_t& a, const panel_name_pair_t& b)
					  {
						return a.second < b.second;
					  });
		}
		for (size_t i = 0; i < picks_count; ++i)
		{
			// The button should automatically truncate long names for us
			const panel_name_pair_t& tab_data = pick_tabs[i];
			mPicksTab->addTabPanel(tab_data.first, tab_data.second);
		}
		mPicksTab->selectFirstTab();
	}

	if (mButtonNew)
	{
		mButtonNew->setVisible(true);
	}
	if (mButtonDelete)
	{
		mButtonDelete->setVisible(true);
	}
	if (mLoadingText)
	{
		mLoadingText->setVisible(false);
	}
}

// Create a new pick panel. It will automatically handle generating its own id
// when it is time to save.
//static
void LLPanelAvatarPicks::onClickNew(void* datap)
{
	LLPanelAvatarPicks* self = (LLPanelAvatarPicks*)datap;
	if (!self) return;

//MK
	if (gRLenabled && gRLInterface.mContainsShowloc)
	{
		return;
	}
//mk

	LLPanelPick* panelp = new LLPanelPick(false);
	panelp->initNewPick();

	if (self->mPicksTab)
	{
		self->mPicksTab->addTabPanel(panelp, panelp->getPickName());
		self->mPicksTab->selectLastTab();
	}
}

//static
void LLPanelAvatarPicks::onClickDelete(void* datap)
{
	LLPanelAvatarPicks* self = (LLPanelAvatarPicks*)datap;
	if (!self) return;

	LLPanelPick* panelp = NULL;
	if (self->mPicksTab)
	{
		panelp = (LLPanelPick*)self->mPicksTab->getCurrentPanel();
	}
	if (!panelp) return;

	LLSD args;
	args["PICK"] = panelp->getPickName();

	gNotifications.add("DeleteAvatarPick", args, LLSD(),
					   std::bind(&LLPanelAvatarPicks::callbackDelete, self, _1,
								 _2));
}

//static
bool LLPanelAvatarPicks::callbackDelete(const LLSD& notification,
										const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) != 0)
	{
		return false;
	}

	LLPanelPick* panelp = NULL;
	if (mPicksTab)
	{
		panelp = (LLPanelPick*)mPicksTab->getCurrentPanel();
	}
	if (!panelp)
	{
		return false;
	}

	LLAvatarProperties::sendPickDelete(panelp->getPickCreatorID(),
									   panelp->getPickID());

	if (mPicksTab)
	{
		mPicksTab->removeTabPanel(panelp);
	}
	delete panelp;

	return false;
}

//-----------------------------------------------------------------------------
// LLPanelAvatar class
//-----------------------------------------------------------------------------

LLPanelAvatar::LLPanelAvatar(const std::string& name, const LLRect& rect,
							 bool allow_edit)
:	LLPanel(name, rect, false),
	LLAvatarPropertiesObserver(LLUUID::null, APT_NONE),
	mDropTarget(NULL),
	mHaveProperties(false),
	mHaveInterests(false),
	mHaveNotes(false),
	mAllowEdit(allow_edit)
{
	LLCallbackMap::map_t factory_map;

	factory_map["2nd Life"] = LLCallbackMap(createPanelAvatarSecondLife, this);
	factory_map["WebProfile"] = LLCallbackMap(createPanelAvatarWeb, this);
	factory_map["Interests"] = LLCallbackMap(createPanelAvatarInterests, this);
	factory_map["Picks"] = LLCallbackMap(createPanelAvatarPicks, this);
	factory_map["Classified"] = LLCallbackMap(createPanelAvatarClassified, this);
	factory_map["1st Life"] = LLCallbackMap(createPanelAvatarFirstLife, this);
	factory_map["My Notes"] = LLCallbackMap(createPanelAvatarNotes, this);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar.xml",
											   &factory_map);

	selectTab(0);
}

bool LLPanelAvatar::postBuild()
{
	if (sLoading.empty())
	{
		sLoading = getString("loading");
		sClickToEnlarge = getString("click_to_enlarge");
		sShowOnMapNonFriend = getString("ShowOnMapNonFriend");
		sShowOnMapFriendOffline = getString("ShowOnMapFriendOffline");
		sShowOnMapFriendOnline = getString("ShowOnMapFriendOnline");
		sTeleportGod = getString("TeleportGod");
		sTeleportPrelude = getString("TeleportPrelude");
		sTeleportNormal = getString("TeleportNormal");
	}

	mTab = getChild<LLTabContainer>("tab");

	mOKButton = getChild<LLButton>("OK");
	mOKButton->setClickedCallback(onClickOK, this);

	mCancelButton = getChild<LLButton>("Cancel");
	mCancelButton->setClickedCallback(onClickCancel, this);

	mKickButton = getChild<LLButton>("kick_btn");
	mKickButton->setClickedCallback(onClickKick, this);
	mKickButton->setVisible(false);
	mKickButton->setEnabled(false);

	mFreezeButton = getChild<LLButton>("freeze_btn");
	mFreezeButton->setClickedCallback(onClickFreeze, this);
	mFreezeButton->setVisible(false);
	mFreezeButton->setEnabled(false);

	mUnfreezeButton = getChild<LLButton>("unfreeze_btn");
	mUnfreezeButton->setClickedCallback(onClickUnfreeze, this);
	mUnfreezeButton->setVisible(false);
	mUnfreezeButton->setEnabled(false);

	mCSRButton = getChild<LLButton>("csr_btn");
	mCSRButton->setClickedCallback(onClickCSR, this);
	mCSRButton->setVisible(false);
	mCSRButton->setEnabled(false);

	if (!sAllowFirstLife)
	{
		mTab->removeTabPanel(mPanelFirstLife);
		mTab->removeTabPanel(mPanelWeb);
	}

	return true;
}

LLPanelAvatar::~LLPanelAvatar()
{
	LLAvatarProperties::removeObserver(this);
}

bool LLPanelAvatar::canClose()
{
	return mPanelClassified->canClose();
}

void LLPanelAvatar::setOnlineStatus(EOnlineStatus online_status)
{
	bool online = online_status == ONLINE_STATUS_YES;
	// Online status NO could be because they are hidden. If they are a friend,
	// we may know the truth !
	if (!online && mIsFriend && gAvatarTracker.isBuddyOnline(mAvatarID))
	{
		online = true;
	}

	mPanelSecondLife->mOnlineText->setVisible(online);

	// Since setOnlineStatus gets called after setAvatarID, we need to make
	// sure that "Offer Teleport" does not get set to true again for yourself
	if (mAvatarID != gAgentID)
	{
		mPanelSecondLife->mOfferTPButton->setVisible(true);
		mPanelSecondLife->mRequestTPButton->setVisible(true);
	}

	if (gAgent.isGodlike())
	{
		mPanelSecondLife->mOfferTPButton->setEnabled(true);
		mPanelSecondLife->mOfferTPButton->setToolTip(sTeleportGod);
	}
	else if (gAgent.inPrelude())
	{
		mPanelSecondLife->mOfferTPButton->setEnabled(false);
		mPanelSecondLife->mOfferTPButton->setToolTip(sTeleportPrelude);
	}
	else
	{
		mPanelSecondLife->mOfferTPButton->setEnabled(true);
		mPanelSecondLife->mOfferTPButton->setToolTip(sTeleportNormal);
	}

	if (!mIsFriend)
	{
		mPanelSecondLife->mFindOnMapButton->setToolTip(sShowOnMapNonFriend);
	}
	else if (!online)
	{
		mPanelSecondLife->mFindOnMapButton->setToolTip(sShowOnMapFriendOffline);
	}
	else
	{
		mPanelSecondLife->mFindOnMapButton->setToolTip(sShowOnMapFriendOnline);
	}
}

void LLPanelAvatar::setAvatarID(const LLUUID& avatar_id,
								const std::string& name,
								EOnlineStatus online_status)
{
	if (avatar_id.isNull()) return;

	if (mAvatarID != avatar_id)
	{
		mAvatarID = avatar_id;
		// Clear out the old data
		mPanelSecondLife->clearControls();
		mPanelPicks->deletePickPanels();
		mPanelPicks->resetDataRequested();
		mPanelClassified->deleteClassifiedPanels();
		mPanelClassified->resetDataRequested();
		mHaveNotes = false;
		mLastNotes.clear();
		mPanelNotes->clearControls();
		mPanelNotes->resetDataRequested();
	}

	// Add the observer for data coming from the server for this avatar
	setObservedAvatarId(avatar_id);
	setObservedUpdateType(APT_ALL);
	LLAvatarProperties::addObserver(this);

	// Determine if they are a friend
	mIsFriend = LLAvatarTracker::isAgentFriend(mAvatarID);

	// setOnlineStatus() uses mIsFriend
	setOnlineStatus(online_status);

	bool own_avatar = mAvatarID == gAgentID;

	mPanelSecondLife->enableControls(own_avatar && mAllowEdit);
	mPanelAdvanced->enableControls(own_avatar && mAllowEdit);

	// Teens do not have this.
	if (sAllowFirstLife)
	{
		mPanelFirstLife->enableControls(own_avatar && mAllowEdit);
		mPanelWeb->enableControls(own_avatar && mAllowEdit);
	}

	LLView* targetp = getChild<LLView>("drop_target_rect");
	if (targetp)
	{
		if (mDropTarget)
		{
			delete mDropTarget;
		}
		mDropTarget = new LLDropTarget("drop target", targetp->getRect(),
									   mAvatarID);
		addChild(mDropTarget);
		mDropTarget->setAgentID(mAvatarID);
	}

	std::string avname = name;
	if (name.empty())
	{
		mPanelSecondLife->mLegacyName->setNameID(avatar_id, false);
	}
	else
	{
		mPanelSecondLife->mLegacyName->setText(avname);
	}
	mPanelSecondLife->mLegacyName->setVisible(true);
	if (LLAvatarNameCache::useDisplayNames())
	{
		LLAvatarName avatar_name;
		if (LLAvatarNameCache::get(avatar_id, &avatar_name))
		{
			// Always show "Display Name [Legacy Name]" for security reasons
			avname = avatar_name.getNames();
			mAvatarUserName = avatar_name.mUserName;
		}
		else
		{
			avname = mPanelSecondLife->mLegacyName->getText();
			LLAvatarNameCache::get(avatar_id,
								   std::bind(&LLPanelAvatar::completeNameCallback,
											 _1, _2, getHandle()));
		}
		mPanelSecondLife->mCompleteName->setText(avname);
		mPanelSecondLife->mCompleteName->setVisible(true);
		mPanelSecondLife->mLegacyName->setVisible(false);
	}
	else
	{
		mPanelSecondLife->mCompleteName->setVisible(false);
	}
	// We cannot set a tooltip on a text input box ("name" or "complete_name"),
	// so we set it on the profile picture... This can be helpful with very
	// long names (which would *appear* truncated in the textbox; even if it's
	// still possible to pan through the name with the mouse).
	std::string tooltip = avname;
	if (!own_avatar)
	{
		tooltip += "\n" + sClickToEnlarge;
	}
	mPanelSecondLife->m2ndLifePicture->setToolTip(tooltip);

	// Send a properties request for the new avatar
	LLAvatarProperties::sendGenericRequest(mAvatarID, APT_AVATAR_INFO);

	bool is_god = gAgent.isGodlike();

	if (own_avatar)
	{
		if (mAllowEdit)
		{
			// OK button disabled until properties data arrives
			mOKButton->setVisible(true);
			mOKButton->setEnabled(false);
			mCancelButton->setVisible(true);
			mCancelButton->setEnabled(true);
		}
		else
		{
			mOKButton->setVisible(false);
			mOKButton->setEnabled(false);
			mCancelButton->setVisible(false);
			mCancelButton->setEnabled(false);
		}
		mPanelSecondLife->mFindOnMapButton->setVisible(false);
		mPanelSecondLife->mFindOnMapButton->setEnabled(false);
		mPanelSecondLife->mOfferTPButton->setVisible(false);
		mPanelSecondLife->mOfferTPButton->setEnabled(false);
		mPanelSecondLife->mRequestTPButton->setVisible(false);
		mPanelSecondLife->mRequestTPButton->setEnabled(false);
		mPanelSecondLife->mAddFriendButton->setVisible(false);
		mPanelSecondLife->mAddFriendButton->setEnabled(false);
		mPanelSecondLife->mPayButton->setVisible(false);
		mPanelSecondLife->mPayButton->setEnabled(false);
		mPanelSecondLife->mIMButton->setVisible(false);
		mPanelSecondLife->mIMButton->setEnabled(false);
		mPanelSecondLife->mMuteButton->setVisible(false);
		mPanelSecondLife->mMuteButton->setEnabled(false);
		if (mDropTarget)
		{
			mDropTarget->setVisible(false);
			mDropTarget->setEnabled(false);
		}
	}
	else
	{
		mOKButton->setVisible(false);
		mOKButton->setEnabled(false);

		mCancelButton->setVisible(false);
		mCancelButton->setEnabled(false);

		mPanelSecondLife->mFindOnMapButton->setVisible(true);
		// Note: we do not always know online status, so always allow gods to
		// try to track
		bool can_map = LLAvatarTracker::isAgentMappable(mAvatarID);
		mPanelSecondLife->mFindOnMapButton->setEnabled(can_map || is_god);
		if (!mIsFriend)
		{
			mPanelSecondLife->mFindOnMapButton->setToolTip(sShowOnMapNonFriend);
		}
		else if (ONLINE_STATUS_YES != online_status)
		{
			mPanelSecondLife->mFindOnMapButton->setToolTip(sShowOnMapFriendOffline);
		}
		else
		{
			mPanelSecondLife->mFindOnMapButton->setToolTip(sShowOnMapFriendOnline);
		}

		mPanelSecondLife->mAddFriendButton->setVisible(true);
		mPanelSecondLife->mAddFriendButton->setEnabled(!mIsFriend);

		mPanelSecondLife->mPayButton->setVisible(true);
		mPanelSecondLife->mPayButton->setEnabled(false);
		mPanelSecondLife->mIMButton->setVisible(true);
		mPanelSecondLife->mIMButton->setEnabled(false);
		mPanelSecondLife->mMuteButton->setVisible(true);
		mPanelSecondLife->mMuteButton->setEnabled(false);
		if (mDropTarget)
		{
			mDropTarget->setVisible(true);
			mDropTarget->setEnabled(false);
		}
	}

	childSetText("avatar_key", mAvatarID.asString());

	mKickButton->setVisible(is_god);
	mKickButton->setEnabled(is_god);
	mFreezeButton->setVisible(is_god);
	mFreezeButton->setEnabled(is_god);
	mUnfreezeButton->setVisible(is_god);
	mUnfreezeButton->setEnabled(is_god);
	mCSRButton->setVisible(is_god);
	mCSRButton->setEnabled(is_god && gIsInSecondLife);
}

//static
void LLPanelAvatar::completeNameCallback(const LLUUID& agent_id,
										 const LLAvatarName& avatar_name,
										 LLHandle<LLPanel> handle)
{
	// Check that the panel was not closed and we are still using display names
	if (handle.isDead() || !LLAvatarNameCache::useDisplayNames())
	{
		return;
	}
	LLPanelAvatar* self = (LLPanelAvatar*)handle.get();

	// Check that the profile still refers to the same avatar...
	if (self->mAvatarID != agent_id)
	{
		return;
	}

	self->mAvatarUserName = avatar_name.mUserName;
	// Always show "Display Name [Legacy Name]" for security reasons
	std::string avname = avatar_name.getNames();
	self->mPanelSecondLife->mCompleteName->setText(avname);

	std::string tooltip = avname;
	if (agent_id != gAgentID)
	{
		tooltip += "\n" + sClickToEnlarge;
	}
	self->mPanelSecondLife->m2ndLifePicture->setToolTip(tooltip);
}

void LLPanelAvatar::listAgentGroups()
{
	// Only get these updates asynchronously via the group floater, which works
	// on the agent only
	if (mAvatarID != gAgentID)
	{
		return;
	}

	LLScrollListCtrl* group_list = mPanelSecondLife->mGroupsListCtrl;
	group_list->deleteAllItems();

	std::string hidden_group = getString("hidden_group");
	for (S32 i = 0, count = gAgent.mGroups.size(); i < count; ++i)
	{
		const LLGroupData& group_data = gAgent.mGroups[i];

		LLSD row;
		row["id"] = group_data.mID;
		LLSD& col = row["columns"][0];
		col["value"] = group_data.mName;
		col["font"] = "SANSSERIF_SMALL";
		bool hidden = !group_data.mListInProfile;
		if (hidden)
		{
			col["font-style"] = "ITALIC";
		}

		LLScrollListItem* itemp = group_list->addElement(row);
		if (hidden && itemp)
		{
			itemp->setToolTip(hidden_group);
		}
	}
	group_list->sortByColumnIndex(0, true);
}

//virtual
void LLPanelAvatar::processProperties(S32 type, void* userdatap)
{
	if (type == APT_AVATAR_INFO)
	{
		LLAvatarInfo* datap = (LLAvatarInfo*)userdatap;
		if (datap->mAvatarId != mAvatarID)	// This should not happen...
		{
			return;
		}

		mPanelSecondLife->mIMButton->setEnabled(true);
		mPanelSecondLife->mPayButton->setEnabled(true);
		mPanelSecondLife->mMuteButton->setEnabled(true);
		if (mDropTarget)
		{
			mDropTarget->setEnabled(true);
		}

		std::string& caption_text = datap->mCaptionText;
		if (caption_text.empty())
		{
			LLStringUtil::format_map_t args;
			caption_text = mPanelSecondLife->getString("CaptionTextAcctInfo");

			static const char* ACCT_TYPE[] =
			{
				"AcctTypeResident",
				"AcctTypeTrial",
				"AcctTypeCharterMember",
				"AcctTypeEmployee"
			};
			constexpr S32 ACCT_TYPE_SIZE = LL_ARRAY_SIZE(ACCT_TYPE);
			U8 caption_index = llmin(datap->mCaptionIndex,
									 (U8)(ACCT_TYPE_SIZE - 1));
			args["[ACCTTYPE]"] =
				mPanelSecondLife->getString(ACCT_TYPE[caption_index]);

			std::string payment_text = " ";
			constexpr S32 DEFAULT_CAPTION_LINDEN_INDEX = 3;
			if (caption_index != DEFAULT_CAPTION_LINDEN_INDEX)
			{
				if (datap->mFlags & AVATAR_TRANSACTED)
				{
					payment_text = "PaymentInfoUsed";
				}
				else if (datap->mFlags & AVATAR_IDENTIFIED)
				{
					payment_text = "PaymentInfoOnFile";
				}
				else
				{
					payment_text = "NoPaymentInfoOnFile";
				}
				args["[PAYMENTINFO]"] =
					mPanelSecondLife->getString(payment_text);
			}
			else
			{
				args["[PAYMENTINFO]"] = " ";
			}
			LLStringUtil::format(caption_text, args);
		}

		mPanelSecondLife->mAccountInfoText->setValue(caption_text);
		mPanelSecondLife->mBornText->setValue(datap->mBirthDate);

		setOnlineStatus(datap->mFlags & AVATAR_ONLINE ? ONLINE_STATUS_YES
													  : ONLINE_STATUS_NO);

		mPanelSecondLife->m2ndLifePicture->setImageAssetID(datap->mImageId);
		mPanelSecondLife->setPartnerID(datap->mPartnerId);
		mPanelSecondLife->updatePartnerName();

		// Do not overwrite the About texts when we received them already via
		// the capability, since the capability transmits way longer texts...
		if (datap->mReceivedViaCap || !mHaveProperties)
		{
			LLTextEditor* editp = mPanelSecondLife->mAbout2ndLifeText;
			editp->clear();
			editp->setParseHTML(true);
			if (mAvatarID == gAgentID)
			{
				editp->setText(datap->mAbout);
			}
			else
			{
				editp->appendColoredText(datap->mAbout, false, false,
										 editp->getReadOnlyFgColor());
				// Support for SLURLs substitutions. HB
				LLSLURL::findSLURLs(editp);
			}
			if (sAllowFirstLife)	// Teens do not get this
			{
				editp = mPanelFirstLife->mAbout1stLifeText;
				editp->clear();
				editp->setParseHTML(true);
				if (mAvatarID == gAgentID)
				{
					editp->setText(datap->mFLAbout);
				}
				else
				{
					editp->appendColoredText(datap->mFLAbout, false, false,
											 editp->getReadOnlyFgColor());
					// Support for SLURLs substitutions. HB
					LLSLURL::findSLURLs(editp);
				}
			}
		}

		if (sAllowFirstLife)	// Teens do not get this
		{
			// The capability does not provide the Web URL... HB
			if (!datap->mReceivedViaCap)
			{
				mPanelWeb->setWebURL(datap->mProfileUrl);
			}

			LLTextureCtrl* imgctrlp = mPanelFirstLife->m1stLifePicture;
			imgctrlp->setImageAssetID(datap->mFLImageId);
			if (mAvatarID == gAgentID || datap->mFLImageId.isNull())
			{
				imgctrlp->setToolTip("");
			}
			else
			{
				imgctrlp->setToolTip(sClickToEnlarge);
			}
		}

		bool allow_publish = (datap->mFlags & AVATAR_ALLOW_PUBLISH) != 0;
		mPanelSecondLife->mShowInSearchCheck->setValue(allow_publish);

		mHaveProperties = true;
		enableOKIfReady();

		// If we do not have the interests, we need to do an UDP request...
		// Sadly, it is not possible to ask only for interests data via an UDP
		// message, so we must re-request the whole shebang (main properties)
		// to get them (as a different UDP reply message). You could ask: "but
		// why bothering at all with the capability, then ?", and the answer is
		// that the latter is able to cope with About texts of more than 500
		// characters, which the UDP messages cannot cope with... HB
		if (datap->mReceivedViaCap && !mHaveInterests)
		{
			LLAvatarProperties::sendAvatarPropertiesRequest(mAvatarID);
		}
	}
	else if (type == APT_GROUPS)
	{
		LLAvatarGroups* groups = (LLAvatarGroups*)userdatap;
		if (groups->mAvatarId != mAvatarID)		// This should always be true
		{
			return;
		}
		if (mAvatarID == gAgentID && !gAgent.mGroups.empty())
		{
			// Use our agent's group list info instead; this also takes care of
			// displaying hidden groups in italics and with an explanatory tool
			// tip... HB
			listAgentGroups();
			return;
		}
		LLScrollListCtrl* group_list = mPanelSecondLife->mGroupsListCtrl;
		if (groups->mGroups.empty())
		{
			group_list->deleteAllItems();
			// *TODO: Translate
			group_list->addCommentText("None");
			return;
		}
#if 0	// Could cause missing groups in the list (see below), especially with
		// the new, larger groups number limits. HB
		group_list->deleteAllItems();
#endif
		for (LLAvatarGroups::list_t::iterator it = groups->mGroups.begin(),
											  end = groups->mGroups.end();
			 it != end; ++it)
		{
			const LLGroupData& data = *it;
			const LLUUID& group_id = data.mID;
#if 1		// An alternative to deleteAllItems() above, in case we would get
			// several processAvatarGroupsReply() for the same avatar (i.e. if
			// the groups list cannot be transmitted as a single message for
			// being too big)...
			// Remove any existing entry.
			S32 index = group_list->getItemIndex(group_id);
			if (index >= 0)
			{
				group_list->deleteSingleItem(index);
			}
#endif
			LLSD row;
			row["id"] = group_id;
			LLSD& col = row["columns"][0];
			col["value"] = data.mName;
			col["font"] = "SANSSERIF_SMALL";
			group_list->addElement(row);
		}
		group_list->sortByColumnIndex(0, true);
	}
	else if (type == APT_INTERESTS)
	{
		LLAvatarInterests* datap = (LLAvatarInterests*)userdatap;
		if (datap->mAvatarId != mAvatarID)	// This should always be true
		{
			return;
		}
		mHaveInterests = true;
		mPanelAdvanced->setWantSkills(datap->mWantsMask, datap->mWantsText,
									  datap->mSkillsMask, datap->mSkillsText,
									  datap->mLanguages);
	}
	else if (type == APT_PICKS)
	{
		LLAvatarPicks* datap = (LLAvatarPicks*)userdatap;
		if (datap->mAvatarId == mAvatarID)	// This should always be true
		{
			mPanelPicks->processAvatarPicksReply(datap);
		}
	}
	else if (type == APT_CLASSIFIEDS)
	{
		LLAvatarClassifieds* datap = (LLAvatarClassifieds*)userdatap;
		if (datap->mAvatarId == mAvatarID)	// This should always be true
		{
			mPanelClassified->processAvatarClassifiedReply(datap);
		}
	}
	else if (type == APT_NOTES)
	{
		LLAvatarNotes* datap = (LLAvatarNotes*)userdatap;
		if (datap->mAvatarId == mAvatarID)	// This should always be true
		{
			mHaveNotes = true;
			mLastNotes = datap->mNotes;
			mPanelNotes->mNotesText->setText(mLastNotes);
			mPanelNotes->mNotesText->setEnabled(true);
		}
	}
}

//static
void LLPanelAvatar::onClickIM(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		LLAvatarActions::startIM(self->mAvatarID);
	}
}

//static
void LLPanelAvatar::onClickTrack(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (gFloaterWorldMapp && self)
	{
		std::string name = self->mPanelSecondLife->mLegacyName->getText();
		gFloaterWorldMapp->trackAvatar(self->mAvatarID, name);
		LLFloaterWorldMap::show(NULL, true);
	}
}

//static
void LLPanelAvatar::onClickAddFriend(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		std::string name = self->mPanelSecondLife->mLegacyName->getText();
		LLAvatarActions::requestFriendshipDialog(self->getAvatarID(), name);
	}
}

void LLPanelAvatar::onClickMute(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (!self) return;

	LLUUID avatar_id = self->getAvatarID();
	std::string avatar_name = self->mPanelSecondLife->mLegacyName->getText();
	if (LLMuteList::isMuted(avatar_id))
	{
		LLFloaterMute::selectMute(avatar_id);
	}
	else
	{
		LLMute mute(avatar_id, avatar_name, LLMute::AGENT);
		if (LLMuteList::add(mute))
		{
			LLFloaterMute::selectMute(mute.mID);
		}
	}
}

//static
void LLPanelAvatar::onClickOfferTeleport(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		LLAvatarActions::offerTeleport(self->mAvatarID);
	}
}

//static
void LLPanelAvatar::onClickRequestTeleport(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		LLAvatarActions::teleportRequest(self->mAvatarID);
	}
}

//static
void LLPanelAvatar::onClickPay(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		LLAvatarActions::pay(self->mAvatarID);
	}
}

//static
void LLPanelAvatar::onClickOK(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;

	// JC: Only save the data if we actually got the original properties.
	// Otherwise we might save blanks into the database.
	if (self && self->mHaveProperties)
	{
		self->sendAvatarPropertiesUpdate();

		LLTabContainer* tabs = self->getChild<LLTabContainer>("tab");
		if (tabs->getCurrentPanel() != self->mPanelClassified)
		{
			self->mPanelClassified->apply();

			LLFloaterAvatarInfo* infop =
				LLFloaterAvatarInfo::getInstance(self->mAvatarID);
			if (infop)
			{
				infop->close();
			}
		}
		else if (self->mPanelClassified->titleIsValid())
		{
			self->mPanelClassified->apply();

			LLFloaterAvatarInfo* infop =
				LLFloaterAvatarInfo::getInstance(self->mAvatarID);
			if (infop)
			{
				infop->close();
			}
		}
	}
}

//static
void LLPanelAvatar::onClickCancel(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (!self)
	{
		return;
	}

	LLFloaterAvatarInfo* infop;
	if ((infop = LLFloaterAvatarInfo::getInstance(self->mAvatarID)))
	{
		infop->close();
		return;
	}

	// We are in the Search directory and are cancelling an edit to our own
	// profile, so reset.
	LLAvatarProperties::sendGenericRequest(self->mAvatarID, APT_AVATAR_INFO);
}

void LLPanelAvatar::sendAvatarNotesUpdate()
{
	const std::string& notes = mPanelNotes->mNotesText->getText();
	if (!mHaveNotes || notes == mLastNotes || notes == sLoading)
	{
		// No note from server and no user updates
		return;
	}

	LLAvatarProperties::sendAvatarNotesUpdate(mAvatarID, notes);
}

// Do not enable the OK button until you actually have the data. Otherwise you
// will write blanks back into the database.
void LLPanelAvatar::enableOKIfReady()
{
	mOKButton->setEnabled(mHaveProperties && mOKButton->getVisible());
}

void LLPanelAvatar::sendAvatarPropertiesUpdate()
{
	LLAvatarInfo avdata;

	avdata.mAbout =
		mPanelSecondLife->mAbout2ndLifeText->getValue().asString();

	avdata.mImageId = mPanelSecondLife->m2ndLifePicture->getImageAssetID();

	if (sAllowFirstLife)
	{
		avdata.mAllowPublish =
			mPanelSecondLife->mShowInSearchCheck->getValue();

		avdata.mProfileUrl = mPanelWeb->getWebURL();

		avdata.mFLAbout =
			mPanelFirstLife->mAbout1stLifeText->getValue().asString();

		avdata.mFLImageId =
			mPanelFirstLife->m1stLifePicture->getImageAssetID();
	}
	else
	{
		avdata.mAllowPublish = false;
	}

	LLAvatarProperties::sendAvatarPropertiesUpdate(avdata);

	LLAvatarInterests interests;
	mPanelAdvanced->getWantSkills(interests.mWantsMask, interests.mWantsText,
								  interests.mSkillsMask,
								  interests.mSkillsText, interests.mLanguages);
	LLAvatarProperties::sendInterestsInfoUpdate(interests);
}

void LLPanelAvatar::selectTab(S32 tabnum)
{
	mTab->selectTab(tabnum);
}

void LLPanelAvatar::selectTabByName(std::string tab_name)
{
	if (tab_name.empty())
	{
		mTab->selectFirstTab();
	}
	else
	{
		mTab->selectTabByName(tab_name);
	}
}

//static
void LLPanelAvatar::onClickKick(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		LLAvatarActions::kick(self->mAvatarID);
	}
}

//static
void LLPanelAvatar::onClickFreeze(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		LLAvatarActions::freeze(self->mAvatarID, true);
	}
}

//static
void LLPanelAvatar::onClickUnfreeze(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (self)
	{
		LLAvatarActions::freeze(self->mAvatarID, false);
	}
}

//static
void LLPanelAvatar::onClickCSR(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	if (!self || !self->mPanelSecondLife) return;

	std::string name = self->mPanelSecondLife->mLegacyName->getText();
	if (name.empty()) return;

	std::string url = "https://csr.lindenlab.com/agent/";

	// Slow and stupid, but it is late
	S32 len = name.length();
	for (S32 i = 0; i < len; ++i)
	{
		if (name[i] == ' ')
		{
			url += "%20";
		}
		else
		{
			url += name[i];
		}
	}

	LLWeb::loadURL(url);
}

void* LLPanelAvatar::createPanelAvatarSecondLife(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	self->mPanelSecondLife = new LLPanelAvatarSecondLife("2nd Life", LLRect(),
														 self);
	return self->mPanelSecondLife;
}

void* LLPanelAvatar::createPanelAvatarWeb(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	self->mPanelWeb = new LLPanelAvatarWeb("Web", LLRect(), self);
	return self->mPanelWeb;
}

void* LLPanelAvatar::createPanelAvatarInterests(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	self->mPanelAdvanced = new LLPanelAvatarAdvanced("Interests", LLRect(),
													 self);
	return self->mPanelAdvanced;
}

void* LLPanelAvatar::createPanelAvatarPicks(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	self->mPanelPicks = new LLPanelAvatarPicks("Picks", LLRect(), self);
	return self->mPanelPicks;
}

void* LLPanelAvatar::createPanelAvatarClassified(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	self->mPanelClassified = new LLPanelAvatarClassified("Classified",
														 LLRect(), self);
	return self->mPanelClassified;
}

void* LLPanelAvatar::createPanelAvatarFirstLife(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	self->mPanelFirstLife = new LLPanelAvatarFirstLife("1st Life", LLRect(),
													   self);
	return self->mPanelFirstLife;
}

void* LLPanelAvatar::createPanelAvatarNotes(void* datap)
{
	LLPanelAvatar* self = (LLPanelAvatar*)datap;
	self->mPanelNotes = new LLPanelAvatarNotes("My Notes", LLRect(), self);
	return self->mPanelNotes;
}
