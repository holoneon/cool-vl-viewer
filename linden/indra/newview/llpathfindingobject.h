/**
 * @file llpathfindingobject.h
 * @brief LLPathfindingObject class declaration
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Linden Research, Inc.
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

#include <functional>
#include <memory>

#include "boost/signals2.hpp"

#include "llavatarnamecache.h"
#include "hbfastmap.h"
#include "lluuid.h"
#include "llvector3.h"

class LLPathfindingCharacter;
class LLPathfindingLinkset;
class LLSD;

class LLPathfindingObject
{
protected:
	LOG_CLASS(LLPathfindingObject);

public:
	typedef std::shared_ptr<LLPathfindingObject> ptr_t;
	typedef fast_hmap<LLUUID, ptr_t> map_t;

	LLPathfindingObject();
	LLPathfindingObject(const LLUUID& id, const LLSD& obj_data);
	LLPathfindingObject(const LLPathfindingObject& obj);
	virtual ~LLPathfindingObject();

	LLPathfindingObject& operator=(const LLPathfindingObject& obj);

	LL_INLINE virtual LLPathfindingLinkset* asLinkset()
	{
		return NULL;
	}

	LL_INLINE virtual const LLPathfindingLinkset* asLinkset() const
	{
		return NULL;
	}

	LL_INLINE virtual LLPathfindingCharacter* asCharacter()
	{
		return NULL;
	}

	LL_INLINE virtual const LLPathfindingCharacter* asCharacter() const
	{
		return NULL;
	}

	LL_INLINE const LLUUID& getUUID() const				{ return mUUID; }
	LL_INLINE const std::string& getName() const		{ return mName; }
	LL_INLINE const std::string& getDescription() const	{ return mDescription; }
	LL_INLINE bool hasOwner() const						{ return mOwnerUUID.notNull(); }
	LL_INLINE bool hasOwnerName() const					{ return mHasOwnerName; }
	std::string getOwnerName() const;
	LL_INLINE bool isGroupOwned() const					{ return mIsGroupOwned; }
	LL_INLINE const LLVector3& getLocation() const		{ return mLocation; }

	typedef std::function<void(const LLPathfindingObject*)> name_callback_t;
	typedef boost::signals2::signal<void(const LLPathfindingObject*)> name_signal_t;
	typedef boost::signals2::connection name_connection_t;

	name_connection_t registerOwnerNameListener(name_callback_t callback);

private:
	void parseObjectData(const LLSD& obj_data);

	void fetchOwnerName();

	static void handleGroupNameFetch(const LLUUID& group_id,
									 const std::string& name, bool is_group,
									 LLPathfindingObject* self);

	void handleAvatarNameFetch(const LLUUID& av_id,
							   const LLAvatarName& av_name);

	void disconnectAvatarNameCacheConnection();

private:
	LLVector3									mLocation;
	LLUUID										mUUID;
	LLUUID										mOwnerUUID;
	LLAvatarName								mOwnerName;
	LLAvatarNameCache::callback_connection_t	mAvatarNameCacheConnection;
	name_signal_t								mOwnerNameSignal;
	bool										mIsGroupOwned;
	bool										mHasOwnerName;
	std::string									mName;
	std::string									mDescription;
	std::string									mGroupName;

	static uuid_list_t							sPendingGroupUUIDs;

	typedef fast_hset<LLPathfindingObject*> pathfinding_obj_list_t;
	static pathfinding_obj_list_t				sGroupQueriesList;
};
