/**
 * @file llfindlocale.h
 * @brief Detect system language setting
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 *
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

// NOTE: this file used to be in llcommon/, but is only used in newview/ (i.e.
// it is not used by SLPlugin or other llcommon.a consummers), and is best
// placed together with the keyboard and display related sources. HB

#pragma once

#include "llpreprocessor.h"

typedef const char* FL_Lang;
typedef const char* FL_Country;
typedef const char* FL_Variant;

struct FL_Locale
{
	FL_Lang		lang;
	FL_Country	country;
	FL_Variant	variant;
};

typedef enum
{
	// For some reason we failed to even guess: this should never happen
	FL_FAILED = 0,
	// Could not query locale: returning a guess (almost always English)
	FL_DEFAULT_GUESS = 1,
	// The returned locale type was found by successfully asking the system
	FL_CONFIDENT     = 2
} FL_Success;

typedef enum
{
	FL_MESSAGES = 0
} FL_Domain;

// This allocates/fills in a FL_Locale structure with pointers to strings
// (which should be treated as static), or NULL for inappropriate or undetected
// fields.
FL_Success FL_FindLocale(FL_Locale** locale, FL_Domain domain);

// This should be used to free the struct written by FL_FindLocale
void FL_FreeLocale(FL_Locale** locale);
