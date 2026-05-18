/**
 * @file llcombobox.cpp
 * @brief LLComboBox base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

// A control that displays the name of the chosen item, which when
// clicked shows a scrolling box of options.

#include "linden_common.h"

#include "llcombobox.h"

#include "llbutton.h"
#include "llcontrol.h"
#include "llfloater.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llscrollbar.h"
#include "llscrolllistctrl.h"
#include "llstring.h"
#include "llwindow.h"
#include "llvector2.h"

// Globals
constexpr S32 MAX_COMBO_WIDTH = 500;

static const std::string LL_COMBO_BOX_TAG = "combo_box";
static LLRegisterWidget<LLComboBox> r03(LL_COMBO_BOX_TAG);

LLComboBox::LLComboBox(const std::string& name,
					   const LLRect& rect,
					   const std::string& label,
					   void (*commit_callback)(LLUICtrl*, void*),
					   void* callback_userdata)
:	LLUICtrl(name, rect, true, commit_callback, callback_userdata,
			 FOLLOWS_LEFT | FOLLOWS_TOP),
	mTextEntry(NULL),
	mArrowImage(NULL),
	mAllowTextEntry(false),
	mMaxChars(20),
	mTextEntryTentative(true),
	mListPosition(BELOW),
	mPrearrangeCallback(NULL),
	mTextEntryCallback(NULL),
	mSuppressTentative(false),
	mLabel(label)
{
	// Text label button
	mButton = new LLButton(mLabel, LLRect(), NULL, NULL, this);
	mButton->setImageUnselected("square_btn_32x128.tga");
	mButton->setImageSelected("square_btn_selected_32x128.tga");
	mButton->setImageDisabled("square_btn_32x128.tga");
	mButton->setImageDisabledSelected("square_btn_selected_32x128.tga");
	mButton->setScaleImage(true);

	mButton->setMouseDownCallback(onButtonDown);
	mButton->setFont(LLFontGL::getFontSansSerifSmall());
	mButton->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mButton->setHAlign(LLFontGL::LEFT);
	mButton->setRightHPad(2);
	addChild(mButton);

	// Disallow multiple selection
	mList = new LLScrollListCtrl("ComboBox", LLRect(),
								 &LLComboBox::onItemSelected, this, false);
	mList->setVisible(false);
	mList->setBgWriteableColor(LLColor4(1, 1, 1, 1));
	mList->setCommitOnKeyboardMovement(false);
	addChild(mList);

	mArrowImage = LLUI::getUIImage("combobox_arrow.tga");
	mButton->setImageOverlay(mArrowImage, LLFontGL::RIGHT);

	updateLayout();
}

LLComboBox::~LLComboBox()
{
	// Children automatically deleted, including mMenu, mButton
}

//virtual
const std::string& LLComboBox::getTag() const
{
	return LL_COMBO_BOX_TAG;
}

//virtual
LLXMLNodePtr LLComboBox::getXML(bool save_children) const
{
	LLXMLNodePtr nodep = LLUICtrl::getXML();

	nodep->setName(LL_COMBO_BOX_TAG);

	// Attributes

	nodep->createChild("allow_text_entry",
					   true)->setBoolValue(mAllowTextEntry);

	nodep->createChild("max_chars", true)->setIntValue(mMaxChars);

	// Contents

	LLScrollListCtrl::items_vec_t data_list = mList->getAllData();
	for (size_t i = 0, count = data_list.size(); i < count; ++i)
	{
		LLScrollListItem* itemp = data_list[i];	// Cannot be NULL. HB
		LLScrollListCell* cellp = itemp->getColumn(0);
		if (cellp)
		{
			LLXMLNodePtr item_nodep = nodep->createChild("combo_item", false);
			LLSD value = itemp->getValue();
			item_nodep->createChild("value",
								   true)->setStringValue(value.asString());
			item_nodep->createChild("enabled",
								   true)->setBoolValue(itemp->getEnabled());
			item_nodep->setStringValue(cellp->getValue().asString());
		}
	}

	return nodep;
}

//static
LLView* LLComboBox::fromXML(LLXMLNodePtr nodep, LLView* parent,
							LLUICtrlFactory* factory)
{
	std::string name = LL_COMBO_BOX_TAG;
	nodep->getAttributeString("name", name);

	std::string label;
	nodep->getAttributeString("label", label);

	LLRect rect;
	createRect(nodep, rect, parent, LLRect());

	bool allow_text_entry = false;
	nodep->getAttributeBool("allow_text_entry", allow_text_entry);

	S32 max_chars = 20;
	nodep->getAttributeS32("max_chars", max_chars);

	LLUICtrlCallback callback = NULL;

	LLComboBox* combop = new LLComboBox(name, rect, label, callback, NULL);
	combop->setAllowTextEntry(allow_text_entry, max_chars);

	combop->initFromXML(nodep, parent);

	const std::string& contents = nodep->getValue();

	if (contents.find_first_not_of(" \n\t") != contents.npos)
	{
		llerrs << "Deprecated combo box item format used !  Please convert to <combo_item> tags !"
			   << llendl;
	}
	else
	{
		LLXMLNodePtr childp;
		for (childp = nodep->getFirstChild(); childp.notNull();
			 childp = childp->getNextSibling())
		{
			if (childp->hasName("combo_item"))
			{
				std::string label = childp->getTextContents();

				std::string value = label;
				childp->getAttributeString("value", value);

				LLScrollListItem* itemp = combop->add(label, LLSD(value));
				if (itemp && childp->hasAttribute("tool_tip"))
				{
					std::string tool_tip = label;
					childp->getAttributeString("tool_tip", tool_tip);
					itemp->setToolTip(tool_tip);
				}
			}
		}
	}

	// If providing user text entry or descriptive label don't select an item
	// under the hood
	if (!combop->acceptsTextInput() && combop->mLabel.empty())
	{
		combop->selectFirstItem();
	}

	return combop;
}

void LLComboBox::setEnabled(bool enabled)
{
	LLView::setEnabled(enabled);
	mButton->setEnabled(enabled);
}

void LLComboBox::clear()
{
	if (mTextEntry)
	{
		mTextEntry->setText(LLStringUtil::null);
	}
	mButton->setLabelSelected(LLStringUtil::null);
	mButton->setLabelUnselected(LLStringUtil::null);
	mButton->setDisabledLabel(LLStringUtil::null);
	mButton->setDisabledSelectedLabel(LLStringUtil::null);
	mList->deselectAllItems();
}

void LLComboBox::onCommit()
{
	if (mAllowTextEntry && getCurrentIndex() != -1)
	{
		// We have selected an existing item, blitz the manual text entry with
		// the properly capitalized item
		mTextEntry->setValue(getSimple());
		mTextEntry->setTentative(false);
	}

	setControlValue(getValue());
	LLUICtrl::onCommit();
}

//virtual
bool LLComboBox::isDirty() const
{
	return mList && mList->isDirty();
}

bool LLComboBox::isTextDirty() const
{
	return mTextEntry && mTextEntry->isDirty();
}

// Clears the dirty state
//virtual
void LLComboBox::resetDirty()
{
	if (mList)
	{
		mList->resetDirty();
	}
}

void LLComboBox::resetTextDirty()
{
	if (mTextEntry)
	{
		mTextEntry->resetDirty();
	}
}

bool LLComboBox::itemExists(const std::string& name) const
{
	return mList->selectItemByLabel(name);
}

bool LLComboBox::valueExists(const std::string& value) const
{
	return mList->getItemByValue(value) != NULL;
}

LLScrollListItem* LLComboBox::findItemByValue(const std::string& value) const
{
	return mList->getItemByValue(value);
}

LLComboBox::items_vec_t LLComboBox::getAllData() const
{
	return mList->getAllData();
}

// Adds item "name" to menu
LLScrollListItem* LLComboBox::add(const std::string& name, EAddPosition pos,
								  bool enabled)
{
	LLScrollListItem* itemp = mList->addSimpleElement(name, pos);
	itemp->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return itemp;
}

// Adds item "name" with a unique id to menu
LLScrollListItem* LLComboBox::add(const std::string& name, const LLUUID& id,
								  EAddPosition pos, bool enabled)
{
	LLScrollListItem* itemp = mList->addSimpleElement(name, pos, id);
	itemp->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return itemp;
}

// Adds item "name" with attached userdatap
LLScrollListItem* LLComboBox::add(const std::string& name, void* userdatap,
								  EAddPosition pos, bool enabled)
{
	LLScrollListItem* itemp = mList->addSimpleElement(name, pos);
	itemp->setEnabled(enabled);
	itemp->setUserdata(userdatap);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return itemp;
}

// Adds item "name" with attached generic data
LLScrollListItem* LLComboBox::add(const std::string& name, LLSD value,
								  EAddPosition pos, bool enabled)
{
	LLScrollListItem* itemp = mList->addSimpleElement(name, pos, value);
	itemp->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return itemp;
}

LLScrollListItem* LLComboBox::addSeparator(EAddPosition pos)
{
	return mList->addSeparator(pos);
}

void LLComboBox::sortByName(bool ascending)
{
	mList->sortOnce(0, ascending);
}

// Chooses an item with a given name in the menu. Returns true if the item was
// found.
bool LLComboBox::setSimple(const std::string& name)
{
	bool found = mList->selectItemByLabel(name, false);
	if (found)
	{
		setLabel(name);
	}
	return found;
}

//virtual
void LLComboBox::setValue(const LLSD& value)
{
	if (mList->selectByValue(value) && mList->getFirstSelected())
	{
		setLabel(mList->getSelectedItemLabel());
	}
}

const std::string LLComboBox::getSimple() const
{
	const std::string res = mList->getSelectedItemLabel();
	if (res.empty() && mAllowTextEntry)
	{
		return mTextEntry->getText();
	}
	else
	{
		return res;
	}
}

const std::string LLComboBox::getSelectedItemLabel(S32 column) const
{
	return mList->getSelectedItemLabel(column);
}

//virtual
LLSD LLComboBox::getValue() const
{
	LLScrollListItem* itemp = mList->getFirstSelected();
	if (itemp)
	{
		return itemp->getValue();
	}
	else if (mAllowTextEntry)
	{
		return mTextEntry->getValue();
	}
	else
	{
		return LLSD();
	}
}

void LLComboBox::setLabel(const std::string& name)
{
	if (mTextEntry)
	{
		mTextEntry->setText(name);
		if (mList->selectItemByLabel(name, false))
		{
			mTextEntry->setTentative(false);
		}
		else
		{
			if (!mSuppressTentative)
			{
				mTextEntry->setTentative(mTextEntryTentative);
			}
		}
	}

	if (!mAllowTextEntry)
	{
		mButton->setLabelUnselected(name);
		mButton->setLabelSelected(name);
		mButton->setDisabledLabel(name);
		mButton->setDisabledSelectedLabel(name);
	}
}

bool LLComboBox::remove(const std::string& name)
{
	bool found = mList->selectItemByLabel(name);
	if (found)
	{
		LLScrollListItem* itemp = mList->getFirstSelected();
		if (itemp)
		{
			mList->deleteSingleItem(mList->getItemIndex(itemp));
		}
	}

	return found;
}

bool LLComboBox::remove(S32 index)
{
	if (index < mList->getItemCount())
	{
		mList->deleteSingleItem(index);
		return true;
	}
	return false;
}

// Keyboard focus lost.
void LLComboBox::onFocusLost()
{
	hideList();
	// if valid selection
	if (mAllowTextEntry && getCurrentIndex() != -1)
	{
		mTextEntry->selectAll();
	}
	LLUICtrl::onFocusLost();
}

void LLComboBox::onLostTop()
{
	hideList();
}

void LLComboBox::setButtonVisible(bool visible)
{
	mButton->setVisible(visible);
	if (mTextEntry)
	{
		LLRect text_entry_rect(0, getRect().getHeight(),
							   getRect().getWidth(), 0);
		if (visible)
		{
			text_entry_rect.mRight -= llmax(8, mArrowImage->getWidth()) +
									  2 * LLUI::sDropShadowButton;
		}
		mTextEntry->reshape(text_entry_rect.getWidth(),
							text_entry_rect.getHeight());
	}
}

void LLComboBox::draw()
{
	mButton->setEnabled(getEnabled() /*&& !mList->isEmpty()*/);

	// Draw children normally
	LLUICtrl::draw();
}

