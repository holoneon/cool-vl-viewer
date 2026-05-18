/**
 * @file lluserrelations.h
 * @author Phoenix
 * @date 2006-10-12
 * @brief Class for handling granted rights.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "stdtypes.h"
#include "llpreprocessor.h"

// This class represents a relationship between two agents, where the related
// agent is stored and the other agent is the relationship is implicit by
// container ownership. This is merely a cache of this information used by the
// sim and viewer.

class LLRelationship
{
public:
	LL_INLINE LLRelationship()
	:	mGrantToAgent(0),
		mGrantFromAgent(0),
		mChangeSerialNum(0),
		mIsOnline(false)
	{
	}

	LL_INLINE LLRelationship(S32 grant_to, S32 grant_from, bool is_online)
	:	mGrantToAgent(grant_to),
		mGrantFromAgent(grant_from),
		mChangeSerialNum(0),
		mIsOnline(is_online)
	{
	}

	// Does this instance believe the related agent is currently online or
	// available.
	// This call does not check any kind of central store or make any deep
	// information calls - it simply checks a cache of online status.
	LL_INLINE bool isOnline() const				{ return mIsOnline; }

	// Sets the online status.
	LL_INLINE void online(bool is_online)
	{
		mIsOnline = is_online;
		++mChangeSerialNum;
	}

	// Granted rights. Anonymous enumeration for specifying rights.
	enum
	{
		GRANT_NONE				= 0x0,
		GRANT_ONLINE_STATUS		= 0x1,
		GRANT_MAP_LOCATION		= 0x2,
		GRANT_MODIFY_OBJECTS	= 0x4,
	};

	// Checks for a set of rights granted to agent. 'rights' is a bitfield to
	// check for rights. Returns true if all rights have been granted.
	LL_INLINE bool isRightGrantedTo(S32 rights) const
	{
		return (mGrantToAgent & rights) == rights;
	}

	// Checks for a set of rights granted from an agent. 'rights' is a bitfield
	// to check for rights. Returns true if all rights have been granted.
	LL_INLINE bool isRightGrantedFrom(S32 rights) const
	{
		return (mGrantFromAgent & rights) == rights;
	}

	// Gets the rights granted to the other agent. Returns the bitmask of
	// granted rights.
	LL_INLINE S32 getRightsGrantedTo() const	{ return mGrantToAgent; }

	// Gets the rights granted from the other agent. Returns the bitmask of
	// granted rights.
	LL_INLINE S32 getRightsGrantedFrom() const	{ return mGrantFromAgent; }

	LL_INLINE void setRightsTo(S32 to_agent)
	{
		mGrantToAgent = to_agent;
		++mChangeSerialNum;
	}

	LL_INLINE void setRightsFrom(S32 from_agent)
	{
		mGrantFromAgent = from_agent;
		++mChangeSerialNum;
	}

	// Gets the change count for this agent. Every change to rights will
	// increment the serial number allowing listeners to determine when a
	// relationship value is actually new. Returns change serial number for
	// relationship
	LL_INLINE S32 getChangeSerialNum() const	{ return mChangeSerialNum; }

	// Grants a set of rights. Any bit which is set will grant that right if it
	// is set in the instance. You can pass in LLGrantedRights::NONE to not
	// change that field. 'to_agent' are the rights to grant to agent_id and
	// 'from_agent' the rights granted from agent_id.
	LL_INLINE void grantRights(S32 to_agent, S32 from_agent)
	{
		mGrantToAgent |= to_agent;
		mGrantFromAgent |= from_agent;
		++mChangeSerialNum;
	}

	// Revokes a set of rights. Any bit which is set will revoke that right if
	// it is set in the instance. You can pass in LLGrantedRights::NONE to not
	// change that field.'to_agent' are the rights to grant to agent_id and
	// 'from_agent' the rights granted from agent_id.
	LL_INLINE void revokeRights(S32 to_agent, S32 from_agent)
	{
		mGrantToAgent &= ~to_agent;
		mGrantFromAgent &= ~from_agent;
		++mChangeSerialNum;
	}

private:
	S32		mGrantToAgent;
	S32		mGrantFromAgent;
	S32		mChangeSerialNum;
	bool	mIsOnline;
};
