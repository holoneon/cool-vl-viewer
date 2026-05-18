/**
 * @file llprocesslauncher.cpp
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

#include "linden_common.h"

#if LL_LINUX
# include <sys/wait.h>
# include <unistd.h>
# include <signal.h>
# include <fcntl.h>
# include <errno.h>
# include <stdlib.h>			// For getenv()/*setenv()
#endif

#include "llprocesslauncher.h"

// Sadly, some gcc/glibc "flavours" (Ubuntu's ones, apparently), warn about
// unused results for (void)chdir() and (void)fchdir(), even though the (void)
// is there to prevent this !  So, let's take more radical measures... HB
#if LL_LINUX && defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wunused-result"
#endif

LLProcessLauncher::LLProcessLauncher()
{
#if LL_WINDOWS
	mProcessHandle = 0;
#else
	mProcessID = 0;
#endif
}

LLProcessLauncher::~LLProcessLauncher()
{
	kill();
}

#if LL_WINDOWS

void LLProcessLauncher::setEnv(const char*, const char*)
{
}

void LLProcessLauncher::unsetEnv(const char*)
{
}

S32 LLProcessLauncher::launch()
{
	// If there was already a process associated with this object, kill it.
	kill();
	orphan();

	S32 result = 0;

	PROCESS_INFORMATION pinfo;
	STARTUPINFOA sinfo;
	memset(&sinfo, 0, sizeof(sinfo));

	std::string args = "\"" + mExecutable + "\"";
	for (S32 i = 0, count = mLaunchArguments.size(); i < count; ++i)
	{
		args += ' ';
		const std::string& arg = mLaunchArguments[i];
		if (arg.find(' ') != std::string::npos)
		{
			// Quote arguments containing spaces !
			args += "\""  + arg + "\"";
		}
		else
		{
			args += arg;
		}
	}
	llinfos << "Executable: " << mExecutable << " arguments: " << args
			<< llendl;

	// So retarded. Windows requires that the second parameter to
	// CreateProcessA be a writable (non-const) string...
	char* args2 = new char[args.size() + 1];
	strcpy(args2, args.c_str());

	if (!CreateProcessA(NULL, args2, NULL, NULL, FALSE, 0, NULL, NULL, &sinfo,
						&pinfo))
	{
		// *TODO: do better than returning the OS-specific error code on
		// failure...
		result = GetLastError();
		if (result == 0)
		{
			// Make absolutely certain we return a non-zero value on failure.
			result = -1;
		}
	}
	else
	{
		// foo = pinfo.dwProcessId; // Get your pid here if you want to use it
		// later on
		mProcessHandle = pinfo.hProcess;
		CloseHandle(pinfo.hThread);	// Stops leaks, nothing else
	}

	delete[] args2;

	return result;
}

bool LLProcessLauncher::isRunning()
{
	if (mProcessHandle)
	{
		if (WaitForSingleObject(mProcessHandle, 0) == WAIT_OBJECT_0)
		{
			// The process has completed.
			mProcessHandle = 0;
		}
	}

	return mProcessHandle != 0;
}

bool LLProcessLauncher::kill()
{
	if (mProcessHandle)
	{
		TerminateProcess(mProcessHandle, 0);

		if (isRunning())
		{
			return false;
		}
	}
	return true;
}

void LLProcessLauncher::orphan()
{
	// Forget about the process
	mProcessHandle = 0;
}

//static
void LLProcessLauncher::reap()
{
	// No actions necessary on Windows.
}

#elif LL_LINUX

static std::list<pid_t> sZombies;
// Set to a string that is most unlikely to represent an actual environment
// variable value. HB
static const std::string UNSET_MARKER("+-+-+_*[U_N_S_E_T]*_+-+-+");

void LLProcessLauncher::setEnv(const char* envname, const char* value)
{
	if (envname && *envname)
	{
		mChildEnvVarMap.emplace(envname, value);
	}
}

void LLProcessLauncher::unsetEnv(const char* envname)
{
	if (envname && *envname)
	{
		mChildEnvVarMap.emplace(envname, UNSET_MARKER);
	}
}

// Attempts to reap a process Id. Returns true if the process has exited and has
// been reaped, false otherwise.
static bool reap_pid(pid_t pid)
{
	bool result = false;

	pid_t wait_result = ::waitpid(pid, NULL, WNOHANG);
	if (wait_result == pid)
	{
		result = true;
	}
	else if (wait_result == -1)
	{
		if (errno == ECHILD)
		{
			// No such process: this may mean we are ignoring SIGCHILD.
			result = true;
		}
	}

	return result;
}

S32 LLProcessLauncher::launch()
{
	// If there was already a process associated with this object, kill it.
	kill();
	orphan();

	// Create an argv vector for the child process (size + 1 for the executable
	// path and + 1 for the NULL terminator)
	const char** fake_argv = new const char *[mLaunchArguments.size() + 2];
	S32 i = 0;
	// Add the executable path
	fake_argv[i++] = mExecutable.c_str();
	// And any arguments
	for (U32 j = 0, count = mLaunchArguments.size(); j < count; ++j)
	{
		fake_argv[i++] = mLaunchArguments[j].c_str();
	}
	// Terminate with a null pointer
	fake_argv[i] = NULL;

	int current_wd = -1;
	if (!mWorkingDir.empty())
	{
		// Save the current working directory
		current_wd = open(".", O_RDONLY);

		// And change to the one the child will be executed in
		(void)chdir(mWorkingDir.c_str());
	}

	// Set or unset the environment variables for the child process to inherit
	// them as configured by the user. HB
	mSavedEnvVarMap.clear();
	for (strings_map_t::const_iterator it = mChildEnvVarMap.begin(),
									   end = mChildEnvVarMap.end();
		 it != end; ++it)
	{
		const std::string& envname = it->first;
		const std::string& envvalue = it->second;
		// Save the current value for our environment variable
		char* envvar = getenv(envname.c_str());
		if (envvar)
		{
			// The variable exists in the current environment: save it.
			LL_DEBUGS("ProcessLauncher") << "Saving parent process environment variable '"
										 << envname << "' value: " << envvar
										 << LL_ENDL;
			mSavedEnvVarMap.emplace(envname, envvar);
		}
		else if (envvalue != UNSET_MARKER)
		{
			// The variable does not exist, and will be created. Mark for
			// removal.
			LL_DEBUGS("ProcessLauncher") << "Marking new child process environment variable '"
										 << envname
										 << "' for removal from parent process."
										 << LL_ENDL;
			mSavedEnvVarMap.emplace(envname, UNSET_MARKER);
		}
		if (envvalue != UNSET_MARKER)
		{
			// Set the new value for this environment variable
			LL_DEBUGS("ProcessLauncher") << "Setting child process environment variable '"
										 << envname << "' to: " << envvalue
										 << LL_ENDL;
			setenv(envname.c_str(), envvalue.c_str(), 1);
		}
		else if (envvar)
		{
			// If the variable exists and must be removed, then do it now.
			LL_DEBUGS("ProcessLauncher") << "Removing environment variable '"
										 << envname << "' for child process."
										 << LL_ENDL;
			unsetenv(envname.c_str());
		}
	}

	LL_DEBUGS("ProcessLauncher") << "Launching child process: " << mExecutable
								 << LL_ENDL;

 	// Flush all buffers before the child inherits them
 	fflush(NULL);

	pid_t id = fork();
	if (id == 0)
	{
		// Child process code path
		execv(mExecutable.c_str(), (char* const*)fake_argv);

		// If we reach this point, the exec failed. Use _exit() instead of
		// exit() per the fork man page.
		_exit(0);
	}

	// Parent process code path
	if (current_wd >= 0)
	{
		// Restore the previous working directory
		(void)fchdir(current_wd);
		(void)close(current_wd);
	}

	// Restore parent process environment variables. HB
	for (strings_map_t::const_iterator it = mSavedEnvVarMap.begin(),
									   end = mSavedEnvVarMap.end();
		 it != end; ++it)
	{
		const std::string& envname = it->first;
		const std::string& envvalue = it->second;
		if (envvalue == UNSET_MARKER)
		{
			LL_DEBUGS("ProcessLauncher") << "Removing environment variable '"
										 << envname << "' from parent process."
										 << LL_ENDL;
			unsetenv(envname.c_str());
		}
		else
		{
			LL_DEBUGS("ProcessLauncher") << "Restoring parent process environment variable '"
										 << envname << "' to: " << envvalue
										 << LL_ENDL;
			setenv(envname.c_str(), envvalue.c_str(), 1);
		}
	}

	delete[] fake_argv;

	mProcessID = id;

	// At this point, the child process will have been created (since that is
	// how fork works: the child borrowed our execution context until it
	// forked). If the process does not exist at this point, the exec failed.
	if (!isRunning())
	{
		llwarns << "Failed to exec: " << mExecutable << llendl;
		return -1;
	}

	LL_DEBUGS("ProcessLauncher") << "Successfully launched: " << mExecutable
								 << " - pid = " << mProcessID << LL_ENDL;
	return 0;
}

bool LLProcessLauncher::isRunning()
{
	// Check whether the process has exited, and reap it if it has.
	if (mProcessID && reap_pid(mProcessID))
	{
		// The process has exited.
		mProcessID = 0;
		LL_DEBUGS("ProcessLauncher") << "Process for " << mExecutable
									 << " is terminated" << LL_ENDL;
	}
	return mProcessID != 0;
}

bool LLProcessLauncher::kill()
{
	if (mProcessID)
	{
		// Try to kill the process. We will do approximately the same thing
		// whether the kill returns an error or not, so we ignore the result.
		(void)::kill(mProcessID, SIGTERM);

		// This will have the side-effect of reaping the zombie if the process
		// has exited.
		if (isRunning())
		{
			return false;
		}
	}
	return true;
}

void LLProcessLauncher::orphan()
{
	// Disassociate the process from this object
	if (mProcessID)
	{
		// We may still need to reap the process's zombie eventually
		sZombies.push_back(mProcessID);
		mProcessID = 0;
	}
}

//static
void LLProcessLauncher::reap()
{
	// Attempt to reap all saved process ID's.
	std::list<pid_t>::iterator iter = sZombies.begin();
	while (iter != sZombies.end())
	{
		if (reap_pid(*iter))
		{
			iter = sZombies.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

#endif	// LL_LINUX
