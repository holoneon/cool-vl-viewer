/**
 * @file llchat.h
 * @author James Cook
 * @brief Chat constants and data structures.
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

#include "lluuid.h"
#include "llvector3.h"

// enumerations used by the chat system
typedef enum e_chat_source_type
{
	CHAT_SOURCE_SYSTEM = 0,
	CHAT_SOURCE_AGENT = 1,
	CHAT_SOURCE_OBJECT = 2,
	CHAT_SOURCE_UNKNOWN = 3
} EChatSourceType;

typedef enum e_chat_type
{
	CHAT_TYPE_WHISPER = 0,
	CHAT_TYPE_NORMAL = 1,
	CHAT_TYPE_SHOUT = 2,
	CHAT_TYPE_START = 4,
	CHAT_TYPE_STOP = 5,
	CHAT_TYPE_DEBUG_MSG = 6,
	CHAT_TYPE_REGION = 7,
	CHAT_TYPE_OWNER = 8,
	CHAT_TYPE_DIRECT = 9		// From llRegionSayTo()
} EChatType;

typedef enum e_chat_audible_level
{
	CHAT_AUDIBLE_NOT = -1,
	CHAT_AUDIBLE_BARELY = 0,
	CHAT_AUDIBLE_FULLY = 1
} EChatAudible;

// A piece of chat
class LLChat
{
public:
	LLChat(const std::string& text = std::string())
	:	mText(text),
		mSourceType(CHAT_SOURCE_UNKNOWN),
		mChatType(CHAT_TYPE_NORMAL),
		mAudible(CHAT_AUDIBLE_FULLY),
		mMuted(false),
		mTime(0.0)
	{
	}

	LLChat(const LLChat& chat)
	:	mText(chat.mText),
		mFromName(chat.mFromName),
		mFromID(chat.mFromID),
		mOwnerID(chat.mOwnerID),
		mSourceType(chat.mSourceType),
		mChatType(chat.mChatType),
		mAudible(chat.mAudible),
		mMuted(chat.mMuted),
		mTime(chat.mTime),
		mPosAgent(chat.mPosAgent),
		mURL(chat.mURL)
	{
	}

public:
	LLUUID			mFromID;	// Agen or object Id
	LLUUID			mOwnerID;	// Owner of chatting object Id
	std::string		mText;		// UTF-8 line of text
	std::string		mFromName;	// Agent or object name
	std::string		mURL;
	F64				mTime;		// Seconds from viewer session start
	LLVector3		mPosAgent;
	EChatSourceType	mSourceType;
	EChatType		mChatType;
	EChatAudible	mAudible;
	bool			mMuted;		// Pass muted chat to maintain list of chatters
};
