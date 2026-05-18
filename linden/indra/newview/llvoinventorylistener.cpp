/**
 * @file llvoinventorylistener.cpp
 * @brief Interface for classes that wish to receive updates about viewer object inventory
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llvoinventorylistener.h"

#include "llviewerobject.h"

//static
LLVOInventoryListener::listeners_list_t LLVOInventoryListener::sListeners;

LLVOInventoryListener::LLVOInventoryListener()
{
	sListeners.insert(this);
}

LLVOInventoryListener::~LLVOInventoryListener()
{
	removeVOInventoryListeners();
	sListeners.erase(this);
}

void LLVOInventoryListener::removeVOInventoryListener(LLViewerObject* object)
{
	if (!object)
	{
		object = mListenerVObject;
	}

	if (object && mListenerVObjects.count(object))
	{
		object->removeInventoryListener(this);
		mListenerVObjects.erase(object);
		if (mListenerVObject == object)
		{
			mListenerVObject = NULL;
		}
	}
}

void LLVOInventoryListener::removeVOInventoryListeners()
{
	while (!mListenerVObjects.empty())
	{
		objects_list_t::iterator it = mListenerVObjects.begin();
		removeVOInventoryListener(*it);
	}
}

void LLVOInventoryListener::registerVOInventoryListener(LLViewerObject* object,
														void* user_data)
{
	if (object && !object->isDead())
	{
		removeVOInventoryListener(object);
		mListenerVObject = object;
		mListenerVObjects.insert(object);
		object->registerInventoryListener(this, user_data);
	}
}

void LLVOInventoryListener::requestVOInventory(LLViewerObject* object)
{
	if (!object)
	{
		object = mListenerVObject;
	}

	if (object && !object->isDead())
	{
		object->requestInventory();
	}
}

void LLVOInventoryListener::clearVOInventoryListener(LLViewerObject* object)
{
	mListenerVObjects.erase(object);
	if (mListenerVObject == object)
	{
		mListenerVObject = NULL;
	}
}

bool LLVOInventoryListener::hasRegisteredListener(LLViewerObject* object)
{
	return object && mListenerVObjects.count(object) != 0;
}

//static
void LLVOInventoryListener::removeObjectFromListeners(LLViewerObject* object)
{
	if (!object) return;

	for (listeners_list_t::iterator it = sListeners.begin(),
									end = sListeners.end();
		 it != end; ++it)
	{
		LLVOInventoryListener* listener = *it;
		if (listener)	// Paranoia
		{
			listener->removeVOInventoryListener(object);
		}
	}
}
