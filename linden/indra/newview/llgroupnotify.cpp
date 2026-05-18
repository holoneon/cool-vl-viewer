/**
 * @file llgroupnotify.cpp
 * @brief Non-blocking notification that doesn't take keyboard focus.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llgroupnotify.h"

#include "llbutton.h"
#include "lliconctrl.h"
#include "lltextbox.h"

#include "llagent.h"
#include "llfloatergroupinfo.h"
#include "llinventory.h"
#include "llinventoryicon.h"		// For getIcon()
#include "llnotify.h"
#include "llslurl.h"
#include "hbviewerautomation.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"		// For LLOfferInfo and formatted_time()
#include "llviewertexteditor.h"

constexpr F32 ANIMATION_TIME = 0.333f;

S32 LLGroupNotifyBox::sGroupNotifyBoxCount = 0;

bool is_openable(LLAssetType::EType type)
{
	switch (type)
	{
		case LLAssetType::AT_LANDMARK:
		case LLAssetType::AT_NOTECARD:
		case LLAssetType::AT_IMAGE_JPEG:
		case LLAssetType::AT_IMAGE_TGA:
		case LLAssetType::AT_TEXTURE:
		case LLAssetType::AT_TEXTURE_TGA:
			return gSavedSettings.getBool("ShowNewInventory");

		default:
			return false;
	}
}

class LLNoticeText : public LLTextBox
{
public:
	LLNoticeText(const std::string& name, const LLRect& rect,
			   const std::string& text = LLStringUtil::null,
			   const LLFontGL* font = NULL)
	:	LLTextBox(name, rect, text, font)
	{
		setHAlign(LLFontGL::RIGHT);
		setFontStyle(LLFontGL::DROP_SHADOW_SOFT);
		setBorderVisible(false);
		setColor(gColors.getColor("GroupNotifyTextColor"));
		setBackgroundColor(gColors.getColor("GroupNotifyBoxColor"));
	}
};

LLGroupNotifyBox::LLGroupNotifyBox(LLNotificationPtr notifp,
								   const std::string& subject,
								   const std::string& message,
								   const std::string& from_name,
								   const LLUUID& group_id,
								   const LLUUID& group_insignia,
								   const std::string& group_name,
								   time_t timestamp, bool has_inventory,
								   const std::string& inventory_name,
								   const LLSD& inventory_offer)
:	LLPanel("groupnotify", LLGroupNotifyBox::getGroupNotifyRect(), BORDER_YES),
	LLInstanceTracker<LLGroupNotifyBox, LLUUID>(notifp->getID()),
	mAnimating(true),
	mGroupID(group_id),
	mHasInventory(has_inventory),
	mInventoryOffer(NULL)
{
	constexpr S32 LABEL_WIDTH = 64;
	constexpr S32 ICON_WIDTH = 64;
	constexpr S32 VPAD = 2;
	constexpr S32 BOTTOM_PAD = VPAD * 2;
	constexpr S32 LINE_HEIGHT = 16;
	const S32 top = getRect().getHeight() - 32; // Get past the top menu bar
	const S32 right = getRect().getWidth() - HPAD - HPAD;
	const S32 btn_top = BOTTOM_PAD + gBtnHeight + VPAD;

	LLFontGL* fontp = LLFontGL::getFontSansSerif();

	mAnimating = sGroupNotifyBoxCount <= 0 &&
				 LLNotifyBox::getNotifyBoxCount() <= 0;

	if (mHasInventory)
	{
		mInventoryOffer = new LLOfferInfo(inventory_offer);
	}

	setFocusRoot(true);
	setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
	setBackgroundVisible(true);
	setBackgroundOpaque(true);
	setBackgroundColor(gColors.getColor("GroupNotifyBoxColor"));

	S32 y = top;
	S32 x = 2 * HPAD;
	// Title. *TODO: translate
	addChild(new LLNoticeText("title",
							  LLRect(x, y, right - HPAD, y - LINE_HEIGHT),
							  "Group notice",
							  LLFontGL::getFontSansSerifHuge()));

	y -= llfloor(1.5f * LINE_HEIGHT);
	x += 2 * HPAD + ICON_WIDTH;

	std::stringstream from;
	from << "Sent by " << from_name << ", " << group_name;

	addChild(new LLNoticeText("group",
							  LLRect(x, y, right - HPAD, y - LINE_HEIGHT),
							  from.str(), fontp));

	y -= LINE_HEIGHT + VPAD;
	x = 2 * HPAD;

	// *TODO: change this to be the group icon.
	LLIconCtrl* iconp;
	if (group_insignia.notNull())
	{
		iconp = new LLIconCtrl("icon",
							   LLRect(x, y, x + ICON_WIDTH, y - ICON_WIDTH),
							   group_insignia);
	}
	else
	{
		iconp = new LLIconCtrl("icon",
							   LLRect(x, y, x + ICON_WIDTH, y - ICON_WIDTH),
							   "notify_box_icon.tga");
	}

	iconp->setMouseOpaque(false);
	addChild(iconp);

	x += 2 * HPAD + ICON_WIDTH;
	// If we have inventory with this message, leave room for the name.
	S32 box_bottom = btn_top + (mHasInventory ? LINE_HEIGHT + 2 * VPAD : 0);

	LLViewerTextEditor* vtextp =
		new LLViewerTextEditor("box", LLRect(x, y, right, box_bottom),
							   DB_GROUP_NOTICE_MSG_STR_LEN,
							   LLStringUtil::null, fontp, false);

	static const LLStyleSP headerstyle(new LLStyle(true, LLColor4::black,
												   "SansSerifBig"));
	static const LLStyleSP datestyle(new LLStyle(true, LLColor4::black,
												 "serif"));

	vtextp->appendStyledText(subject + "\n", false, false, headerstyle);

	vtextp->appendStyledText(formatted_time(timestamp), false, false,
							 datestyle);
	// Sadly, our LLTextEditor cannot handle both styled and unstyled text
	// at the same time. Hence this space must be styled. JC
	vtextp->appendColoredText(" ", false, false, LLColor4::grey4);
	vtextp->setParseHTML(true);
	vtextp->appendColoredText("\n\n" + message, false, false,
							 LLUI::sTextDefaultColor);

	LLColor4 bg_color = LLColor4(gColors.getColor("GroupNotifyTextBgColor"));
	vtextp->setCursor(0,0);
	vtextp->setEnabled(false);
	vtextp->setWordWrap(true);
	//vtextp->setTabStop(false);		// Was interfering with copy-and-paste
	vtextp->setTabsToNextField(true);
	vtextp->setMouseOpaque(true);
	vtextp->setBorderVisible(true);
	vtextp->setHideScrollbarForShortDocs(true);
	vtextp->setReadOnlyBgColor(bg_color);
	vtextp->setWriteableBgColor(bg_color);
	addChild(vtextp);
	// Support for SLURLs substitutions. HB
	LLSLURL::findSLURLs(vtextp);

	y = box_bottom - VPAD;

	if (mHasInventory)
	{
		// *TODO: translate
		addChild(new LLNoticeText("subjecttitle",
								  LLRect(x, y, x + LABEL_WIDTH,
										 y - LINE_HEIGHT),
								  "Attached: ", fontp));

		LLUIImagePtr itemiconp =
			LLInventoryIcon::getIcon(mInventoryOffer->mType,
									 LLInventoryType::IT_TEXTURE, 0,
									 false);
		x += LABEL_WIDTH + HPAD;

		std::stringstream ss;
		ss << "        " << inventory_name;
		LLTextBox* textp = new LLTextBox("object_name",
										 LLRect(x, y, right - HPAD,
												y - LINE_HEIGHT),
										 ss.str(), fontp);
		textp->setEnabled(false);
		textp->setBorderVisible(true);
		textp->setDisabledColor(LLColor4::blue4);
		textp->setFontStyle(LLFontGL::NORMAL);
		textp->setBackgroundVisible(true);
		textp->setBackgroundColor(bg_color);
		addChild(textp);

		iconp = new LLIconCtrl("icon", LLRect(x, y, x + 16, y - 16),
							   itemiconp->getName());
		iconp->setMouseOpaque(false);
		addChild(iconp);
	}

	LLRect btn_rect(getRect().getWidth() - 26, BOTTOM_PAD + 20,
					getRect().getWidth() - 2, BOTTOM_PAD);
	LLButton* btnp =
		new LLButton("next", btn_rect, "notify_next.png", "notify_next.png",
					 NULL, onClickNext, this, fontp);
	btnp->setToolTip("Next");	// *TODO: Translate
	btnp->setScaleImage(true);
	addChild(btnp);
	mNextBtn = btnp;

	S32 btn_width = 80;
	S32 wide_btn_width = 120;
	x = 3 * HPAD;
	btn_rect.setOriginAndSize(x, BOTTOM_PAD, btn_width, gBtnHeight);
	btnp = new LLButton("OK", btn_rect, NULL, onClickOk, this);
	addChild(btnp, -1);
	setDefaultBtn(btnp);

	x += btn_width + HPAD;

	btn_rect.setOriginAndSize(x, BOTTOM_PAD, wide_btn_width, gBtnHeight);

	// *TODO: Translate
	btnp = new LLButton("Group notices", btn_rect, NULL,
					   onClickGroupInfo, this);
	btnp->setToolTip("View past notices or opt-out of receiving these messages here.");
	addChild(btnp, -1);

	if (mHasInventory)
	{
		x += wide_btn_width + HPAD;

		btn_rect.setOriginAndSize(x, BOTTOM_PAD, wide_btn_width, gBtnHeight);

		std::string btn_lbl;
		if (is_openable(mInventoryOffer->mType))
		{
			btn_lbl = "Open attachment";
		}
		else
		{
			btn_lbl = "Save attachment";
		}
		mSaveInventoryBtn = new LLButton(btn_lbl, btn_rect, NULL,
										 onClickSaveInventory, this);
		mSaveInventoryBtn->setVisible(mHasInventory);
		addChild(mSaveInventoryBtn);
	}

	++sGroupNotifyBoxCount;
}

//virtual
LLGroupNotifyBox::~LLGroupNotifyBox()
{
	--sGroupNotifyBoxCount;
}

//virtual
bool LLGroupNotifyBox::handleRightMouseDown(S32, S32, MASK)
{
	moveToBack();
	return true;
}

//virtual
void LLGroupNotifyBox::draw()
{
	if (!LLNotifyBox::areNotificationsShown())
	{
		setVisible(false);
		return;
	}

	if (mNextBtn)
	{
		mNextBtn->setVisible(sGroupNotifyBoxCount > 1);
	}

	F32 display_time = mTimer.getElapsedTimeF32();

	if (mAnimating && display_time < ANIMATION_TIME)
	{
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		LLUI::pushMatrix();

		S32 height = getRect().getHeight();
		F32 fraction = display_time / ANIMATION_TIME;
		F32 voffset = (1.f - fraction) * height;

		LLUI::translate(0.f, voffset, 0.f);

		LLPanel::draw();

		LLUI::popMatrix();
	}
	else
	{
		mAnimating = false;
		LLPanel::draw();
	}
}

void LLGroupNotifyBox::close()
{
	// The group notice dialog may be an inventory offer. If it has an
	// inventory save button and that button is still enabled. Then we need to
	// send the inventory declined message.
	if (mHasInventory)
	{
		mInventoryOffer->forceResponse(IOR_DECLINE);
		mInventoryOffer = NULL;
		mHasInventory = false;
	}
	if (gNotifyBoxViewp)
	{
		gNotifyBoxViewp->removeChild(this);
	}

	die();
}

//static
void LLGroupNotifyBox::initClass()
{
	LLNotificationChannel::buildChannel("Group Notifications", "Visible",
										LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "groupnotify"));
	gNotifications.getChannel("Group Notifications")->connectChanged(&LLGroupNotifyBox::onNewNotification);
}

//static
bool LLGroupNotifyBox::onNewNotification(const LLSD& notify)
{
	LLNotificationPtr notifp = gNotifications.find(notify["id"].asUUID());
	if (!notifp)
	{
		return false;
	}

	if (notify["sigtype"].asString() == "delete")
	{
		LLGroupNotifyBox* self = getNamedInstance(notifp->getID()).get();
		if (self && !self->isDead())
		{
			self->close();
			return false;
		}
	}

	if (gNotifyBoxViewp)
	{
		const LLSD& payload = notifp->getPayload();
		// Get the group data
		LLGroupData group_data;
		if (!gAgent.getGroupData(payload["group_id"].asUUID(),group_data))
		{
			llwarns << "Group notice for unknown group: "
					<< payload["group_id"].asUUID() << llendl;
			return false;
		}

		LLUUID group_id = payload["group_id"].asUUID();
		std::string subject = payload["subject"].asString();
		std::string message = payload["message"].asString();
		std::string sender_name = payload["sender_name"].asString();
		std::string inventory_name;
		bool has_inventory = payload["inventory_offer"].isDefined();
		if (has_inventory)
		{
			inventory_name = payload["inventory_name"].asString();
		}
		const LLDate& date = notifp->getDate();
		time_t timestamp = date.secondsSinceEpoch();
		if (!timestamp)
		{
			time(&timestamp);
		}
		LLGroupNotifyBox* self =
			new LLGroupNotifyBox(notifp, subject, message, sender_name,
								 group_id, group_data.mInsigniaID,
								 group_data.mName, timestamp, has_inventory,
								 inventory_name,
								 has_inventory ? payload["inventory_offer"]
											   : LLSD());
		gNotifyBoxViewp->addChild(self);

		if (gAutomationp)
		{
			gAutomationp->onGroupNotification(group_data.mName, group_id,
											  notifp->getID(), timestamp,
											  sender_name, subject, message,
											  inventory_name);
		}
	}

	return false;
}

void LLGroupNotifyBox::moveToBack()
{
	if (!gNotifyBoxViewp) return;

	// Move this dialog to the back.
	gNotifyBoxViewp->removeChild(this);
	gNotifyBoxViewp->addChildAtEnd(this);
}

//static
LLRect LLGroupNotifyBox::getGroupNotifyRect()
{
	static LLCachedControl<S32> notify_height(gSavedSettings,
											  "GroupNotifyBoxHeight");
	S32 height =  notify_height;
	if (height < 150)
	{
		height = 150;
	}
	static LLCachedControl<S32> notify_width(gSavedSettings,
											 "GroupNotifyBoxWidth");
	S32 width = notify_width;
	if (width < 250)
	{
		width = 250;
	}
	S32 top, right;
	if (gNotifyBoxViewp)
	{
		top = gNotifyBoxViewp->getRect().getHeight();
		right = gNotifyBoxViewp->getRect().getWidth();
	}
	else
	{
		top = height;
		right = 0;
	}

	return LLRect(right - width, top, right, top - height);
}

//static
void LLGroupNotifyBox::onClickOk(void* datap)
{
	LLGroupNotifyBox* self = (LLGroupNotifyBox*)datap;
	if (self)
	{
		self->close();
	}
}

void LLGroupNotifyBox::onClickGroupInfo(void* datap)
{
	LLGroupNotifyBox* self = (LLGroupNotifyBox*)datap;
	if (self)
	{
		LLFloaterGroupInfo::showFromUUID(self->mGroupID, "notices_tab");
	}
	// Leave notice open until explicitly closed
}

void LLGroupNotifyBox::onClickSaveInventory(void* datap)
{
	LLGroupNotifyBox* self = (LLGroupNotifyBox*)datap;
	if (!self) return;

	self->mInventoryOffer->forceResponse(IOR_ACCEPT);
	self->mInventoryOffer = NULL;
	self->mHasInventory = false;

	// Each item can only be received once, so disable the button.
	self->mSaveInventoryBtn->setEnabled(false);
}

//static
void LLGroupNotifyBox::onClickNext(void* datap)
{
	LLGroupNotifyBox* self = (LLGroupNotifyBox*)datap;
	if (self)
	{
		self->moveToBack();
	}
}
