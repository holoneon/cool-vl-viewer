/**
 * @file llstl.h
 * @brief helper object & functions for use with the stl.
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

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "stdtypes.h"

#include "llpreprocessor.h"

// DeletePointer is a simple helper for deleting all pointers in a container.
// The general form is:
//
//  std::for_each(cont.begin(), cont.end(), DeletePointer());
//  somemap.clear();
//
// Do not forget to clear() !

struct DeletePointer
{
	template<typename T> void operator()(T* ptr) const
	{
		delete ptr;
	}
};

// DeletePairedPointer is a simple helper for deleting all pointers in a map.
// The general form is:
//
//  std::for_each(somemap.begin(), somemap.end(), DeletePairedPointer());
//  somemap.clear();		// Do not leave dangling pointers around
// WARNING: this does NOT work with Tessil fast hash maps and sets, that
// return a const pair as iter->second !
struct DeletePairedPointer
{
	template<typename T> void operator()(T& ptr) const
	{
		delete ptr.second;
		ptr.second = NULL;
	}
};

template<typename T, typename ALLOC>
void delete_and_clear(std::list<T*, ALLOC>& list)
{
	std::for_each(list.begin(), list.end(), DeletePointer());
	list.clear();
}

template<typename T, typename ALLOC>
void delete_and_clear(std::vector<T*, ALLOC>& vector)
{
	std::for_each(vector.begin(), vector.end(), DeletePointer());
	vector.clear();
}

template<typename T, typename COMPARE, typename ALLOC>
void delete_and_clear(std::set<T*, COMPARE, ALLOC>& set)
{
	std::for_each(set.begin(), set.end(), DeletePointer());
	set.clear();
}

template<typename K, typename V, typename COMPARE, typename ALLOC>
void delete_and_clear(std::map<K, V*, COMPARE, ALLOC>& map)
{
	std::for_each(map.begin(), map.end(), DeletePairedPointer());
	map.clear();
}

template<typename T>
void delete_and_clear(T*& ptr)
{
	if (ptr)
	{
		delete ptr;
		ptr = NULL;
	}
}

// Similar to get_ptr_in_map, but for any type with a valid T(0) constructor.
// WARNING: Make sure default_value (generally 0) is not a valid map entry !
template <typename T>
LL_INLINE typename T::mapped_type get_if_there(const T& inmap,
											   typename T::key_type const& key,
											   typename T::mapped_type default_value)
{
	// Typedef here avoids warnings because of new C++ naming rules.
	typedef typename T::const_iterator map_it_t;
	map_it_t iter = inmap.find(key);
	if (iter == inmap.end())
	{
		return default_value;
	}
	return iter->second;
};

// Simple function to help with finding pointers in maps.
// For example:
// 	typedef  map_t;
//  std::map<int, const char*> foo;
//	foo[18] = "there";
//	foo[2] = "hello";
// 	const char* bar = get_ptr_in_map(foo, 2); // bar -> "hello"
//  const char* baz = get_ptr_in_map(foo, 3); // baz == NULL
template <typename T>
LL_INLINE typename T::mapped_type get_ptr_in_map(const T& inmap,
												 typename T::key_type const& key)
{
	// Typedef here avoids warnings because of new C++ naming rules.
	typedef typename T::const_iterator map_it_t;
	map_it_t iter = inmap.find(key);
	if (iter == inmap.end())
	{
		return NULL;
	}
	return iter->second;
};

// Example:
//  for (std::vector<T>::iterator iter = mList.begin(); iter != mList.end(); )
//  {
//    if ((*iter)->isMarkedForRemoval())
//      iter = vector_replace_with_last(mList, iter);
//    else
//      ++iter;
//  }
template <typename T>
LL_INLINE typename std::vector<T>::iterator vector_replace_with_last(std::vector<T>& invec,
																	 typename std::vector<T>::iterator iter)
{
	typename std::vector<T>::iterator last = invec.end();
	if (iter == last)
	{
		return iter;
	}

	if (iter == --last)
	{
		invec.pop_back();
		return invec.end();
	}

	*iter = *last;
	invec.pop_back();
	return iter;
};

// Example: vector_replace_with_last(mList, x);
template <typename T>
LL_INLINE bool vector_replace_with_last(std::vector<T>& invec, const T& val)
{
	typename std::vector<T>::iterator last = invec.end();
	typename std::vector<T>::iterator it = std::find(invec.begin(), last, val);
	if (it == last)
	{
		return false;
	}
	if (it != --last)
	{
		*it = *last;
	}
	invec.pop_back();
	return true;
}
