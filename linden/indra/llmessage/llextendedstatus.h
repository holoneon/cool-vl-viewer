/** 
 * @file llextendedstatus.h
 * @date August 2007
 * @brief Extended status codes for curl/resident asset storage & delivery
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

enum class LLExtStat : U32
{
	// Status provider groups. Top bits indicate which status type it is
	// Zero is common status code (next section).
	CURL_RESULT	= 1UL << 30,		// Serviced by curl
	RES_RESULT	= 2UL << 30,		// Serviced by resident copy
	CACHE_RESULT	= 3UL << 30,	// Serviced by cache

	// Common status codes
	NONE				= 0x00000,	// No extra info here, sorry !
	NULL_UUID			= 0x10001,	// Null asset ID
	NO_UPSTREAM			= 0x10002,	// Attempt to upload without valid upstream
	REQUEST_DROPPED		= 0x10003,	// Request was dropped unserviced
	NONEXISTENT_FILE	= 0x10004,	// Tried to upload non existent file
	BLOCKED_FILE		= 0x10005,	// Tried to upload a file we cannot open

	// Curl status codes:
	// Mask off CURL_RESULT for original result and
	// See: include/curl/curl.h
	
	// Cache status codes:
	CACHE_CACHED		= CACHE_RESULT | 0x0001,
	CACHE_CORRUPT		= CACHE_RESULT | 0x0002,
};
