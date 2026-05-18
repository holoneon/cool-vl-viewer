/**
 * @file llscriptpermissions.h
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

#pragma once

#include "stdtypes.h"

typedef enum e_lscript_runtime_permissions
{
	SCRIPT_PERMISSION_DEBIT,
	SCRIPT_PERMISSION_TAKE_CONTROLS,
	SCRIPT_PERMISSION_REMAP_CONTROLS,
	SCRIPT_PERMISSION_TRIGGER_ANIMATION,
	SCRIPT_PERMISSION_ATTACH,
	SCRIPT_PERMISSION_RELEASE_OWNERSHIP,
	SCRIPT_PERMISSION_CHANGE_LINKS,
	SCRIPT_PERMISSION_CHANGE_JOINTS,
	SCRIPT_PERMISSION_CHANGE_PERMISSIONS,
	SCRIPT_PERMISSION_TRACK_CAMERA,
	SCRIPT_PERMISSION_CONTROL_CAMERA,
	SCRIPT_PERMISSION_TELEPORT,
	SCRIPT_PERMISSION_EXPERIENCE,
	SCRIPT_PERMISSION_SILENT_ESTATE_MANAGEMENT,
	SCRIPT_PERMISSION_OVERRIDE_ANIMATIONS,
	SCRIPT_PERMISSION_RETURN_OBJECTS,
	SCRIPT_PERMISSION_FORCE_SIT_AVATAR,
	SCRIPT_PERMISSION_CHANGE_ENV_SETTINGS,
	SCRIPT_PERMISSION_PRIVILEDGED_LAND_ACCESS,
	SCRIPT_PERMISSION_EOF
} LSCRIPTRunTimePermissions;

extern const U32 LSCRIPTRunTimePermissionBits[];
