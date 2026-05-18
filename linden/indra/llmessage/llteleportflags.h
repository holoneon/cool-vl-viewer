/**
 * @file llteleportflags.h
 * @brief Teleport flags
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

constexpr U32 TELEPORT_FLAGS_DEFAULT 			= 0;
constexpr U32 TELEPORT_FLAGS_SET_HOME_TO_TARGET	= 1 << 0;	// Newbie leaving prelude
constexpr U32 TELEPORT_FLAGS_SET_LAST_TO_TARGET	= 1 << 1;
constexpr U32 TELEPORT_FLAGS_VIA_LURE 			= 1 << 2;
constexpr U32 TELEPORT_FLAGS_VIA_LANDMARK 		= 1 << 3;
constexpr U32 TELEPORT_FLAGS_VIA_LOCATION		= 1 << 4;
constexpr U32 TELEPORT_FLAGS_VIA_HOME			= 1 << 5;
constexpr U32 TELEPORT_FLAGS_VIA_TELEHUB		= 1 << 6;
constexpr U32 TELEPORT_FLAGS_VIA_LOGIN			= 1 << 7;
constexpr U32 TELEPORT_FLAGS_VIA_GODLIKE_LURE	= 1 << 8;
constexpr U32 TELEPORT_FLAGS_GODLIKE 			= 1 << 9;
constexpr U32 TELEPORT_FLAGS_911 				= 1 << 10;
constexpr U32 TELEPORT_FLAGS_DISABLE_CANCEL		= 1 << 11;	// Used for llTeleportAgentHome()
constexpr U32 TELEPORT_FLAGS_VIA_REGION_ID  	= 1 << 12;
constexpr U32 TELEPORT_FLAGS_IS_FLYING			= 1 << 13;
constexpr U32 TELEPORT_FLAGS_SHOW_RESET_HOME	= 1 << 14;
// Used to force a redirect to some random location - used when kicking someone
// from land.
constexpr U32 TELEPORT_FLAGS_FORCE_REDIRECT		= 1 << 15;

constexpr U32 TELEPORT_FLAGS_MASK_VIA = TELEPORT_FLAGS_VIA_LURE
										| TELEPORT_FLAGS_VIA_LANDMARK
										| TELEPORT_FLAGS_VIA_LOCATION
										| TELEPORT_FLAGS_VIA_HOME
										| TELEPORT_FLAGS_VIA_TELEHUB
										| TELEPORT_FLAGS_VIA_LOGIN
										| TELEPORT_FLAGS_VIA_REGION_ID;




constexpr U32 LURE_FLAG_NORMAL_LURE  	= 1 << 0;
constexpr U32 LURE_FLAG_GODLIKE_LURE 	= 1 << 1;
constexpr U32 LURE_FLAG_GODLIKE_PURSUIT = 1 << 2;
