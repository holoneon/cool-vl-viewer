/**
 * @file llurldispatcher.h
 * @brief Central registry for all SL URL handlers
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

#include <string>

class LLMediaCtrl;

class LLURLDispatcher
{
public:
	// Called at startup time and on clicks in internal web browsers, teleport,
	// open map, or run requested command. Where:
	//	 -'url' is in the form:
	//		secondlife://RegionName/123/45/67/
	//		secondlife:///app/agent/3d6181b0-6a4b-97ef-18d8-722652995cf1/show
	//	- 'nav_type' can be either of "clicked", "external" or "navigated" or an
	//     empty string.
	//	- 'webp' is apointer to the LLMediaCtrl sending URL or NULL when none.
	//	- 'trusted_browser' is true if coming inside the app AND from a brower
	//	  instance which navigates to trusted (Linden Lab) pages.
	// Returns true if someone handled the URL.
	static bool dispatch(const std::string& url, const std::string& nav_type,
						 LLMediaCtrl* webp, bool trusted_browser);

	static bool dispatchRightClick(const std::string& url);
	static bool dispatchFromTextEditor(const std::string& url);
};
