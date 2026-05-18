/**
 * @file llstreamtools.h
 * @brief some helper functions for parsing legacy simstate and asset files.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 *
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include <deque>
#include <iostream>
#include <string>
#include <vector>

#include "llpreprocessor.h"

// Unless specifed otherwise these all return input_stream.good()

// Skips emptyspace and lines that start with a #
bool skip_comments_and_emptyspace(std::istream& input_stream);

// Skips to character after the end of next keyword. A 'keyword' is defined as
// the first word on a line
bool skip_to_end_of_next_keyword(const char* keyword,
								 std::istream& input_stream);

bool get_line(std::string& output_string, std::istream& input_stream, int n);

// *TODO: move these string manipulator functions to a different file

// The 'keyword' is defined as the first word on a line
// The 'value' is everything after the keyword on the same line starting at the
// first non-whitespace and ending right before the newline.
void get_keyword_and_value(std::string& keyword, std::string& value,
						   const std::string& line);

// Continue to read from the stream until you really cannot read anymore or
// until we hit the count. Some istream implementations have a max that they
// will read. Returns the number of bytes read.
std::streamsize fullread(std::istream& istr, char* buf,
						 std::streamsize requested);

std::istream& operator>>(std::istream& str, const char* tocheck);

// cat_streambuf is a std::streambuf subclass that accepts a variadic number
// of std::streambuf* (e.g. some_istream.rdbuf()) and virtually concatenates
// their contents. Derived from https://stackoverflow.com/a/49441066/5533635
class cat_streambuf : public std::streambuf
{
public:
	// Only valid for std::streambuf* arguments
	template <typename... Inputs>
	cat_streambuf(Inputs... inputs)
	:	mInputs{inputs...},
		mBuffer(1024)
	{
	}

	int underflow() override;

private:
	std::deque<std::streambuf*>	mInputs;
	std::vector<char>			mBuffer;
};
