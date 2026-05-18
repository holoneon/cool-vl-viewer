/**
 * @file bitpack.cpp
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

#include "linden_common.h"

#include "llbitpack.h"

U32 LLBitPack::bitPack(U8* total_data, U32 total_dsize)
{
	while (total_dsize > 0)
	{
		U32 dsize;
		if (total_dsize > MAX_DATA_BITS)
		{
			dsize = MAX_DATA_BITS;
			total_dsize -= MAX_DATA_BITS;
		}
		else
		{
			dsize = total_dsize;
			total_dsize = 0;
		}

		U8 data = *total_data++;

		data <<= (MAX_DATA_BITS - dsize);
		while (dsize > 0)
		{
			if (mLoadSize == MAX_DATA_BITS)
			{
				*(mBuffer + mBufferSize++) = mLoad;
				if (mBufferSize > mMaxSize)
				{
					llerrs << "mBufferSize exceeding mMaxSize !" << llendl;
				}
				mLoadSize = 0;
				mLoad = 0x00;
			}
			mLoad <<= 1;
			mLoad |= data >> (MAX_DATA_BITS - 1);
			data <<= 1;
			++mLoadSize;
			++mTotalBits;
			--dsize;
		}
	}

	return mBufferSize;
}

U32 LLBitPack::bitCopy(U8* total_data, U32 total_dsize)
{
	while (total_dsize > 0)
	{
		U32 dsize;
		if (total_dsize > MAX_DATA_BITS)
		{
			dsize = MAX_DATA_BITS;
			total_dsize -= MAX_DATA_BITS;
		}
		else
		{
			dsize = total_dsize;
			total_dsize = 0;
		}

		U8 data = *total_data++;

		while (dsize > 0)
		{
			if (mLoadSize == MAX_DATA_BITS)
			{
				*(mBuffer + mBufferSize++) = mLoad;
				if (mBufferSize > mMaxSize)
				{
					llerrs << "mBufferSize exceeding mMaxSize !" << llendl;
				}
				mLoadSize = 0;
				mLoad = 0x00;
			}
			mLoad <<= 1;
			mLoad |= (data >> (MAX_DATA_BITS - 1));
			data <<= 1;
			++mLoadSize;
			++mTotalBits;
			--dsize;
		}
	}

	return mBufferSize;
}

U32 LLBitPack::bitUnpack(U8* total_retval, U32 total_dsize)
{
	while (total_dsize > 0)
	{
		U32 dsize;
		if (total_dsize > MAX_DATA_BITS)
		{
			dsize = MAX_DATA_BITS;
			total_dsize -= MAX_DATA_BITS;
		}
		else
		{
			dsize = total_dsize;
			total_dsize = 0;
		}

		U8* retval = total_retval++;
		*retval = 0x00;
		while (dsize > 0)
		{
			if (mLoadSize == 0)
			{
#if LL_DEBUG
				if (mBufferSize > mMaxSize)
				{
					llerrs << "mBufferSize exceeding mMaxSize" << llendl;
					llerrs << mBufferSize << " > " << mMaxSize << llendl;
				}
#endif
				mLoad = *(mBuffer + mBufferSize++);
				mLoadSize = MAX_DATA_BITS;
			}
			*retval <<= 1;
			*retval |= (mLoad >> (MAX_DATA_BITS - 1));
			--mLoadSize;
			mLoad <<= 1;
			--dsize;
		}
	}

	return mBufferSize;
}

U32 LLBitPack::flushBitPack()
{
	if (mLoadSize)
	{
		mLoad <<= (MAX_DATA_BITS - mLoadSize);
		*(mBuffer + mBufferSize++) = mLoad;
		if (mBufferSize > mMaxSize)
		{
			llerrs << "mBufferSize exceeding mMaxSize !" << llendl;
		}
		mLoadSize = 0;
	}
	return mBufferSize;
}
