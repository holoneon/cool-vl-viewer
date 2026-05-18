/**
 * @file llcorehttpcommon.cpp
 * @brief
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include <sstream>

#include "curl/curl.h"				// For curl errro codes

#include "llcorehttpcommon.h"

#include "hbfastmap.h"
#include "llhttpconstants.h"
#include "llstring.h"

// Some commonly used statuses, defined as globals, so to avoid having to
// construct and destruct them each time we need them in tests...
const LLCore::HttpStatus gStatusPartialContent(HTTP_PARTIAL_CONTENT);
const LLCore::HttpStatus gStatusBadRequest(HTTP_BAD_REQUEST);
const LLCore::HttpStatus gStatusForbidden(HTTP_FORBIDDEN);
const LLCore::HttpStatus gStatusNotFound(HTTP_NOT_FOUND);
const LLCore::HttpStatus gStatusNotSatisfiable(HTTP_REQUESTED_RANGE_NOT_SATISFIABLE);
const LLCore::HttpStatus gStatusInternalError(HTTP_INTERNAL_ERROR);
const LLCore::HttpStatus gStatusServerInternalError(HTTP_INTERNAL_SERVER_ERROR);
const LLCore::HttpStatus gStatusBadGateway(HTTP_BAD_GATEWAY);
const LLCore::HttpStatus gStatusUnavailable(HTTP_SERVICE_UNAVAILABLE);
const LLCore::HttpStatus gStatusCantConnect(LLCore::HttpStatus::EXT_CURL_EASY,
											CURLE_COULDNT_CONNECT);
const LLCore::HttpStatus gStatusTimeout(LLCore::HttpStatus::EXT_CURL_EASY,
										CURLE_OPERATION_TIMEDOUT);
const LLCore::HttpStatus gStatusCancelled(LLCore::HttpStatus::LLCORE,
										  LLCore::HE_OP_CANCELLED);

namespace LLCore
{

namespace LLHttp
{
bool gEnabledHTTP2 = false;
}

HttpStatus::type_enum_t EXT_CURL_EASY;
HttpStatus::type_enum_t EXT_CURL_MULTI;
HttpStatus::type_enum_t LLCORE;

HttpStatus::HttpStatus()
{
	constexpr type_enum_t type = LLCORE;
	mDetails = std::make_shared<Details>(type, HE_SUCCESS);
}

HttpStatus::HttpStatus(type_enum_t type, type_enum_t status)
{
	mDetails = std::make_shared<Details>(type, status);
}

HttpStatus::HttpStatus(int http_status)
{
	mDetails =
		std::make_shared<Details>(http_status,
								  http_status >= 200 &&
								  http_status <= 299 ? HE_SUCCESS
													 : HE_REPLY_ERROR);
	llassert(http_status >= 100 && http_status <= 999);
}

HttpStatus::HttpStatus(int http_status, const std::string& message)
{
	mDetails =
		std::make_shared<Details>(http_status,
								  http_status >= 200 &&
								  http_status <= 299 ? HE_SUCCESS
													 : HE_REPLY_ERROR);
	llassert(http_status >= 100 && http_status <= 999);
	mDetails->mMessage = message;
}

HttpStatus::operator U32() const
{
	// Effectively, concatenate mType (high) with mStatus (low).
	static const int shift = 8 * sizeof(mDetails->mStatus);
	return U32(mDetails->mType) << shift | U32((int)mDetails->mStatus);
}

std::string HttpStatus::toHex() const
{
	std::ostringstream result;
	result.width(8);
	result.fill('0');
	result << std::hex << operator U32();
	return result.str();
}

const char* HttpStatus::toString() const
{
	static const char* llcore_errors[] =
	{
		"",
		"HTTP error reply status",
		"Services shutting down",
		"Operation cancelled",
		"Invalid Content-Range header encountered",
		"Request handle not found",
		"Invalid datatype for argument or option",
		"Option has not been explicitly set",
		"Option is not dynamic and must be set early",
		"Invalid HTTP status code received from server",
		"Could not allocate required resource"
	};
	constexpr type_enum_t LLCORE_ERRORS_COUNT = LL_ARRAY_SIZE(llcore_errors);

	static const fast_hmap<type_enum_t, const char*> http_errors =
	{
		// Keep sorted by mCode, we binary search this list.
		{ HTTP_CONTINUE, "Continue" },
		{ HTTP_SWITCHING_PROTOCOLS, "Switching Protocols" },
		{ HTTP_OK, "OK" },
		{ HTTP_CREATED, "Created" },
		{ HTTP_ACCEPTED, "Accepted" },
		{ HTTP_NON_AUTHORITATIVE_INFORMATION,
		  "Non-Authoritative Information" },
		{ HTTP_NO_CONTENT, "No Content" },
		{ HTTP_RESET_CONTENT, "Reset Content" },
		{ HTTP_PARTIAL_CONTENT, "Partial Content" },
		{ HTTP_MULTIPLE_CHOICES, "Multiple Choices" },
		{ HTTP_MOVED_PERMANENTLY, "Moved Permanently" },
		{ HTTP_FOUND, "Found" },
		{ HTTP_SEE_OTHER, "See Other" },
		{ HTTP_NOT_MODIFIED, "Not Modified" },
		{ HTTP_USE_PROXY, "Use Proxy" },
		{ HTTP_TEMPORARY_REDIRECT, "Temporary Redirect" },
		{ HTTP_BAD_REQUEST, "Bad Request" },
		{ HTTP_UNAUTHORIZED, "Unauthorized" },
		{ HTTP_PAYMENT_REQUIRED, "Payment Required" },
		{ HTTP_FORBIDDEN, "Forbidden" },
		{ HTTP_NOT_FOUND, "Not Found" },
		{ HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed" },
		{ HTTP_NOT_ACCEPTABLE, "Not Acceptable" },
		{ HTTP_PROXY_AUTHENTICATION_REQUIRED,
		  "Proxy Authentication Required" },
		{ HTTP_REQUEST_TIME_OUT, "Request Time-out" },
		{ HTTP_CONFLICT, "Conflict" },
		{ HTTP_GONE, "Gone" },
		{ HTTP_LENGTH_REQUIRED, "Length Required" },
		{ HTTP_PRECONDITION_FAILED, "Precondition Failed" },
		{ HTTP_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large" },
		{ HTTP_REQUEST_URI_TOO_LARGE, "Request-URI Too Large" },
		{ HTTP_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type" },
		{ HTTP_REQUESTED_RANGE_NOT_SATISFIABLE,
		  "Requested range not satisfiable" },
		{ HTTP_EXPECTATION_FAILED, "Expectation Failed" },
		{ HTTP_LINDEN_CATCH_ALL, "Linden Catch-All" },
		{ HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error" },
		{ HTTP_NOT_IMPLEMENTED, "Not Implemented" },
		{ HTTP_BAD_GATEWAY, "Bad Gateway" },
		{ HTTP_SERVICE_UNAVAILABLE, "Service Unavailable" },
		{ HTTP_GATEWAY_TIME_OUT, "Gateway Time-out" },
		{ HTTP_VERSION_NOT_SUPPORTED, "HTTP Version not supported" }
	};

	if (*this)
	{
		return "";
	}

	type_enum_t type = getType();
	switch (type)
	{
		case EXT_CURL_EASY:
			return curl_easy_strerror(CURLcode(getStatus()));

		case EXT_CURL_MULTI:
			return curl_multi_strerror(CURLMcode(getStatus()));

		case LLCORE:
		{
			type_enum_t status = getStatus();
			if (status >= 0 && status < LLCORE_ERRORS_COUNT)
			{
				return llcore_errors[status];
			}
			break;
		}

		default:
		{
			if (isHttpStatus())
			{
				// Special handling for status 499 "Linden Catchall"
				if (type == HTTP_LINDEN_CATCH_ALL && !getMessage().empty())
				{
					return getMessage().c_str();
				}

				auto it = http_errors.find(type);
				if (it != http_errors.end())
				{
					return it->second;
				}
			}
		}
	}

	return "Unknown error";
}

std::string HttpStatus::toTerseString() const
{
	std::ostringstream result;
	type_enum_t error_value = getStatus();

	type_enum_t type = getType();
	switch (type)
	{
		case EXT_CURL_EASY:
			result << "Easy_";
			break;

		case EXT_CURL_MULTI:
			result << "Multi_";
			break;

		case LLCORE:
			result << "Core_";
			break;

		default:
			if (isHttpStatus())
			{
				result << "Http_";
				error_value = type;
			}
			else
			{
				result << "Unknown_";
			}
			break;
	}

	result << error_value;
	return result.str();
}

// Pass true on statuses that might actually be cleared by a retry. Library
// failures, calling problems, etc aren't going to be fixed by squirting bits
// all over the Net.
bool HttpStatus::isRetryable() const
{
	static const HttpStatus bad_proxy(EXT_CURL_EASY,
									  CURLE_COULDNT_RESOLVE_PROXY);
	static const HttpStatus bad_host(EXT_CURL_EASY,
									 CURLE_COULDNT_RESOLVE_HOST);
	static const HttpStatus send_error(EXT_CURL_EASY, CURLE_SEND_ERROR);
	static const HttpStatus recv_error(EXT_CURL_EASY, CURLE_RECV_ERROR);
	static const HttpStatus upload_failed(EXT_CURL_EASY, CURLE_UPLOAD_FAILED);
	static const HttpStatus post_error(EXT_CURL_EASY, CURLE_HTTP_POST_ERROR);
	static const HttpStatus partial_file(EXT_CURL_EASY, CURLE_PARTIAL_FILE);
	static const HttpStatus bad_range(LLCORE, HE_INV_CONTENT_RANGE_HDR);

	// HE_INVALID_HTTP_STATUS is special. As of 7.37.0, there are some
	// scenarios where response processing in libcurl appear to go wrong and
	// response data is corrupted. A side-effect of this is that the HTTP
	// status is read as 0 from the library. See libcurl bug report 1420
	// (https://sourceforge.net/p/curl/bugs/1420/) for details.
	static const HttpStatus inv_status(HttpStatus::LLCORE,
									   HE_INVALID_HTTP_STATUS);
	type_enum_t type = getType();
	return ((isHttpStatus() && type <= 599 &&
			 // Include special HTTP_LINDEN_CATCH_ALL in retryables
			 type >= HTTP_LINDEN_CATCH_ALL) ||
			*this == gStatusCantConnect ||	// Connection reset/endpoint problems
			*this == bad_proxy ||			// DNS problems
			*this == bad_host ||			// DNS problems
			*this == send_error ||			// General socket problems
			*this == recv_error ||			// General socket problems
			*this == upload_failed ||		// Transport problem
			*this == gStatusTimeout ||		// Timer expired
			*this == post_error ||			// Transport problem
			*this == partial_file ||		// Data inconsistency in response
#if 1		// disable for "[curl:bugs] #1420" tests.
			*this == inv_status ||			// Inv status can reflect internal state problem in libcurl
#endif
			*this == bad_range);			// Short data read disagrees with content-range
}

} // End namespace LLCore
