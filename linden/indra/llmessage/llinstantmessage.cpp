/**
 * @file llinstantmessage.cpp
 * @author Phoenix
 * @date 2005-08-29
 * @brief Constants and functions used in IM.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 *
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llinstantmessage.h"

#include "llmessage.h"

const char EMPTY_BINARY_BUCKET[] = "";
const std::string SYSTEM_FROM("Second Life");
const std::string INCOMING_IM("Incoming IM");
const std::string INTERACTIVE_SYSTEM_FROM("F387446C-37C4-45f2-A438-D99CBDBB563B");

void pack_instant_message(const LLUUID& from_id, bool from_group,
						  const LLUUID& session_id, const LLUUID& to_id,
						  const std::string& name, const std::string& message,
						  U8 offline, EInstantMessage dialog, const LLUUID& id,
						  U32 parent_estate_id, const LLUUID& region_id,
						  const LLVector3& position, U32 timestamp,
						  const U8* binary_bucket, S32 binary_bucket_size)
{
	LLMessageSystem* msg = gMessageSystemp;
	if (!msg) return;

	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, from_id);
	msg->addUUIDFast(_PREHASH_SessionID, session_id);
	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBoolFast(_PREHASH_FromGroup, from_group);
	msg->addUUIDFast(_PREHASH_ToAgentID, to_id);
	msg->addU32Fast(_PREHASH_ParentEstateID, parent_estate_id);
	msg->addUUIDFast(_PREHASH_RegionID, region_id);
	msg->addVector3Fast(_PREHASH_Position, position);
	msg->addU8Fast(_PREHASH_Offline, offline);
	msg->addU8Fast(_PREHASH_Dialog, (U8) dialog);
	msg->addUUIDFast(_PREHASH_ID, id);
	msg->addU32Fast(_PREHASH_Timestamp, timestamp);
	msg->addStringFast(_PREHASH_FromAgentName, name);
	S32 bytes_left = MTUBYTES;
	if (!message.empty())
	{
		char buffer[MTUBYTES];
		int num_written = snprintf(buffer, MTUBYTES, "%s", message.c_str());
		// snprintf returns number of bytes that would have been written had
		// the output not being truncated. In that case, it will return either
		// -1 or value >= passed in size value . So a check needs to be added
		// to detect truncation, and if there is any, only account for the
		// actual number of bytes written..and not what could have been
		// written.
		if (num_written < 0 || num_written >= MTUBYTES)
		{
			num_written = MTUBYTES - 1;
			llwarns << "message truncated: " << message << llendl;
		}

		bytes_left -= num_written;
		bytes_left = llmax(0, bytes_left);
		msg->addStringFast(_PREHASH_Message, buffer);
	}
	else
	{
		msg->addStringFast(_PREHASH_Message, NULL);
	}
	const U8* bb;
	if (binary_bucket)
	{
		bb = binary_bucket;
		binary_bucket_size = llmin(bytes_left, binary_bucket_size);
	}
	else
	{
		bb = (const U8*)EMPTY_BINARY_BUCKET;
		binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE;
	}
	msg->addBinaryDataFast(_PREHASH_BinaryBucket, bb, binary_bucket_size);
}


std::string im_status_to_string(EInstantMessage status)
{
	std::string result = "UNKNOWN";

	// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break

	switch (status)
	{
		CASE(IM_NOTHING_SPECIAL);
		CASE(IM_MESSAGEBOX);
		CASE(IM_GROUP_INVITATION);
		CASE(IM_INVENTORY_OFFERED);
		CASE(IM_INVENTORY_ACCEPTED);
		CASE(IM_INVENTORY_DECLINED);
		CASE(IM_GROUP_VOTE);
		CASE(IM_GROUP_MESSAGE_DEPRECATED);
		CASE(IM_TASK_INVENTORY_OFFERED);
		CASE(IM_TASK_INVENTORY_ACCEPTED);
		CASE(IM_TASK_INVENTORY_DECLINED);
		CASE(IM_NEW_USER_DEFAULT);
		CASE(IM_SESSION_INVITE);
		CASE(IM_SESSION_P2P_INVITE);
		CASE(IM_SESSION_GROUP_START);
		CASE(IM_SESSION_CONFERENCE_START);
		CASE(IM_SESSION_SEND);
		CASE(IM_SESSION_LEAVE);
		CASE(IM_FROM_TASK);
		CASE(IM_BUSY_AUTO_RESPONSE);
		CASE(IM_CONSOLE_AND_CHAT_HISTORY);
		CASE(IM_LURE_USER);
		CASE(IM_LURE_ACCEPTED);
		CASE(IM_LURE_DECLINED);
		CASE(IM_GODLIKE_LURE_USER);
		CASE(IM_TELEPORT_REQUEST);
		CASE(IM_GROUP_ELECTION_DEPRECATED);
		CASE(IM_GOTO_URL);
		CASE(IM_FROM_TASK_AS_ALERT);
		CASE(IM_GROUP_NOTICE);
		CASE(IM_GROUP_NOTICE_INVENTORY_ACCEPTED);
		CASE(IM_GROUP_NOTICE_INVENTORY_DECLINED);
		CASE(IM_GROUP_INVITATION_ACCEPT);
		CASE(IM_GROUP_INVITATION_DECLINE);
		CASE(IM_GROUP_NOTICE_REQUESTED);
		CASE(IM_FRIENDSHIP_OFFERED);
		CASE(IM_FRIENDSHIP_ACCEPTED);
		CASE(IM_FRIENDSHIP_DECLINED_DEPRECATED);
		CASE(IM_TYPING_START);
		CASE(IM_TYPING_STOP);

		default:
			break;
	}

#undef CASE

	return result;
}
