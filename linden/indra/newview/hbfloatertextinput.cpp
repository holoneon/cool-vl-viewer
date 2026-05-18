/**
 * @file hbfloatertextinput.cpp
 * @brief HBFloaterTextInput class implementation
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012-2025, Henri Beauchamp.
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

#include "hbfloatertextinput.h"

#include "lllineeditor.h"
#include "lltexteditor.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llchatbar.h"
#include "llviewercontrol.h"

//static
std::map<LLLineEditor*, HBFloaterTextInput*>  HBFloaterTextInput::sInstancesMap;

HBFloaterTextInput::HBFloaterTextInput(LLLineEditor* input_linep,
									   const std::string& dest,
									   void (*typing_callback)(void*, bool),
									   void* callback_datap)
:	LLFloater("text input"),
	mCallerLineEditor(input_linep),
	mIsChatInput(dest.empty()),
	mTypingCallback(typing_callback),
	mTypingCallbackData(callback_datap),
	mMustClose(false)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_text_input.xml");
	std::string title;
	if (mIsChatInput)
	{
		title = getString("chat");
		mRectControl = "ChatInputEditorRect";
	}
	else
	{
		LLStringUtil::format_map_t arg;
		arg["[NAME]"] = dest;
		title = getString("im", arg);
		mRectControl = "IMInputEditorRect";
	}
	setTitle(title);

	LLRect rect = gSavedSettings.getRect(mRectControl.c_str());
	reshape(rect.getWidth(), rect.getHeight());
	setRect(rect);

	sInstancesMap.emplace(input_linep, this);
}

//virtual
HBFloaterTextInput::~HBFloaterTextInput()
{
	if (mCallerLineEditor)
	{
		std::map<LLLineEditor*, HBFloaterTextInput*>::iterator it;
		it = sInstancesMap.find(mCallerLineEditor);
		if (it != sInstancesMap.end())
		{
			sInstancesMap.erase(it);
		}
		else
		{
			llwarns << "Could not find the floater in the instances map"
					<< llendl;
		}
		mCallerLineEditor->setText(mTextEditor->getText());
		mCallerLineEditor->setCursorToEnd();
		mCallerLineEditor->setFocus(true);
		if (mIsChatInput)
		{
			gAgent.stopTyping();
		}
		else if (mTypingCallback)
		{
			mTypingCallback(mTypingCallbackData, false);
		}
	}

	gSavedSettings.setRect(mRectControl.c_str(), getRect());
}

//virtual
bool HBFloaterTextInput::postBuild()
{
	mTextEditor = getChild<LLTextEditor>("text");
	mTextEditor->setFocusLostCallback(onTextEditorFocusLost, this);
	mTextEditor->setKeystrokeCallback(onTextEditorKeystroke, this);
	mTextEditor->setOnHandleKeyCallback(onHandleKeyCallback, this);
	mTextEditor->setFocus(true);
	mTextEditor->setCustomMenuType("text_input");
	if (mIsChatInput)
	{
		if (gSavedSettings.getBool("TabAutoCompleteName"))
		{
			mTextEditor->setTabsToNextField(false);
		}
		U32 len = llclamp((U32)gSavedSettings.getU32("ChatInputMaxCharacters"),
						  DB_CHAT_MSG_STR_LEN, 4 * DB_CHAT_MSG_STR_LEN - 40);
		mTextEditor->setMaxLength(len);
	}
	if (mCallerLineEditor)
	{
		mTextEditor->setText(mCallerLineEditor->getText());
		mTextEditor->setCursorPos(mCallerLineEditor->getCursor());
	}
	return true;
}

//virtual
void HBFloaterTextInput::draw()
{
	if (mMustClose)
	{
		close();
	}
	else
	{
		LLFloater::draw();
	}
}

//static
HBFloaterTextInput* HBFloaterTextInput::show(LLLineEditor* input_linep,
											 const std::string& dest,
									 		 void (*typing_callback)(void*, bool),
											 void* callback_datap)
{
	HBFloaterTextInput* instance = NULL;

	std::map<LLLineEditor*, HBFloaterTextInput*>::iterator it;
	it = sInstancesMap.find(input_linep);
	if (it != sInstancesMap.end())
	{
		instance = it->second;
		instance->setFocus(true);
		instance->open();
	}
	else
	{
		instance = new HBFloaterTextInput(input_linep, dest, typing_callback,
										  callback_datap);
	}

	return instance;
}

// This method *must* be invoked by the caller of show() when it gets
// destroyed. It ensures the text input floater gets destroyed in its turn
// and does not attempt to call back methods pertaining to the destroyed
// caller object or its children.
//static
void HBFloaterTextInput::abort(LLLineEditor* input_linep)
{
	std::map<LLLineEditor*, HBFloaterTextInput*>::iterator it;
	it = sInstancesMap.find(input_linep);
	if (it != sInstancesMap.end())
	{
		HBFloaterTextInput* self = it->second;
		self->mCallerLineEditor = NULL;
		self->close();
		sInstancesMap.erase(it);
	}
}

//static
bool HBFloaterTextInput::hasFloaterFor(LLLineEditor* input_linep)
{
	return sInstancesMap.find(input_linep) != sInstancesMap.end();
}

//static
void HBFloaterTextInput::onTextEditorFocusLost(LLFocusableElement*,
											   void* userdatap)
{
	HBFloaterTextInput* self = (HBFloaterTextInput*)userdatap;
	if (self)
	{
		if (self->mIsChatInput)
		{
			gAgent.stopTyping();
		}
		else if (self->mTypingCallback)
		{
			self->mTypingCallback(self->mTypingCallbackData, false);
		}
	}
}

//static
void HBFloaterTextInput::onTextEditorKeystroke(LLTextEditor*, void* userdatap)
{
	HBFloaterTextInput* self = (HBFloaterTextInput*)userdatap;
	if (self)
	{
		if (self->mIsChatInput)
		{
			std::string text = self->mTextEditor->getText();
			if (text.length() > 0 && text[0] != '/')
			{
				gAgent.startTyping();
			}
		}
		else if (self->mTypingCallback)
		{
			self->mTypingCallback(self->mTypingCallbackData, true);
		}
	}
}

//static
bool HBFloaterTextInput::onHandleKeyCallback(KEY key, MASK mask, LLTextEditor*,
											 void* userdatap)
{
	bool handled = false;
	HBFloaterTextInput* self = (HBFloaterTextInput*)userdatap;
	if (self)
	{
		if (key == KEY_RETURN)
		{
			if (mask == MASK_NONE)
			{
				// Flag for closing. We cannot close now because then we would
				// destroy the object to which pertains the method that called
				// us...
				self->mMustClose = true;
				handled = true;
			}
			else if (mask == (MASK_SHIFT | MASK_CONTROL))
			{
				S32 cursor = self->mTextEditor->getCursorPos();
				std::string text = self->mTextEditor->getText();
				// For some reason, the event is triggered twice: let's insert
				// only one newline character.
				if (cursor == 0 || text[cursor - 1] != '\n')
				{
					text = text.insert(cursor, "\n");
					self->mTextEditor->setText(text);
					self->mTextEditor->setCursorPos(cursor + 1);
				}
				handled = true;
			}
		}
		else if (KEY_TAB == key && mask == MASK_NONE)
		{
			std::string text = self->mTextEditor->getText();
			S32 word_start = 0;
			S32 word_len = 0;
			S32 cursor = self->mTextEditor->getCursorPos();
			S32 pos = cursor;
			if (pos > 0 && pos != (S32)text.length() - 1)
			{
				// Make sure the word will be found if the cursor is at its
				// end
				--pos;
			}
			if (self->mTextEditor->getWordBoundriesAt(pos, &word_start,
													  &word_len))
			{
				std::string word = text.substr(word_start, word_len);
				std::string suggestion = LLChatBar::getMatchingAvatarName(word);
				if (suggestion != word)
				{
					text = text.replace(word_start, word_len, suggestion);
					self->mTextEditor->setText(text);
					S32 word_end = cursor + suggestion.length() -
								   word.length();
					if (gSavedSettings.getBool("SelectAutoCompletedPart"))
					{
						self->mTextEditor->setSelection(cursor, word_end);
					}
					else
					{
						self->mTextEditor->setCursorPos(word_end);
					}
				}
			}
			handled = true;
		}
	}

	return handled;
}
