/**
 * @file llavataractions.h
 * @brief avatar-related actions (IM, teleporting, etc)
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

#pragma once

#include "lluuid.h"

class LLAvatarName;
class LLViewerRegion;

// Purely static class
class LLAvatarActions
{
	LLAvatarActions() = delete;
	~LLAvatarActions() = delete;

protected:
	LOG_CLASS(LLAvatarActions);

public:
	// Friendship offers.

	// Request with avatar name resolution and a dialog
	static void requestFriendshipDialog(const LLUUID& id);
	// Request with known avatar name and a dialog
	static void requestFriendshipDialog(const LLUUID& id,
										const std::string& name);
	// Request with known name and withour dialog
	static void requestFriendship(const LLUUID& id,
								  const std::string& name,
								  const std::string& message);

	// Send teleport offers.
	static void offerTeleport(const LLUUID& id);
	static void offerTeleport(const uuid_vec_t& ids);

	// Request teleport from another avatar.
	static void teleportRequest(const LLUUID& id);

	// Start instant messaging session.
	static void startIM(const LLUUID& id);
	static void startIM(const uuid_vec_t& ids, bool friends = false);

	// Give money to the avatar.
	static void pay(const LLUUID& id);

	// Builds a string containing a list of avatar names.
	//
	// If force_legacy is true, then lecacy names are used, regardless of the
	// name displaying settings (useful for mute or ban lists and other
	// security sensitive lists). HB
	static void buildAvatarsList(std::vector<LLAvatarName> avatar_names,
								 std::string& avatars,
								 bool force_legacy = false,
								 const std::string& separator = ", ");

	// Returns the avatar region when you have permission to eject or freeze
	// this avatar, or NULL otherwise. HB
	static LLViewerRegion* canEjectOrFreeze(const LLUUID& avatar_id);

	// These methods kick (log out) or (un)freeze a given avatar (would work
	// only if you have permission to do so). They ask for confirmation via a
	// dialog. When executed by a God, the God kick message is used, otherwise
	// a user freeze or eject (not kick/logout !) message is sent, when on a
	// controlled parcel or land. HB
	static void kick(const LLUUID& avatar_id);
	static void freeze(const LLUUID& avatar_id, bool freeze);

	// User (not God) eject (with optional ban) and freeze/unfreeze messages
	// sending, with prior land/parcel permission verification. No confirmation
	// is requested. The returned value is true when the permission was correct
	// and the message actually sent. HB
	static bool sendEject(const LLUUID& avatar_id, bool ban);
	static bool sendFreeze(const LLUUID& avatar_id, bool freeze);
};
