/**
 * @file llchatbar.h
 * @brief LLChatBar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llpanel.h"
#include "llframetimer.h"

#include "llchat.h"
#include "llviewercontrol.h"

class LLButton;
class LLChatBarGestureObserver;
class LLComboBox;
class LLFlyoutButton;
class LLFrameTimer;
class LLLineEditor;
class LLUICtrl;

constexpr S32 CHAT_BAR_HEIGHT = 28;

class LLChatBar final : public LLPanel
{
protected:
	LOG_CLASS(LLChatBar);

public:
	LLChatBar(const std::string& name);
	LLChatBar(const std::string& name, const LLRect& rect);
		~LLChatBar() override;

	bool postBuild() override;

	void reshape(S32 width, S32 height, bool called_from_parent) override;
	void refresh() override;

	bool handleKeyHere(KEY key, MASK mask) override;

	// Adjust buttons and input field for width
	void layout();

	void refreshGestures();

	// Move cursor into chat input field.
	void setKeyboardFocus(bool b);

	// Ignore arrow keys for chat bar
	void setIgnoreArrowKeys(bool b);

	bool hasTextEditor() const;
	bool inputEditorHasFocus() const;
	bool hasChatText() const;

	// Since chat bar logic is reused for chat history gesture combo box might
	// not be a direct child
	void setGestureCombo(LLComboBox* combo);

	// Send a chat (after stripping /20foo channel chats). "animate" triggers
	// the nodding, whispering or shouting animations.
	void sendChatFromViewer(LLWString wtext, EChatType type, bool animate,
							bool lua_propagate = true);
	void sendChatFromViewer(const std::string& utf8text, EChatType type,
							bool animate, bool lua_propagate = true);

	// If input of the form "/20foo" or "/20 foo", returns "foo" and channel
	// 20. Otherwise returns input and channel 0.
	LLWString stripChannelNumber(const LLWString& mesg, S32* channel);

	static void startChat(const char* line);
	static void stopChat();

	static std::string getMatchingAvatarName(const std::string& match);

private:
	void setVisible(bool visible) override;

	void setMaxTextLength();

	void sendChat(EChatType type);
	void updateChat();

	static void toggleChatHistory(void*);

	static void	onClickSay(LLUICtrl*, void* userdata);
	static void onClickOpenTextEditor(void* userdata);

	static void	onTabClick(void* userdata);
	static void	onInputEditorKeystroke(LLLineEditor* caller, void* userdata);
	static void	onInputEditorScrolled(LLLineEditor* caller, void* userdata);
	static void	onInputEditorFocusLost(LLFocusableElement* caller, void*);
	static void	onInputEditorGainFocus(LLFocusableElement* caller, void*);

	static void onCommitGesture(LLUICtrl* ctrl, void* data);

private:
	LLButton*					mOpenTextEditorButton;
	LLButton*					mHistoryButton;
	LLComboBox*					mGestureCombo;
	LLFlyoutButton*				mSayFlyoutButton;
	LLLineEditor*				mInputEditor;

	LLFrameTimer				mGestureLabelTimer;

	// Which non-zero channel did we last chat on?
	S32							mLastSpecialChatChannel;

	LLChatBarGestureObserver*	mObserver;

	bool 						mSecondary;
	bool						mIsBuilt;
	bool						mHasScrolledOnce;
	bool						mLastSwappedShortcuts;

	static strings_set_t		sIgnoredNames;

public:
	static bool					sSwappedShortcuts;
};

extern LLChatBar* gChatBarp;
