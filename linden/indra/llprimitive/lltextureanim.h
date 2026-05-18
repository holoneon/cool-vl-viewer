/**
 * @file lltextureanim.h
 * @brief LLTextureAnim base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "stdtypes.h"
#include "llpreprocessor.h"
#include "llsd.h"

class LLMessageSystem;
class LLDataPacker;

class LLTextureAnim
{
protected:
	LOG_CLASS(LLTextureAnim);

public:
	LLTextureAnim();
	virtual ~LLTextureAnim() = default;

	virtual void reset();

	void packTAMessage(LLMessageSystem* mesgsys) const;
	void packTAMessage(LLDataPacker& dp) const;
	void unpackTAMessage(LLMessageSystem* mesgsys, S32 block_num);
	void unpackTAMessage(LLDataPacker& dp);

	LL_INLINE bool equals(const LLTextureAnim& other) const
	{
		return mMode == other.mMode && mFace == other.mFace &&
			   mSizeX == other.mSizeX && mSizeY == other.mSizeY &&
			   mStart == other.mStart && mLength == other.mLength &&
			   mRate == other.mRate;
	}

	LLSD asLLSD() const;
	LL_INLINE operator LLSD() const					{ return asLLSD(); }
	bool fromLLSD(LLSD& sd);

	enum
	{
		ON				= 0x01,
		LOOP			= 0x02,
		REVERSE			= 0x04,
		PING_PONG		= 0x08,
		SMOOTH			= 0x10,
		ROTATE			= 0x20,
		SCALE			= 0x40,
	};

public:
	U8 mMode;
	S8 mFace;
	U8 mSizeX;
	U8 mSizeY;
	F32 mStart;
	F32 mLength;
	F32 mRate; // Rate in frames per second.
};
