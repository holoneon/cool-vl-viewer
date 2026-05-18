/**
 * @file llworkqueue.cpp
 * @brief Queue used for inter-thread work passing.
 * @author Nat Goodspeed
 * @date   2021-09-30
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

#include "linden_common.h"

#include "llworkqueue.h"

#include "llatomic.h"
#include "llevents.h"

LLWorkQueue::LLWorkQueue(const std::string& name, U32 capacity)
:	super(makeName(name)),
	mQueue(capacity)
{
	LLEventPump& pump = gEventPumps.obtain("LLApp");
	pump.listen(getKey(),
				[this](const LLSD& stat)
				{
					std::string status = stat["status"];
					if (status != "running")
					{
						// Note: on crash, the app first goes to "error" status,
						// then to "stopped" status as soon as it ran its error
						// handler. HB
						close(true, status != "quitting");
					}
					return false;
				});
}

LLWorkQueue::~LLWorkQueue()
{
	close(false, false);
}

void LLWorkQueue::close(bool on_shutdown, bool on_crash)
{
	if (mQueue.isClosed())
	{
		return;
	}

	if (on_crash)
	{
		llinfos << "Queue \"" << getKey()
				<< "\" was informed of viewer crash. Closing" << llendl;
	}
	else if (on_shutdown)
	{
		llinfos << "Queue \"" << getKey()
				<< "\" was informed of viewer shutdown. Closing" << llendl;
	}
	else
	{
		llinfos << "Closing queue: " << getKey() << llendl;
	}

	mQueue.close();

	if (!LLEventPumps::destroyed())
	{
		gEventPumps.obtain("LLApp").stopListening(getKey());
	}
}

void LLWorkQueue::runUntilClose()
{
	try
	{
		while (true)
		{
			callWork(mQueue.pop());
			if (mQueue.empty())
			{
				LLThread::yield();
			}
		}
	}
	catch (const Closed&)
	{
	}
}

bool LLWorkQueue::runPending()
{
	try
	{
		for (Work work; mQueue.tryPop(work); )
		{
			callWork(work);
		}
	}
	catch (const Closed&)
	{
	}
	return !mQueue.done();
}

bool LLWorkQueue::runOne()
{
	try
	{
		Work work;
		if (mQueue.tryPop(work))
		{
			callWork(work);
		}
	}
	catch (const Closed&)
	{
	}
	return !mQueue.done();
}

bool LLWorkQueue::runUntil(const TimePoint& until, size_t* work_remaining)
{
	try
	{
		// Should we subtract some slop to allow for typical Work execution
		// time and how much slop ?
		for (Work work; TimePoint::clock::now() < until && mQueue.tryPop(work); )
		{
			callWork(work);
		}
	}
	catch (const Closed&)
	{
	}
	return !mQueue.done(work_remaining);
}

void LLWorkQueue::callWork(const Work& work)
{
	try
	{
		work();
	}
	catch (...)
	{
		// No matter what goes wrong with any individual work item, the worker
		// thread must go on !... Log our own instance name with the exception.
		llwarns << "Work failed for: " << getKey() << llendl;
	}
}

//static
std::string LLWorkQueue::makeName(const std::string& name)
{
	if (!name.empty())
	{
		return name;
	}

	// We use an atomic static variable to avoid bothering with mutex and
	// locks. HB
	static LLAtomicU32 discriminator(0);

	U32 num = discriminator++;
	return llformat("WorkQueue%d", num);
}

//static
void LLWorkQueue::error(const std::string& msg)
{
	llerrs << msg << llendl;
}

#if LL_WAIT_FOR_RESULT
//static
void LLWorkQueue::checkCoroutine(const std::string& method)
{
	// By convention, the default coroutine on each thread has an empty name
	// string.
	if (LLCoros::getName().empty())
	{
		throw(Error("Do not call " + method +
					" from a thread's default coroutine"));
	}
}
#endif
