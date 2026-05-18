/**
 * @file llexperiencelog.h
 * @brief llexperiencelog and related class definitions
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 *
 * Copyright (c) 2014, Linden Research, Inc.
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

#include "boost/signals2.hpp"

#include "llerror.h"
#include "llsingleton.h"

extern const std::string PUMP_EXPERIENCE;

class LLExperienceLog : public LLSingleton<LLExperienceLog>
{
	friend class LLExperienceLogDispatchHandler;
	friend class LLSingleton<LLExperienceLog>;

protected:
	LOG_CLASS(LLExperienceLog);

	LLExperienceLog();

	void loadEvents();
	void saveEvents();
	void eraseExpired();

public:
	virtual ~LLExperienceLog();

	typedef boost::signals2::signal<void(LLSD&)> callback_signal_t;
	typedef callback_signal_t::slot_type callback_slot_t;
	typedef boost::signals2::connection callback_connection_t;
	callback_connection_t addUpdateSignal(const callback_slot_t& cb);

	void initialize();

	LL_INLINE U32 getMaxDays() const			{ return mMaxDays; }
	LL_INLINE void setMaxDays(U32 val)			{ mMaxDays = val; }

	LL_INLINE bool getNotifyNewEvent() const	{ return mNotifyNewEvent; }
	void setNotifyNewEvent(bool val);

	LL_INLINE U32 getPageSize() const			{ return mPageSize; }
	LL_INLINE void setPageSize(U32 val)			{ mPageSize = val; }

	LL_INLINE const LLSD& getEvents()const		{ return mEvents; }

	LL_INLINE void clear()						{ mEvents.clear(); }

	static void notify(LLSD& message);
	static std::string getFilename();
	static std::string getPermissionString(const LLSD& message,
										   const std::string& base);

	LL_INLINE void setEventsToSave(LLSD event)	{ mEventsToSave = event; }
	bool isNotExpired(std::string& date);

	void handleExperienceMessage(LLSD& message);

protected:
	callback_signal_t		mSignals;
	callback_connection_t	mNotifyConnection;
	U32						mMaxDays;
	U32						mPageSize;
	LLSD					mEvents;
	LLSD					mEventsToSave;
	bool					mNotifyNewEvent;
};
