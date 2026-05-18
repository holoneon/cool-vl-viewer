/**
 * @file llsdjson.h
 * @brief LLSD flexible data system
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 *
 * Copyright (c) 2015, Linden Research, Inc.
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

#include "json.hpp"

using lljson = nlohmann::json;

#include "llsd.h"

// Convert a parsed JSON structure into LLSD maintaining member names and
// array indexes.
// JSON/JavaScript types are converted as follows:
//
// JSON Type     | LLSD Type
// --------------+--------------
//  null         |  undefined
//  integer      |  LLSD::Integer
//  unsigned     |  LLSD::Integer
//  real/numeric |  LLSD::Real
//  string       |  LLSD::String
//  boolean      |  LLSD::Boolean
//  array        |  LLSD::Array
//  object       |  LLSD::Map
//
// For maps and arrays child entries will be converted and added to the structure.
// Order is preserved for an array but not for objects.
LLSD llsd_from_json(const lljson& val);

// Convert an LLSD object into Parsed JSON object maintaining member names and
// array indexs.
//
// Types are converted as follows:
// LLSD Type     |  JSON Type
// --------------+----------------
// TypeUndefined | null
// TypeBoolean   | boolean
// TypeInteger   | integer
// TypeReal      | real/numeric
// TypeString    | string
// TypeURI       | string
// TypeDate      | string
// TypeUUID      | string
// TypeMap       | object
// TypeArray     | array
// TypeBinary    | unsupported
lljson llsd_to_json(const LLSD& val);
