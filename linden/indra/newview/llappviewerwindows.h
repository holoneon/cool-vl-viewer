/**
 * @file llappviewerwindows.h
 * @brief The LLAppViewerWindows class declaration
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

#include "llappviewer.h"

class LLAppViewerWindows final : public LLAppViewer
{
protected:
	LOG_CLASS(LLAppViewerWindows);

public:
	LLAppViewerWindows(const char* cmd_line);

	InitState init() override;
	bool cleanup() override;

	bool probeVulkan(std::string& version) override;

	// Handle the 'login completed' event.
	void handleLoginComplete() override;

protected:
	void initLogging() override; // Override to clean stack_trace info.
	void initConsole() override; // Initialize OS level debugging console.
	bool initHardwareTest() override; // Windows uses DX9 to test hardware.
	bool initParseCommandLine(LLCommandLineParser& clp) override;

	bool beingDebugged() override;

	bool restoreErrorTrap() override;
	void handleSyncCrashTrace() override;

	bool sendURLToOtherInstance(const std::string& url) override;

	std::string generateSerialNumber() override;

protected:
	static const std::string	sWindowClass;

private:
	bool						mIsConsoleAllocated;
    std::string					mCmdLine;
};
