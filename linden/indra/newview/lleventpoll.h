/**
 * @file lleventpoll.h
 * @brief LLEvDescription of the LLEventPoll class.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2018, Linden Research, Inc.
 * Copyright (c) 2019-2023, Henri Beauchamp.
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

#include <string>

#include "llpointer.h"

class LLEventPollImpl;
class LLHost;
struct LLEventPollReplies;

// Implements the viewer side of server-to-viewer pushed events.

class LLEventPoll
{
public:
	// Starts polling the URL.
	LLEventPoll(U64 handle, const LLHost& sender, const std::string& poll_url);

	// Stops polling, cancelling any poll in progress.
	~LLEventPoll();

	void setRegionName(const std::string& region_name);

	// Returns true when a poll request is waiting for server events and its
	// age is within the "safe" window (i.e. when it is believed to be old
	// enough for the server to have received it and not too close from the
	// timeout). HB
	bool isPollInFlight() const;
	// Returns the age of the active poll request. HB
	F32 getPollAge() const;

	// Margin in seconds. HB
	static F32 getMargin();

	// Must be called at least once per frame, when it is safe to process
	// messages (outside the rendering routines, in particular). HB
	static void dispatchMessages();

private:
	LLPointer<LLEventPollImpl>	mImpl;
};
