/**
 * @file llthreadpool.h
 * @brief Configures a LLWorkQueue along with a pool of threads to service it.
 * @author Nat Goodspeed
 * @date   2021-10-21
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 *
 * Copyright (c) 2021, Linden Research, Inc. (c) 2022 Henri Beauchamp.
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

#include "hbfastmap.h"
#include "llworkqueue.h"

class LLThreadPool
{
protected:
	LOG_CLASS(LLThreadPool);

public:
	// LLThreadPool takes a string name. This can be used to look up the
	// relevant LLWorkQueue.
	LLThreadPool(const std::string& name, U32 threads = 1,
				 // The default capacity is huge to avoid blocking the main
				 // thread due to a starvation.
				 U32 capacity = 1024 * 1024);
	virtual ~LLThreadPool() = default;

	// Launch the LLThreadPool. Until this call, a constructed LLThreadPool
	// launches no threads. That permits coders to derive from LLThreadPool,
	// or store it as a member of some other class, but refrain from launching
	// it until all other construction is complete.
	// If wait_for_start is true, wait until all threads have actually started
	// before returning to the caller. HB
	void start(bool wait_for_start = false);

	// LLThreadPool listens for application shutdown messages on the "LLApp"
	// LLEventPump. Call close() to shut down this LLThreadPool early. Note
	// that this is a wrapper to the "real" close(), so that the "on_shutdown"
	// and "on_crash" booleans cannot be wrongly used in the latter. HB
	LL_INLINE void close()							{ close(false, false); }

	LL_INLINE const std::string& getName() const	{ return mName; }
	LL_INLINE U32 getWidth() const					{ return mThreads.size(); }

	// Number of threads used to service the queue. HB
	LL_INLINE U32 getThreadsCount() const			{ return mThreadCount; }
	// Number of threads actually and currently started. HB
	LL_INLINE U32 getStartedThreads() 				{ return mStartedThreads; }

	// Override this if you do not want your thread to be accounted as
	// "started" by LLThreadPool::run(const std::string& name) before some
	// initialization work is fully performed in your own run() method; in this
	// case, simply override this with a no-op method, and do call the second,
	// non overridable method below when appropriate in your overriden run()
	// method. HB
	LL_INLINE virtual void maybeIncStartedThreads()	{ ++mStartedThreads; }
	LL_INLINE void doIncStartedThreads()			{ ++mStartedThreads; }

	// Returns the name for a thread with a given thread Id hash, or "invalid"
	// when that hash is not found. HB
	const std::string& getThreadName(U64 id_hash);

	// Obtains a non-const reference to the LLWorkQueue to post work to it.
	LL_INLINE LLWorkQueue& getQueue()				{ return mQueue; }

	// Override run() if you need special processing. The default run()
	// implementation simply calls LLWorkQueue::runUntilClose().
	virtual void run();

private:
	void close(bool on_shutdown, bool on_crash);
	void run(const std::string& name);
	void closeOnShutdown();

private:
	LLWorkQueue		mQueue;
	std::string		mName;
	typedef std::vector<std::pair<std::string, std::thread> > threads_list_t;
	threads_list_t	mThreads;
	LLMutex			mThreadNamesMutex;
	typedef safe_hmap<U64, std::string> tnames_map_t;
	tnames_map_t	mThreadNames;
	// mStartedThreads is incremented each time a new thread is actually
	// started since threads launch is itself a threaded operation; thus why
	// we also must use an atomic counter here. HB
	LLAtomicU32		mStartedThreads;
	U32				mThreadCount;
};
