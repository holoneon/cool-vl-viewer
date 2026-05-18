/**
 * @file lldir_windows.cpp
 * @brief Implementation of Windows-specific directory related methods
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#if LL_WINDOWS

#include "linden_common.h"

#include <shlobj.h>
#include <direct.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lldir.h"

void LLDir::init()
{
	WCHAR w_str[MAX_PATH];

	// Application Data is where user settings go
	SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
					SHGFP_TYPE_DEFAULT, w_str);
	mOSBaseAppDir = ll_convert_wide_to_string(w_str);

	// This is the user's directory
	SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, w_str);
	mOSUserDir = ll_convert_wide_to_string(w_str);

	// We want cache files to go on the local disk, even if the user is on a
	// network with a "roaming profile": E.g. C:\Users\James\AppData\Local
	SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL,
					SHGFP_TYPE_DEFAULT, w_str);
	mOSCacheDir = ll_convert_wide_to_string(w_str);

	if (GetTempPath(MAX_PATH, w_str))
	{
		if (wcslen(w_str))
		{
			w_str[wcslen(w_str) - 1] = '\0';  // Remove trailing slash
		}
		mTempDir = ll_convert_wide_to_string(w_str);
	}
	else
	{
		mTempDir = mOSBaseAppDir;
	}

#if 1
	// Do not use the real app path for now, as we will have to add parsing to
	// detect if we are in a developer tree, which has a different structure
	// from the installed product.

	S32 size = GetModuleFileName(NULL, w_str, MAX_PATH);
	if (size)
	{
		w_str[size] = '\0';
		mExecutablePathAndName = ll_convert_wide_to_string(w_str);
		size_t path_end = mExecutablePathAndName.find_last_of('\\');
		if (path_end != std::string::npos)
		{
			mExecutableDir = mExecutablePathAndName.substr(0, path_end);
			mExecutableFilename = mExecutablePathAndName.substr(path_end + 1);
		}
		else
		{
			mExecutableFilename = mExecutablePathAndName;
		}
		GetCurrentDirectory(MAX_PATH, w_str);
		mWorkingDir = ll_convert_wide_to_string(w_str);
	}
	else
	{
		fprintf(stderr,
				"Could not get APP path, assuming current directory !");
		GetCurrentDirectory(MAX_PATH, w_str);
		mExecutableDir = ll_convert_wide_to_string(w_str);
		// Assume it is the current directory
	}
#else
	GetCurrentDirectory(MAX_PATH, w_str);
	mExecutableDir = utf16str_to_utf8str(llutf16string(w_str));
#endif

	mAppRODataDir = mExecutableDir;

	// Build the default cache directory
	mDefaultCacheDir = buildSLOSCacheDir();

	// Make sure it exists
	if (!LLFile::mkdir(mDefaultCacheDir))
	{
		llwarns << "Could not create LL_PATH_CACHE dir " << mDefaultCacheDir
				<< llendl;
	}

	mLLPluginDir = mExecutableDir + "\\llplugin";

	dumpCurrentDirectories();
}

void LLDir::initAppDirs(const std::string& app_name)
{
	mAppName = app_name;
	mOSUserAppDir = mOSBaseAppDir;
	mOSUserAppDir += "\\";
	mOSUserAppDir += app_name;

	if (!LLFile::mkdir(mOSUserAppDir))
	{
		llwarns << "Could not create app user dir " << mOSUserAppDir
				<< " - Default to base dir " << mOSBaseAppDir << llendl;
		mOSUserAppDir = mOSBaseAppDir;
	}

	if (!LLFile::mkdir(getFullPath(LL_PATH_LOGS)))
	{
		llwarns << "Could not create LL_PATH_LOGS dir "
				<< getFullPath(LL_PATH_LOGS) << llendl;
	}

	if (!LLFile::mkdir(getFullPath(LL_PATH_USER_SETTINGS)))
	{
		llwarns << "Could not create LL_PATH_USER_SETTINGS dir "
				<< getFullPath(LL_PATH_USER_SETTINGS) << llendl;
	}

	if (!LLFile::mkdir(getFullPath(LL_PATH_CACHE)))
	{
		llwarns << "Could not create LL_PATH_CACHE dir "
				<< getFullPath(LL_PATH_CACHE) << llendl;
	}

	mCRTFile = getFullPath(LL_PATH_APP_SETTINGS, "ca-bundle.crt");

	dumpCurrentDirectories();
}

std::string LLDir::getCurPath()
{
	WCHAR w_str[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, w_str);

	return ll_convert_wide_to_string(w_str);
}

std::string LLDir::getLLPluginLauncher()
{
	return mExecutableDir + "\\SLPlugin.exe";
}

std::string LLDir::getLLPluginFilename(std::string base_name)
{
	return mLLPluginDir + "\\" + base_name + ".dll";
}

#endif	// LL_WINDOWS
