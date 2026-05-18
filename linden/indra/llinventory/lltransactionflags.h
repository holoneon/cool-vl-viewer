/**
 * @file lltransactionflags.h
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

class LLUUID;

typedef U8 TransactionFlags;

constexpr U8 TRANSACTION_FLAGS_NONE = 0;
constexpr U8 TRANSACTION_FLAG_SOURCE_GROUP = 1;
constexpr U8 TRANSACTION_FLAG_DEST_GROUP = 2;
constexpr U8 TRANSACTION_FLAG_OWNER_GROUP = 4;
constexpr U8 TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION = 8;
constexpr U8 TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION_REMOVAL = 16;

// very simple helper functions
TransactionFlags pack_transaction_flags(bool is_source_group,
										bool is_dest_group);

// stupid helper functions which should be replaced with some kind of
// internationalizeable message.
std::string build_transfer_message_to_source(S32 amount,
											 const LLUUID& source_id,
											 const LLUUID& dest_id,
											 const std::string& dest_name,
											 S32 transaction_type,
											 const std::string& description);

std::string build_transfer_message_to_destination(S32 amount,
												  const LLUUID& dest_id,
												  const LLUUID& source_id,
                                                  const std::string& source_name,
												  S32 transaction_type,
												  const std::string& description);
