/**
 * @file hbcookiesmerger.h
 * @brief A CEF cookies database merger.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (C) 2024, Henri Beauchamp.
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

#include "llfile.h"
#include "llstring.h"

struct sqlite3;

class HBCookiesMerger
{
public:
	HBCookiesMerger(const std::string& source_db, const std::string& dest_db,
					const std::string& debug_log = LLStringUtil::null);
	~HBCookiesMerger();

	bool merge();

	// Allows to retreive the last error message (e.g for when debug_log was
	// not used).
	LL_INLINE const std::string& getErrorMessage()	{ return mErrMsg; }

private:
	void close();
	bool hasError(sqlite3* db, int result);
	strings_set_t getTables();
	bool mergeTable(const std::string& table_name);

private:
	sqlite3*	mSrcDb;
	sqlite3*	mDstDb;
	std::string mErrMsg;
	std::string mSrcFileName;
	std::string mDstFileName;
	std::string mLogFileName;
	llofstream*	mLogStream;
};