bool LLComboBox::setCurrentByIndex(S32 index)
{
	bool found = mList->selectNthItem(index);
	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
	}
	return found;
}

S32 LLComboBox::getCurrentIndex() const
{
	LLScrollListItem* itemp = mList->getFirstSelected();
	return itemp ? mList->getItemIndex(itemp) : -1;
}

void LLComboBox::updateLayout()
{
	LLRect rect = getLocalRect();
	if (mAllowTextEntry)
	{
		mButton->setRect(LLRect(getRect().getWidth() -
								llmax(8, mArrowImage->getWidth()) -
								2 * LLUI::sDropShadowButton,
								rect.mTop, rect.mRight, rect.mBottom));
		mButton->setTabStop(false);

		if (!mTextEntry)
		{
			LLRect text_entry_rect(0, getRect().getHeight(),
								   getRect().getWidth(), 0);
			text_entry_rect.mRight -= llmax(8, mArrowImage->getWidth()) +
									  2 * LLUI::sDropShadowButton;
			// clear label on button
			std::string cur_label = mButton->getLabelSelected();
			mTextEntry = new LLLineEditor("combo_text_entry", text_entry_rect,
										  LLStringUtil::null,
										  LLFontGL::getFontSansSerifSmall(),
										  mMaxChars, onTextCommit, onTextEntry,
										  NULL, this);
			mTextEntry->setSelectAllonFocusReceived(true);
			mTextEntry->setHandleEditKeysDirectly(true);
			mTextEntry->setCommitOnFocusLost(false);
			mTextEntry->setText(cur_label);
			mTextEntry->setIgnoreTab(true);
			mTextEntry->setFollowsAll();
			addChild(mTextEntry);
		}
		else
		{
			mTextEntry->setVisible(true);
			mTextEntry->setMaxTextLength(mMaxChars);
		}

		// Clear label on button
		setLabel(LLStringUtil::null);

		mButton->setFollows(FOLLOWS_BOTTOM | FOLLOWS_TOP | FOLLOWS_RIGHT);
	}
	else if (!mAllowTextEntry)
	{
		mButton->setRect(rect);
		mButton->setTabStop(true);

		if (mTextEntry)
		{
			mTextEntry->setVisible(false);
		}
		mButton->setFollowsAll();
	}
}

