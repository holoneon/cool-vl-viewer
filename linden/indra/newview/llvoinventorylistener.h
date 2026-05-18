/**
 * @file llvoinventorylistener.h
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

// Description of LLVOInventoryListener class, which is an interface
// for windows that are interested in updates to a ViewerObject's inventory.

#pragma once

#include "llinventory.h"

class LLViewerObject;

class LLVOInventoryListener
{
public:
	virtual void inventoryChanged(LLViewerObject* object,
								 LLInventoryObject::object_list_t* inventory,
								 S32 serial_num,
								 void* user_data) = 0;

	// Remove the listener from the object and clear this listener
	// When object == NULL, mListenerVObject is used.
	void removeVOInventoryListener(LLViewerObject* object = NULL);

	// Remove all the listeners from the object and clear them
	void removeVOInventoryListeners();

	// Just clear this listener, don't worry about the object.
	// This assumes mListenerVObjects are clearing their own lists.
	// Used only in LLViewerObject::LLInventoryCallbackInfo's destructor.
	void clearVOInventoryListener(LLViewerObject* object);

	// Did we already register a listener with that object ?
	bool hasRegisteredListener(LLViewerObject* object);

	// This does the cleaning-up by removing object from all existing
	// listeners. This is called by LLViewerObject::markDead()
	static void removeObjectFromListeners(LLViewerObject* object);

protected:
	LLVOInventoryListener();
	virtual ~LLVOInventoryListener();

	void registerVOInventoryListener(LLViewerObject* object, void* user_data);

	// When object == NULL, mListenerVObject is used.
	void requestVOInventory(LLViewerObject* object = NULL);

private:
	// LLViewerObject is normally wrapped by an LLPointer, but not in this
	// case, because the listeners are cleaned up from an object as soon as it
	// is marked dead.

	// This holds the last added object (for compatibility with the old
	// one-object request per listener interface)
	LLViewerObject*			mListenerVObject;

	// This holds the data for multiple objects.
	typedef safe_hset<LLViewerObject*> objects_list_t;
	objects_list_t			mListenerVObjects;

	typedef safe_hset<LLVOInventoryListener*> listeners_list_t;
	static listeners_list_t	sListeners;
};
