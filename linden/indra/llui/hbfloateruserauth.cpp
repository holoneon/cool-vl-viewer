/** 
 * @file hbfloateruserauth.cpp
 * @brief The HBFloaterUserAuth class definition
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

#include "linden_common.h"

#include "hbfloateruserauth.h"

#include "llapp.h"				// For isQuitting()
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"

// NOTE: we allow an empty password field, since it might be a valid login.
#define ALLOW_EMPTY_PASSWORD 1

//static
uuid_list_t HBFloaterUserAuth::sAuthList;

//static
void HBFloaterUserAuth::request(const std::string& host,
								const std::string& realm,
								const LLUUID& auth_id,
								HBFloaterUserAuthCallback callback)
{
	if (!sAuthList.count(auth_id))
	{
		(void)new HBFloaterUserAuth(host, realm, auth_id, callback);
	}
}

HBFloaterUserAuth::HBFloaterUserAuth(const std::string& host,
									 const std::string& realm,
									 const LLUUID& auth_id,
									 HBFloaterUserAuthCallback callback)
:	mUserAuthCallback(callback),
	mAuthId(auth_id),
	mHost(host),
	mRealm(realm),
	mMustClose(false),
	mCallbackDone(false)
{
	sAuthList.emplace(auth_id);
    LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_user_auth.xml");
}

//virtual
HBFloaterUserAuth::~HBFloaterUserAuth()
{
	if (!mCallbackDone && !LLApp::isQuitting())
	{
		doCallback(false);
	}
	sAuthList.erase(mAuthId);
}

//virtual
bool HBFloaterUserAuth::postBuild()
{
	mUserNameInputLine = getChild<LLLineEditor>("user_name");
	mUserNameInputLine->setOnHandleKeyCallback(onHandleKeyCallback, this);
	mUserNameInputLine->setKeystrokeCallback(onKeystrokeCallback);
	mUserNameInputLine->setCallbackUserData(this);

	mPasswordInputLine = getChild<LLLineEditor>("password");
	mPasswordInputLine->setOnHandleKeyCallback(onHandleKeyCallback, this);
	mPasswordInputLine->setCallbackUserData(this);
	mPasswordInputLine->setDrawAsterixes(true);

	childSetCommitCallback("show_password", onCommitCheckBox, this);

	mOKBtn = getChild<LLButton>("ok");
	mOKBtn->setClickedCallback(onButtonOK, this);
	mOKBtn->setEnabled(false);

	childSetAction("cancel", onButtonCancel, this);

	setTitle(getTitle() + " " + mHost);

	childSetTextArg("prompt_text", "[REALM]", mRealm);

	center();

	return true;
}

//virtual
void HBFloaterUserAuth::draw()
{
	if (mMustClose)
	{
		onButtonOK(this);
	}
	else
	{
		LLFloater::draw();
	}
}

void HBFloaterUserAuth::doCallback(bool validated)
{
	if (!mCallbackDone)
	{
		if (mUserAuthCallback)
		{
			mUserAuthCallback(mAuthId, mUserNameInputLine->getText(),
							  mPasswordInputLine->getText(), validated);
		}
		mCallbackDone = true;
	}
}

//static
void HBFloaterUserAuth::onButtonOK(void* user_data)
{
	HBFloaterUserAuth* self = (HBFloaterUserAuth*)user_data;
	if (self)
	{
		self->doCallback(true);
		self->close();
	}
}

//static
void HBFloaterUserAuth::onButtonCancel(void* user_data)
{
	HBFloaterUserAuth* self = (HBFloaterUserAuth*)user_data;
	if (self)
	{
		self->close();
	}
}

//static
void HBFloaterUserAuth::onCommitCheckBox(LLUICtrl* ctrl, void* user_data)
{
	HBFloaterUserAuth* self = (HBFloaterUserAuth*)user_data;
	if (self && ctrl)
	{
		LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
		self->mPasswordInputLine->setDrawAsterixes(!check->get());
#if 0	// Does not work...
		self->mPasswordInputLine->setFocus(true);
#endif
	}
}

//static
bool HBFloaterUserAuth::onHandleKeyCallback(KEY key, MASK mask,
											LLLineEditor* caller,
											void* user_data)
{
	bool handled = false;

	HBFloaterUserAuth* self = (HBFloaterUserAuth*)user_data;
	if (self && key == KEY_RETURN && mask == MASK_NONE &&
		!self->mUserNameInputLine->getText().empty())
	{
		if (caller == self->mUserNameInputLine)
		{
			self->mPasswordInputLine->setFocus(true);
			handled = true;
		}
#if ALLOW_EMPTY_PASSWORD
		else
#else
		else if (!self->mPasswordInputLine->getText().empty())
#endif
		{
			self->mMustClose = true;
			handled = true;
		}
	}

	return handled;
}

//static
void HBFloaterUserAuth::onKeystrokeCallback(LLLineEditor*, void* user_data)
{
	HBFloaterUserAuth* self = (HBFloaterUserAuth*)user_data;
	if (self)
	{
		bool cant_validate =
#if !ALLOW_EMPTY_PASSWORD
		self->mPasswordInputLine->getText().empty()) ||
#endif
		self->mUserNameInputLine->getText().empty();

		self->mOKBtn->setEnabled(!cant_validate);
	}
}