void* LLComboBox::getCurrentUserdata()
{
	LLScrollListItem* itemp = mList->getFirstSelected();
	return itemp ? itemp->getUserdata() : NULL;
}

void LLComboBox::showList()
{
	// Make sure we do not go off top of screen.
	LLCoordWindow window_size;
	gWindowp->getSize(&window_size);
	// *HACK: we should not have to know about scale here
	mList->fitContents(192,
					   llfloor((F32)window_size.mY /
							   LLUI::sGLScaleFactor.mV[VY]) - 50);

	// Make sure that we can see the whole list
	LLRect root_view_local;
	LLView* root_view = getRootView();
	root_view->localRectToOtherView(root_view->getLocalRect(),
									&root_view_local, this);

	LLRect rect = mList->getRect();

	S32 min_width = getRect().getWidth();
	S32 max_width = llmax(min_width, MAX_COMBO_WIDTH);
	// Make sure we have up to date content width metrics
	S32 list_width = llclamp(mList->calcMaxContentWidth(), min_width,
							 max_width);

	if (mListPosition == BELOW)
	{
		if (rect.getHeight() <= -root_view_local.mBottom)
		{
			// Move rect so it hangs off the bottom of this view
			rect.setLeftTopAndSize(0, 0, list_width, rect.getHeight());
		}
		else
		{
			// Stack on top or bottom, depending on which has more room
			if (-root_view_local.mBottom >
					root_view_local.mTop - getRect().getHeight())
			{
				// Move rect so it hangs off the bottom of this view
				rect.setLeftTopAndSize(0, 0, list_width,
									   llmin(-root_view_local.mBottom,
											 rect.getHeight()));
			}
			else
			{
				// Move rect so it stacks on top of this view (clipped to size
				// of screen)
				rect.setOriginAndSize(0, getRect().getHeight(), list_width,
									 llmin(root_view_local.mTop -
										   getRect().getHeight(),
										   rect.getHeight()));
			}
		}
	}
	else // ABOVE
	{
		if (rect.getHeight() <= root_view_local.mTop - getRect().getHeight())
		{
			// Move rect so it stacks on top of this view (clipped to size of
			// screen)
			rect.setOriginAndSize(0, getRect().getHeight(), list_width,
								  llmin(root_view_local.mTop -
										getRect().getHeight(),
										rect.getHeight()));
		}
		else
		{
			// Stack on top or bottom, depending on which has more room
			if (-root_view_local.mBottom >
					root_view_local.mTop - getRect().getHeight())
			{
				// Move rect so it hangs off the bottom of this view
				rect.setLeftTopAndSize(0, 0, list_width,
									   llmin(-root_view_local.mBottom,
											 rect.getHeight()));
			}
			else
			{
				// Move rect so it stacks on top of this view (clipped to size
				// of screen)
				rect.setOriginAndSize(0, getRect().getHeight(), list_width,
									  llmin(root_view_local.mTop -
											getRect().getHeight(),
											rect.getHeight()));
			}
		}

	}
	mList->setOrigin(rect.mLeft, rect.mBottom);
	mList->reshape(rect.getWidth(), rect.getHeight());
	mList->translateIntoRect(root_view_local, false);

	// Make sure we did not go off bottom of screen
	S32 x, y;
	mList->localPointToScreen(0, 0, &x, &y);

	if (y < 0)
	{
		mList->translate(0, -y);
	}

	// NB: this call will trigger the focuslost callback which will hide the
	// list, so do it first before finally showing the list
	mList->setFocus(true);

	// Register ourselves as a "top" control effectively putting us into a
	// special draw layer and not affecting the bounding rectangle calculation
	gFocusMgr.setTopCtrl(this);

	// Show the list and push the button down
	mButton->setToggleState(true);
	mList->setVisible(true);

	setUseBoundingRect(true);
}

