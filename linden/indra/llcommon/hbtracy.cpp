/**
 * @file hbtracy.h
 * @brief Tracy profiler constants.
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 *
 * Copyright (c) 2021, Henri Beauchamp.
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

#include "hbtracy.h"

#if TRACY_ENABLE == 2 || TRACY_ENABLE == 4

// When TRACY_ENABLE is defined to 2 or 4, we also enable memory logging.
// Memory pools/allocation types must then be flagged with unique pointers to
// C strings. Here they are:
const char* trc_mem_align = "MEM_ALIGNED";
const char* trc_mem_align16 = "MEM_ALIGNED_16";
const char* trc_mem_image = "MEM_IMAGE";
const char* trc_mem_volume = "MEM_VOLUME_16";
const char* trc_mem_volume64 = "MEM_VOLUME_64";
const char* trc_mem_vertex = "MEM_VERTEX_BUFFER";

#else

// To avoid the LNK4221 warning under Windows while compiling...
# if LL_WINDOWS && !LL_CLANG
namespace
{
	void* dummy;
}
# endif

#endif
