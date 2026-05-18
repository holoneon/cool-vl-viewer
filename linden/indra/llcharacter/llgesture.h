/**
 * @file llgesture.h
 * @brief A gesture is a combination of a triggering chat phrase or
 * key, a sound, an animation, and a chat string.
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

#include <vector>

#include "llanimationstates.h"
#include "llstring.h"
#include "lluuid.h"

class LLGesture
{
protected:
	LOG_CLASS(LLGesture);

public:
	LLGesture();
	LLGesture(KEY key, MASK mask, const std::string& trigger,
			  const LLUUID& sound_item_id, const std::string& animation,
			  const std::string& output_string);

	LLGesture(U8** buffer, S32 max_size);	// Deserializes, advances buffer
	LLGesture(const LLGesture& gesture);
	const LLGesture& operator=(const LLGesture& rhs);

	virtual ~LLGesture() = default;

	// Accessors
	LL_INLINE KEY getKey() const							{ return mKey; }
	LL_INLINE MASK getMask() const							{ return mMask; }
	LL_INLINE const std::string& getTrigger() const			{ return mTrigger; }
	LL_INLINE const LLUUID& getSound() const				{ return mSoundItemID; }
	LL_INLINE const std::string& getAnimation() const		{ return mAnimation; }
	LL_INLINE const std::string& getOutputString() const	{ return mOutputString; }

	// Triggers if a key/mask matches it
	virtual bool trigger(KEY key, MASK mask);

	// Triggers if case-insensitive substring matches (assumes string is
	// lowercase)
	virtual bool trigger(const std::string& string);

	// Non-endian-neutral serialization
	U8* serialize(U8* buffer) const;
	U8* deserialize(U8* buffer, S32 max_size);
	static S32 getMaxSerialSize();

protected:
	KEY				mKey;			// usually a function key
	MASK			mMask;			// usually MASK_NONE, or MASK_SHIFT
	std::string		mTrigger;		// string, no whitespace allowed
	std::string		mTriggerLower;	// lowercase version of mTrigger
	LLUUID			mSoundItemID;	// ItemID of sound to play, LLUUID::null if none
	std::string		mAnimation;		// canonical name of animation or face animation
	std::string		mOutputString;	// string to say

	// For allocating serialization buffers; need to be updated when members
	// change
	static constexpr S32 MAX_SERIAL_SIZE = sizeof(KEY) + sizeof(MASK) +
										   16 + 26 + 41 + 41;
};

class LLGestureList
{
protected:
	LOG_CLASS(LLGestureList);

public:
	LLGestureList();
	virtual ~LLGestureList();

	// Triggers if a key/mask matches one in the list
	bool trigger(KEY key, MASK mask);

	// Triggers if substring matches and generates revised string.
	bool triggerAndReviseString(const std::string& string,
								std::string* revised_string);

	// Used for construction from UI
	LL_INLINE S32 count() const								{ return mList.size(); }
	virtual LLGesture* get(S32 i) const						{ return mList[i]; }
	virtual void put(LLGesture* gesture)					{ mList.push_back(gesture); }
	void deleteAll();

	// non-endian-neutral serialization
	U8* serialize(U8* buffer) const;
	U8* deserialize(U8* buffer, S32 max_size);
	S32 getMaxSerialSize();

protected:
	// overridden by child class to use local LLGesture implementation
	virtual LLGesture *create_gesture(U8** buffer, S32 max_size);

protected:
	std::vector<LLGesture*> mList;

	// For allocating serialization buffers; need to be updated when members
	// change
	static constexpr S32 SERIAL_HEADER_SIZE = sizeof(S32);
};
