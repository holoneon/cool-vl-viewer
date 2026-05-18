/**
 * @file llscriptpermissions.cpp
 * @brief Shared code between compiler and assembler and LSL
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

#include "llscriptpermissions.h"

const U32 LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_EOF] =
{
	(0x1 << 1),		//	SCRIPT_PERMISSION_DEBIT
	(0x1 << 2),		//	SCRIPT_PERMISSION_TAKE_CONTROLS
	(0x1 << 3),		//	SCRIPT_PERMISSION_REMAP_CONTROLS
	(0x1 << 4),		//	SCRIPT_PERMISSION_TRIGGER_ANIMATION
	(0x1 << 5),		//	SCRIPT_PERMISSION_ATTACH
	(0x1 << 6),		//	SCRIPT_PERMISSION_RELEASE_OWNERSHIP
	(0x1 << 7),		//	SCRIPT_PERMISSION_CHANGE_LINKS
	(0x1 << 8),		//	SCRIPT_PERMISSION_CHANGE_JOINTS
	(0x1 << 9),		//	SCRIPT_PERMISSION_CHANGE_PERMISSIONS
	(0x1 << 10),	//	SCRIPT_PERMISSION_TRACK_CAMERA
	(0x1 << 11),	//	SCRIPT_PERMISSION_CONTROL_CAMERA
	(0x1 << 12),	//	SCRIPT_PERMISSION_TELEPORT
	(0x1 << 13),	//	SCRIPT_PERMISSION_EXPERIENCE
	(0x1 << 14),	//	SCRIPT_PERMISSION_SILENT_ESTATE_MANAGEMENT
	(0x1 << 15),	//	SCRIPT_PERMISSION_OVERRIDE_ANIMATIONS
	(0x1 << 16),	//	SCRIPT_PERMISSION_RETURN_OBJECTS
	(0x1 << 17),	//	SCRIPT_PERMISSION_FORCE_SIT_AVATAR
	(0x1 << 18),	//	SCRIPT_PERMISSION_CHANGE_ENV_SETTINGS
	(0x1 << 19),	//	SCRIPT_PERMISSION_PRIVILEDGED_LAND_ACCESS
};
