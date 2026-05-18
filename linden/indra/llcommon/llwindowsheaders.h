/**
 * @file llwindowsheaders.h
 * @brief sanitized include of windows header files
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

// IMPORTANT: when needed, this header *MUST* be #included before the
// linden_common.h (or llviewerprecompiledheaders.h) header, since the latter
// also #includes llwindowsheaderslean.h (which actual inclusion will then be
// skipped thanks to the LL_LLWINDOWSHEADERS_H #define guard set here). HB
// IMPORTANT: Do not change the define guard for a pragma once HB

#ifndef LL_LLWINDOWSHEADERS_H
#define LL_LLWINDOWSHEADERS_H

#ifdef LL_WINDOWS
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# undef WIN32_LEAN_AND_MEAN
# include <winsock2.h>
# include <windows.h>
// Reset to default, which is lean
# define WIN32_LEAN_AND_MEAN
#endif

#endif
