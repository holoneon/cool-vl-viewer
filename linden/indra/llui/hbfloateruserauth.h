/** 
 * @file hbfloateruserauth.h
 * @brief The HBFloaterUserAuth class declaration
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * 
 * Copyright (c) 2015, Henri Beauchamp
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

class LLButton;
class LLLineEditor;

class HBFloaterUserAuth final : public LLFloater
{
public:
	typedef void (*HBFloaterUserAuthCallback)(const LLUUID auth_id,
											  const std::string username,
											  const std::string password,
											  bool validated);

	~HBFloaterUserAuth() override;

	bool postBuild() override;
	void draw() override;

	static void request(const std::string& host, const std::string& realm,
						const LLUUID& auth_id,
						HBFloaterUserAuthCallback callback);

private:
	HBFloaterUserAuth(const std::string& host, const std::string& realm,
					  const LLUUID& auth_id,
					  HBFloaterUserAuthCallback callback);

	void doCallback(bool validated);

	static void onButtonOK(void* user_data);
	static void onButtonCancel(void* user_data);
	static void onCommitCheckBox(LLUICtrl* ctrl, void* user_data);
	static bool onHandleKeyCallback(KEY key, MASK mask, LLLineEditor* caller,
									void* user_data);
	static void onKeystrokeCallback(LLLineEditor*, void* user_data);

private:
	void				(*mUserAuthCallback)(const LLUUID auth_id,
											 const std::string username,
											 const std::string password,
											 bool validated);

	LLButton*			mOKBtn;
	LLLineEditor*		mUserNameInputLine;
	LLLineEditor*		mPasswordInputLine;

	LLUUID				mAuthId;

	std::string			mHost;
	std::string			mRealm;

	bool				mMustClose;
	bool				mCallbackDone;

	static uuid_list_t	sAuthList;
};
