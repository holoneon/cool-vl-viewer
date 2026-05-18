/**
 * @file lllogchat.h
 * @brief LLLogChat class definition
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

#include <time.h>		// For time_t

#include "lluuid.h"
#include "llsd.h"

// Purely static class
class LLLogChat
{
protected:
	LOG_CLASS(LLLogChat);

	LLLogChat() = delete;
	~LLLogChat() = delete;

public:
	// Status values for callback function
	enum EResponseType
	{
		LOG_FILENAME,
		LOG_SERVER_FETCH,
		LOG_SERVER,
		LOG_LINE,
		LOG_END,
	};

	// Returns a time stamp with the SL (or UTC for OpenSim) time zone, which
	// format (date and time format, with or without date, with or without the
	// seconds) follows the user preferences. When 'no_date' is true, then the
	// date is always omitted, regardless of the said preferences.
	// When 'ts' is omitted or 0, the current time is used, else it it supposed
	// to correspond to the grid time. HB
	static std::string timestamp(bool no_date = false, time_t ts = 0);

	static std::string makeLogFileName(std::string filename);

	static void saveHistory(const std::string& filename,
							const std::string& line);
	static void loadHistory(const std::string& filename,
		                    void (*callback)(S32, const LLSD&, void*),
							void* userdata,
							const LLUUID& session_id = LLUUID::null);
private:
	static void fetchHistoryCoro(const std::string& url, LLUUID session_id,
								 void (*callback)(S32, const LLSD&, void*),
								 time_t last_modified);
};
