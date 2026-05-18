/**
 * @file llpointer.h
 * @brief A reference-counted pointer for objects derived from LLRefCount
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

#include "llpreprocessor.h"
#include "stdtypes.h"

//----------------------------------------------------------------------------
// NOTE: LLPointer<LLFoo> x = new LLFoo(); MAY NOT BE THREAD SAFE if
// LLFoo::LLFoo() does anything like put itself in an update queue. The queue
// may get accessed before it gets assigned to x. The correct implementation
// is:
// 	LLPointer<LLFoo> x = new LLFoo;	// constructor does not do anything
//									// interesting
//   x->instantiate(); // does stuff like place x into an update queue
//----------------------------------------------------------------------------

// This is to avoid inlining a llwarns...
LL_NO_INLINE void warnUnreferenceDidAssignment();

// Note: relies on Type having ref() and unref() methods
template <class Type> class LLPointer
{
public:
	LL_INLINE LLPointer() noexcept
	:	mPointer(NULL)
	{
	}

	LL_INLINE LLPointer(Type* ptr) noexcept
	:	mPointer(ptr)
	{
		ref();
	}

	LL_INLINE LLPointer(const LLPointer<Type>& ptr) noexcept
	:	mPointer(ptr.mPointer)
	{
		ref();
	}

	// This C++11 move constructor saves us an unref() on the moved pointer
	// and consequently a corresponding ref() on the constructed pointer.
	// Note: LLPointer::unref() still happens in ~LLPointer() of the moved
	// pointer, but it is not propagated down to the unref() method of the
	// object we point at, since mPointer is NULL when ~LLPointer() is called.
	LL_INLINE LLPointer(LLPointer<Type>&& ptr) noexcept
	:	mPointer(ptr.mPointer)
	{
		ptr.mPointer = NULL;
	}

	// Support conversion up the type hierarchy. See Item 45 in Effective C++,
	// 3rd Ed.
	template<typename Subclass>
	LL_INLINE LLPointer(const LLPointer<Subclass>& ptr) noexcept
	:	mPointer(ptr.get())
	{
		ref();
	}

	LL_INLINE ~LLPointer()
	{
		unref();
	}

	LL_INLINE Type* get() const									{ return mPointer; }
	LL_INLINE const Type* operator->() const					{ return mPointer; }
	LL_INLINE Type* operator->()								{ return mPointer; }
	LL_INLINE const Type& operator*() const						{ return *mPointer; }
	LL_INLINE Type& operator*()									{ return *mPointer; }

	LL_INLINE operator bool() const								{ return mPointer != NULL; }
	LL_INLINE bool operator!() const							{ return mPointer == NULL; }
	LL_INLINE bool isNull() const								{ return mPointer == NULL; }
	LL_INLINE bool notNull() const								{ return mPointer != NULL; }

	LL_INLINE operator Type*() const							{ return mPointer; }
	LL_INLINE bool operator!=(Type* ptr) const					{ return mPointer != ptr; }
	LL_INLINE bool operator==(Type* ptr) const					{ return mPointer == ptr; }
	LL_INLINE bool operator==(const LLPointer<Type>& ptr) const	{ return mPointer == ptr.mPointer; }
	LL_INLINE bool operator<(const LLPointer<Type>& ptr) const	{ return mPointer < ptr.mPointer; }
	LL_INLINE bool operator>(const LLPointer<Type>& ptr) const	{ return mPointer > ptr.mPointer; }

	LL_INLINE LLPointer<Type>& operator=(Type* ptr)
	{
		if (mPointer != ptr)
		{
			unref();
			mPointer = ptr;
			ref();
		}

		return *this;
	}

	LL_INLINE LLPointer<Type>& operator=(const LLPointer<Type>& ptr)
	{
		if (mPointer != ptr.mPointer)
		{
			unref();
			mPointer = ptr.mPointer;
			ref();
		}
		return *this;
	}

	// C++11 move assigment (saves a ref() in the assigned pointer and an
	// unref() in the moved pointer destructor).
	LL_INLINE LLPointer<Type>& operator=(LLPointer<Type>&& ptr) noexcept
	{
		if (mPointer != ptr.mPointer)
		{
			unref();
			mPointer = ptr.mPointer;
			ptr.mPointer = NULL;
		}
		return *this;
	}

	// Support assignment up the type hierarchy. See Item 45 in Effective C++,
	// 3rd Ed.
	template<typename Subclass>
	LL_INLINE LLPointer<Type>& operator=(const LLPointer<Subclass>& ptr)
	{
		if (mPointer != ptr.get())
		{
			unref();
			mPointer = ptr.get();
			ref();
		}
		return *this;
	}

protected:
	LL_INLINE void ref() noexcept
	{
		if (mPointer)
		{
			mPointer->ref();
		}
	}

	LL_INLINE void unref()
	{
		if (mPointer)
		{
			Type* tempp = mPointer;
			mPointer = NULL;
			tempp->unref();
			if (mPointer)
			{
				warnUnreferenceDidAssignment();
#if 0			// Attempting to work around a strange crash seen (by one user,
				// under Windows), in LLSpatialBridge::cleanupReferences()...
				// At worst, this change should result in a memory leak. HB
				unref();
#else
				mPointer = NULL;
#endif
			}
		}
	}

protected:
	Type* mPointer;
};

// std::hash implementation
namespace std
{
	template <typename Subclass> struct hash<LLPointer<Subclass>>
	{
		LL_INLINE size_t operator()(const LLPointer<Subclass>& p) const noexcept
		{
			return (size_t)p.get();
		}
	};
}

// For use with boost::unordered_map and boost::unordered_set
template <typename Subclass>
LL_INLINE size_t hash_value(const LLPointer<Subclass>& p) noexcept
{
	return (size_t)p.get();
}
