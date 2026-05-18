/** 
 * @file llclassifiedflags.h
 * @brief Flags used in the classifieds.
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

#pragma once

#include "llpreprocessor.h"

typedef U8 ClassifiedFlags;

constexpr U8 CLASSIFIED_FLAG_NONE   	= 1 << 0;
constexpr U8 CLASSIFIED_FLAG_MATURE 	= 1 << 1;
//constexpr U8 CLASSIFIED_FLAG_ENABLED	= 1 << 2;	// see llclassifiedflags.cpp
//constexpr U8 CLASSIFIED_FLAG_HAS_PRICE= 1 << 3;	// deprecated
constexpr U8 CLASSIFIED_FLAG_UPDATE_TIME= 1 << 4;
constexpr U8 CLASSIFIED_FLAG_AUTO_RENEW = 1 << 5;

constexpr U8 CLASSIFIED_QUERY_FILTER_MATURE		= 1 << 1;
//constexpr U8 CLASSIFIED_QUERY_FILTER_ENABLED	= 1 << 2;
//constexpr U8 CLASSIFIED_QUERY_FILTER_PRICE	= 1 << 3;

// These are new with Adult-enabled viewers (1.23 and later)
constexpr U8 CLASSIFIED_QUERY_INC_PG			= 1 << 2;
constexpr U8 CLASSIFIED_QUERY_INC_MATURE		= 1 << 3;
constexpr U8 CLASSIFIED_QUERY_INC_ADULT			= 1 << 6;
constexpr U8 CLASSIFIED_QUERY_INC_NEW_VIEWER	= (CLASSIFIED_QUERY_INC_PG |
												   CLASSIFIED_QUERY_INC_MATURE |
												   CLASSIFIED_QUERY_INC_ADULT);

constexpr S32 MAX_CLASSIFIEDS = 100;

// This function is used in Adult-flag-aware viewers to pack old query flags
// into the request so that they can talk to old dataservers properly. When all
// OpenSim servers will be able to deal with adult flags, we can revert back to
// ClassifiedFlags pack_classified_flags and get rider of this one.
ClassifiedFlags pack_classified_flags_request(bool auto_renew, bool is_pg,
											  bool is_mature, bool is_adult);

ClassifiedFlags pack_classified_flags(bool auto_renew, bool is_pg,
									  bool is_mature, bool is_adult);

LL_INLINE bool is_cf_mature(ClassifiedFlags flags)
{
	return (flags & CLASSIFIED_FLAG_MATURE) != 0 ||
		   (flags & CLASSIFIED_QUERY_INC_MATURE) != 0;
}

LL_INLINE bool is_cf_update_time(ClassifiedFlags flags)
{
	return (flags & CLASSIFIED_FLAG_UPDATE_TIME) != 0;
}

LL_INLINE bool is_cf_auto_renew(ClassifiedFlags flags)
{
	return (flags & CLASSIFIED_FLAG_AUTO_RENEW) != 0;
}
