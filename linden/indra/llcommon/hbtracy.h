/**
 * @file hbtracy.h
 * @brief Tracy profiler configuration and memory logging macros.
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

#pragma once

#if TRACY_ENABLE

// Tracy configuration: this must match exactly the configuration used to
// compile the Tracy library (since we use a pre-compiled library for the
// server side linked to the viewer).
//
// Tracy server/profiler communications configuration:
# define TRACY_ON_DEMAND 1
# define TRACY_NO_BROADCAST 1
# define TRACY_ONLY_LOCALHOST 1
# define TRACY_ONLY_IPV4 1
// No frame images capture:
# define TRACY_NO_FRAME_IMAGE 1

// For Debug and RelWithDebInfo builds, capture call stacks as well.
# if LL_DEBUG || LL_NO_FORCE_INLINE
// Capture three levels max.
#  define TRACY_CALLSTACK 3
// Do not allow the collection of the program code.
#  define TRACY_NO_CODE_TRANSFER 1
# endif

# include "Tracy.hpp"

// Allows to add messages to the trace: msg_str must be a std::string.
# define LL_TRACY_MSG(msg_str) TracyMessage(msg_str.c_str(), msg_str.size())
// Allows to add messages to the trace: msg must be a C-string litteral.
# define LL_TRACY_MSGL(msg) TracyMessageL(msg)

// This offers the possibility to add Tracy-only timers (e.g. for threads, or
// parts of the code that are costly to time and we do not want to see slowed
// down in release builds): the 'name' parameter can be anything (and shall not
// be added to EFastTimerType) but, by convention, all such timers are named
// following the TRC_* pattern in the viewer code.
# define LL_TRACY_TIMER(name) ZoneScopedN(#name)

// When TRACY_ENABLE is defined and greater than 2, we also enable fast timers.
# if TRACY_ENABLE > 2
#  define LL_FAST_TIMERS_ENABLED 1
#  else
#  define LL_FAST_TIMERS_ENABLED 0
# endif

# if LL_FAST_TIMERS_ENABLED

// Use both Tracy and fast timers
#  define LL_FAST_TIMER(name) LLFastTimer name(LLFastTimer::name); \
							  ZoneScopedN(#name)

#  define LL_FAST_TIMERS(cond, name1, name2) \
	LLFastTimer ftm(cond ? LLFastTimer::name1 : LLFastTimer::name2); \
	ZoneScopedN(#name1)

# else	// LL_FAST_TIMERS_ENABLED

// Replace fast timers with Tracy
#  define LL_FAST_TIMER(name) ZoneScopedN(#name)

// Tracy only accepts constexpr parameters for zone names, so that name cannot
// be made conditional. We simply use the first passed name, regardless of the
// condition (we need two names for fast timers only when a (hard coded) dual-
// parenting of that timer is needed; Tracy does not need this since parenting
// is auto-determined).
#  define LL_FAST_TIMERS(cond, name1, name2) ZoneScopedN(#name1)

# endif	// LL_FAST_TIMERS_ENABLED

// When TRACY_ENABLE is defined to 2 or 4, we also enable memory logging.
// Note that only allocations done via the viewer custom allocators are
// actually logged (which represents only part of the total used memory).
// *TODO: use a malloc hook to log everything...
# if TRACY_ENABLE == 2 || TRACY_ENABLE == 4
#  define LL_TRACY_ALLOC(ptr, size, name) if (ptr) TracyAllocN(ptr, size, name)
#  define LL_TRACY_FREE(ptr, name) if (ptr) TracyFreeN(ptr, name)
// These string constant pointers are to be used for 'name' with the above two
// defines, to flag the various memory pools/allocation types.
extern const char* trc_mem_align;
extern const char* trc_mem_align16;
extern const char* trc_mem_image;
// Only builds using jemalloc will see those used/reported (they will appear as
// aligned memory types for the other builds):
extern const char* trc_mem_volume;
extern const char* trc_mem_volume64;
extern const char* trc_mem_vertex;
# else
#  define LL_TRACY_ALLOC(ptr, size, name)
#  define LL_TRACY_FREE(ptr, name)
# endif

#else 	// TRACY_ENABLE

# define LL_FAST_TIMERS_ENABLED 1
# define LL_FAST_TIMER(name) LLFastTimer name(LLFastTimer::name)
# define LL_FAST_TIMERS(cond, name1, name2) \
	LLFastTimer ftm(cond ? LLFastTimer::name1 : LLFastTimer::name2)
// This is a no-op for fast timers
# define LL_TRACY_TIMER(name)

// These are always no-operations when Tracy is not in use
# define LL_TRACY_ALLOC(ptr, size, name)
# define LL_TRACY_FREE(ptr, name)

#endif 	// TRACY_ENABLE
