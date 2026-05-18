/**
 * @file llbase64.h
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * See llbase64.cpp for the Apache license applicable to part of the code.
 *
 * Copyright (c) 2004 Apache Foundation (for parts borrowed from APR-util),
 *           (c) 2007-2009, Linden Research, Inc.
 *           (c) 2009-2023, Henri Beauchamp.
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

#include "llpreprocessor.h"

// Purely static class
class LLBase64
{
	LLBase64() = delete;
	~LLBase64() = delete;

public:
	static std::string encode(const char* input, size_t input_size);

	LL_INLINE static std::string encode(const std::string& input)
	{
		return encode(input.c_str(), input.size());
	}

	static std::string decode(const char* input);

	LL_INLINE static std::string decode(const std::string& input)
	{
		return decode(input.c_str());
	}

	// Low-level API, derived from APR-util's code (see llbase64.cpp for the
	// Apache license applicable to this code). Only used in lldserialize.cpp
	// (keep it that way, please and use instead the above methods for any new
	// code needing base64 coding, since the underlying code may change in the
	// future for a better/faster implementation). HB

	// Returns an estimation of the required maximum buffer size for encoding.
	LL_INLINE static size_t encodeLen(size_t len)
	{
		return ((len + 2) / 3 * 4) + 1;
	}

	// Returns an estimation of the required maximum buffer size for decoding.
	static size_t decodeLen(const char* input);

	static size_t decode(unsigned char* output, const char* input);
	static size_t encode(char* output, const unsigned char* input, size_t len);
};
