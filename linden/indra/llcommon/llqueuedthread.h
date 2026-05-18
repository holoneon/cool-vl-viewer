/**
 * @file llqueuedthread.h
 * @brief
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc. (c) 2022 Henri Beauchamp.
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

#include <map>
#include <queue>
#include <set>

#include "llatomic.h"
#include "hbfastmap.h"
#include "llthread.h"

// Note: ~LLQueuedThread is O(N) N=# of queued threads, assumed to be small
// It is assumed that LLQueuedThreads are rarely created/destroyed.

class LLQueuedThread : public LLThread
{
protected:
	LOG_CLASS(LLQueuedThread);

public:
	enum priority_t
	{
		PRIORITY_IMMEDIATE = 0x7FFFFFFF,
		PRIORITY_URGENT =    0x40000000,
		PRIORITY_HIGH =      0x30000000,
		PRIORITY_NORMAL =    0x20000000,
		PRIORITY_LOW =       0x10000000,
		PRIORITY_LOWBITS =   0x0FFFFFFF,
		PRIORITY_HIGHBITS =  0x70000000
	};

	enum status_t
	{
		STATUS_EXPIRED = -1,
		STATUS_UNKNOWN = 0,
		STATUS_QUEUED = 1,
		STATUS_INPROGRESS = 2,
		STATUS_COMPLETE = 3,
		STATUS_ABORTED = 4,
		STATUS_DELETE = 5
	};

	enum flags_t
	{
		FLAG_AUTO_COMPLETE = 1,
		FLAG_AUTO_DELETE = 2,	// Child-class dependent
		FLAG_ABORT = 4,
	};

	typedef U32 handle_t;

	class QueuedRequest
	{
		friend class LLQueuedThread;

	protected:
		LOG_CLASS(QueuedRequest);

		virtual ~QueuedRequest();	// Use deleteRequest()

	public:
		LL_INLINE QueuedRequest(handle_t handle, U32 priority, U32 flags = 0)
		:	mStatus(STATUS_UNKNOWN),
			mHandle(handle),
			mPriority(priority),
			mFlags(flags)
		{
		}

		LL_INLINE status_t getStatus()
		{
			return mStatus;
		}

		LL_INLINE U32 getFlags() const
		{
			return mFlags;
		}

		LL_INLINE U32 getPriority() const
		{
			return mPriority;
		}

		LL_INLINE bool higherPriority(const QueuedRequest& second) const
		{
			if (mPriority == second.mPriority)
			{
				return mHandle < second.mHandle;
			}
			return mPriority > second.mPriority;
		}

	protected:
		LL_INLINE status_t setStatus(status_t newstatus)
		{
			status_t oldstatus = mStatus;
			mStatus = newstatus;
			return oldstatus;
		}

		LL_INLINE void setFlags(U32 flags)
		{
			// NOTE: flags are |'d
			mFlags |= flags;
		}

		// Returns true when request has completed
		virtual bool processRequest() = 0;

		// Always called from thread after request has completed or aborted
		LL_INLINE virtual void finishRequest(bool completed)
		{
		}

		// Only method to delete a request
		virtual void deleteRequest();

		LL_INLINE void setPriority(U32 pri)
		{
			// Only do this on a request that is not in a queued list !
			mPriority = pri;
		}

	protected:
		LLAtomic<status_t>	mStatus;
		handle_t			mHandle;
		U32					mFlags;
		U32					mPriority;
	};

protected:
	struct queued_request_less
	{
		LL_INLINE bool operator()(const QueuedRequest* lhs,
								  const QueuedRequest* rhs) const
		{
			// Higher priority in front of queue (set)
			return lhs->higherPriority(*rhs);
		}
	};

public:
	static handle_t nullHandle()				{ return handle_t(0); }

public:
	LLQueuedThread(const std::string& name);
	~LLQueuedThread() override;

	void shutdown() override;

private:
	// No copy constructor or copy assignment
	LLQueuedThread(const LLQueuedThread&) = delete;
	LLQueuedThread& operator=(const LLQueuedThread&) = delete;

	bool runCondition() override;
	void run() override;

	LL_INLINE virtual void startThread()		{}
	LL_INLINE virtual void endThread()			{}
	LL_INLINE virtual void threadedUpdate()		{}

protected:
	handle_t generateHandle();
	bool addRequest(QueuedRequest* req);
	size_t processNextRequest();
	void setRequestResult(QueuedRequest* req, bool result);

public:
	virtual size_t update();

	void waitOnPending();
	void printQueueStats();

	virtual size_t getPending();

	// Request accessors
	status_t getRequestStatus(handle_t handle);
	void abortRequest(handle_t handle, bool autocomplete);
	void setFlags(handle_t handle, U32 flags);
	void setPriority(handle_t handle, U32 priority);
	bool completeRequest(handle_t handle);

	// This is public for support classes like LLWorkerThread, but generally
	// the methods above should be used.
	QueuedRequest* getRequest(handle_t handle);

protected:
	typedef std::set<QueuedRequest*, queued_request_less> request_queue_t;
	request_queue_t	mRequestQueue;

	handle_t		mNextHandle;

	typedef fast_hmap<handle_t, QueuedRequest*> request_map_t;
	request_map_t	mRequestMap;

	// Request queue is empty (or we are quitting) and the thread is idle
	LLAtomicBool	mIdleThread;
};
