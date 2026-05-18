/**
 * @file llbufferstream.h
 * @author Phoenix
 * @date 2005-10-10
 * @brief Classes to treat an LLBufferArray as a c++ iostream.
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

#include <iosfwd>
#include <iostream>

#include "llbuffer.h"

/**
 * @class LLBufferStreamBuf
 * @brief This implements the buffer wrapper for an istream
 *
 * The buffer array passed in is not owned by the stream buf object.
 */
class LLBufferStreamBuf final : public std::streambuf
{
public:
	LLBufferStreamBuf(const LLChannelDescriptors& channels,
					  LLBufferArray* buffer);
	virtual ~LLBufferStreamBuf();

protected:
	typedef std::streambuf::pos_type pos_type;
	typedef std::streambuf::off_type off_type;

	// streambuf vrtual implementations

	// Called when we hit the end of input. Returns the character at the
	// current position or EOF.
	int underflow() override;

	// Called when we hit the end of output
	// - c: the character to store at the current put position
	// Returns EOF if the function failed. Any other value on success.
	int overflow(int c) override;

	// Synchronizes the buffer.Returns 0 on success or -1 on failure.
	int sync() override;

	// Seeks to an offset position in a stream.
	// - off: offset value relative to way paramter
	// - way: the seek direction. One of ios::beg, ios::cur, and ios::end.
	// - which: which pointer to modify. One of ios::in, ios::out, or both
    //   masked together.
	// Returns the new position or an invalid position on failure.
	pos_type seekoff(off_type off, std::ios::seekdir way,
					 std::ios::openmode which) override;

protected:
	// This channels we are working on.
	LLChannelDescriptors mChannels;

	// The buffer we work on
	LLBufferArray* mBuffer;
};

/**
 * @class LLBufferStream
 * @brief This implements an istream based wrapper around an LLBufferArray.
 *
 * This class does not own the buffer array, and does not hold a shared pointer
 * to it. Since the class itself is fairly ligthweight, just make one on the
 * stack when needed and let it fall out of scope.
 */
class LLBufferStream final : public std::iostream
{
public:
	LLBufferStream(const LLChannelDescriptors& channels,
				   LLBufferArray* buffer);

protected:
	LLBufferStreamBuf mStreamBuf;
};
