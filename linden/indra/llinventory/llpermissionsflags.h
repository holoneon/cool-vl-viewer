/**
 * @file llpermissionsflags.h
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

// Flags for various permissions bits. Shared between viewer and simulator.

// Permission bits
typedef U32 PermissionMask;
typedef U32 PermissionBit;

// Do you have permission to transfer ownership of the object or item. Fair use
// rules dictate that if you cannot copy, you can always transfer.
constexpr PermissionBit PERM_TRANSFER           = (1 << 13); // 0x00002000

// Objects, scale or change textures parcels, allow building on it
constexpr PermissionBit PERM_MODIFY				= (1 << 14); // 0x00004000

// Objects, allow copy
constexpr PermissionBit PERM_COPY				= (1 << 15); // 0x00008000

// Objects, allow exporting (OpenSIM extension). Was formerly used in SL for
// the now deprecated PERM_ENTER flag (to allow entry in parcel).
constexpr PermissionBit PERM_EXPORT				= (1 << 16); // 0x00010000

// Objects, can grab/translate/rotate
constexpr PermissionBit PERM_MOVE				= (1 << 19); // 0x00080000

// Do not use bit 31: printf/scanf with "%x" assume signed numbers
constexpr PermissionBit PERM_RESERVED			= ((U32)1) << 31;

#if 0	// Deprecated flags
// Parcels, allow terraform
constexpr PermissionBit PERM_TERRAFORM			= (1 << 17); // 0x00020000

// NB: while this flag is no longer used, it is possible that some objects in
// the universe have it set so DO NOT USE IT going forward.
constexpr PermissionBit PERM_OWNER_DEBIT		= (1 << 18); // 0x00040000

// Parcels, avatars take damage
constexpr PermissionBit	PERM_DAMAGE				= (1 << 20); // 0x00100000

constexpr PermissionMask PERM_ALL_PARCEL 		= PERM_MODIFY | PERM_ENTER |
												  PERM_TERRAFORM | PERM_DAMAGE;
#endif

constexpr PermissionMask PERM_NONE				= 0x00000000;
constexpr PermissionMask PERM_ALL				= 0x7FFFFFFF;
constexpr PermissionMask PERM_ITEM_UNRESTRICTED = PERM_MODIFY | PERM_COPY |
												  PERM_TRANSFER;

// Useful stuff for transmission.
// Which permissions field are we trying to change ?
constexpr U8 PERM_BASE		= 0x01;
// *TODO: Add another PERM_OWNER operation type for allowOperationBy
// DK 04/03/06
constexpr U8 PERM_OWNER		= 0x02;
constexpr U8 PERM_GROUP		= 0x04;
constexpr U8 PERM_EVERYONE	= 0x08;
constexpr U8 PERM_NEXT_OWNER = 0x10;

// This is just a quickie debugging key
// no modify: PERM_ALL & ~PERM_MODIFY                  = 0x7fffbfff
// no copy:   PERM_ALL & ~PERM_COPY                    = 0x7fff7fff
// no modify or copy:                                  = 0x7fff3fff
// no transfer: PERM_ALL & ~PERM_TRANSFER              = 0x7fffdfff
// no modify, no transfer                              = 0x7fff9fff
// no copy, no transfer (INVALID!)                     = 0x7fff5fff
// no modify, no copy, no transfer (INVALID!)          = 0x7fff1fff
