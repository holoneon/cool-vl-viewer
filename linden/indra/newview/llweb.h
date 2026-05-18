/** 
 * @file llweb.h
 * @brief Functions dealing with web browsers
 * @author James Cook
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

#pragma once

#include "llalertdialog.h"

class LLWeb
{
public:
	static void initClass();

	// Loads unescaped URL in either internal web browser or external
	// browser, depending on user settings.
	static void loadURL(const std::string& url);
	static void loadURL(std::string url, const std::string& target);

	LL_INLINE static void loadURL(const char* url)
	{
		loadURL(ll_safe_string(url));
	}

	// Loads unescaped URL in external browser.
	static void loadURLExternal(const std::string& url);
	static void loadURLExternal(const std::string& url, bool async);

	// Loads unescaped URL in internal browser.
	static void loadURLInternal(const std::string& url);

	// Returns escaped (eg, " " to "%20") URL
	static std::string escapeURL(const std::string& url);

	// Expands various strings like [LANG], [VERSION], etc in an URL
	static std::string expandURLSubstitutions(const std::string& url,
											  const LLStringUtil::format_map_t& subs);

	class URLLoader : public LLAlertDialog::URLLoader
	{
		void load(const std::string& url) override;
	};

public:
	static URLLoader sAlertURLLoader;
};
