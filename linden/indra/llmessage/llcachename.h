/**
 * @file llcachename.h
 * @brief A cache of names from UUIDs.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llavatarnamecache.h"

class LLMessageSystem;
class LLHost;
class LLUUID;

typedef boost::signals2::signal<void(const LLUUID& id, const std::string& name,
									 bool is_group)> LLCacheNameSignal;
typedef LLCacheNameSignal::slot_type LLCacheNameCallback;

// Here is the theory: if you request a name which is not in the cache, it
// returns "waiting" and requests the data. After the data arrives, you get
// that on subsequent calls. If the data has not been updated in an hour, it
// requests it again, but keeps giving you the old value until new data
// arrives. If you have not requested the data in an hour, it releases it.

class LLCacheName
{
protected:
	LOG_CLASS(LLCacheName);

public:
	LLCacheName(LLMessageSystem* msgp, const std::string& loading,
				const std::string& nobody, const std::string& none);
	~LLCacheName();

	// Registers the upstream host, i.e. the currently connected simulator
	void setUpstream(const LLHost& upstream_host);

	boost::signals2::connection addObserver(const LLCacheNameCallback& callback);

	// Storing cache on disk; for viewer, in name.cache
	bool importFile(std::istream& istr);
	void exportFile(std::ostream& ostr);

	// If available, copies name ("bobsmith123" or "James Linden") into string
	// If not available, copies the string "waiting".
	// Returns true if available.
	bool getName(const LLUUID& id, std::string& first, std::string& last);
	bool getFullName(const LLUUID& id, std::string& full_name);

	// Reverse lookup of UUID from name
	bool getUUID(const std::string& first, const std::string& last,
				 LLUUID& id);
	bool getUUID(const std::string& fullname, LLUUID& id);

	// If available, this method copies the group name into the string
	// provided. If not available, this method copies the string "waiting".
	// Returns true if available.
	bool getGroupName(const LLUUID& id, std::string& group);

	// Call the callback with the group or avatar name.
	// If the data is currently available, may call the callback immediately
	// otherwise, will request the data, and will call the callback when
	// available. There is no guarantee the callback will ever be called.
	boost::signals2::connection get(const LLUUID& id, bool is_group,
									const LLCacheNameCallback& callback);

	// This method needs to be called from time to time to send out requests.
	void processPending();

	// Expire entries created more than "secs" seconds ago.
	void deleteEntriesOlderThan(S32 secs);

	// Debugging
	void dump();		// Dumps the contents of the cache
	void dumpStats();	// Dumps the sizes of the cache and associated queues.
	void clear();		// Deletes all entries from the cache

	const std::string& getDefaultName() const;

	// Clean up new-style "bobsmith123 Resident" names to "bobsmith123" for
	// display
	static std::string buildFullName(const std::string& first,
									 const std::string& last);

	// Clean up legacy "bobsmith123 Resident" to "bobsmith123".
	// If the name does not contain "Resident", returns it unchanged.
	static std::string cleanFullName(const std::string& full_name);

	// Converts a standard legacy name to a username
	// "bobsmith123 Resident" -> "bobsmith"
	// "Random Linden" -> "random.linden"
	static std::string buildUsername(const std::string& name);

	// Converts a complete display name to a legacy name
	// if possible, otherwise returns the input
	// "Alias (random.linden)" -> "Random Linden"
	// "Something random" -> "Something random"
	static std::string buildLegacyName(const std::string& name);

private:
	class Impl;
	Impl& impl;
};

extern LLCacheName* gCacheNamep;
