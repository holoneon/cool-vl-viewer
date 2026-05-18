/**
 * @file slplugin.cpp
 * @brief Loader shell for plugins, intended to be launched by the plugin host application, which directly loads a plugin dynamic library.
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

#include "linden_common.h"

#include "llapr.h"
#include "llerrorcontrol.h"
#include "llpluginprocesschild.h"
#include "llpluginmessage.h"
#include "llstring.h"

#include <iostream>
#include <fstream>
using namespace std;

#if LL_WINDOWS
# include <windows.h>
#endif

#if LL_LINUX
# include <signal.h>
# include <X11/Xlib.h>

// Signal handlers to make crashes not show an OS dialog...
static void crash_handler(int sig)
{
	// Just exit cleanly. *TODO: add our own crash reporting
	_exit(1);
}
#endif

#if LL_WINDOWS
// Our exception handler: it will probably just exit and the host application
// will miss the heartbeat and log the error in the usual fashion.
LONG WINAPI customExceptionHandler(struct _EXCEPTION_POINTERS* exception_infop)
{
	// *TODO: replace exception handler before we exit ?
	return EXCEPTION_EXECUTE_HANDLER;
}

bool checkExceptionHandler()
{
	bool ok = true;
	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter =
		SetUnhandledExceptionFilter(customExceptionHandler);

	if ((void*)prev_filter != (void*)customExceptionHandler)
	{
		llwarns << "Our exception handler (" << (void*)customExceptionHandler
				<< ") replaced with " << (void*)prev_filter << "!" << llendl;
		ok = false;
	}

	if (!prev_filter)
	{
		ok = false;
		llwarns << "Our exception handler (" << (void*)customExceptionHandler
				<< ") replaced with NULL !" << llendl;
	}

	return ok;
}
#endif

// If this application on Windows platform is a console application, a console
// is always created which is bad. Making it a Windows "application" via CMake
// settings but not adding any code to explicitly create windows does the right
// thing.
#if LL_WINDOWS
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR cmd_line, int)
#else
int main(int argc, char** argv)
#endif
{
#if LL_LINUX
	// Ensure Xlib is started in thread-safe state
	XInitThreads();
#endif

	ll_init_apr();

	// Set up llerror logging
	{
		LLError::initForApplication(".");
		LLError::setDefaultLevel(LLError::LEVEL_INFO);
#if 0
		LLError::setTagLevel("Plugin", LLError::LEVEL_DEBUG);
		LLError::logToFile("slplugin.log");
#endif
	}

#if LL_WINDOWS
	if (strlen(cmd_line) == 0)
	{
		llerrs << "Usage: SLPlugin launcher_port" << llendl;
	}

	U32 port = 0;
	if (!LLStringUtil::convertToU32(cmd_line, port))
	{
		llerrs << "Port number must be numeric" << llendl;
	}

	// Insert our exception handler into the system so this plugin doesn't
	// display a crash message if something bad happens. The host app will
	// see the missing heartbeat and log appropriately.
	SetUnhandledExceptionFilter(customExceptionHandler);
#elif LL_LINUX
	if (argc < 2)
	{
		llerrs << "Usage: " << argv[0] << " launcher_port" << llendl;
	}

	U32 port = 0;
	if (!LLStringUtil::convertToU32(argv[1], port))
	{
		llerrs << "Port number must be numeric" << llendl;
	}

	// Catch signals that most kinds of crashes will generate, and exit cleanly
	// so the system crash dialog isn't shown.
	signal(SIGILL, &crash_handler);		// illegal instruction
	signal(SIGFPE, &crash_handler);		// floating-point exception
	signal(SIGBUS, &crash_handler);		// bus error
	signal(SIGSEGV, &crash_handler);	// segmentation violation
	signal(SIGSYS, &crash_handler);		// non-existent system call invoked
#endif

	LLPluginProcessChild* pluginp = new LLPluginProcessChild();
	pluginp->init(port);

	LLTimer timer;
	timer.start();

#if LL_WINDOWS
	checkExceptionHandler();
#endif

	while (!pluginp->isDone())
	{
		timer.reset();
		pluginp->idle();

		F64 elapsed = timer.getElapsedTimeF64();
		F64 remaining = pluginp->getSleepTime() - elapsed;

		if (remaining <= 0.f)
		{
			// We have already used our full allotment. Still need to service
			// the network...
			pluginp->pump();
		}
		else
		{
			// This also services the network as needed.
			pluginp->sleep(remaining);
		}

#if LL_WINDOWS && 0	// Does not appear to be required so far, even for plugins
					// that do crash with a single call to the intercept
					// exception handler.
		// More agressive checking of interfering exception handlers.
		checkExceptionHandler();
#endif
	}

	delete pluginp;

	ll_cleanup_apr();

	return 0;
}
