/**
 * @file llcallbacklist.h
 * @brief A simple list of callback functions to call.
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

#include <functional>
#include <list>

#include "llerror.h"
#include "llstl.h"

class LLCallbackList
{
protected:
	LOG_CLASS(LLCallbackList);

public:
	typedef void (*callback_t)(void*);
	typedef std::pair<callback_t, void*> callback_pair_t;
	typedef std::list<callback_pair_t> callback_list_t;

	LLCallbackList() = default;

	// Registers a callback, which will be called as func(data)
	void addFunction(callback_t func, void* datap = NULL);

	// true if list already contains the function/data pair
	LL_INLINE bool containsFunction(callback_t func, void* datap = NULL)
	{
		return find(func, datap) != mCallbackList.end();
	}

	// Removes the first instance of this function/data pair from the list,
	// false if not found
	bool deleteFunction(callback_t func, void* data = NULL);
	void callFunctions();		// Calls all functions
	void deleteAllFunctions();

	static void test();

protected:
	LL_INLINE callback_list_t::iterator find(callback_t func, void* datap)
	{
		callback_pair_t t(func, datap);
		return std::find(mCallbackList.begin(), mCallbackList.end(), t);
	}

protected:
	// Use a list so that the callbacks are ordered in case that matters
	callback_list_t	mCallbackList;
};

typedef std::function<void()> nullary_func_t;
typedef std::function<bool()> bool_func_t;

// Calls a given callable once in idle loop.
// Now also auto-aborts when the application exits. HB
void doOnIdleOneTime(nullary_func_t callable);

// Repeatedly calls a callable in idle loop until it returns true.
// Now also auto-aborts when the application exits. HB
void doOnIdleRepeating(bool_func_t callable);

// Calls a given callable once after specified interval.
void doAfterInterval(nullary_func_t callable, F32 seconds);

// Calls a given callable every specified number of seconds, until it returns
// true. Now also auto-aborts when the application exits. HB
void doPeriodically(bool_func_t callable, F32 seconds);

extern LLCallbackList gIdleCallbacks;
