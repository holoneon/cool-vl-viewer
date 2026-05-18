/**
 * @file bitpack.h
 * @brief Convert data to packed bit stream
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "llerror.h"

constexpr U32 MAX_DATA_BITS = 8;

class LLBitPack
{
protected:
	LOG_CLASS(LLBitPack);

public:
	LL_INLINE LLBitPack(U8* buffer, U32 max_size)
	:	mBuffer(buffer),
		mBufferSize(0),
		mLoad(0),
		mLoadSize(0),
		mTotalBits(0),
		mMaxSize(max_size)
	{
	}

	LL_INLINE void resetBitPacking()
	{
		mLoad = 0;
		mLoadSize = 0;
		mTotalBits = 0;
		mBufferSize = 0;
	}

	U32 bitPack(U8* total_data, U32 total_dsize);
	U32 bitCopy(U8* total_data, U32 total_dsize);
	U32 bitUnpack(U8* total_retval, U32 total_dsize);
	U32 flushBitPack();

public:
	U8*	mBuffer;
	U32	mBufferSize;
	U8	mLoad;
	U32	mLoadSize;
	U32	mTotalBits;
	U32	mMaxSize;
};
