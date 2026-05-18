/**
 * @file llpathfindingobjectlist.cpp
 * @brief Implementation of llpathfindingobjectlist
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

#include "llviewerprecompiledheaders.h"

#include "llpathfindingobjectlist.h"

#include "llpathfindingobject.h"

LLPathfindingObjectList::LLPathfindingObjectList()
{
}

LLPathfindingObjectList::~LLPathfindingObjectList()
{
	clear();
}

void LLPathfindingObjectList::clear()
{
	for (LLPathfindingObject::map_t::iterator it = mObjectMap.begin(),
											  end = mObjectMap.end();
			it != end; ++it)
	{
		it->second.reset();
	}
	mObjectMap.clear();
}

LLPathfindingObject::ptr_t LLPathfindingObjectList::find(const LLUUID& obj_id) const
{
	LLPathfindingObject::ptr_t objectp;

	LLPathfindingObject::map_t::const_iterator it = mObjectMap.find(obj_id);
	if (it != mObjectMap.end())
	{
		objectp = it->second;
	}

	return objectp;
}

void LLPathfindingObjectList::update(LLPathfindingObject::ptr_t objectp)
{
	if (!objectp) return;

	const LLUUID& object_id = objectp->getUUID();

	LLPathfindingObject::map_t::iterator it = mObjectMap.find(object_id);
	if (it == mObjectMap.end())
	{
		mObjectMap.emplace(object_id, objectp);
	}
	else
	{
		it->second = objectp;
	}
}

void LLPathfindingObjectList::update(ptr_t object_listp)
{
	if (object_listp && !object_listp->isEmpty())
	{
		for (const_iterator it = object_listp->begin();
			it != object_listp->end(); ++it)
		{
			LLPathfindingObject::ptr_t objectp = it->second;
			update(objectp);
		}
	}
}
