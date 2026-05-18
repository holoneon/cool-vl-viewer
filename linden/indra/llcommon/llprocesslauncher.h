/**
 * @file llprocesslauncher.h
 * @brief Utility class for launching, terminating, and tracking the state of
 *        processes.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 *
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * Copyright (c) 2019-2025, Henri Beauchamp.
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

#if LL_WINDOWS
# include <windows.h>
#endif

#include "llstring.h"

// LLProcessLauncher handles launching external processes with specified
// command line arguments, working directory, and environment variables. It
// also keeps track of whether the process is still running, and can kill it if
// required.

class LLProcessLauncher
{
protected:
	LOG_CLASS(LLProcessLauncher);

public:
	LLProcessLauncher();
	virtual ~LLProcessLauncher();

	LL_INLINE void setExecutable(const std::string& filename)
	{
		mExecutable = filename;
	}

	LL_INLINE void setWorkingDirectory(const std::string& dir)
	{
		mWorkingDir = dir;
	}

	LL_INLINE void clearArguments()				{ mLaunchArguments.clear(); }

	LL_INLINE void addArgument(const std::string& arg)
	{
		mLaunchArguments.push_back(arg);
	}

	LL_INLINE void addArgument(const char* arg)
	{
		mLaunchArguments.push_back(std::string(arg));
	}

	// Used to set or unset environment variables for the child process. These
	// are no-operations under Windows. HB
	void setEnv(const char* envname, const char* value);
	void unsetEnv(const char* envname);

	S32 launch();
	bool isRunning();

	// Attempt to kill the process. Returns true if the process is no longer
	// running when it returns. Note that even if this returns false, the
	// process may exit some time after it is called.
	bool kill();

	// Use this if you want the external process to continue execution after
	// the LLProcessLauncher instance controlling it is deleted. Normally, the
	// destructor will attempt to kill the process and wait for termination.
	// This should only be used if the viewer is about to exit, otherwise the
	// child process will become a zombie after it exits.
	void orphan();

	// This needs to be called periodically on Mac/Linux to clean up zombie
	// processes.
	static void reap();

	// Accessors for platform-specific process ID
#if LL_WINDOWS
	LL_INLINE HANDLE getProcessHandle()		{ return mProcessHandle; }
#else
	LL_INLINE pid_t getProcessID()			{ return mProcessID; }
#endif

private:
#if LL_WINDOWS
	HANDLE						mProcessHandle;
#else
	pid_t						mProcessID;
#endif
	std::string					mExecutable;
	std::string					mWorkingDir;
	std::vector<std::string>	mLaunchArguments;
#if !LL_WINDOWS
	strings_map_t				mSavedEnvVarMap;
	strings_map_t				mChildEnvVarMap;
#endif
};
