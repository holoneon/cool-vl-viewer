/**
 * @file llmutex.cpp
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

#include "llmutex.h"

#include "lltimer.h"		// For ms_sleep()

bool LLMutex::isLocked()
{
	if (!mMutex.try_lock())
	{
		return true;
	}
	mMutex.unlock();
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// LLMutexTrylock class
///////////////////////////////////////////////////////////////////////////////

LLMutexTrylock::LLMutexTrylock(LLMutex* mutex, U32 attempts)
:	mMutex(mutex),
	mLocked(false)
{
	if (mMutex && attempts > 0)
	{
		while (!(mLocked = mMutex->trylock()) && --attempts > 0)
		{
			ms_sleep(10);
		}
	}
}
