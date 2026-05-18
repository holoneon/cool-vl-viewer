/** 
 * @file llxmlrpctransaction.h
 * @brief LLXMLRPCTransaction and related class header file
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include <string>

// An asynchronous request and responses via XML-RPC
class LLXMLRPCTransaction
{
protected:
	LOG_CLASS(LLXMLRPCTransaction);

public:
	LLXMLRPCTransaction(const std::string& uri, const std::string& method,
						const LLSD& params);

	~LLXMLRPCTransaction();

	typedef enum
	{
		StatusNotStarted,
		StatusStarted,
		StatusDownloading,	// Not used any more (downloading done in request)
		StatusComplete,
		StatusCURLError,
		StatusXMLRPCError,
		StatusOtherError
	} EStatus;

	// Runs the request a little, returns true when done
	bool process();

	// Returns status, and extended CURL code, if curl_code is not NULL.
	EStatus status(S32* curl_code);
	// Returns a message string, suitable for showing the user.
	std::string statusMessage();
	// Returns a URI for the user with more information. Can be empty.
	std::string statusURI();

	// Only non-empty if StatusComplete, otherwise Undefined
	const LLSD& response();

	// Used from newview to set various URLs, parameters and messages,
	// depending on the grid we logged into, the user preferences and the UI
	// language. This avoids dependencies on newview and llui classes. HB
	static void setSupportURL(const std::string& url);
	static void setWebsiteURL(const std::string& url);
	static void setVerifyCert(bool verify);
	static void setMessages(const std::string& server_down,
							const std::string& not_resolving,
							const std::string& not_verified,
							const std::string& connect_error);

private:
	class Handler;
	class Impl;

	Impl& impl;
};
