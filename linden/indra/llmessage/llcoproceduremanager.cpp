/**
 * @file llcoproceduremanager.cpp
 * @author Rider Linden
 * @brief Singleton class for managing asset uploads to the sim.
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 *
 * Copyright (c) 2015-2022, Linden Research, Inc.
 * Copyright (c) 2019-2025, Henri Beauchamp.
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

#include <deque>
#include <mutex>

#include "boost/fiber/condition_variable.hpp"

#include "llcoproceduremanager.h"

#include "llatomic.h"

#define DEFAULT_POOL_SIZE 5

// Map of pool sizes for known pools
static std::map<std::string, U32, std::less<> > sDefaultPoolSizes
{
	{ std::string("Upload"), 1 },
	{ std::string("AssetStorage"), 16 },
	// Keep AIS serialized to avoid getting COF out-of-sync
	{ std::string("AIS"), 1 }
};

///////////////////////////////////////////////////////////////////////////////
// LLCoprocedureQueue template class. It used to be LLThreadSafeQueue, but was
// only used here by LLCoprocedurePool, and the "performance viewer" changes
// (i.e. the complexification) to the new LLThreadSafeQueue are of no interest
// to LLCoprocedurePool, much to the contrary, since the new queue can throw()
// (while we thoroughly avoid that with this old implementation), and cannot
// use fiber-aware mutexes (because they break normal mutexes used elsewhere)
// unlike here, where they *are* needed to avoid promises being badly locked
// (which only causes spurious, harmless warnings, but still)... So I moved the
// old queue code here, further simplified it (to remove unused methods) and
// renamed it as LLCoprocedureQueue. HB
///////////////////////////////////////////////////////////////////////////////

template<typename ElementT>
class LLCoprocedureQueue
{
public:
	typedef ElementT value_type;

	LL_INLINE LLCoprocedureQueue()
	:	mMaxUsage(0)
	{
	}

	// Tries to add an element to the front of the queue without blocking.
	// Returns true only if the element was actually added.
	bool tryPushFront(const ElementT& element)
	{
		std::unique_lock<decltype(mLock)> lock1(mLock, std::defer_lock);
		if (!lock1.try_lock())
		{
			return false;
		}

		mStorage.push_front(element);

		// Keep track of max usage for mStorage. HB
		U32 usage = mStorage.size();
		if (usage > mMaxUsage)
		{
			mMaxUsage = usage;
		}

		// Notify that the queue is no more empty and we can popBack() again.
		mEmptyCond.notify_one();
		return true;
	}

	// Pops the element at the end of the queue (will block if the queue is
	// empty).
	ElementT popBack()
	{
		std::unique_lock<decltype(mLock)> lock1(mLock);
		while (true)
		{
			if (!mStorage.empty())
			{
				ElementT value = mStorage.back();
				mStorage.pop_back();
				return value;
			}

			// Storage empty. Wait for signal.
			mEmptyCond.wait(lock1);
		}
	}

	// Returns the size of the queue.
	LL_INLINE size_t size()
	{
		std::unique_lock<decltype(mLock)> lock(mLock);
		return mStorage.size();
	}

	// Returns the largest size mStorage ever reached during the session. For
	// statistics purposes. HB
	LL_INLINE U32 getMaxUsage() const				{ return mMaxUsage; }

private:
	std::deque<ElementT>				mStorage;
	boost::fibers::mutex				mLock;
	boost::fibers::condition_variable	mEmptyCond;
	U32									mMaxUsage;
};

///////////////////////////////////////////////////////////////////////////////
// LLCoprocedurePool class
///////////////////////////////////////////////////////////////////////////////

class LLCoprocedurePool
{
protected:
	LOG_CLASS(LLCoprocedurePool);

public:
	// Non-copyable
	LLCoprocedurePool(const LLCoprocedurePool&) = delete;
	LLCoprocedurePool& operator=(const LLCoprocedurePool&) = delete;

	typedef LLCoprocedureManager::coprocedure_t coprocedure_t;

	LLCoprocedurePool(const std::string& name, size_t size);

	LLUUID enqueueCoprocedure(const std::string& name, coprocedure_t proc);

	void shutdown();

	LL_INLINE U32 countActive() 			{ return mNumActiveCoprocs; }
	LL_INLINE U32 countPending() 			{ return mNumPendingCoprocs; }

	LL_INLINE U32 count()
	{
		return mNumActiveCoprocs + mNumPendingCoprocs;
	}

private:
	void coprocedureInvokerCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adaptp);

private:
	struct QueuedCoproc
	{
		typedef std::shared_ptr<QueuedCoproc> ptr_t;

		QueuedCoproc(const std::string& name, const LLUUID& id,
					 coprocedure_t proc)
		:	mName(name),
			mId(id),
			mProc(proc)
		{
		}

		std::string					mName;
		LLUUID						mId;
		coprocedure_t				mProc;
	};

	std::string						mPoolName;

	LLEventStream					mWakeupTrigger;

	typedef std::map<std::string,
					 LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t> adapter_map_t;
	adapter_map_t					mCoroMapping;

	typedef LLCoprocedureQueue<QueuedCoproc::ptr_t> coproc_queue_t;
	coproc_queue_t					mPendingCoprocs;

	// Atomic to save us from using costly mutexes. HB
	LLAtomicU32						mNumActiveCoprocs;
	LLAtomicU32						mNumPendingCoprocs;

	LLCore::HttpRequest::policy_t	mHTTPPolicy;

	bool							mShutdown;
};

LLCoprocedurePool::LLCoprocedurePool(const std::string& pool_name, size_t size)
:	mPoolName(pool_name),
	mNumActiveCoprocs(0),
	mNumPendingCoprocs(0),
	mShutdown(false),
	mPendingCoprocs(),
	mWakeupTrigger("CoprocedurePool" + pool_name, true),
	mHTTPPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID)
{
	std::string adapt_name = mPoolName + "Adapter";
	std::string full_name = "LLCoprocedurePool(" + mPoolName +
							")::coprocedureInvokerCoro";
	std::string coro_name;
	for (size_t count = 0; count < size; ++count)
	{
		LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adaptp =
			std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>(adapt_name,
																   mHTTPPolicy);
		coro_name =
			gCoros.launch(full_name,
						  std::bind(&LLCoprocedurePool::coprocedureInvokerCoro,
									this, adaptp));
		mCoroMapping.emplace(coro_name, adaptp);
	}

	llinfos << "Created coprocedure pool \"" << mPoolName << "\" with " << size
			<< " coroutine" << (size > 1 ? "s." : ".") << llendl;

	mWakeupTrigger.post(LLSD());
}

void LLCoprocedurePool::shutdown()
{
	llinfos << "Maximum pending coprocedures queued for pool \"" << mPoolName
			<< "\": " << mPendingCoprocs.getMaxUsage() << llendl; 
	mShutdown = true;
	mWakeupTrigger.post(LLSD());
}

LLUUID LLCoprocedurePool::enqueueCoprocedure(const std::string& name,
											 coprocedure_t proc)
{
	LLUUID id;
	id.generate();
	if (mPendingCoprocs.tryPushFront(std::make_shared<QueuedCoproc>(name, id,
																	proc)))
	{
		++mNumPendingCoprocs;
		LL_DEBUGS("CoreHttp") << "Coprocedure(" << name
							  << ") enqueued with id=" << id << " in pool: "
							  << mPoolName << LL_ENDL;

		mWakeupTrigger.post(LLSD());
		return id;
	}

	llwarns << "Failure to enqueue new coprocedure " << name << " in pool: "
			<< mPoolName << llendl;
	return LLUUID::null;
}

void LLCoprocedurePool::coprocedureInvokerCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adaptp)
{
	while (!mShutdown)
	{
		llcoro::suspendUntilEventOn(mWakeupTrigger);
		while (!mShutdown && mPendingCoprocs.size())
		{
			QueuedCoproc::ptr_t coproc = mPendingCoprocs.popBack();
			if (!coproc)
			{
				break;
			}
			++mNumActiveCoprocs;
			--mNumPendingCoprocs;
			LL_DEBUGS("CoreHttp") << "Dequeued and invoking coprocedure("
								  << coproc->mName << ") with id="
								  << coproc->mId << " in pool: " << mPoolName
								  << LL_ENDL;

			try
			{
				coproc->mProc(adaptp, coproc->mId);
			}
			catch (std::exception& e)
			{
				llwarns << "Coprocedure(" << coproc->mName << ") id="
						<< coproc->mId << " threw an exception !  Message=\""
						<< e.what() << "\"" << " in pool: " << mPoolName
						<< llendl;
			}
			catch (...)
			{
				llwarns << "A non std::exception was thrown from "
						<< coproc->mName << " with id=" << coproc->mId
						<< " in pool: " << mPoolName << llendl;
			}

			--mNumActiveCoprocs;
			LL_DEBUGS("CoreHttp") << "Finished coprocedure("
								  << coproc->mName << ") in pool: "
								  << mPoolName
								  << " - Coprocedures still active: "
								  << mNumActiveCoprocs
								  << " - Coprocedures still pending: "
								  << mNumPendingCoprocs << LL_ENDL;
		}
	}

	llinfos << "Exiting coroutine for pool: " << mPoolName << llendl;
}

///////////////////////////////////////////////////////////////////////////////
// LLCoprocedureManager class
///////////////////////////////////////////////////////////////////////////////

LLCoprocedureManager::~LLCoprocedureManager()
{
	mPropertyQueryFn = nullptr;
	mPropertyDefineFn = nullptr;
}

LLCoprocedureManager::pool_ptr_t LLCoprocedureManager::initializePool(const std::string& pool_name)
{
	// Attempt to look up a pool size in the configuration. If found use it.
	std::string key_name = "PoolSize" + pool_name;
	size_t size = 0;

	if (pool_name.empty())
	{
		llerrs << "Poolname must not be empty" << llendl;
	}

	if (mPropertyQueryFn)
	{
		size = mPropertyQueryFn(key_name);
	}

	if (size == 0)
	{
		// If not found grab the known default... If there is no known default
		// use a reasonable number like 5.
		std::map<std::string, U32>::iterator it =
			sDefaultPoolSizes.find(pool_name);
		size = it == sDefaultPoolSizes.end() ? DEFAULT_POOL_SIZE : it->second;

		if (mPropertyDefineFn)
		{
			mPropertyDefineFn(key_name, size);
		}
		llinfos << "No setting for \"" << key_name
				<< "\" setting pool size to default of " << size << llendl;
	}

	pool_ptr_t pool = std::make_shared<LLCoprocedurePool>(pool_name, size);
	if (!pool)
	{
		llerrs << "Unable to create pool named \"" << pool_name << "\" FATAL !"
			   << llendl;
	}
	mPoolMap.emplace(pool_name, pool);

	return pool;
}

// Attempts to find the pool and enqueue the procedure. If the pool does not
// exist, creates it.
LLUUID LLCoprocedureManager::enqueueCoprocedure(const std::string& pool,
												const std::string& name,
												coprocedure_t proc)
{
	pool_ptr_t poolp;

	pool_map_t::iterator it = mPoolMap.find(pool);
	if (it == mPoolMap.end() || !it->second)	// Should never happen... HB
	{
		// This should be a llerrs, really...
		llwarns << "Pool " << pool
				<< " was not initialized. Initializing it now (could cause a crash)."
				<< llendl;
		llassert(false);	// ... so make DEBUG builds crash here. HB
		poolp = initializePool(pool);
	}
	else
	{
		poolp = it->second;
	}

	return poolp->enqueueCoprocedure(name, proc);
}

void LLCoprocedureManager::cleanup()
{
	// Notify all pool coroutines that they must shut down and exit as soon as
	// they will resume.
	for (pool_map_t::const_iterator it = mPoolMap.begin(), end = mPoolMap.end();
		 it != end; ++it)
	{
		if (it->second)
		{
			it->second->shutdown();
		}
	}
	// Note: we do NOT mPoolMap.clear() here, because this method is called
	// from the main coroutine in LLAppViewer::disconnectViewer(), before
	// LLAppViewer::cleanup() pumps the 'mainloop' and yields to the coroutines
	// which will cause llcoro::suspendUntilEventOn(mWakeupTrigger) to return
	// in the loop of LLCoprocedurePool::coprocedureInvokerCoro(), which still
	// needs to have its pools around !  mPoolMap will instead be cleared
	// implicitely in the destructor of LLCoprocedureManager. HB
}

void LLCoprocedureManager::setPropertyMethods(setting_query_t queryfn,
											  setting_upd_t updatefn)
{
	mPropertyQueryFn = queryfn;
	mPropertyDefineFn = updatefn;

	// Workaround until we get mutex into initializePool
	initializePool("Upload");
}

U32 LLCoprocedureManager::countPending() const
{
	U32 count = 0;
	for (pool_map_t::const_iterator it = mPoolMap.begin(), end = mPoolMap.end();
		 it != end; ++it)
	{
		if (it->second)
		{
			count += it->second->countPending();
		}
	}
	return count;
}

U32 LLCoprocedureManager::countPending(const std::string& pool) const
{
	pool_map_t::const_iterator it = mPoolMap.find(pool);
	return it != mPoolMap.end() && it->second ? it->second->countPending() : 0;
}

U32 LLCoprocedureManager::countActive() const
{
	U32 count = 0;
	for (pool_map_t::const_iterator it = mPoolMap.begin(), end = mPoolMap.end();
		 it != end; ++it)
	{
		if (it->second)
		{
			count += it->second->countActive();
		}
	}
	return count;
}

U32 LLCoprocedureManager::countActive(const std::string& pool) const
{
	pool_map_t::const_iterator it = mPoolMap.find(pool);
	return it != mPoolMap.end() && it->second ? it->second->countActive() : 0;
}

U32 LLCoprocedureManager::count() const
{
	U32 count = 0;
	for (pool_map_t::const_iterator it = mPoolMap.begin(), end = mPoolMap.end();
		 it != end; ++it)
	{
		if (it->second)
		{
			count += it->second->count();
		}
	}
	return count;
}

U32 LLCoprocedureManager::count(const std::string& pool) const
{
	pool_map_t::const_iterator it = mPoolMap.find(pool);
	return it != mPoolMap.end() && it->second ? it->second->count() : 0;
}
