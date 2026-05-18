/**
 * @file llstrider.h
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

template <class Object> class LLStrider
{
public:
	LL_INLINE LLStrider()
	:	mObjectp(NULL),
		mSkip(sizeof(Object))
	{
	}

	LL_INLINE LLStrider(Object* firstp)
	:	mObjectp(firstp),
		mSkip(sizeof(Object))
	{
	}

	~LLStrider() = default;

	LL_INLINE const LLStrider<Object>& operator=(const LLStrider<Object>& rhs)
	{
		mBytep = rhs.mBytep;
		mSkip = rhs.mSkip;
		return *this;
	}

	LL_INLINE const LLStrider<Object>& operator=(Object* firstp)
	{
		mObjectp = firstp;
		return *this;
	}

	LL_INLINE void setStride(S32 skip)
	{
		mSkip = skip ? skip : sizeof(Object);
	}

	LL_INLINE LLStrider<Object> operator+(const S32& index)
	{
		LLStrider<Object> ret;
		ret.mBytep = mBytep + mSkip * index;
		ret.mSkip = mSkip;
		return ret;
	}

	LL_INLINE void skip(U32 index)				{ mBytep += mSkip * index;}
	LL_INLINE U32 getSkip() const				{ return mSkip; }
	LL_INLINE Object* get()						{ return mObjectp; }
	LL_INLINE const Object* get() const			{ return mObjectp; }
	LL_INLINE Object* operator->()				{ return mObjectp; }
	LL_INLINE Object& operator*()				{ return *mObjectp; }

	LL_INLINE Object* operator++(int)
	{
		Object* old = mObjectp;
		mBytep += mSkip;
		return old;
	}

	LL_INLINE Object* operator+=(int i)
	{
		mBytep += mSkip * i;
		return mObjectp;
	}

	LL_INLINE Object& operator[](U32 index)
	{
		return *(Object*)(mBytep + mSkip * index);
	}

private:
	union
	{
		Object*	mObjectp;
		U8*		mBytep;
	};

	U32			mSkip;
};