void LLComboBox::hideList()
{
#if 0	// Do not do this !  mTextEntry->getText() can be truncated, in which
		// case selectItemByLabel fails and this only resets the selection :/

	// *HACK: store the original value explicitly somewhere, not just in label
	std::string orig_selection = mAllowTextEntry ? mTextEntry->getText()
												 : mButton->getLabelSelected();
	// Assert selection in list
	mList->selectItemByLabel(orig_selection, false);
#endif

	mButton->setToggleState(false);
	mList->setVisible(false);
	mList->highlightNthItem(-1);

	setUseBoundingRect(false);
	if (gFocusMgr.getTopCtrl() == this)
	{
		gFocusMgr.setTopCtrl(NULL);
	}
}

//static
void LLComboBox::onButtonDown(void* userdatap)
{
	LLComboBox* self = (LLComboBox*)userdatap;
	if (!self) return;

	if (!self->mList->getVisible())
	{
		LLScrollListItem* last_sel_item = self->mList->getLastSelectedItem();
		if (last_sel_item)
		{
			// highlight the original selection before potentially selecting a
			// new item
			self->mList->highlightNthItem(self->mList->getItemIndex(last_sel_item));
		}

		if (self->mPrearrangeCallback)
		{
			self->mPrearrangeCallback(self, self->mCallbackUserData);
		}

		if (self->mList->getItemCount() != 0)
		{
			self->showList();
		}

		self->setFocus(true);

		// pass mouse capture on to list if button is depressed
		if (self->mButton->hasMouseCapture())
		{
			gFocusMgr.setMouseCapture(self->mList);
		}
	}
	else
	{
		self->hideList();
	}
}

