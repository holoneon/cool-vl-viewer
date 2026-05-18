/**
 * @file hbfloatertextinput.h
 * @brief HBFloaterTextInput class definition
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

#pragma once

#include "llfloater.h"

class LLLineEditor;
class LLTextEditor;

class HBFloaterTextInput final : public LLFloater
{
protected:
	LOG_CLASS(HBFloaterTextInput);

public:
	HBFloaterTextInput(LLLineEditor* input_linep, const std::string& dest,
					   void (*typing_callback)(void*, bool),
					   void* callback_datap);
	~HBFloaterTextInput() override;

	static HBFloaterTextInput* show(LLLineEditor* input_linep,
									const std::string& dest = LLStringUtil::null,
									void (*typing_callback)(void*, bool) = NULL,
									void* callback_datap = NULL);

	static void abort(LLLineEditor* input_linep);

	static bool hasFloaterFor(LLLineEditor* input_linep);

private:
	bool postBuild() override;
	void draw() override;

	static void onTextEditorFocusLost(LLFocusableElement* callerp,
									  void* userdatap);
	static void onTextEditorKeystroke(LLTextEditor* callerp, void* userdatap);
	static bool onHandleKeyCallback(KEY key, MASK mask, LLTextEditor* callerp,
									void* userdatap);

private:
	static std::map<LLLineEditor*, HBFloaterTextInput*> sInstancesMap;

	void			(*mTypingCallback)(void*, bool);
	void*			mTypingCallbackData;

	LLLineEditor*	mCallerLineEditor;
	LLTextEditor*	mTextEditor;

	std::string 	mRectControl;
	bool			mIsChatInput;
	bool			mMustClose;
};
