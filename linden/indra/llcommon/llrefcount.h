/**
 * @file llrefcount.h
 * @brief Base class for reference counted objects for use with LLPointer
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llatomic.h"
#include "llerror.h"

//-----------------------------------------------------------------------------
// RefCount objects should generally only be accessed by way of LLPointer<>'s.
// See llpointer.h for LLPointer<> definition
//-----------------------------------------------------------------------------

class LLRefCount
{
protected:
	LL_INLINE LLRefCount(const LLRefCount&) noexcept
	:	mRef(0)
	{
	}

	LL_INLINE LLRefCount& operator=(const LLRefCount&) noexcept
	{
		// Do nothing, since ref count is specific to *this* reference
		return *this;
	}

	virtual ~LLRefCount();	// Use unref()

public:
	LL_INLINE LLRefCount() noexcept
	:	mRef(0)
	{
	}

	LL_INLINE void ref() const noexcept
	{
		++mRef;
	}

	LL_INLINE void unref() const
	{
		llassert(mRef >= 1);
		if (--mRef == 0)
		{
			// If we hit zero, the caller should be the only smart pointer
			// owning the object and we can delete it.
			delete this;
		}
	}

	// NOTE: when passing around a const LLRefCount object, this can return
	// different results at different types, since mRef is mutable
	LL_INLINE S32 getNumRefs() const
	{
		return mRef;
	}

private:
	mutable S32	mRef;
};

//-----------------------------------------------------------------------------
// LLThreadSafeRefCount class
//-----------------------------------------------------------------------------

class LLThreadSafeRefCount
{
protected:
	virtual ~LLThreadSafeRefCount();	// Use unref()

public:
	LL_INLINE LLThreadSafeRefCount() noexcept
	:	mRef(0)
	{
	}

	// Non-copyable because LLAtomicS32 (std::atomic<S32>) is non-copyable. HB
	LLThreadSafeRefCount(const LLThreadSafeRefCount&) noexcept = delete;
	LLThreadSafeRefCount& operator=(const LLThreadSafeRefCount&) noexcept = delete;

	LL_INLINE void ref() noexcept
	{
		++mRef;
	}

	LL_INLINE void unref()
	{
		llassert(mRef >= 1);
		if (--mRef == 0)
		{
			// If we hit zero, the caller should be the only smart pointer
			// owning the object and we can delete it. It is technically
			// possible for a vanilla pointer to mess this up, or another
			// thread to jump in, find this object, create another smart
			// pointer and end up dangling, but if the code is that bad and not
			// thread-safe, it is trouble already.
			delete this;
		}
	}

	LL_INLINE S32 getNumRefs() const
	{
		const S32 current_val = mRef.get();
		return current_val;
	}

private:
	LLAtomicS32 mRef;
};