//static
void LLComboBox::onItemSelected(LLUICtrl*, void* userdatap)
{
	// Note: item is the LLScrollListCtrl
	LLComboBox* self = (LLComboBox*)userdatap;
	if (!self) return;

	const std::string name = self->mList->getSelectedItemLabel();

	S32 cur_id = self->getCurrentIndex();
	if (cur_id != -1)
	{
		self->setLabel(name);

		if (self->mAllowTextEntry)
		{
			gFocusMgr.setKeyboardFocus(self->mTextEntry);
			self->mTextEntry->selectAll();
		}
	}

	// Hiding the list reasserts the old value stored in the text editor/
	// dropdown button
	self->hideList();

	// Commit does the reverse, asserting the value in the list
	self->onCommit();
}

bool LLComboBox::handleToolTip(S32 x, S32 y, std::string& msg,
							   LLRect* sticky_rect_screen)
{
    std::string tool_tip;

	if (LLUICtrl::handleToolTip(x, y, msg, sticky_rect_screen))
	{
		return true;
	}

	if (LLUI::sShowXUINames)
	{
		tool_tip = getShowNamesToolTip();
	}
	else
	{
		tool_tip = getToolTip();
		if (tool_tip.empty())
		{
			tool_tip = getSelectedItemLabel();
		}
	}

	if (!tool_tip.empty())
	{
		msg = tool_tip;

		// Convert rect local to screen coordinates
		localPointToScreen(0, 0, &(sticky_rect_screen->mLeft),
						   &(sticky_rect_screen->mBottom));
		localPointToScreen(getRect().getWidth(), getRect().getHeight(),
						   &(sticky_rect_screen->mRight),
						   &(sticky_rect_screen->mTop));
	}

	return true;
}

bool LLComboBox::handleKeyHere(KEY key, MASK mask)
{
	bool result = false;

	if (hasFocus())
	{
		if (mList->getVisible() && key == KEY_ESCAPE && mask == MASK_NONE)
		{
			hideList();
			return true;
		}
		// Give the list a chance to pop up and handle key
		LLScrollListItem* last_sel_item = mList->getLastSelectedItem();
		if (last_sel_item)
		{
			// Highlight the original selection before potentially selecting a
			// new item
			mList->highlightNthItem(mList->getItemIndex(last_sel_item));
		}
		result = mList->handleKeyHere(key, mask);

		// Will only see return key if it is originating from line editor
		// since the dropdown button eats the key
		if (key == KEY_RETURN)
		{
			// Do not show list and do not eat key input when committing
			// free-form text entry with RETURN since user already knows what
			// they are trying to select
			return false;
		}
		// If selection has changed, pop open the list
		else if (mList->getLastSelectedItem() != last_sel_item)
		{
			showList();
		}
	}

	return result;
}

bool LLComboBox::handleUnicodeCharHere(llwchar uni_char)
{
	bool result = false;

	if (gFocusMgr.childHasKeyboardFocus(this))
	{
		// Space bar just shows the list
		if (uni_char != ' ')
		{
			LLScrollListItem* last_sel_item = mList->getLastSelectedItem();
			if (last_sel_item)
			{
				// Highlight the original selection before potentially
				// selecting a new item
				mList->highlightNthItem(mList->getItemIndex(last_sel_item));
			}
			result = mList->handleUnicodeCharHere(uni_char);
			if (mList->getLastSelectedItem() != last_sel_item)
			{
				showList();
			}
		}
	}

	return result;
}

