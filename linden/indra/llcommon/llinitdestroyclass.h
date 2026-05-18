/**
 * @file llinitdestroyclass.h
 * @brief LLInitClass / LLDestroyClass mechanism
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

#include "boost/signals2.hpp"

#include "llerror.h"
#include "llsingleton.h"	// Also includes <typeinfo>

// LLCallbackRegistry is an implementation detail base class for
// LLInitClassList and LLDestroyClassList. It is a very thin wrapper around a
// boost.signals2 signal object.
class LLCallbackRegistry
{
public:
	typedef boost::signals2::signal<void()> callback_signal_t;

	void registerCallback(const callback_signal_t::slot_type& slot)
	{
		mCallbacks.connect(slot);
	}

	void fireCallbacks()
	{
		mCallbacks();
	}

private:
	callback_signal_t mCallbacks;
};

// LLInitClassList is the LLCallbackRegistry for LLInitClass. It stores the
// registered initClass() methods. It must be an LLSingleton because
// LLInitClass registers its initClass() method at static construction time
// (before main()), requiring LLInitClassList to be fully constructed on
// demand regardless of module initialization order.
class LLInitClassList : public LLCallbackRegistry,
						public LLSingleton<LLInitClassList>
{
	friend class LLSingleton<LLInitClassList>;

private:
	LLInitClassList()								{}
};

// LLDestroyClassList is the LLCallbackRegistry for LLDestroyClass. It stores
// the registered destroyClass() methods. It must be an LLSingleton because
// LLDestroyClass registers its destroyClass() method at static construction
// time (before main()), requiring LLDestroyClassList to be fully constructed
// on demand regardless of module initialization order.
class LLDestroyClassList : public LLCallbackRegistry,
						   public LLSingleton<LLDestroyClassList>
{
	friend class LLSingleton<LLDestroyClassList>;

private:
	LLDestroyClassList()							{}
};

// LLRegisterWith is an implementation detail for LLInitClass and
// LLDestroyClass. It is intended to be used as a static class member whose
// constructor registers the specified callback with the LLMumbleClassList
// singleton registry specified as the template argument.
template<typename T>
class LLRegisterWith
{
public:
	LLRegisterWith(std::function<void()> func)
	{
		T::getInstance()->registerCallback(func);
	}

	// This avoids a MSVC bug where non-referenced static members are
	// "optimized" away even if their constructors have side effects
	void reference()
	{
#if LL_WINDOWS && !LL_CLANG
		S32 dummy;
		dummy = 0;
#endif
	}
};

// Derive MyClass from LLInitClass<MyClass> (the Curiously Recurring Template
// Pattern) to ensure that the static method MyClass::initClass() will be
// called (along with all other LLInitClass<T> subclass initClass() methods)
// when someone calls LLInitClassList::getInstance()->fireCallbacks(). This
// gives the application specific control over the timing of all such
// initializations, without having to insert calls for every such class into
// generic application code.
template<typename T>
class LLInitClass
{
public:
	LLInitClass()									{ sRegister.reference(); }

public:
	// When this static member is initialized, the subclass initClass() method
	// is registered on LLInitClassList. See sRegister definition below.
	static LLRegisterWith<LLInitClassList> sRegister;

private:
	// Provides a default initClass() method in case subclass misspells (or
	// omits) initClass(). This turns a potential build error into a fatal
	// runtime error.
	static void initClass()
	{
		llerrs << "No static initClass() method defined for "
			   << typeid(T).name() << llendl;
	}
};

// Derive MyClass from LLDestroyClass<MyClass> (the Curiously Recurring
// Template Pattern) to ensure that the static method MyClass::destroyClass()
// will be called (along with other LLDestroyClass<T> subclass destroyClass()
// methods) when someone calls
// LLDestroyClassList::getInstance()->fireCallbacks().
// This gives the application specific control over the timing of all such
// cleanup calls, without having to insert calls for every such class into
// generic application code.
template<typename T>
class LLDestroyClass
{
public:
	LLDestroyClass()								{ sRegister.reference(); }

public:
	// When this static member is initialized, the subclass destroyClass()
	// method is registered on LLInitClassList. See sRegister definition below.
	static LLRegisterWith<LLDestroyClassList> sRegister;

private:
	// Provides a default destroyClass() method in case subclass misspells (or
	// omits) destroyClass(). This turns a potential build error into a fatal
	// runtime error.
	static void destroyClass()
	{
		llerrs << "No static destroyClass() method defined for "
			   << typeid(T).name() << llendl;
	}
};

// Here is where LLInitClass<T> specifies the subclass initClass() method.
template <typename T> LLRegisterWith<LLInitClassList> LLInitClass<T>::sRegister(&T::initClass);
// Here is where LLDestroyClass<T> specifies the subclass destroyClass() method
template <typename T> LLRegisterWith<LLDestroyClassList> LLDestroyClass<T>::sRegister(&T::destroyClass);
