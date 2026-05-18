/**
 * @file llsingleton.h
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

#include <typeinfo>

#include "hbfastmap.h"
#include "llstring.h"		// For hash_value(const std::string&) override

// A global registry of all singletons to prevent duplicate allocations across
// shared library boundaries
class LLSingletonRegistry
{
private:
	typedef safe_hmap<std::string, void*> type_map_t;
	static type_map_t* sSingletonMap;

	static void checkInit()
	{
		if (!sSingletonMap)
		{
			sSingletonMap = new type_map_t();
		}
	}

public:
	template<typename T> static void*& get()
	{
		std::string name(typeid(T).name());
		checkInit();

		// The first entry of the pair returned by insert will be either the
		// existing iterator matching our key, or the newly inserted NULL
		// initialized entry see "Insert element" in:
		// http://www.sgi.com/tech/stl/UniqueAssociativeContainer.html
		type_map_t::iterator it = sSingletonMap->emplace(name, nullptr).first;
		return it->second;
	}
};

// LLSingleton implements the getInstance() method part of the Singleton
// pattern. It cannot make the derived class constructors protected, though, so
// you have to do that yourself.
//
// Derive your class from LLSingleton, passing your subclass name as
// LLSingleton's template parameter, like so:
//
//	class Foo : public LLSingleton<Foo>
//	{
//	 	friend class LLSingleton<Foo>;
//
//	protected:
//		LOG_CLASS(Foo);
//
//	  .../...
//   };
//
//   Foo* instance = Foo::getInstance();
//
// As currently written, LLSingleton is not thread-safe.

// This is to avoid inlining llerrs and llwarns...
LL_NO_INLINE void errorUsedInConstructor(const std::type_info& type);
LL_NO_INLINE void warnAccessingDeletedSingleton(const std::type_info& type);

template <typename DERIVED_TYPE>
class LLSingleton
{
private:
	typedef enum e_init_state
	{
		UNINITIALIZED,
		CONSTRUCTING,
		INITIALIZING,
		INITIALIZED,
		DELETED
	} EInitState;

	// Stores pointer to singleton instance and tracks initialization state of
	// the singleton.
	struct SingletonInstanceData
	{
		EInitState		mInitState;
		DERIVED_TYPE*	mSingletonInstance;

		SingletonInstanceData()
		:	mSingletonInstance(NULL),
			mInitState(UNINITIALIZED)
		{
		}

		~SingletonInstanceData()
		{
			if (mInitState != DELETED)
			{
				deleteSingleton();
			}
		}
	};

public:
	virtual ~LLSingleton()
	{
		SingletonInstanceData& data = getData();
		data.mSingletonInstance = NULL;
		data.mInitState = DELETED;
	}

	// Non-copyable
	LLSingleton& operator=(const LLSingleton&) = delete;

	// This method merely exists to minimize the amount of inlined code, since
	// it normally only gets called once for each singleton and is therefore
	// not time-critical
	static void createSingleton()
	{
		SingletonInstanceData& data = getData();
		data.mInitState = CONSTRUCTING;
		data.mSingletonInstance = new DERIVED_TYPE();
		data.mInitState = INITIALIZING;
		data.mSingletonInstance->initSingleton();
		data.mInitState = INITIALIZED;
	}

	/**
	 * @brief Immediately delete the singleton.
	 *
	 * A subsequent call to LLProxy::getInstance() will construct a new
	 * instance of the class.
	 *
	 * LLSingletons are normally destroyed after main() has exited and the C++
	 * runtime is cleaning up statically-constructed objects. Some classes
	 * derived from LLSingleton have objects that are part of a runtime system
	 * that is terminated before main() exits. Calling the destructor of those
	 * objects after the termination of their respective systems can cause
	 * crashes and other problems during termination of the project. Using this
	 * method to destroy the singleton early can prevent these crashes.
	 *
	 * An example where this is needed is for a LLSingleton that has an APR
	 * object as a member that makes APR calls on destruction. The APR system
	 * is shut down explicitly before main() exits. This causes a crash on
	 * exit. Using this method before the call to apr_terminate() and NOT
	 * calling getInstance() again will prevent the crash.
	 */
	static void deleteSingleton()
	{
		delete getData().mSingletonInstance;
		getData().mSingletonInstance = NULL;
		getData().mInitState = DELETED;
	}

	static LL_INLINE SingletonInstanceData& getData()
	{
		// This is static to cache the lookup results
		static void*& registry = LLSingletonRegistry::get<DERIVED_TYPE>();

		// *TODO - look into making this threadsafe
		if (!registry)
		{
			static SingletonInstanceData data;
			registry = &data;
		}

		return *static_cast<SingletonInstanceData*>(registry);
	}

	static LL_INLINE DERIVED_TYPE* getInstance()
	{
		SingletonInstanceData& data = getData();

		if (data.mInitState == CONSTRUCTING)
		{
			errorUsedInConstructor(typeid(DERIVED_TYPE));
		}
		else if (data.mInitState == DELETED)
		{
			warnAccessingDeletedSingleton(typeid(DERIVED_TYPE));
		}

		if (!data.mSingletonInstance)
		{
			createSingleton();
		}

		return data.mSingletonInstance;
	}

	// Has this singleton been created yet ?  Use this to avoid accessing
	// singletons before they can safely be constructed.
	static LL_INLINE bool instanceExists()
	{
		return getData().mInitState == INITIALIZED;
	}

	// Has this singleton already been deleted ?
	// Use this to avoid accessing singletons from a static object destructor.
	static LL_INLINE bool destroyed()
	{
		return getData().mInitState == DELETED;
	}

private:
	virtual void initSingleton() {}
};
