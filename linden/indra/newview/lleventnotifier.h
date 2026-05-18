/**
 * @file lleventnotifier.h
 * @brief Viewer code for managing event notifications
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

#include <map>

#include "llframetimer.h"
#include "llvector3d.h"

class LLMessageSystem;

class LLEventInfo
{
public:
	LLEventInfo()		{}

	void unpack(LLMessageSystem* msg);

	static void loadCategories(const LLSD& event_options);

public:
	std::string		mName;
	U32				mID;
	std::string		mDesc;
	std::string		mCategoryStr;
	U32				mDuration;
	std::string		mTimeStr;
	LLUUID			mRunByID;
	std::string		mSimName;
	LLVector3d		mPosGlobal;
	time_t			mUnixTime;
	U32				mCover;
	U32				mEventFlags;
	bool			mHasCover;
	bool			mSelected;

	typedef std::map<U32, std::string> map_t;
	static map_t	sCategories;
};

class LLEventNotification final
{
protected:
	LOG_CLASS(LLEventNotification);

public:
	LLEventNotification();

	// In the format it comes in from login
	bool load(const LLSD& event_options);

	// From existing event_info on the viewer.
	bool load(const LLEventInfo& event_info);

#if 0
	void setEventID(U32 event_id);
	void setEventName(std::string& event_name);
#endif

	LL_INLINE U32 getEventID() const						{ return mEventID; }
	LL_INLINE const std::string& getEventName() const		{ return mEventName; }
	LL_INLINE time_t getEventDate() const					{ return mEventDate; }
	LL_INLINE const std::string& getEventDateStr() const	{ return mEventDateStr; }
	LL_INLINE LLVector3d getEventPosGlobal() const			{ return mEventPosGlobal; }
	LL_INLINE bool handleResponse(const LLSD& notif, const LLSD& payload);

protected:
	U32			mEventID;			// EventID for this event
	std::string	mEventName;
	std::string mEventDateStr;
	time_t		mEventDate;
	LLVector3d	mEventPosGlobal;
};

class LLEventNotifier final
{
public:
	LLEventNotifier();
	~LLEventNotifier();

	void update();	// Notify the user of the event if it is coming up

	// In the format that it comes in from login
	void load(const LLSD& event_options);

	// Add a new notification for an event
	void add(LLEventInfo& event_info);
	void remove(U32 event_id);

	bool hasNotification(U32 event_id);

	typedef std::map<U32, LLEventNotification*> map_t;

protected:
	map_t			mEventNotifications;
	LLFrameTimer	mNotificationTimer;
};

extern LLEventNotifier gEventNotifier;

constexpr U32 EVENT_FLAG_NONE   = 0x0000;
constexpr U32 EVENT_FLAG_MATURE = 0x0001;
constexpr U32 EVENT_FLAG_ADULT  = 0x0002;
