/**
 * @file lluserauth.cpp
 * @brief LLUserAuth class implementation
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

#include "linden_common.h"

#include "curl/curl.h"

#include "lluserauth.h"

#include "llsdutil.h"				// For ll_pretty_print_sd
#include "llxmlrpctransaction.h"

// Do not define PLATFORM_STRING for unknown platforms: they need to get added
// to the login.cgi script, so we want this to cause an error if we get
// compiled for a different platform.
#if LL_WINDOWS
static const char* PLATFORM_STRING = "Win";
#elif LL_LINUX
static const char* PLATFORM_STRING = "Lnx";
#else
# error("Unknown platform !")
#endif

LLUserAuth gUserAuth;

LLUserAuth::LLUserAuth()
:	mTransaction(NULL),
	mAuthResponse(E_NO_RESPONSE_YET),
	mUseMFA(false)
{
}

LLUserAuth::~LLUserAuth()
{
	reset();
}

void LLUserAuth::reset()
{
	delete mTransaction;
	mTransaction = NULL;
	mResponses.clear();
}

void LLUserAuth::init(const std::string& platform_version,
					  const std::string& os_string,
					  const std::string& version, const std::string& channel,
					  const std::string& serial_hash,
					  const std::string& mac_hash)
{
	mPlatformVersion = platform_version;
	mPlatformOSString = os_string;
	mViewerVersion = version;
	mViewerChannel = channel;
	mHashedSerial = serial_hash;
	mHashedMAC = mac_hash;
}

void LLUserAuth::setMFA(bool use_mfa, const std::string& mfa_hash,
						const std::string& mfa_token)
{
	mUseMFA = use_mfa;
	if (!use_mfa)
	{
		mMFAHash.clear();
		mMFAToken.clear();
	}
	// When replying to a MFA challenge (i.e. the token string is not empty),
	// we pass the token and an empty hash. Else, we use any last known good
	// MFA hash we got, with an empty token. HB
	// See: https://wiki.secondlife.com/wiki/User:Brad_Linden/Login_MFA
	else if (mfa_token.empty())
	{
		mMFAHash = mfa_hash;
		mMFAToken.clear();
	}
	else
	{
		mMFAHash.clear();
		mMFAToken = mfa_token;
	}
}

void LLUserAuth::authenticate(const std::string& auth_uri,
							  const std::string& method,
							  const std::string& firstname,
							  const std::string& lastname,
							  const std::string& passwd,
							  const std::string& start,
							  bool skip_optional,
							  bool accept_tos, bool accept_critical_message,
							  S32 last_exec_event,
							  const std::vector<const char*>& req_options)
{
	if (mHashedSerial.empty())
	{
		llerrs << "LLUserAuth was not properly initialized !" << llendl;
	}

	llinfos << "Authenticating: " << firstname << " " << lastname << llendl;
	llinfos << "Start place: " << start << llendl;

	// NOTE: passwd is already MD5 hashed by the time we get to it.
	std::string dpasswd("$1$");
	dpasswd.append(passwd);

	std::ostringstream option_str;
	option_str << "Options: ";
	std::ostream_iterator<const char*> appender(option_str, ", ");
	std::copy(req_options.begin(), req_options.end(), appender);
	option_str << "END";

	llinfos << option_str.str().c_str() << llendl;

	mResponses.clear();
	mAuthResponse = E_NO_RESPONSE_YET;
	//mDownloadTimer.reset();

	// Create the request parameters bloc
	LLSD params;
	params["first"] = firstname;
	params["last"] = lastname;
	params["passwd"] = dpasswd;
	params["start"] = start;
	params["version"] = mViewerVersion;
	params["channel"] = mViewerChannel;
	params["platform"] = PLATFORM_STRING;
	params["address_size"] = 64;
	params["platform_version"] = mPlatformVersion;
	params["platform_string"] = mPlatformOSString;
	params["mac"] = mHashedMAC;
	params["id0"] = mHashedSerial;
	if (mUseMFA)
	{
		params["mfa_hash"] = mMFAHash;
		params["token"] = mMFAToken;
	}
	if (skip_optional)
	{
		params["skipoptional"] = "true";
	}
	if (accept_tos)
	{
		params["agree_to_tos"] = "true";
	}
	if (accept_critical_message)
	{
		params["read_critical"] = "true";
	}
	params["last_exec_event"] = last_exec_event;

	// Append optional requests in an array
	LLSD options;
	for (std::vector<const char*>::const_iterator it = req_options.begin(),
												  end = req_options.end();
		 it < end; ++it)
	{
		options.append(*it);
	}
	params.insert("options", options);

#if LL_DEBUG
	// Note: this shows password and MFA hashes, so only enabled for debug
	// builds. HB
	LL_DEBUGS("UserAuth") << "Request LLSD:\n" << ll_pretty_print_sd(params)
						  << LL_ENDL;
#endif

	if (mTransaction)
	{
		delete mTransaction;
	}
	mTransaction = new LLXMLRPCTransaction(auth_uri, method, params);

	llinfos << "URI: " << auth_uri << llendl;
}

LLUserAuth::UserAuthcode LLUserAuth::authResponse()
{
	if (!mTransaction)
	{
		return mAuthResponse;
	}

	if (!mTransaction->process())	// All done ?
	{
		if (mTransaction->status(0) == LLXMLRPCTransaction::StatusDownloading)
		{
			mAuthResponse = E_DOWNLOADING;
		}
		return mAuthResponse;
	}

	S32 result;
	mTransaction->status(&result);
	mErrorMessage = mTransaction->statusMessage();

	// If curl was ok, parse the download area.
	switch (result)
	{
		case CURLE_OK:
			mResponses = mTransaction->response();
			mAuthResponse = E_OK;
			LL_DEBUGS("UserAuth") << "Responses LLSD:\n"
								  << ll_pretty_print_sd(mResponses) << LL_ENDL;
			break;

		case CURLE_COULDNT_RESOLVE_HOST:
			mAuthResponse = E_COULDNT_RESOLVE_HOST;
			LL_DEBUGS("UserAuth") << "Could not resolve host" << LL_ENDL;
			break;

#if CURLE_SSL_CACERT != CURLE_SSL_PEER_CERTIFICATE
		// Note: CURLE_SSL_CACERT and CURLE_SSL_CACERT may expand to the same
		// value in recent curl versions (seen with curl v7.68).
		case CURLE_SSL_PEER_CERTIFICATE:
			mAuthResponse = E_SSL_PEER_CERTIFICATE;
			LL_DEBUGS("UserAuth") << "Invalid peer SSL certificate" << LL_ENDL;
			break;
#endif

		case CURLE_SSL_CACERT:
			mAuthResponse = E_SSL_CACERT;
			LL_DEBUGS("UserAuth") << "Invalid SSL certificate" << LL_ENDL;
			break;

		case CURLE_SSL_CONNECT_ERROR:
			mAuthResponse = E_SSL_CONNECT_ERROR;
			LL_DEBUGS("UserAuth") << "Connection error" << LL_ENDL;
			break;

		default:
			mAuthResponse = E_UNHANDLED_ERROR;
			LL_DEBUGS("UserAuth") << "Unhandled error: " << result << LL_ENDL;
	}

	llinfos << "Processed response: " << result << llendl;

	delete mTransaction;
	mTransaction = NULL;

	return mAuthResponse;
}

const LLSD& LLUserAuth::getResponse1stMap(const std::string& name) const
{
	if (mResponses.has(name) && mResponses[name].isArray() &&
		mResponses[name][0].isMap())
	{
		return mResponses[name][0];
	}

	static const LLSD empty;
	return empty;
}
