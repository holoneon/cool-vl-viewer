/**
 * @file llmultisliderctrl.cpp
 * @brief LLMultiSliderCtrl base class
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llmultisliderctrl.h"

#include "llcontrol.h"
#include "llgl.h"
#include "llimagegl.h"
#include "llkeyboard.h"			// For the MASK constants
#include "lllineeditor.h"
#include "lllocale.h"
#include "lltextbox.h"
#include "llwindow.h"

static const std::string LL_MULTI_SLIDER_CTRL_TAG = "multi_slider";
static LLRegisterWidget<LLMultiSliderCtrl> r11(LL_MULTI_SLIDER_CTRL_TAG);

constexpr U32 MAX_STRING_LENGTH = 10;
// Space between label, slider, and text
constexpr S32 MULTI_SLIDERCTRL_SPACING = 4;
constexpr S32 MULTI_THUMB_WIDTH = 8;
constexpr S32 MULTI_TRACK_HEIGHT = 6;
constexpr F32 FLOAT_THRESHOLD = 0.00001f;
constexpr S32 EXTRA_TRIANGLE_WIDTH = 2;
constexpr S32 EXTRA_TRIANGLE_HEIGHT = -2;

///////////////////////////////////////////////////////////////////////////////
// LLMultiSlider class (slider UI element for the milti-slider)
///////////////////////////////////////////////////////////////////////////////

S32 LLMultiSlider::mNameCounter = 0;

LLMultiSlider::LLMultiSlider(const std::string& name, const LLRect& rect,
							 void (*on_commit_callback)(LLUICtrl*, void*),
							 void* callback_userdata, F32 initial_value,
							 F32 min_value, F32 max_value, F32 increment,
							 S32 max_sliders, F32 overlap_threshold,
							 bool allow_overlap, bool loop_overlap,
							 bool draw_track, bool use_triangle, bool vertical,
							 const char* control_name)
:	LLUICtrl(name, rect, true, on_commit_callback, callback_userdata,
			 FOLLOWS_LEFT | FOLLOWS_TOP),
	mInitialValue(initial_value),
	mMinValue(min_value),
	mMaxValue(max_value),
	mIncrement(increment),
	mMaxNumSliders(max_sliders),
	mOverlapThreshold(overlap_threshold),
	mAllowOverlap(allow_overlap),
	mLoopOverlap(loop_overlap),
	mDrawTrack(draw_track),
	mUseTriangle(use_triangle),
	mVertical(vertical),
	mMouseOffset(0),
	mMouseDownCallback(NULL),
	mMouseUpCallback(NULL)
{
	if (mVertical)
	{
		mDragStartThumbRect = LLRect(0, MULTI_THUMB_WIDTH,
									 getRect().getWidth(), 0);
	}
	else
	{
		mDragStartThumbRect = LLRect(0, getRect().getHeight(),
									 MULTI_THUMB_WIDTH, 0);
	}

	if (mOverlapThreshold && mOverlapThreshold > mIncrement)
	{
		mOverlapThreshold -= mIncrement;
	}
	else
	{
		mOverlapThreshold = 0.f;
	}

	// Properly handle setting the starting thumb rect. Do it this way to
	// handle both the operating-on-settings and standalone ways of using this.
	setControlName(control_name, NULL);
	setValue(getValue());
}

void LLMultiSlider::setSliderValue(const std::string& name, F32 value,
								   bool from_event)
{
	// Exit if not there
	if (!mValue.has(name))
	{
		return;
	}

	// Round to nearest increment (bias towards rounding down)
	value = llclamp(value, mMinValue, mMaxValue) - mMinValue +
			mIncrement / 2.0001f;
	value -= fmodf(value, mIncrement);
	F32 new_value = mMinValue + value;

	// Now, make sure no overlap if we want that
	if (!mAllowOverlap)
	{
		// Increment is our distance between points; use it to eliminate
		// rounding error.
		F32 threshold = mOverlapThreshold + mIncrement / 4.f;
		// If loop overlap is enabled, check if we overlap with points 'after'
		// max value (project to lower).
		F32 loop_up_check;
		if (mLoopOverlap && value + threshold > mMaxValue)
		{
			loop_up_check = value + threshold - mMaxValue + mMinValue;
		}
		else
		{
			loop_up_check = mMinValue - 1.f;
		}
		// If loop overlap is enabled, check if we overlap with points 'before'
		// min value (project to upper).
		F32 loop_down_check;
		if (mLoopOverlap && value - threshold < mMinValue)
		{
			loop_down_check = value - threshold - mMinValue + mMaxValue;
		}
		else
		{
			loop_down_check = mMaxValue + 1.f;
		}
		// Look at the current spot and see if anything is there
		for (LLSD::map_iterator it = mValue.beginMap(), end = mValue.endMap();
			 it != end; ++it)
		{
			F32 loc_val = (F32)it->second.asReal();
			F32 test_val = loc_val - new_value;
			if (test_val > -threshold && test_val < threshold &&
				it->first != name)
			{
				// Already occupied !
				return;
			}
			if (mLoopOverlap &&
				(loc_val < loop_up_check || loc_val > loop_down_check))
			{
				return;
			}
		}
	}

	// Now set it in the map
	mValue[name] = new_value;

	// Set the control if it's the current slider and not from an event
	if (!from_event && name == mCurSlider)
	{
		setControlValue(mValue);
	}

	F32 t = (new_value - mMinValue) / (mMaxValue - mMinValue);
	if (mVertical)
	{
		S32 bottom_edge = MULTI_THUMB_WIDTH / 2;
		S32 top_edge = getRect().getHeight() - MULTI_THUMB_WIDTH / 2;
		S32 x = bottom_edge + S32(t * (top_edge - bottom_edge));
		mThumbRects[name].mTop = x + MULTI_THUMB_WIDTH / 2;
		mThumbRects[name].mBottom = x - MULTI_THUMB_WIDTH / 2;
	}
	else
	{
		S32 left_edge = MULTI_THUMB_WIDTH / 2;
		S32 right_edge = getRect().getWidth() - MULTI_THUMB_WIDTH / 2;
		S32 x = left_edge + S32(t * (right_edge - left_edge));
		mThumbRects[name].mLeft = x - MULTI_THUMB_WIDTH / 2;
		mThumbRects[name].mRight = x + MULTI_THUMB_WIDTH / 2;
	}
}

//virtual
void LLMultiSlider::setValue(const LLSD& value)
{
	// Only do if it is a map
	if (value.isMap())
	{
		// Add each value... the first in the map becomes the current
		LLSD::map_const_iterator it = value.beginMap();
		LLSD::map_const_iterator end = value.endMap();
		mCurSlider = it->first;
		for ( ; it != end; ++it)
		{
			setSliderValue(it->first, (F32)it->second.asReal(), true);
		}
	}
}

F32 LLMultiSlider::getSliderValue(const std::string& name) const
{
	return mValue.has(name) ? (F32)mValue[name].asReal() : 0.f;
}

void LLMultiSlider::setCurSlider(const std::string& name)
{
	if (mValue.has(name))
	{
		mCurSlider = name;
	}
}

F32 LLMultiSlider::getSliderValueFromPos(S32 xpos, S32 ypos) const
{
	F32 t;
	if (mVertical)
	{
		S32 bottom_edge = MULTI_THUMB_WIDTH / 2;
		S32 top_edge = getRect().getHeight() - MULTI_THUMB_WIDTH / 2;
		ypos += mMouseOffset;
		ypos = llclamp(ypos, bottom_edge, top_edge);
		t = F32(ypos - bottom_edge) / F32(top_edge - bottom_edge);
	}
	else
	{
		S32 left_edge = MULTI_THUMB_WIDTH / 2;
		S32 right_edge = getRect().getWidth() - MULTI_THUMB_WIDTH / 2;
		xpos += mMouseOffset;
		xpos = llclamp(xpos, left_edge, right_edge);
		t = F32(xpos - left_edge) / F32(right_edge - left_edge);
	}
	return t * (mMaxValue - mMinValue) + mMinValue;
}

const std::string& LLMultiSlider::addSlider(F32 val)
{
	F32 init_val = val;

	if (mValue.size() >= mMaxNumSliders)
	{
		return LLStringUtil::null;
	}

	// Create a new name
	std::string new_name = llformat("sldr%d", mNameCounter++);

	if (!findUnusedValue(init_val))
	{
		return LLStringUtil::null;
	}

	// Add a new thumb rect
	if (mVertical)
	{
		mThumbRects[new_name] = LLRect(0, MULTI_THUMB_WIDTH,
									   getRect().getWidth(), 0);
	}
	else
	{
		mThumbRects[new_name] = LLRect(0, getRect().getHeight(),
									   MULTI_THUMB_WIDTH, 0);
	}

	// Add the value and set the current slider to this one
	mValue.insert(new_name, init_val);
	mCurSlider = new_name;

	// Move the slider
	setSliderValue(mCurSlider, init_val, true);

	return mCurSlider;
}

bool LLMultiSlider::addSlider(F32 val, const std::string& name)
{
	F32 init_val = val;

	if (mValue.size() >= mMaxNumSliders)
	{
		return false;
	}

	if (!findUnusedValue(init_val))
	{
		return false;
	}

	// Add a new thumb rect
	if (mVertical)
	{
		mThumbRects[name] = LLRect(0, MULTI_THUMB_WIDTH,
								   getRect().getWidth(), 0);
	}
	else
	{
		mThumbRects[name] = LLRect(0, getRect().getHeight(),
								   MULTI_THUMB_WIDTH, 0);
	}

	// Add the value and set the current slider to this one
	mValue.insert(name, init_val);
	mCurSlider = name;

	// Move the slider
	setSliderValue(mCurSlider, init_val, true);

	return true;
}

bool LLMultiSlider::findUnusedValue(F32& init_val)
{
	bool first_try = true;

	// Find the first open slot starting with the initial value
	while (true)
	{
		bool hit = false;

		// Look at the current spot and see if anything is there
		F32 threshold = mAllowOverlap ? FLOAT_THRESHOLD
									  : mOverlapThreshold + mIncrement / 4.f;
		for (LLSD::map_iterator it = mValue.beginMap(), end = mValue.endMap();
			 it != end; ++it)
		{
			F32 test_val = (F32)it->second.asReal() - init_val;
			if (test_val > -threshold && test_val < threshold)
			{
				hit = true;
				break;
			}
		}

		// If we found one
		if (!hit)
		{
			break;
		}

		// Increment and wrap if need be
		init_val += mIncrement;
		if (init_val > mMaxValue)
		{
			init_val = mMinValue;
		}

		// Stop if it is filled
		if (init_val == mInitialValue && !first_try)
		{
			llwarns << "Too many multi slider elements !" << llendl;
			return false;
		}

		first_try = false;
		continue;
	}

	return true;
}

void LLMultiSlider::deleteSlider(const std::string& name)
{
	// Cannot delete the last slider
	if (mValue.size() <= 0)
	{
		return;
	}

	// Get rid of value from mValue and its thumb rect
	mValue.erase(name);
	mThumbRects.erase(name);

	// Set to the last created
	if (mValue.size() > 0)
	{
		rect_map_t::iterator it = mThumbRects.end();
		mCurSlider = (--it)->first;
	}
}

//virtual
void LLMultiSlider::clear()
{
	while (mThumbRects.size() > 0 && mValue.size() > 0)
	{
		deleteCurSlider();
	}
	LLUICtrl::clear();
}

//virtual
bool LLMultiSlider::handleHover(S32 x, S32 y, MASK mask)
{
	if (gFocusMgr.getMouseCapture() == this)
	{
		setCurSliderValue(getSliderValueFromPos(x, y));
		onCommit();

		gWindowp->setCursor(UI_CURSOR_ARROW);
		LL_DEBUGS("UserInput") << "hover handled by " << getName()
							   << " (active)" << LL_ENDL;
	}
	else
	{
		gWindowp->setCursor(UI_CURSOR_ARROW);
		LL_DEBUGS("UserInput") << "hover handled by " << getName()
							   << " (inactive)" << LL_ENDL;
	}

	return true;
}

//virtual
bool LLMultiSlider::handleMouseUp(S32 x, S32 y, MASK mask)
{
	bool handled = false;

	if (gFocusMgr.getMouseCapture() == this)
	{
		gFocusMgr.setMouseCapture(NULL);

		if (mMouseUpCallback)
		{
			mMouseUpCallback(x, y, mCallbackUserData);
		}
		handled = true;
		make_ui_sound("UISndClickRelease");
	}
	else
	{
		handled = true;
	}

	return handled;
}

//virtual
bool LLMultiSlider::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Only do sticky-focus on non-chrome widgets
	if (!getIsChrome())
	{
		setFocus(true);
	}
	if (mMouseDownCallback)
	{
		mMouseDownCallback(x, y, mCallbackUserData);
	}

	if (mask & MASK_CONTROL) // if CTRL is modifying
	{
		setCurSliderValue(mInitialValue);
		onCommit();
	}
	else
	{
		// Scroll through thumbs to see if we have a new one selected and
		// select that one
		for (rect_map_t::iterator it = mThumbRects.begin(),
								  end = mThumbRects.end();
			 it != end; ++it)
		{
			// Check if inside. If so, set current slider and continue.
			if (it->second.pointInRect(x, y))
			{
				mCurSlider = it->first;
				break;
			}
		}

		if (!mCurSlider.empty())
		{
			// Find the offset of the actual mouse location from the center of
			// the thumb.
			if (mThumbRects[mCurSlider].pointInRect(x, y))
			{
				mMouseOffset = mThumbRects[mCurSlider].mLeft +
							   MULTI_THUMB_WIDTH / 2 - x;
			}
			else
			{
				mMouseOffset = 0;
			}

			// Start dragging the thumb. No handler needed for focus lost since
			// this class has no state that depends on it.
			gFocusMgr.setMouseCapture(this);
			mDragStartThumbRect = mThumbRects[mCurSlider];
		}
	}
	make_ui_sound("UISndClick");

	return true;
}

//virtual
bool LLMultiSlider::handleKeyHere(KEY key, MASK mask)
{
	switch (key)
	{
		case KEY_UP:
			if (mVertical)
			{
				setCurSliderValue(getCurSliderValue() + getIncrement());
				onCommit();
			}
			return true;

		case KEY_DOWN:
			if (mVertical)
			{
				setCurSliderValue(getCurSliderValue() - getIncrement());
				onCommit();
			}
			return true;

		case KEY_LEFT:
			if (!mVertical)
			{
				setCurSliderValue(getCurSliderValue() - getIncrement());
				onCommit();
			}
			return true;

		case KEY_RIGHT:
			if (!mVertical)
			{
				setCurSliderValue(getCurSliderValue() + getIncrement());
				onCommit();
			}
			return true;

		default:
			break;
	}

	return false;
}

//virtual
void LLMultiSlider::draw()
{
	rect_map_t::iterator it;
	rect_map_t::iterator begin = mThumbRects.begin();
	rect_map_t::iterator end = mThumbRects.end();
	rect_map_t::iterator cur_sldr_it;

	// Draw background and thumb.

	// Drawing solids requires texturing to be disabled
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	LLRect rect(mDragStartThumbRect);

	F32 opacity = getEnabled() ? 1.f : 0.3f;

	// Track
	S32 height_offset = 0;
	S32 width_offset = 0;
	if (mVertical)
	{
		width_offset = (getRect().getWidth() - MULTI_TRACK_HEIGHT) / 2;
	}
	else
	{
		height_offset = (getRect().getHeight() - MULTI_TRACK_HEIGHT) / 2;
	}
	LLRect track_rect(width_offset, getRect().getHeight() - height_offset,
					  getRect().getWidth() - width_offset, height_offset);
	if (mDrawTrack)
	{
		track_rect.stretch(-1);
		LLUIImage::sRoundedSquare->draw(track_rect,
										LLUI::sMultiSliderTrackColor %
										opacity);
	}

	// If we are supposed to use a drawn triangle simple GL call for the
	// triangle
	if (mUseTriangle)
	{
		for (it = begin; it != end; ++it)
		{
			gl_triangle_2d(it->second.mLeft - EXTRA_TRIANGLE_WIDTH,
						   it->second.mTop + EXTRA_TRIANGLE_HEIGHT,
						   it->second.mRight + EXTRA_TRIANGLE_WIDTH,
						   it->second.mTop + EXTRA_TRIANGLE_HEIGHT,
						   it->second.mLeft + it->second.getWidth() / 2,
						   it->second.mBottom - EXTRA_TRIANGLE_HEIGHT,
						   LLUI::sMultiSliderTriangleColor, true);
		}
	}
	else if (gFocusMgr.getMouseCapture() == this)
	{
		// Draw drag start
		LLUIImage::sRoundedSquare->drawSolid(mDragStartThumbRect,
											 LLUI::sMultiSliderThumbCenterColor %
											 0.3f);

		// Draw the highlight
		if (hasFocus())
		{
			LLUIImage::sRoundedSquare->drawBorder(mThumbRects[mCurSlider],
												  gFocusMgr.getFocusColor(),
												  gFocusMgr.getFocusFlashWidth());
		}

		// Draw the thumbs
		cur_sldr_it = end;
		// Choose the color
		LLColor4 cur_thumb_col = LLUI::sMultiSliderThumbCenterColor;
		for (it = begin; it != end; ++it)
		{
			if (it->first == mCurSlider)
			{
				// Do not draw now, draw last
				cur_sldr_it = it;
				continue;
			}

			// The draw command
			LLUIImage::sRoundedSquare->drawSolid(it->second, cur_thumb_col);
		}
		// Draw current slider last
		if (cur_sldr_it != end)
		{
			LLUIImage::sRoundedSquare->drawSolid(cur_sldr_it->second,
												 LLUI::sMultiSliderThumbCenterSelectedColor);
		}
	}
	else
	{
		// Draw highlight
		if (hasFocus())
		{
			LLUIImage::sRoundedSquare->drawBorder(mThumbRects[mCurSlider],
												  gFocusMgr.getFocusColor(),
												  gFocusMgr.getFocusFlashWidth());
		}

		// Draw thumbs
		cur_sldr_it = end;
		// Choose the color
		LLColor4 cur_thumb_col = LLUI::sMultiSliderThumbCenterColor % opacity;
		for (it = begin; it != end; ++it)
		{
			if (it->first == mCurSlider)
			{
				cur_sldr_it = it;
				continue;
			}
			LLUIImage::sRoundedSquare->drawSolid(it->second, cur_thumb_col);
		}
		// Draw current slider last
		if (cur_sldr_it != end)
		{
			LLUIImage::sRoundedSquare->drawSolid(cur_sldr_it->second,
												 LLUI::sMultiSliderThumbCenterSelectedColor %
												 opacity);
		}
	}

	LLUICtrl::draw();
}

///////////////////////////////////////////////////////////////////////////////
// LLMultiSliderCtrl class proper
///////////////////////////////////////////////////////////////////////////////

LLMultiSliderCtrl::LLMultiSliderCtrl(const std::string& name,
									 const LLRect& rect,
						 			 const std::string& label,
									 const LLFontGL* fontp,
									 S32 label_width, S32 text_left,
									 bool show_text, bool can_edit_text,
									 void (*commit_callback)(LLUICtrl*, void*),
									 void* callback_user_data,
									 F32 initial_value, F32 min_value,
									 F32 max_value, F32 increment,
									 S32 max_sliders, F32 overlap_threshold,
									 bool allow_overlap, bool loop_overlap,
									 bool draw_track, bool use_triangle,
									 bool vertical, const char* ctrl_name)
:	LLUICtrl(name, rect, true, commit_callback, callback_user_data),
	mFont(fontp),
	mShowText(show_text),
	mCanEditText(can_edit_text),
	mPrecision(3),
	mLabelBox(NULL),
	mLabelWidth(label_width),
	mEditor(NULL),
	mTextBox(NULL),
	mTextEnabledColor(LLUI::sColorsGroup->getColor("LabelTextColor")),
	mTextDisabledColor(LLUI::sColorsGroup->getColor("LabelDisabledColor")),
	mSliderMouseUpCallback(NULL),
	mSliderMouseDownCallback(NULL)
{
	S32 top = getRect().getHeight();
	S32 bottom = 0;
	S32 left = 0;

	// Label
	if (!label.empty())
	{
		if (label_width == 0)
		{
			label_width = fontp->getWidth(label);
		}
		LLRect label_rect(left, top, label_width, bottom);
		mLabelBox = new LLTextBox("MultiSliderCtrl Label", label_rect, label,
								  fontp);
		addChild(mLabelBox);
	}

	S32 slider_right = getRect().getWidth();
	if (show_text)
	{
		slider_right = text_left - MULTI_SLIDERCTRL_SPACING;
	}

	S32 slider_left = label_width ? label_width + MULTI_SLIDERCTRL_SPACING : 0;
	LLRect slider_rect(slider_left, top, slider_right, bottom);
	mMultiSlider = new LLMultiSlider(LL_MULTI_SLIDER_CTRL_TAG, slider_rect,
									 LLMultiSliderCtrl::onSliderCommit, this,
									 initial_value, min_value, max_value,
									 increment, max_sliders, overlap_threshold,
									 allow_overlap, loop_overlap, draw_track,
									 use_triangle, vertical, ctrl_name);
	addChild(mMultiSlider);
	mCurValue = mMultiSlider->getCurSliderValue();

	if (show_text)
	{
		LLRect text_rect(text_left, top, getRect().getWidth(), bottom);
		if (can_edit_text)
		{
			mEditor = new LLLineEditor("MultiSliderCtrl Editor", text_rect,
									   LLStringUtil::null, fontp,
									   MAX_STRING_LENGTH,
									   &LLMultiSliderCtrl::onEditorCommit,
									   NULL, NULL, this,
									   &LLLineEditor::prevalidateFloat);
			mEditor->setFollowsLeft();
			mEditor->setFollowsBottom();
			mEditor->setFocusReceivedCallback(&LLMultiSliderCtrl::onEditorGainFocus,
											  this);
			mEditor->setIgnoreTab(true);
#if 0		// Do not do this, as selecting the entire text is single clicking
			// in some cases and double clicking in others:
			mEditor->setSelectAllonFocusReceived(true);
#endif
			addChild(mEditor);
		}
		else
		{
			mTextBox = new LLTextBox("MultiSliderCtrl Text", text_rect,
									 LLStringUtil::null, fontp);
			mTextBox->setFollowsLeft();
			mTextBox->setFollowsBottom();
			addChild(mTextBox);
		}
	}

	updateText();
}

//virtual
LLMultiSliderCtrl::~LLMultiSliderCtrl()
{
	// Children all cleaned up by default view destructor.
}

//static
void LLMultiSliderCtrl::onEditorGainFocus(LLFocusableElement* caller,
										  void* userdata)
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*) userdata;
	llassert(caller == self->mEditor);

	self->onFocusReceived();
}

//virtual
void LLMultiSliderCtrl::setValue(const LLSD& value)
{
	mMultiSlider->setValue(value);
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
}

void LLMultiSliderCtrl::setSliderValue(const std::string& name, F32 v,
									   bool from_event)
{
	mMultiSlider->setSliderValue(name, v, from_event);
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
}

void LLMultiSliderCtrl::setCurSlider(const std::string& name)
{
	mMultiSlider->setCurSlider(name);
	mCurValue = mMultiSlider->getCurSliderValue();
}

void LLMultiSliderCtrl::setLabel(const std::string& label)
{
	if (mLabelBox)
	{
		mLabelBox->setText(label);
	}
}

//virtual
bool LLMultiSliderCtrl::setLabelArg(const std::string& key,
									const std::string& text)
{
	bool res = false;
	if (mLabelBox)
	{
		res = mLabelBox->setTextArg(key, text);
		if (res && mLabelWidth == 0)
		{
			S32 label_width = mFont->getWidth(mLabelBox->getText());
			LLRect rect = mLabelBox->getRect();
			S32 prev_right = rect.mRight;
			rect.mRight = rect.mLeft + label_width;
			mLabelBox->setRect(rect);

			S32 delta = rect.mRight - prev_right;
			rect = mMultiSlider->getRect();
			S32 left = rect.mLeft + delta;
			left = llclamp(left, 0, rect.mRight - MULTI_SLIDERCTRL_SPACING);
			rect.mLeft = left;
			mMultiSlider->setRect(rect);
		}
	}
	return res;
}

const std::string& LLMultiSliderCtrl::addSlider()
{
	const std::string& name = mMultiSlider->addSlider();

	// If it returns null, pass it on
	if (name == LLStringUtil::null)
	{
		return LLStringUtil::null;
	}

	// Otherwise, update stuff
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
	return name;
}

const std::string& LLMultiSliderCtrl::addSlider(F32 val)
{
	const std::string& name = mMultiSlider->addSlider(val);
	// If it returns empty, pass it on
	if (name.empty())
	{
		return LLStringUtil::null;
	}

	// Otherwise, update stuff
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
	return name;
}

bool LLMultiSliderCtrl::addSlider(F32 val, const std::string& name)
{
	bool res = mMultiSlider->addSlider(val, name);
	if (res)
	{
		mCurValue = mMultiSlider->getCurSliderValue();
		updateText();
	}
	return res;
}

void LLMultiSliderCtrl::deleteSlider(const std::string& name)
{
	mMultiSlider->deleteSlider(name);
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
}

//virtual
void LLMultiSliderCtrl::clear()
{
	setCurSliderValue(0.f);
	if (mEditor)
	{
		mEditor->setText("");
	}
	if (mTextBox)
	{
		mTextBox->setText("");
	}

	// Get rid of sliders
	mMultiSlider->clear();
}

bool LLMultiSliderCtrl::isMouseHeldDown()
{
	return gFocusMgr.getMouseCapture() == mMultiSlider;
}

void LLMultiSliderCtrl::updateText()
{
	if (mEditor || mTextBox)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		// Do not display very small negative values as -0.000
		F32 displayed_value = floorf(getCurSliderValue() *
									 powf(10.f, mPrecision) + 0.5f) /
									 powf(10.f, mPrecision);

		std::string format = llformat("%%.%df", mPrecision);
		std::string text = llformat(format.c_str(), displayed_value);
		if (mEditor)
		{
			mEditor->setText(text);
		}
		else
		{
			mTextBox->setText(text);
		}
	}
}

//static
void LLMultiSliderCtrl::onEditorCommit(LLUICtrl* caller, void* userdata)
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*) userdata;
	llassert(caller == self->mEditor);

	F32 val = self->mCurValue;
	F32 saved_val = self->mCurValue;

	bool success = false;
	std::string text = self->mEditor->getText();
	if (LLLineEditor::postvalidateFloat(text))
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		val = (F32) atof(text.c_str());
		if (self->mMultiSlider->getMinValue() <= val &&
			val <= self->mMultiSlider->getMaxValue())
		{
			if (self->mValidateCallback)
			{
				// Set the value temporarily so that the callback can retrieve
				// it:
				self->setCurSliderValue(val);
				if (self->mValidateCallback(self, self->mCallbackUserData))
				{
					success = true;
				}
			}
			else
			{
				self->setCurSliderValue(val);
				success = true;
			}
		}
	}

	if (success)
	{
		self->onCommit();
	}
	else
	{
		if (self->getCurSliderValue() != saved_val)
		{
			self->setCurSliderValue(saved_val);
		}
		self->reportInvalidData();
	}
	self->updateText();
}

//static
void LLMultiSliderCtrl::onSliderCommit(LLUICtrl* caller, void* userdata)
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*)userdata;

	F32 saved_val = self->mCurValue;
	F32 new_val = self->mMultiSlider->getCurSliderValue();

	bool success = false;
	if (self->mValidateCallback)
	{
		// Set the value temporarily so that the callback can retrieve it:
		self->mCurValue = new_val;
		if (self->mValidateCallback(self, self->mCallbackUserData))
		{
			success = true;
		}
	}
	else
	{
		self->mCurValue = new_val;
		success = true;
	}

	if (success)
	{
		self->onCommit();
	}
	else
	{
		if (self->mCurValue != saved_val)
		{
			self->setCurSliderValue(saved_val);
		}
		self->reportInvalidData();
	}
	self->updateText();
}

//virtual
void LLMultiSliderCtrl::setEnabled(bool b)
{
	LLUICtrl::setEnabled(b);

	if (mLabelBox)
	{
		mLabelBox->setColor(b ? mTextEnabledColor : mTextDisabledColor);
	}

	mMultiSlider->setEnabled(b);

	if (mEditor)
	{
		mEditor->setEnabled(b);
	}

	if (mTextBox)
	{
		mTextBox->setColor(b ? mTextEnabledColor : mTextDisabledColor);
	}
}

//virtual
void LLMultiSliderCtrl::setTentative(bool b)
{
	if (mEditor)
	{
		mEditor->setTentative(b);
	}
	LLUICtrl::setTentative(b);
}

//virtual
void LLMultiSliderCtrl::onCommit()
{
	setTentative(false);

	if (mEditor)
	{
		mEditor->setTentative(false);
	}

	LLUICtrl::onCommit();
}

void LLMultiSliderCtrl::setPrecision(S32 precision)
{
	if (precision < 0 || precision > 10)
	{
		S32 p = precision;
		precision = llclamp(precision, 0, 10);
		llwarns << "Precision out of range: " << p << " - clamped to: "
				<< precision << llendl;
	}

	mPrecision = precision;
	updateText();
}

void LLMultiSliderCtrl::setSliderMouseDownCallback(void (*cb)(S32, S32,
															  void* userdata))
{
	mSliderMouseDownCallback = cb;
	mMultiSlider->setMouseDownCallback(LLMultiSliderCtrl::onSliderMouseDown);
}

//static
void LLMultiSliderCtrl::onSliderMouseDown(S32 x, S32 y, void* userdata)
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*)userdata;
	if (self->mSliderMouseDownCallback)
	{
		self->mSliderMouseDownCallback(x, y, self->mCallbackUserData);
	}
}

void LLMultiSliderCtrl::setSliderMouseUpCallback(void (*cb)(S32, S32,
															void* userdata))
{
	mSliderMouseUpCallback = cb;
	mMultiSlider->setMouseUpCallback(LLMultiSliderCtrl::onSliderMouseUp);
}

//static
void LLMultiSliderCtrl::onSliderMouseUp(S32 x, S32 y, void* userdata)
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*)userdata;
	if (self->mSliderMouseUpCallback)
	{
		self->mSliderMouseUpCallback(x, y, self->mCallbackUserData);
	}
}

//virtual
void LLMultiSliderCtrl::onTabInto()
{
	if (mEditor)
	{
		mEditor->onTabInto();
	}
}

void LLMultiSliderCtrl::reportInvalidData()
{
	make_ui_sound("UISndBadKeystroke");
}

//virtual
const std::string& LLMultiSliderCtrl::getControlName() const
{
	return mMultiSlider->getControlName();
}

//virtual
void LLMultiSliderCtrl::setControlName(const char* ctr_name, LLView* context)
{
	mMultiSlider->setControlName(ctr_name, context);
}

//virtual
const std::string& LLMultiSliderCtrl::getTag() const
{
	return LL_MULTI_SLIDER_CTRL_TAG;
}

//virtual
LLXMLNodePtr LLMultiSliderCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->setName(LL_MULTI_SLIDER_CTRL_TAG);

	node->createChild("show_text", true)->setBoolValue(mShowText);
	node->createChild("can_edit_text", true)->setBoolValue(mCanEditText);
	node->createChild("decimal_digits", true)->setIntValue(mPrecision);

	if (mLabelBox)
	{
		node->createChild("label", true)->setStringValue(mLabelBox->getText());
	}

	// TomY TODO: Do we really want to export the transient state of the
	// slider ?
	node->createChild("value", true)->setFloatValue(mCurValue);

	node->createChild("initial_val",
					  true)->setFloatValue(mMultiSlider->getInitialValue());
	node->createChild("min_val",
					  true)->setFloatValue(mMultiSlider->getMinValue());
	node->createChild("max_val",
					  true)->setFloatValue(mMultiSlider->getMaxValue());
	node->createChild("increment",
					  true)->setFloatValue(mMultiSlider->getIncrement());
	node->createChild("max_sliders",
					  true)->setFloatValue(mMultiSlider->mMaxNumSliders);
	if (mMultiSlider->mOverlapThreshold)
	{
		F32 actual = mMultiSlider->mOverlapThreshold +
					 mMultiSlider->getIncrement();
		node->createChild("overlap_threshold", true)->setFloatValue(actual);
	}
	if (mMultiSlider->mAllowOverlap)
	{
		node->createChild("allow_overlap", true)->setBoolValue(true);
	}
	if (mMultiSlider->mLoopOverlap)
	{
		node->createChild("loop_overlap", true)->setBoolValue(true);
	}
	if (!mMultiSlider->mDrawTrack)
	{
		node->createChild("draw_track", true)->setBoolValue(false);
	}
	if (mMultiSlider->mUseTriangle)
	{
		node->createChild("use_triangle", true)->setBoolValue(true);
	}
	if (mMultiSlider->mVertical)
	{
		node->createChild("orientation", true)->setStringValue("vertical");
	}

	addColorXML(node, mTextEnabledColor, "text_enabled_color",
				"LabelTextColor");
	addColorXML(node, mTextDisabledColor, "text_disabled_color",
				"LabelDisabledColor");

	return node;
}

//virtual
LLView* LLMultiSliderCtrl::fromXML(LLXMLNodePtr node, LLView* parent,
								   LLUICtrlFactory* factory)
{
	std::string name = LL_MULTI_SLIDER_CTRL_TAG;
	node->getAttributeString("name", name);

	std::string label;
	node->getAttributeString("label", label);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLFontGL* fontp = LLView::selectFont(node);
	// *HACK: font might not be specified.
	if (!fontp)
	{
		fontp = LLFontGL::getFontSansSerifSmall();
	}

	S32 label_width = 0;
	node->getAttributeS32("label_width", label_width);

	bool show_text = true;
	node->getAttributeBool("show_text", show_text);

	bool can_edit_text = false;
	node->getAttributeBool("can_edit_text", can_edit_text);

	F32 overlap_threshold = 0.f;
	node->getAttributeF32("overlap_threshold", overlap_threshold);

	bool allow_overlap = false;
	node->getAttributeBool("allow_overlap", allow_overlap);

	bool loop_overlap = false;
	node->getAttributeBool("loop_overlap", loop_overlap);

	bool draw_track = true;
	node->getAttributeBool("draw_track", draw_track);

	bool use_triangle = false;
	node->getAttributeBool("use_triangle", use_triangle);

	F32 initial_value = 0.f;
	node->getAttributeF32("initial_val", initial_value);

	F32 min_value = 0.f;
	node->getAttributeF32("min_val", min_value);

	F32 max_value = 1.f;
	node->getAttributeF32("max_val", max_value);

	F32 increment = 0.1f;
	node->getAttributeF32("increment", increment);

	U32 precision = 3;
	node->getAttributeU32("decimal_digits", precision);

	S32 max_sliders = 1;
	node->getAttributeS32("max_sliders", max_sliders);

	std::string orientation;
	node->getAttributeString("orientation", orientation);

	S32 text_left = 0;
	if (show_text)
	{
		// Calculate the size of the text box (log max_value is number of
		// digits - 1 so plus 1)
		if (max_value)
		{
			text_left = fontp->getWidth("0") *
						(static_cast<S32>(log10f(max_value)) + precision + 1);
		}

		if (increment < 1.f)
		{
			// (mostly) take account of decimal point in value
			text_left += fontp->getWidth(".");
		}

		if (min_value < 0.f || max_value < 0.f)
		{
			// (mostly) take account of minus sign
			text_left += fontp->getWidth("-");
		}

		// Padding to make things look nicer
		text_left += 8;
	}

	LLUICtrlCallback callback = NULL;

	if (label.empty())
	{
		label.assign(node->getTextContents());
	}

	LLMultiSliderCtrl* slider =
		new LLMultiSliderCtrl(name, rect, label, fontp, label_width,
							  rect.getWidth() - text_left, show_text,
							  can_edit_text, callback, NULL, initial_value,
							  min_value, max_value, increment, max_sliders,
							  overlap_threshold, allow_overlap, loop_overlap,
							  draw_track, use_triangle,
							  orientation == "vertical");

	slider->setPrecision(precision);

	slider->initFromXML(node, parent);

	slider->updateText();

	return slider;
}
