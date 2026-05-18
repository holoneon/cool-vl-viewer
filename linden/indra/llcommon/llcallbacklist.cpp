/**
 * @file llcallbacklist.cpp
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

#include "linden_common.h"

#include "llcallbacklist.h"

#include "llapp.h"
#include "lleventtimer.h"

LLCallbackList gIdleCallbacks;

void LLCallbackList::addFunction(callback_t func, void* datap)
{
	if (!func)
	{
		llerrs << "Function is NULL" << llendl;
	}

	// Only add one callback per func/data pair
	if (!containsFunction(func, datap))
	{
		callback_pair_t t(func, datap);
		mCallbackList.push_back(t);
	}
}

bool LLCallbackList::deleteFunction(callback_t func, void* datap)
{
	callback_list_t::iterator iter = find(func, datap);
	if (iter != mCallbackList.end())
	{
		mCallbackList.erase(iter);
		return true;
	}
	return false;
}

void LLCallbackList::deleteAllFunctions()
{
	mCallbackList.clear();
}

void LLCallbackList::callFunctions()
{
	for (callback_list_t::iterator iter = mCallbackList.begin(),
								   end = mCallbackList.end();
		 iter != end; )
	{
		callback_list_t::iterator curiter = iter++;
		curiter->first(curiter->second);
	}
}

// Shim class to allow arbitrary std::bind expressions to be run as one-time
// idle callbacks.
class OnIdleCallbackOneTime
{
public:
	OnIdleCallbackOneTime(nullary_func_t callable)
	:	mCallable(callable)
	{
	}

	static void onIdle(void* datap)
	{
		gIdleCallbacks.deleteFunction(onIdle, datap);
		OnIdleCallbackOneTime* self =
			reinterpret_cast<OnIdleCallbackOneTime*>(datap);
		self->mCallable();
		delete self;
	}

private:
	nullary_func_t mCallable;
};

void doOnIdleOneTime(nullary_func_t callable)
{
	OnIdleCallbackOneTime* cb_functor = new OnIdleCallbackOneTime(callable);
	gIdleCallbacks.addFunction(&OnIdleCallbackOneTime::onIdle, cb_functor);
}

// Shim class to allow generic boost functions to be run as recurring idle
// callbacks. Callable should return true when done, false to continue getting
// called.
class OnIdleCallbackRepeating
{
public:
	OnIdleCallbackRepeating(bool_func_t callable)
	:	mCallable(callable)
	{
	}

	// Will keep getting called until the callable returns true.
	static void onIdle(void* datap)
	{
		OnIdleCallbackRepeating* self =
			reinterpret_cast<OnIdleCallbackRepeating*>(datap);
		// Commit suicide without attempting a new call when application is
		// exiting. HB
		bool done = LLApp::isExiting() || self->mCallable();
		if (done)
		{
			gIdleCallbacks.deleteFunction(onIdle, datap);
			delete self;
		}
	}

private:
	bool_func_t mCallable;
};

void doOnIdleRepeating(bool_func_t callable)
{
	OnIdleCallbackRepeating* cb_functor = new OnIdleCallbackRepeating(callable);
	gIdleCallbacks.addFunction(&OnIdleCallbackRepeating::onIdle, cb_functor);
}

class NullaryFuncEventTimer : public LLEventTimer
{
public:
	NullaryFuncEventTimer(nullary_func_t callable, F32 seconds)
	:	LLEventTimer(seconds),
		mCallable(callable)
	{
	}

private:
	LL_INLINE bool tick() override
	{
		if (!LLApp::isExiting())
		{
			mCallable();
		}
		return true;
	}

private:
	nullary_func_t mCallable;
};

// Call a given callable once after specified interval.
void doAfterInterval(nullary_func_t callable, F32 seconds)
{
	new NullaryFuncEventTimer(callable, seconds);
}

class BoolFuncEventTimer : public LLEventTimer
{
public:
	BoolFuncEventTimer(bool_func_t callable, F32 seconds)
	:	LLEventTimer(seconds),
		mCallable(callable)
	{
	}

private:
	LL_INLINE bool tick() override
	{
		// Auto-abort (meaning committing suicide too) when application is
		// exiting. HB
		return LLApp::isExiting() || mCallable();
	}

private:
	bool_func_t	mCallable;
};

void doPeriodically(bool_func_t callable, F32 seconds)
{
	new BoolFuncEventTimer(callable, seconds);
}
