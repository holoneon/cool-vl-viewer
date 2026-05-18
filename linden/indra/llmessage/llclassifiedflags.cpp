/** 
 * @file llclassifiedflags.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

//*****************************************************************************
// llclassifiedflags.cpp
//
// Some exported symbols and functions for dealing with classified flags.
//
// Copyright 2005, Linden Research, Inc
//*****************************************************************************

#include "linden_common.h"

#include "llclassifiedflags.h"

ClassifiedFlags pack_classified_flags_request(bool auto_renew, bool inc_pg,
											  bool inc_mature, bool inc_adult)
{
	U8 rv = 0;
	if (inc_pg)					rv |= CLASSIFIED_QUERY_INC_PG;
	if (inc_mature)				rv |= CLASSIFIED_QUERY_INC_MATURE;
	if (inc_pg && !inc_mature)	rv |= CLASSIFIED_FLAG_MATURE;
	if (inc_adult)				rv |= CLASSIFIED_QUERY_INC_ADULT;
	if (auto_renew)				rv |= CLASSIFIED_FLAG_AUTO_RENEW;
	return rv;
}

ClassifiedFlags pack_classified_flags(bool auto_renew, bool inc_pg,
									  bool inc_mature, bool inc_adult)
{
	U8 rv = 0;
	if (inc_pg)
	{
		rv |= CLASSIFIED_QUERY_INC_PG;
	}
	if (inc_mature)
    {
        rv |= CLASSIFIED_QUERY_INC_MATURE;
        rv |= CLASSIFIED_FLAG_MATURE;
    }
	if (inc_adult)
	{
		rv |= CLASSIFIED_QUERY_INC_ADULT;
	}
	if (auto_renew)
	{
		rv |= CLASSIFIED_FLAG_AUTO_RENEW;
	}
	return rv;
}
