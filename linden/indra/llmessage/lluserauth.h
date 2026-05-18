/**
 * @file lluserauth.h
 * @brief LLUserAuth class header file
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
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

#include <map>
#include <string>
#include <vector>

class LLXMLRPCTransaction;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This class encapsulates the authentication and initialization from the login
// server. Construct an instance of this object, and call the authenticate()
// method, and call authResponse() until it returns a non-negative value. If
// that method returns E_OK, you can start asking for responses via the
// getResponse*() methods.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLUserAuth
{
protected:
	LOG_CLASS(LLUserAuth);

public:
	LLUserAuth();
	~LLUserAuth();

	// These codes map to the curl return codes...
	typedef enum {
		E_NO_RESPONSE_YET = -2,
		E_DOWNLOADING = -1,
		E_OK = 0,
		E_COULDNT_RESOLVE_HOST,
		E_SSL_PEER_CERTIFICATE,
		E_SSL_CACERT,
		E_SSL_CONNECT_ERROR,
		E_UNHANDLED_ERROR,
		E_LAST						// Never use this !
	} UserAuthcode;

	// Clears out internal data cache.
	void reset();

	// Used in llappviewer.cpp to transmit all the constant data to us. HB
	void init(const std::string& platform_ver, const std::string& os_string,
			  const std::string& viewer_version, const std::string& channel,
			  const std::string& serial_hash, const std::string& mac_hash);

	// Used in llstartup.cpp to transmit the MFA token and MFA hash prior to
	// authentication. HB
	void setMFA(bool use_mfa, const std::string& mfa_hash,
				const std::string& mfa_token);

	void authenticate(const std::string& auth_uri,
					  const std::string& auth_method,
					  const std::string& firstname,
					  const std::string& lastname,
					  const std::string& password,
					  const std::string& start,
					  bool skip_optional_update, bool accept_tos,
					  bool accept_critical_message,
					  S32 last_exec_event,
					  const std::vector<const char*>& requested_options);

	UserAuthcode authResponse();

	LL_INLINE const std::string& errorMessage() const
	{
		return mErrorMessage;
	}

	LL_INLINE const LLSD& getResponse() const		{ return mResponses; }

	// Method to get a direct reponse from the login API by name.
	LL_INLINE const LLSD& getResponse(const std::string& name) const
	{
		return mResponses[name];
	}

	LL_INLINE std::string getResponseStr(const std::string& name) const
	{
		return mResponses.has(name) ? mResponses[name].asString() : "";
	}

	// Returns the mResponses[name][0] LLSD map when it exists. HB
	const LLSD& getResponse1stMap(const std::string& name) const;

private:
	LLXMLRPCTransaction*	mTransaction;

	std::string				mPlatformVersion;
	std::string				mPlatformOSString;

	std::string				mViewerVersion;
	std::string				mViewerChannel;

	std::string				mHashedSerial;
	std::string				mHashedMAC;

	std::string				mMFAHash;
	std::string				mMFAToken;

	std::string				mErrorMessage;
	std::string				mIndentation;

	LLSD					mResponses;

	UserAuthcode			mAuthResponse;

	bool					mUseMFA;
};

extern LLUserAuth gUserAuth;
