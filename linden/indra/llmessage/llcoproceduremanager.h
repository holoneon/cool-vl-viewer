/**
 * @file llcoproceduremanager.h
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

#pragma once

#include "llcorehttputil.h"
#include "llsingleton.h"

class LLCoprocedurePool;

class LLCoprocedureManager final : public LLSingleton<LLCoprocedureManager>
{
	friend class LLSingleton<LLCoprocedureManager>;

protected:
	LOG_CLASS(LLCoprocedureManager);

public:
	typedef std::function<U32(const std::string&)> setting_query_t;

	typedef std::function<void(const std::string&, U32)> setting_upd_t;

	typedef std::function<void(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t&,
							   const LLUUID&)> coprocedure_t;

	LLCoprocedureManager() = default;
	~LLCoprocedureManager();

	// Places a coprocedure on the queue for processing. 'name' is used for
	// debugging and should identify this coroutine. 'proc' is a bound function
	// to be executed. This method returns a generated UUID on success and a
	// null UUID on failure to enqueue 'proc' (queue mutex locked by some other
	// fiber or thread): the caller should then attempt a new enqueuing at next
	// frame.
	LLUUID enqueueCoprocedure(const std::string& pool, const std::string& name,
							  coprocedure_t proc);

	void setPropertyMethods(setting_query_t queryfn, setting_upd_t updatefn);

	// Requests an exit for all the coprocedure manager coroutines.
	void cleanup();

	// Returns the number of coprocedures in the queue awaiting processing.
	U32 countPending() const;
	U32 countPending(const std::string& pool) const;

	// Returns the number of coprocedures actively being processed.
	U32 countActive() const;
	U32 countActive(const std::string& pool) const;

	// Returns the total number of coprocedures either queued or in active
	// processing.
	U32 count() const;
	U32 count(const std::string& pool) const;

	typedef std::shared_ptr<LLCoprocedurePool> pool_ptr_t;
	pool_ptr_t initializePool(const std::string& pool_name);

private:
	typedef std::map<std::string, pool_ptr_t, std::less<> > pool_map_t;
	pool_map_t		mPoolMap;

	setting_query_t	mPropertyQueryFn;
	setting_upd_t	mPropertyDefineFn;
};
