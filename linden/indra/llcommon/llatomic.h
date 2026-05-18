/**
 * @file llatomic.h
 * @brief Atomic data handling.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include <atomic>

#include "llpreprocessor.h"
#include "stdtypes.h"

template <typename Type,
		  typename AtomicType = std::atomic<Type> > class LLAtomic
{
public:
	LL_INLINE LLAtomic()				{}
	LL_INLINE LLAtomic(Type x)			{ mData.store(x); }

	LL_INLINE ~LLAtomic() = default;

	LL_INLINE operator const Type()		{ return mData.load(); }
	LL_INLINE Type get() const			{ return mData.load(); }

	LL_INLINE Type operator=(Type x)
	{
		mData.store(x);
		return x;
	}

	LL_INLINE void operator-=(Type x)	{ mData -= x; }
	LL_INLINE void operator+=(Type x)	{ mData += x; }
	LL_INLINE Type operator++(int)		{ return mData++; }
	LL_INLINE Type operator--(int)		{ return mData--; }
	LL_INLINE Type operator++()			{ return ++mData; }
	LL_INLINE Type operator--()			{ return --mData; }

	LL_INLINE Type swap(Type x)			{ return mData.exchange(x); }

private:
	AtomicType mData;
};

typedef LLAtomic<U32>	LLAtomicU32;
typedef LLAtomic<S32>	LLAtomicS32;
typedef LLAtomic<U64>	LLAtomicU64;
typedef LLAtomic<S64>	LLAtomicS64;
typedef LLAtomic<bool>	LLAtomicBool;
