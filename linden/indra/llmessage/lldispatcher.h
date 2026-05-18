/**
 * @file lldispatcher.h
 * @brief LLDispatcher class header file.
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

#pragma once

#include "llstring.h"

class LLDispatcher;
class LLMessageSystem;
class LLUUID;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDispatchHandler
//
// Abstract base class for handling dispatches. Derive your own classes,
// construct them, and add them to the dispatcher you want to use.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLDispatchHandler
{
public:
	LLDispatchHandler() = default;
	virtual ~LLDispatchHandler() = default;

	virtual bool operator()(const LLDispatcher* dispatcher,
							const std::string& key,
							const LLUUID& invoice,
							const strings_vec_t& string) = 0;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDispatcher
//
// Basic utility class that handles dispatching keyed operations to function
// objects implemented as LLDispatchHandler derivations.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLDispatcher
{
protected:
	LOG_CLASS(LLDispatcher);

public:
	LLDispatcher() = default;
	virtual ~LLDispatcher() = default;

	// Returns true if they keyed handler exists in this dispatcher.
	LL_INLINE bool isHandlerPresent(const std::string& name) const
	{
		return mHandlers.count(name) != 0;
	}

	// Call this method with the name of the request that has come in. If the
	// handler is present, it is called with the params and returns the return
	// value from.
	bool dispatch(const std::string& name, const LLUUID& invoice,
				  const strings_vec_t& strings) const;
				  //const iparam_t& itegers) const;

	// Add a handler. If one with the same key already exists, its pointer is
	// returned, otherwise returns NULL. This object does not do memory
	// management of the LLDispatchHandler, and relies on the caller to delete
	// the object if necessary.
	LLDispatchHandler* addHandler(const std::string& name,
								  LLDispatchHandler* func);

	// Helper method to unpack the dispatcher message bus format. Returns true
	// on success.
	static bool unpackMessage(LLMessageSystem* msg, std::string& method,
							  LLUUID& invoice, strings_vec_t& parameters);

	static bool unpackLargeMessage(LLMessageSystem* msg, std::string& method,
								   LLUUID& invoice, strings_vec_t& parameters);

protected:
	typedef std::map<std::string, LLDispatchHandler*,
					 std::less<> > dispatch_map_t;
	dispatch_map_t mHandlers;
};
