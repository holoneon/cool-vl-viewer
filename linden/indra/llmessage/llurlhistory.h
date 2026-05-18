/**
 * @file llurlhistory.h
 * @brief Manages a list of recent URLs
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llstring.h"

class LLSD;

class LLURLHistory
{
protected:
	LOG_CLASS(LLURLHistory);

public:
	// Loads an xml file of URLs.
	static bool loadFile(const std::string& filename);

	// Saves the current history to XML
	static bool saveFile(const std::string& filename);

	static LLSD getURLHistory(const std::string& collection);

	static void addURL(const std::string& collection, const std::string& url);
	static void removeURL(const std::string& collection,
						  const std::string& url);
	static void clear(const std::string& collection);

	static void limitSize(const std::string& collection);

private:
	static LLSD sHistorySD;
};
