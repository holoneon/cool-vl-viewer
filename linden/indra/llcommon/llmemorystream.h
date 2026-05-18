/**
 * @file llmemorystream.h
 * @author Phoenix
 * @date 2005-06-03
 * @brief Implementation of a simple fixed memory stream
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

// This is a simple but effective optimization when you want to treat a chunk
// of memory as an istream. I wrote this to avoid turning a buffer into a
// string, and then throwing the string into an iostringstream just to parse it
// into another datatype, eg, LLSD.

// The memory passed in is NOT owned by an instance. The caller must be careful
// to always pass in a valid memory location that exists for at least as long
// as this streambuf.

#include <iostream>

#include "llpreprocessor.h"
#include "stdtypes.h"

// LLMemoryStreamBuf class

class LLMemoryStreamBuf : public std::streambuf
{
public:
	LL_INLINE LLMemoryStreamBuf(const U8* start, S32 length)
	{
		reset(start, length);
	}

	LL_INLINE void reset(const U8* start, S32 length)
	{
		setg((char*)start, (char*)start, (char*)start + length);
	}

protected:
	LL_INLINE int underflow()
	{
		if (gptr() >= egptr())
		{
			return EOF;
		}
		return *gptr();
	}
};

// LLMemoryStream class

class LLMemoryStream : public std::istream
{
public:
	LL_INLINE LLMemoryStream(const U8* start, S32 length)
	:	std::istream(&mStreamBuf),
		mStreamBuf(start, length)
	{
	}

protected:
	LLMemoryStreamBuf mStreamBuf;
};
