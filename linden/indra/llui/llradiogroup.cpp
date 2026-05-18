/**
 * @file llradiogroup.cpp
 * @brief LLRadioGroup base class
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

#include "linden_common.h"

#include "llradiogroup.h"

#include "indra_constants.h"
#include "llcontrol.h"
#include "llviewborder.h"

static const std::string LL_RADIO_GROUP_TAG = "radio_group";
static const std::string LL_RADIO_ITEM_TAG = "radio_item";

static LLRegisterWidget<LLRadioGroup> r17(LL_RADIO_GROUP_TAG);

///////////////////////////////////////////////////////////////////////////////
// LLRadioCtrl class
///////////////////////////////////////////////////////////////////////////////

//virtual
const std::string& LLRadioCtrl::getTag() const
{
	return LL_RADIO_ITEM_TAG;
}

void LLRadioCtrl::setValue(const LLSD& value)
{
	LLCheckBoxCtrl::setValue(value);
	mButton->setTabStop(value.asBoolean());
}

//virtual
LLXMLNodePtr LLRadioCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr nodep = LLCheckBoxCtrl::getXML();
	nodep->setName(LL_RADIO_ITEM_TAG);
	nodep->setStringValue(getLabel());
	return nodep;
}

///////////////////////////////////////////////////////////////////////////////
// LLRadioGroup class
///////////////////////////////////////////////////////////////////////////////

LLRadioGroup::LLRadioGroup(const std::string& name, const LLRect& rect,
						   const char* control_name,
						   LLUICtrlCallback callback, void* userdata,
						   bool border)
:	LLUICtrl(name, rect, true, callback, userdata, FOLLOWS_LEFT | FOLLOWS_TOP),
	mSelectedIndex(0)
{
	setControlName(control_name, NULL);
	init(border);
}

LLRadioGroup::LLRadioGroup(const std::string& name, const LLRect& rect,
						   S32 initial_index, LLUICtrlCallback callback,
						   void* userdata, bool border)
:	LLUICtrl(name, rect, true, callback, userdata, FOLLOWS_LEFT | FOLLOWS_TOP),
	mSelectedIndex(initial_index)
{
	init(border);
}

void LLRadioGroup::init(bool border)
{
	if (border)
	{
		addChild(new LLViewBorder("radio group border",
								  LLRect(0, getRect().getHeight(),
										 getRect().getWidth(), 0),
								  LLViewBorder::BEVEL_NONE,
								  LLViewBorder::STYLE_LINE, 1));
	}
	mHasBorder = border;
}

//virtual
void LLRadioGroup::setEnabled(bool enabled)
{
	for (child_list_const_iter_t it = getChildList()->begin(),
								 end = getChildList()->end();
		 it != end; ++it)
	{
		(*it)->setEnabled(enabled);
	}
	LLView::setEnabled(enabled);
}

void LLRadioGroup::setIndexEnabled(S32 index, bool enabled)
{
	button_list_t::iterator begin = mRadioButtons.begin();
	button_list_t::iterator end = mRadioButtons.end();
	S32 count = 0;
	for (button_list_t::iterator iter = begin; iter != end; ++iter)
	{
		LLRadioCtrl* childp = *iter;
		if (count == index)
		{
			childp->setEnabled(enabled);
			if (index == mSelectedIndex && !enabled)
			{
				setSelectedIndex(-1);
			}
			break;
		}
		++count;
	}
	count = 0;
	if (mSelectedIndex < 0)
	{
		// Set to highest enabled value < index, or lowest value above index if
		// none lower are enabled, or 0 if none are enabled
		for (button_list_t::iterator iter = begin; iter != end; ++iter)
		{
			if (count >= index && mSelectedIndex >= 0)
			{
				break;
			}
			LLRadioCtrl* childp = *iter;
			if (childp->getEnabled())
			{
				setSelectedIndex(count);
			}
			++count;
		}
		if (mSelectedIndex < 0)
		{
			setSelectedIndex(0);
		}
	}
}

bool LLRadioGroup::setSelectedIndex(S32 index, bool from_event)
{
	if (index < 0 || index >= (S32)mRadioButtons.size())
	{
		return false;
	}

	mSelectedIndex = index;

	if (!from_event)
	{
		setControlValue(getSelectedIndex());
	}

	return true;
}

bool LLRadioGroup::handleKeyHere(KEY key, MASK mask)
{
	bool handled = false;

	// do any of the tab buttons have keyboard focus ?
	if (mask == MASK_NONE)
	{
		handled = true;
		switch (key)
		{
			case KEY_DOWN:
				if (!setSelectedIndex(getSelectedIndex() + 1))
				{
					make_ui_sound("UISndInvalidOp");
				}
				else
				{
					onCommit();
				}
				break;

			case KEY_UP:
				if (!setSelectedIndex(getSelectedIndex() - 1))
				{
					make_ui_sound("UISndInvalidOp");
				}
				else
				{
					onCommit();
				}
				break;

			case KEY_LEFT:
				if (!setSelectedIndex(getSelectedIndex() - 1))
				{
					make_ui_sound("UISndInvalidOp");
				}
				else
				{
					onCommit();
				}
				break;

			case KEY_RIGHT:
				if (!setSelectedIndex(getSelectedIndex() + 1))
				{
					make_ui_sound("UISndInvalidOp");
				}
				else
				{
					onCommit();
				}
				break;

			default:
				handled = false;
		}
	}

	return handled;
}

void LLRadioGroup::draw()
{
	S32 current_button = 0;

	bool take_focus = false;
	if (gFocusMgr.childHasKeyboardFocus(this))
	{
		take_focus = true;
	}

	for (button_list_t::iterator iter = mRadioButtons.begin(),
								 end = mRadioButtons.end();
		 iter != end; ++iter)
	{
		LLRadioCtrl* radiop = *iter;
		bool selected = current_button == mSelectedIndex;
		radiop->setValue(selected);
		if (take_focus && selected && !gFocusMgr.childHasKeyboardFocus(radiop))
		{
			// Do not flash keyboard focus when navigating via keyboard
			bool dont_flash = false;
			radiop->focusFirstItem(false, dont_flash);
		}
		++current_button;
	}

	LLView::draw();
}

// When adding a button, we need to ensure that the radio group gets a message
// when the button is clicked.
LLRadioCtrl* LLRadioGroup::addRadioButton(const std::string& name,
										  const std::string& label,
										  const LLRect& rect,
										  const LLFontGL* fontp)
{
	// Highlight will get fixed in draw method above
	LLRadioCtrl* radiop = new LLRadioCtrl(name, rect, label, fontp,
										  onClickButton, this);
	addChild(radiop);
	mRadioButtons.push_back(radiop);
	return radiop;
}

// Handle one button being clicked. All child buttons must have this method as
// their callback.
//static
void LLRadioGroup::onClickButton(LLUICtrl* ctrlp, void* userdata)
{
	LLRadioCtrl* clicked_radiop = (LLRadioCtrl*)ctrlp;
	LLRadioGroup* self = (LLRadioGroup*)userdata;

	S32 counter = 0;
	for (button_list_t::iterator iter = self->mRadioButtons.begin(),
								 end = self->mRadioButtons.end();
		 iter != end; ++iter)
	{
		LLRadioCtrl* radiop = *iter;
		if (radiop == clicked_radiop)
		{
#if 1		// Note: original code did not have this check and unconditionnaly
			// called onCommit(), which was commented as a bug; this fix should
			// not have any bad consequence... HB
			if (self->mSelectedIndex != counter)
#endif
			{
				self->setSelectedIndex(counter);
				self->setControlValue(counter);
				self->onCommit();
			}
			return;
		}

		++counter;
	}

	// This should never happen.
	llerrs << "Passed LLUICtrl pointer is not a child button of radio group: "
		   << self->getName() << llendl;
}

void LLRadioGroup::setValue(const LLSD& value)
{
	std::string value_name = value.asString();
	S32 idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin(),
									   end = mRadioButtons.end();
		 iter != end; ++iter)
	{
		LLRadioCtrl* radiop = *iter;
		if (radiop->getName() == value_name)
		{
			setSelectedIndex(idx);
			idx = -1;
			break;
		}
		++idx;
	}
	if (idx != -1)
	{
		// String not found, try integer
		if (value.isInteger())
		{
			setSelectedIndex(value.asInteger(), true);
		}
		else
		{
			llwarns << "Value not found: " << value_name << llendl;
		}
	}
}

LLSD LLRadioGroup::getValue() const
{
	S32 index = getSelectedIndex();
	S32 idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin(),
									   end = mRadioButtons.end();
		 iter != end; ++iter)
	{
		if (idx == index)
		{
			return LLSD((*iter)->getName());
		}
		++idx;
	}
	return LLSD();
}

//virtual
const std::string& LLRadioGroup::getTag() const
{
	return LL_RADIO_GROUP_TAG;
}

//virtual
LLXMLNodePtr LLRadioGroup::getXML(bool save_children) const
{
	LLXMLNodePtr nodep = LLUICtrl::getXML();
	nodep->setName(LL_RADIO_GROUP_TAG);

	// Attributes
	nodep->createChild("draw_border", true)->setBoolValue(mHasBorder);

	// Contents
	for (button_list_t::const_iterator iter = mRadioButtons.begin(),
									   end = mRadioButtons.end();
		 iter != end; ++iter)
	{
		LLXMLNodePtr child_nodep = (*iter)->getXML();
		nodep->addChild(child_nodep);
	}

	return nodep;
}

// static
LLView* LLRadioGroup::fromXML(LLXMLNodePtr nodep, LLView* parentp,
							  LLUICtrlFactory*)
{
	std::string name = LL_RADIO_GROUP_TAG;
	nodep->getAttributeString("name", name);

	U32 initial_value = 0;
	nodep->getAttributeU32("initial_value", initial_value);

	bool draw_border = true;
	nodep->getAttributeBool("draw_border", draw_border);

	LLRect rect;
	createRect(nodep, rect, parentp, LLRect());

	LLRadioGroup* groupp = new LLRadioGroup(name, rect, initial_value, NULL,
											NULL, draw_border);
	LLFontGL* fontp = LLView::selectFont(nodep);

	LLXMLNodePtr childp;
	for (LLXMLNodePtr childp = nodep->getFirstChild(); childp.notNull();
		 childp = childp->getNextSibling())
	{
		if (childp->hasName("radio_item"))
		{
			LLRect item_rect;
			createRect(childp, item_rect, groupp, rect);

			std::string radioname("radio");
			childp->getAttributeString("name", radioname);
			std::string item_label = childp->getTextContents();
			LLRadioCtrl* radiop = groupp->addRadioButton(radioname, item_label,
														item_rect, fontp);
			radiop->initFromXML(childp, groupp);
		}
	}

	groupp->initFromXML(nodep, parentp);

	return groupp;
}

bool LLRadioGroup::setSelectedByValue(const LLSD& value, bool selected)
{
	S32 idx = 0;
	std::string value_string = value.asString();
	for (button_list_t::const_iterator iter = mRadioButtons.begin(),
									   end = mRadioButtons.end();
		 iter != end; ++iter)
	{
		if ((*iter)->getName() == value_string)
		{
			setSelectedIndex(idx);
			return true;
		}
		++idx;
	}

	return false;
}

bool LLRadioGroup::isSelected(const LLSD& value) const
{
	S32 idx = 0;
	std::string value_string = value.asString();
	for (button_list_t::const_iterator iter = mRadioButtons.begin(),
									   end = mRadioButtons.end();
		 iter != end; ++iter)
	{
		if ((*iter)->getName() == value_string)
		{
			if (idx == mSelectedIndex)
			{
				return true;
			}
		}
		++idx;
	}
	return false;
}
