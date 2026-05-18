/**
 * @file llthread.cpp
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

#include "linden_common.h"

#include "llthread.h"

#include "lltimer.h"
#include "llsys.h"

#if LL_WINDOWS
constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

# pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;		// Must be 0x1000.
	LPCSTR szName;		// Pointer to name (in user addr space).
	DWORD dwThreadID;	// Thread ID (-1=caller thread).
	DWORD dwFlags;		// Reserved for future use, must be zero.
} THREADNAME_INFO;
# pragma pack(pop)

// LL_NO_INLINE is required here, so that the _try/__except stays out of
// LLThread::setThreadName(). HB
LL_NO_INLINE void set_thread_name(THREADNAME_INFO info)
{
	__try
	{
		::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD),
						 (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
#endif

//static
void LLThread::setThreadName(std::string thread_name)
{
#if LL_WINDOWS
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = thread_name.c_str();
	info.dwThreadID = GetCurrentThreadId();
	info.dwFlags = 0;
	// We need to set this out of the LLThread class because VS2022 refuses to
	// use __try otherwise, complaining about stack unwinding issue in objects,
	// even though LLThread::setThreadName() is a static method !!!  HB
	set_thread_name(info);
#else
	// pthread_setname_np() is limited to 15 characters...
	size_t len = thread_name.size();
	if (len > 15)
	{
		// Our thread names can be thread_class:thread_name:thread_id, so let's
		// shorten such names by droping the 'thread_class:' prefix. HB
		size_t i = thread_name.find(':');
		if (i != std::string::npos && i + 2 < len)
		{
			thread_name = thread_name.substr(i + 1);
			if (thread_name.size() > 15)
			{
				thread_name.erase(15);
			}
		}
		else
		{
			thread_name.erase(15);
		}
	}
	pthread_setname_np(pthread_self(), thread_name.c_str());
#endif
}

// Caching the current thread Id in a thread_local variable for speed... HB
static thread_local LLThread::id_t tThreadId = std::this_thread::get_id();

static LLThread::id_t get_main_thread_id()
{
	// Using a function-static variable to identify the main thread requires
	// that control reaches here from the main thread before it reaches here
	// from any other thread. We simply trust that whichever thread gets here
	// first is the main thread.
	static LLThread::id_t main_thread_id = tThreadId;
	return main_thread_id;
}

bool is_main_thread()
{
	return tThreadId == get_main_thread_id();
}

void assert_main_thread()
{
	if (tThreadId != get_main_thread_id())
	{
		llerrs << "Illegal execution from thread id " << tThreadId
			   << " outside main thread " << get_main_thread_id() << llendl;
	}
}

//static
LLThread::id_t LLThread::currentID()
{
	return tThreadId;
}

//static
U64 LLThread::thisThreadIdHash()
{
	// Caching the hash in a thread_local static variable for speed. HB
	thread_local U64 id_hash = std::hash<id_t>()(tThreadId);
	return id_hash;
}

//static
void LLThread::yield()
{
	std::this_thread::yield();
}

LLThread::LLThread(const std::string& name)
:	mName(name),
	mThreadp(NULL),
	mStatus(STOPPED),
	mRetries(1),
	mPaused(false),
	mNeedsAffinity(false)
{
	mRunCondition = new LLCondition();
	mDataLock = new LLMutex();
}

LLThread::~LLThread()
{
	shutdown();
}

void LLThread::threadRun()
{
	setThreadName(mName);
	mID = tThreadId;
	llinfos << "Running thread " << mName << " with Id: " << mID << llendl;

	// Set the CPU affinity for this child thread to the complementary of the
	// main thread affinity, so that they run on different cores.
	// When the main thread affinity is 0 this call is a no-operation and no
	// affinity is set for any thread. HB
	S32 result = LLCPUInfo::setThreadCPUAffinity();
	if (!result)
	{
		llwarns << "Failed to set CPU affinity for thread: " << mName
				<< " - Id: " << mID << llendl;
	}
	else if (result == -1)
	{
		mNeedsAffinity = true;
	}

	while (mRetries)
	{
		--mRetries;
		LL_DEBUGS("Threads") << "Running: " << mName << " - Retries left: "
							 << mRetries << LL_ENDL;
		try
		{
			// Run the user supplied function
			run();
		}
		catch (std::runtime_error& e)
		{
			llwarns << "Caught exception '" << e.what() << "' in thread: "
					<< mName << " - Id: " << mID << llendl;
			continue;
		}
		catch (...)
		{
			llwarns << "An unknown exception occurred during thread"
					<< mName << " - Id: " << mID << llendl;
		}
		break;
	}

	LL_DEBUGS("Threads") << "Exiting: " << mName << " - Id: "
						 << mID << LL_ENDL;

	// We are done with the run function, this thread is done executing now.
	mStatus = STOPPED;
}

void LLThread::shutdown()
{
	// WARNING: if you somehow call the thread destructor from itself, the
	// thread will die in an unclean fashion !
	if (mThreadp)
	{
		if (!isStopped())
		{
			// The thread is not already stopped. First, set the flag
			// indicating that we are ready to die
			setQuitting();

			LL_DEBUGS("Threads") << "Killing thread: " << mName << " Status: "
								 << mStatus << LL_ENDL;
			// Now wait a bit for the thread to exit. It is unclear whether I
			// should even bother doing this; this destructor should never get
			// called unless we are already stopped, really...
			S32 counter = 0;
			constexpr S32 MAX_WAIT = 250;
			while (counter < MAX_WAIT)
			{
				if (isStopped())
				{
					break;
				}
				ms_sleep(1);
				yield();
				++counter;
			}
		}

		if (!isStopped())
		{
			// This thread just would not stop, even though we gave it time
			llwarns << "Exiting thread before clean exit !" << llendl;
			// Note: since the thread has been detached when started, we can
			// safely terminate it now, without any risk to cause a termination
			// of the main process.
#if LL_WINDOWS
			TerminateThread(mNativeHandle, 0);
#else
			pthread_cancel(mNativeHandle);
#endif
		}
		delete mThreadp;
		mThreadp = NULL;
	}

	delete mRunCondition;
	mRunCondition = NULL;

	delete mDataLock;
	mDataLock = NULL;
}

void LLThread::start()
{
	llassert(isStopped());

	// Set thread state to running
	mStatus = RUNNING;

	try
	{
		mThreadp = new std::thread(std::bind(&LLThread::threadRun, this));
		mNativeHandle = mThreadp->native_handle();
		// Detach immediately the thread from the main process, so that it will
		// run independently until its termination.
		mThreadp->detach();
	}
	catch ( ...)
	{
		mStatus = STOPPED;
		llwarns << "Failed to start thread: " << mName << " - Id: " << mID
				<< llendl;
	}
}

// Called from MAIN THREAD. Requests that the thread pauses. The thread will
// pause when (and if) it calls checkPause()
void LLThread::pause()
{
	if (!mPaused)
	{
		// This will cause the thread to stop execution as soon as checkPause()
		// is called. Does not need to be atomic since this is only set/unset
		// from the main thread
		mPaused = true;
	}
}

// Request that the thread pause/resume.
// Called from MAIN THREAD. Requests that the thread resumes.
void LLThread::unpause()
{
	if (mPaused)
	{
		mPaused = false;
	}

	wake(); // Wake up the thread if necessary
}

// Virtual predicate function. Returns true if the thread should wake up, false
// if it should sleep.
bool LLThread::runCondition()
{
	// By default, always run. Handling of pause/unpause is done regardless of
	// this function's result.
	return true;
}

// Called from run() (CHILD THREAD). Stops thread execution if requested until
// unpaused.
void LLThread::checkPause()
{
	if (mNeedsAffinity)
	{
		S32 result = LLCPUInfo::setThreadCPUAffinity();
		if (result == 1)
		{
			mNeedsAffinity = false;
		}
		else if (!result)
		{
			llwarns << "Failed to set CPU affinity for thread: " << mName
					<< " - Id: " << mID << llendl;
		}
	}

	mDataLock->lock();

	// This is in a while loop because the pthread API allows for spurious
	// wakeups.
	while (shouldSleep())
	{
		mDataLock->unlock();
		mRunCondition->wait(); // Locks mRunCondition
		mDataLock->lock();
		// mRunCondition is locked when the thread wakes up
	}

 	mDataLock->unlock();
}

void LLThread::setQuitting()
{
	mDataLock->lock();
	if (mStatus == RUNNING)
	{
		mStatus = QUITTING;
	}
	// It is only safe to remove mRunCondition if all locked threads were
	// notified
	mRunCondition->broadcast();
	mDataLock->unlock();
}

void LLThread::wake()
{
	mDataLock->lock();
	if (!shouldSleep())
	{
		mRunCondition->signal();
	}
	mDataLock->unlock();
}

void LLThread::wakeLocked()
{
	if (!shouldSleep())
	{
		mRunCondition->signal();
	}
}