void LLComboBox::setAllowTextEntry(bool allow, S32 max_chars,
								   bool set_tentative)
{
	mAllowTextEntry = allow;
	mTextEntryTentative = set_tentative;
	mMaxChars = max_chars;
	updateLayout();
}

void LLComboBox::setTextEntry(const std::string& text)
{
	if (mTextEntry)
	{
		mTextEntry->setText(text);
		updateSelection();
	}
}

//static
void LLComboBox::onTextEntry(LLLineEditor* line_editor, void* userdatap)
{
	LLComboBox* self = (LLComboBox*)userdatap;
	if (!self || !gKeyboardp) return;

	if (self->mTextEntryCallback)
	{
		(*self->mTextEntryCallback)(line_editor, self->mCallbackUserData);
	}

	KEY key = gKeyboardp->currentKey();
	if (key == KEY_BACKSPACE || key == KEY_DELETE)
	{
		if (self->mList->selectItemByLabel(line_editor->getText(), false))
		{
			line_editor->setTentative(false);
		}
		else
		{
			if (!self->mSuppressTentative)
			{
				line_editor->setTentative(self->mTextEntryTentative);
			}
			self->mList->deselectAllItems();
		}
		return;
	}

	if (key == KEY_LEFT || key == KEY_RIGHT)
	{
		return;
	}

	if (key == KEY_DOWN)
	{
		self->setCurrentByIndex(llmin(self->getItemCount() - 1,
									  self->getCurrentIndex() + 1));
		if (!self->mList->getVisible())
		{
			if (self->mPrearrangeCallback)
			{
				self->mPrearrangeCallback(self, self->mCallbackUserData);
			}

			if (self->mList->getItemCount() != 0)
			{
				self->showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(false);
	}
	else if (key == KEY_UP)
	{
		self->setCurrentByIndex(llmax(0, self->getCurrentIndex() - 1));
		if (!self->mList->getVisible())
		{
			if (self->mPrearrangeCallback)
			{
				self->mPrearrangeCallback(self, self->mCallbackUserData);
			}

			if (self->mList->getItemCount() != 0)
			{
				self->showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(false);
	}
	else
	{
		// RN: presumably text entry
		self->updateSelection();
	}
}

void LLComboBox::updateSelection()
{
	LLWString left_wstring =
		mTextEntry->getWText().substr(0, mTextEntry->getCursor());
	// User-entered portion of string, based on assumption that any selected
    // text was a result of auto-completion
	LLWString user_wstring =
		mTextEntry->hasSelection() ? left_wstring : mTextEntry->getWText();
	std::string full_string = mTextEntry->getText();

	// Go ahead and arrange drop down list on first typed character, even
	// though we are not showing it... Some code relies on prearrange callback
	// to populate content
	if (mPrearrangeCallback && mTextEntry->getWText().size() == 1)
	{
		mPrearrangeCallback(this, mCallbackUserData);
	}

	if (mList->selectItemByLabel(full_string, false))
	{
		mTextEntry->setTentative(false);
	}
	else if (!mList->selectItemByPrefix(left_wstring, false))
	{
		mList->deselectAllItems();
		mTextEntry->setText(wstring_to_utf8str(user_wstring));
		if (!mSuppressTentative)
		{
			mTextEntry->setTentative(mTextEntryTentative);
		}
	}
	else
	{
		LLWString selected_item =
			utf8str_to_wstring(mList->getSelectedItemLabel());
		LLWString wtext = left_wstring +
						  selected_item.substr(left_wstring.size(),
											   selected_item.size());
		mTextEntry->setText(wstring_to_utf8str(wtext));
		mTextEntry->setSelection(left_wstring.size(),
								 mTextEntry->getWText().size());
		mTextEntry->endSelection();
		mTextEntry->setTentative(false);
	}
}

//static
void LLComboBox::onTextCommit(LLUICtrl* caller, void* userdatap)
{
	LLComboBox* self = (LLComboBox*)userdatap;
	if (self)
	{
		self->setSimple(self->mTextEntry->getText());
		self->onCommit();
		self->mTextEntry->selectAll();
	}
}

void LLComboBox::setSuppressTentative(bool suppress)
{
	mSuppressTentative = suppress;
	if (mTextEntry && mSuppressTentative)
	{
		mTextEntry->setTentative(false);
	}
}

void LLComboBox::setFocusText(bool b)
{
	LLUICtrl::setFocus(b);

	if (b && mTextEntry)
	{
		if (mTextEntry->getVisible())
		{
			mTextEntry->setFocus(true);
		}
	}
}

void LLComboBox::setFocus(bool b)
{
	LLUICtrl::setFocus(b);

	if (b)
	{
		mList->clearSearchString();
		if (mList->getVisible())
		{
			mList->setFocus(true);
		}
	}
}

void LLComboBox::setPrevalidate(bool (*func)(const LLWString&))
{
	if (mTextEntry)
	{
		mTextEntry->setPrevalidate(func);
	}
}

//============================================================================
// LLCtrlListInterface functions

S32 LLComboBox::getItemCount() const
{
	return mList->getItemCount();
}

void LLComboBox::addColumn(const LLSD& column, EAddPosition pos)
{
	mList->clearColumns();
	mList->addColumn(column, pos);
}

void LLComboBox::clearColumns()
{
	mList->clearColumns();
}

void LLComboBox::setColumnLabel(const std::string& column,
								const std::string& label)
{
	mList->setColumnLabel(column, label);
}

LLScrollListItem* LLComboBox::addElement(const LLSD& value, EAddPosition pos,
										 void* userdatap)
{
	return mList->addElement(value, pos, userdatap);
}

LLScrollListItem* LLComboBox::addSimpleElement(const std::string& value,
											   EAddPosition pos,
											   const LLSD& id)
{
	return mList->addSimpleElement(value, pos, id);
}

void LLComboBox::clearRows()
{
	mList->clearRows();
}

void LLComboBox::sortByColumn(const std::string& name, bool ascending)
{
	mList->sortByColumn(name, ascending);
}

LLScrollListItem* LLComboBox::getItemByIndex(S32 index) const
{
	return mList->getItemByIndex(index);
}

bool LLComboBox::setCurrentByID(const LLUUID& id)
{
	bool found = mList->selectByID(id);
	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
	}

	return found;
}

LLUUID LLComboBox::getCurrentID() const
{
	return mList->getStringUUIDSelectedItem();
}

bool LLComboBox::setSelectedByValue(const LLSD& value, bool selected)
{
	bool found = mList->setSelectedByValue(value, selected);
	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
	}
	return found;
}

LLSD LLComboBox::getSelectedValue()
{
	return mList->getSelectedValue();
}

bool LLComboBox::isSelected(const LLSD& value) const
{
	return mList->isSelected(value);
}

bool LLComboBox::selectItemRange(S32 first, S32 last)
{
	return mList->selectItemRange(first, last);
}

bool LLComboBox::operateOnSelection(EOperation op)
{
	if (op == OP_DELETE)
	{
		mList->deleteSelectedItems();
		return true;
	}
	return false;
}

bool LLComboBox::operateOnAll(EOperation op)
{
	if (op == OP_DELETE)
	{
		clearRows();
		return true;
	}
	return false;
}

//
// LLFlyoutButton
//

static const std::string LL_FLYOUT_BUTTON_ITEM_TAG = "flyout_button_item";

static const std::string LL_FLYOUT_BUTTON_TAG = "flyout_button";
static LLRegisterWidget<LLFlyoutButton> r04(LL_FLYOUT_BUTTON_TAG);

constexpr S32 FLYOUT_BUTTON_ARROW_WIDTH = 24;

LLFlyoutButton::LLFlyoutButton(const std::string& name, const LLRect& rect,
							   const std::string& label,
							   void (*commit_callback)(LLUICtrl*, void*),
							   void* callback_userdata)
:	LLComboBox(name, rect, LLStringUtil::null, commit_callback,
			   callback_userdata),
	mToggleState(false),
	mActionButton(NULL)
{
	// Text label button
	mActionButton = new LLButton(label, LLRect(), NULL, NULL, this);
	mActionButton->setScaleImage(true);

	mActionButton->setClickedCallback(onActionButtonClick);
	mActionButton->setFollowsAll();
	mActionButton->setHAlign(LLFontGL::HCENTER);
	mActionButton->setLabel(label);
	addChild(mActionButton);

	mActionButtonImage = LLUI::getUIImage("flyout_btn_left.tga");
	mExpanderButtonImage = LLUI::getUIImage("flyout_btn_right.tga");
	mActionButtonImageSelected = LLUI::getUIImage("flyout_btn_left_selected.tga");
	mExpanderButtonImageSelected = LLUI::getUIImage("flyout_btn_right_selected.tga");
	mActionButtonImageDisabled = LLUI::getUIImage("flyout_btn_left_disabled.tga");
	mExpanderButtonImageDisabled = LLUI::getUIImage("flyout_btn_right_disabled.tga");

	mActionButton->setImageSelected(mActionButtonImageSelected);
	mActionButton->setImageUnselected(mActionButtonImage);
	mActionButton->setImageDisabled(mActionButtonImageDisabled);
	mActionButton->setImageDisabledSelected(LLUIImagePtr(NULL));

	mButton->setImageSelected(mExpanderButtonImageSelected);
	mButton->setImageUnselected(mExpanderButtonImage);
	mButton->setImageDisabled(mExpanderButtonImageDisabled);
	mButton->setImageDisabledSelected(LLUIImagePtr(NULL));
	mButton->setRightHPad(6);

	updateLayout();
}

//virtual
const std::string& LLFlyoutButton::getTag() const
{
	return LL_FLYOUT_BUTTON_TAG;
}

//virtual
LLXMLNodePtr LLFlyoutButton::getXML(bool save_children) const
{
	LLXMLNodePtr nodep = LLComboBox::getXML();

	nodep->setName(LL_FLYOUT_BUTTON_TAG);

	for (LLXMLNodePtr childp = nodep->getFirstChild(); childp.notNull(); )
	{
		if (childp->hasName("combo_item"))
		{
			childp->setName(LL_FLYOUT_BUTTON_ITEM_TAG);

			// setName() does a delete and add, so we have to start over
			childp = nodep->getFirstChild();
		}
		else
		{
			childp = childp->getNextSibling();
		}
	}

	return nodep;
}

//static
LLView* LLFlyoutButton::fromXML(LLXMLNodePtr nodep, LLView* parent,
								LLUICtrlFactory* factory)
{
	std::string name = LL_FLYOUT_BUTTON_TAG;
	nodep->getAttributeString("name", name);

	std::string label;
	nodep->getAttributeString("label", label);

	LLRect rect;
	createRect(nodep, rect, parent, LLRect());

	LLUICtrlCallback callback = NULL;
	LLFlyoutButton* flyout_button = new LLFlyoutButton(name, rect, label,
													   callback, NULL);

	std::string list_position;
	nodep->getAttributeString("list_position", list_position);
	if (list_position == "below")
	{
		flyout_button->mListPosition = BELOW;
	}
	else if (list_position == "above")
	{
		flyout_button->mListPosition = ABOVE;
	}

	flyout_button->initFromXML(nodep, parent);

	for (LLXMLNodePtr childp = nodep->getFirstChild(); childp.notNull();
		 childp = childp->getNextSibling())
	{
		if (childp->hasName(LL_FLYOUT_BUTTON_ITEM_TAG))
		{
			std::string label = childp->getTextContents();

			std::string value = label;
			childp->getAttributeString("value", value);

			LLScrollListItem* itemp = flyout_button->add(label, LLSD(value));
			if (itemp && childp->hasAttribute("tool_tip"))
			{
				std::string tool_tip = label;
				childp->getAttributeString("tool_tip", tool_tip);
				itemp->setToolTip(tool_tip);
			}
		}
	}

	flyout_button->updateLayout();

	return flyout_button;
}

void LLFlyoutButton::updateLayout()
{
	LLComboBox::updateLayout();

	mButton->setOrigin(getRect().getWidth() - FLYOUT_BUTTON_ARROW_WIDTH, 0);
	mButton->reshape(FLYOUT_BUTTON_ARROW_WIDTH, getRect().getHeight());
	mButton->setFollows(FOLLOWS_RIGHT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
	mButton->setTabStop(false);
	mButton->setImageOverlay(mListPosition == BELOW ? "down_arrow.tga"
													: "up_arrow.tga",
							 LLFontGL::RIGHT);

	mActionButton->setOrigin(0, 0);
	mActionButton->reshape(getRect().getWidth() - FLYOUT_BUTTON_ARROW_WIDTH,
						   getRect().getHeight());
}

void LLFlyoutButton::setLabel(const std::string& label)
{
	mActionButton->setLabel(label);
}

//static
void LLFlyoutButton::onActionButtonClick(void* userdatap)
{
	LLFlyoutButton* buttonp = (LLFlyoutButton*)userdatap;
	// remember last list selection ?
	buttonp->mList->deselect();
	buttonp->onCommit();
}

void LLFlyoutButton::draw()
{
	mActionButton->setToggleState(mToggleState);
	mButton->setToggleState(mToggleState);

	// *FIXME: this should be an attribute of comboboxes, whether they have a
	// distinct label or the label reflects the last selected item, for now we
	// have to manually remove the label
	mButton->setLabel(LLStringUtil::null);
	LLComboBox::draw();
}

void LLFlyoutButton::setEnabled(bool enabled)
{
	mActionButton->setEnabled(enabled);
	LLComboBox::setEnabled(enabled);
}
