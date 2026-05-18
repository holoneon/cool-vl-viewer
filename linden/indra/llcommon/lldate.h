/**
 * @file lldate.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of a simple date class.
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

#include <iosfwd>
#include <string>

#include "llerror.h"
#include "stdtypes.h"

// Represents a point in time after UNIX epoch (1970-01-01 00:00:00 UTC).
class LLDate
{
protected:
	LOG_CLASS(LLDate);

public:
	// Constructs a date equal to the UTC epoch start date.
	LL_INLINE constexpr LLDate()
	:	mSecondsSinceEpoch(0.0)
	{
	}

	// Constructs a date from a number of seconds since the UTC epoch value.
	LL_INLINE LLDate(F64 seconds_since_epoch)
	:	mSecondsSinceEpoch(seconds_since_epoch)
	{
	}

	// Constructs a date from a string representation
	// The date is constructed in the fromString() method. See that method for
	// details of supported formats. If that method fails to parse the date,
	// the date is set to epoch.
	LLDate(const std::string& iso8601_date);

	// Returns the date as in ISO-8601 string.
	std::string asString() const;

	// A more "human readable" timestamp format: same as ISO-8601, but with the
	// "T" between date and time replaced with a space and the "Z" replaced
	// with " UTC"
	std::string asTimeStamp(bool with_utc = true) const;

	void toStream(std::ostream&) const;
	bool split(S32* year, S32* month = NULL, S32* day = NULL,
			   S32* hour = NULL, S32* min = NULL, S32* sec = NULL) const;
	std::string toHTTPDateString(const char* fmt) const;
	static std::string toHTTPDateString(tm* gmt, const char* fmt);

	// These two methods set the date from an ISO-8601 string. The parser only
	// supports strings conforming to YYYYF-MM-DDTHH:MM:SS.FFZ where Y is year,
	// M is month, D is day, H is hour, M is minute, S is second, F is sub-
	// second, and all other characters are literal. If these method fail to
	// parse the date, the previous date is retained. Reurn true if the string
	// was successfully parsed.
	bool fromString(const std::string& iso8601_date);
	bool fromStream(std::istream&);

	bool fromYMDHMS(S32 year, S32 month = 1, S32 day = 0, S32 hour = 0,
					S32 min = 0, S32 sec = 0);

	// Returns the date in seconds since epoch.
	LL_INLINE F64 secondsSinceEpoch() const				{ return mSecondsSinceEpoch; }

	// Sets the date in seconds since epoch.
	LL_INLINE void secondsSinceEpoch(F64 seconds)		{ mSecondsSinceEpoch = seconds; }

	// Creates a LLDate object set to the current time.
	static LLDate now();

	// Compares dates using operator< so we can order them using STL.
	bool operator<(const LLDate& rhs) const;

	// Remaining comparison operators in terms of operator<
	// This conforms to the expectation of STL.
	LL_INLINE bool operator>(const LLDate& rhs) const	{ return rhs < *this; }
	LL_INLINE bool operator<=(const LLDate& rhs) const	{ return !(rhs < *this); }
	LL_INLINE bool operator>=(const LLDate& rhs) const	{ return !(*this < rhs); }
	LL_INLINE bool operator!=(const LLDate& rhs) const	{ return *this < rhs || rhs < *this; }
	LL_INLINE bool operator==(const LLDate& rhs) const	{ return !(*this != rhs); }

	// Compare to epoch UTC.
	LL_INLINE bool isNull() const						{ return mSecondsSinceEpoch == 0.0; }
	LL_INLINE bool notNull() const						{ return mSecondsSinceEpoch != 0.0; }

private:
	F64 mSecondsSinceEpoch;
};

// Helper function to stream out a date
std::ostream& operator<<(std::ostream& s, const LLDate& date);

// Helper function to stream in a date
std::istream& operator>>(std::istream& s, LLDate& date);
