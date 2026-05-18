/**
 * @file llappviewerlinux.h
 * @brief The LLAppViewerLinux class declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * Copyright (c) 2013-2022, Henri Beauchamp.
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

#include "llappviewer.h"

class LLCommandLineParser;

class LLAppViewerLinux final : public LLAppViewer
{
protected:
	LOG_CLASS(LLAppViewerLinux);

public:
	LLAppViewerLinux() = default;

	std::string generateSerialNumber() override;

#if !LL_CALL_SLURL_DISPATCHER_IN_CALLBACK
	// Used by the DBus callback, thus static and public.
	LL_INLINE static void setReceivedSLURL(std::string slurl)
	{
		sReceivedSLURL = slurl;
	}
#endif

	static void pumpGlib();

	bool probeVulkan(std::string& version) override;

protected:
	bool beingDebugged() override;

	// Not needed under Linux
	LL_INLINE bool restoreErrorTrap() override		{ return true; }

	void handleSyncCrashTrace() override;

	void initLogging() override;
	bool initParseCommandLine(LLCommandLineParser& clp) override;

	bool initAppMessagesHandler() override;
	bool sendURLToOtherInstance(const std::string& url) override;

#if !LL_CALL_SLURL_DISPATCHER_IN_CALLBACK
	// Yes, these are non-static methods dealing with static data... But
	// LLAppViewerLinux is actually a singleton, so...

	LL_INLINE const std::string& getReceivedSLURL() override
	{
		return sReceivedSLURL;
	}

	LL_INLINE void clearReceivedSLURL() override	{ sReceivedSLURL.clear(); }

private:
	static std::string sReceivedSLURL;
#endif
};
