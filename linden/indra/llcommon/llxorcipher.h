/**
 * @file llxorcipher.h
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llpreprocessor.h"

#include "stdtypes.h"

class LLXORCipher
{
public:
	LLXORCipher(const U8* pad, U32 pad_len);
	LLXORCipher(const std::string& pad);
	LLXORCipher(const LLXORCipher& cipher);

	virtual ~LLXORCipher();

	LLXORCipher& operator=(const LLXORCipher& cipher);

	// Cipher methods
	U32 encrypt(const U8* src, U32 src_len, U8* dst);
	U32 encrypt(const std::string& src, U8* dst);

	LL_INLINE U32 decrypt(const U8* src, U32 src_len, U8* dst)
	{
		// Since XOR is a symetric cipher, just call the encrypt() method.
		return encrypt(src, src_len, dst);
	}

	LL_INLINE U32 decrypt(const std::string& src, U8* dst)
	{
		// Since XOR is a symetric cipher, just call the encrypt() method.
		return encrypt(src, dst);
	}

	// Special syntactic-sugar since xor can be performed in place.
	// *BUG: THIS MEANS THAT THE COMPILER GETS FOOLED ABOUT THE CONSTNESS OF
	// THE INPUT BUFFER: DO MAKE SURE TO COPY THE CONST INPUT STRING INTO THE
	// DESTINATION BEFORE HAND, OR YOUR CONST INPUT SOURCE WILL GET CORRUPTED !
	// *TODO: change to fix the above bug.
	LL_INLINE U32 encrypt(U8* buf, U32 len)
	{
		return encrypt((const U8*)buf, len, buf);
	}

	LL_INLINE U32 decrypt(U8* buf, U32 len)
	{
		return encrypt((const U8*)buf, len, buf);
	}

	LL_INLINE U32 encrypt(std::string& src)
	{
		return encrypt(src, (U8*)src.data());
	}

	LL_INLINE U32 decrypt(std::string& src)
	{
		return encrypt(src, (U8*)src.data());
	}

protected:
	void init(const U8* pad, U32 pad_len);

protected:
	U8* mPad;
	U8* mHead;
	U32 mPadLen;
};
