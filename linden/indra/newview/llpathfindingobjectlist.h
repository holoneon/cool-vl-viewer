/**
 * @file llpathfindingobjectlist.h
 * @brief Header file for llpathfindingobjectlist
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

#include <map>
#include <memory>
#include <string>

#include "llpathfindingobject.h"

class LLPathfindingCharacterList;
class LLPathfindingLinksetList;

class LLPathfindingObjectList
{
public:
	typedef std::shared_ptr<LLPathfindingObjectList> ptr_t;

	LLPathfindingObjectList();
	virtual ~LLPathfindingObjectList();

	LL_INLINE virtual LLPathfindingCharacterList* asCharacterList()
	{
		return NULL;
	}

	LL_INLINE virtual const LLPathfindingCharacterList* asCharacterList() const
	{
		return NULL;
	}

	LL_INLINE virtual LLPathfindingLinksetList* asLinksetList()
	{
		return NULL;
	}

	LL_INLINE virtual const LLPathfindingLinksetList* asLinksetList() const
	{
		return NULL;
	}

	void clear();

	LLPathfindingObject::ptr_t find(const LLUUID& obj_id) const;

	LL_INLINE bool isEmpty() const				{ return mObjectMap.empty(); }

	typedef LLPathfindingObject::map_t::const_iterator const_iterator;

	LL_INLINE const_iterator begin() const		{ return mObjectMap.begin(); }

	LL_INLINE const_iterator end() const		{ return mObjectMap.end(); }

	void update(LLPathfindingObject::ptr_t objectp);
	void update(ptr_t object_listp);

protected:
	LL_INLINE LLPathfindingObject::map_t& getObjectMap()
	{
		return mObjectMap;
	}

private:
	LLPathfindingObject::map_t mObjectMap;
};
