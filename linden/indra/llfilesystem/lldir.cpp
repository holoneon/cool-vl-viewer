/**
 * @file lldir.cpp
 * @brief Implementation of directory utilities base class
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

#include "linden_common.h"

#if LL_WINDOWS
# include <direct.h>
#else
# include <sys/stat.h>
# include <sys/types.h>
# include <errno.h>
#endif

#include "lldir.h"

LLDir gDirUtil;

LLDir::LLDir()
:	mUsingDefaultSkin(false),
	mHasLegacySkinOverrides(false)
{
	init();
}

std::string LLDir::findFile(const std::string& filename,
							const std::string& path1,
							const std::string& path2,
							const std::string& path3,
							const std::string& path4) const
{
	std::vector<std::string> search_paths;
	search_paths.push_back(path1);
	search_paths.push_back(path2);
	search_paths.push_back(path3);
	search_paths.push_back(path4);

	std::string fullpath;
	for (S32 i = 0, count = search_paths.size(); i < count; ++i)
	{
		fullpath = search_paths[i];
		if (!fullpath.empty())
		{
			fullpath += LL_DIR_DELIM_STR + filename;
			if (LLFile::exists(fullpath))
			{
				return fullpath;
			}
		}
	}

	return LLStringUtil::null;
}

std::string LLDir::getCacheDir(bool get_default) const
{
	if (mCacheDir.empty() || get_default)
	{
		if (!mDefaultCacheDir.empty())
		{
			// Set at startup: cannot set here due to const API
			return mDefaultCacheDir;
		}

		std::string res = buildSLOSCacheDir();
		return res;
	}
	return mCacheDir;
}

// Return the default cache directory
std::string LLDir::buildSLOSCacheDir() const
{
	if (mOSCacheDir.empty())
	{
		if (mOSUserAppDir.empty())
		{
			return "data";
		}
		return mOSUserAppDir + LL_DIR_DELIM_STR "cache_coolvlviewer";
	}
	return mOSCacheDir + LL_DIR_DELIM_STR "CoolVLViewer";
}

std::string LLDir::getSkinBaseDir() const
{
	return mAppRODataDir + LL_DIR_DELIM_STR "skins";
}

std::string LLDir::getFullPath(ELLPath location, const std::string& subdir1,
							   const std::string& subdir2,
							   const std::string& in_filename) const
{
	std::string prefix;
	switch (location)
	{
		case LL_PATH_NONE:
			// Do nothing
			break;

		case LL_PATH_APP_SETTINGS:
			prefix = mAppRODataDir + LL_DIR_DELIM_STR "app_settings";
			break;

		case LL_PATH_CHARACTER:
			prefix = mAppRODataDir + LL_DIR_DELIM_STR "character";
			break;
#if 0
		case LL_PATH_MOTIONS:
			prefix = mAppRODataDir + LL_DIR_DELIM_STR "motions";
			break;
#endif
		case LL_PATH_HELP:
			prefix = "help";
			break;

		case LL_PATH_CACHE:
	   	 	prefix = getCacheDir();
			break;

		case LL_PATH_USER_SETTINGS:
			prefix = mOSUserAppDir + LL_DIR_DELIM_STR "user_settings";
			break;

		case LL_PATH_PER_ACCOUNT:
			prefix = mLindenUserDir;
			break;

		case LL_PATH_CHAT_LOGS:
			prefix = mChatLogsDir;
			break;

		case LL_PATH_PER_ACCOUNT_CHAT_LOGS:
			prefix = mPerAccountChatLogsDir;
			break;

		case LL_PATH_LOGS:
			prefix = mOSUserAppDir + LL_DIR_DELIM_STR "logs";
			break;

		case LL_PATH_TEMP:
			prefix = mTempDir;
			break;

		case LL_PATH_TOP_SKIN:
			prefix = mSkinDir;
			break;

		case LL_PATH_SKINS:
			prefix = mAppRODataDir + LL_DIR_DELIM_STR "skins";
			break;

		case LL_PATH_EXECUTABLE:
			prefix = mExecutableDir;
			break;

		default:
			llwarns << "Invalid ELLPath number: " << location << llendl;
			llassert(false);
	}

	std::string filename = in_filename;
	if (!subdir2.empty())
	{
		filename = subdir2 + LL_DIR_DELIM_STR + filename;
	}

	if (!subdir1.empty())
	{
		filename = subdir1 + LL_DIR_DELIM_STR + filename;
	}

	std::string expanded_filename;
	if (!filename.empty())
	{
		if (!prefix.empty())
		{
			expanded_filename += prefix + LL_DIR_DELIM_STR + filename;
		}
		else
		{
			expanded_filename = filename;
		}
	}
	else if (!prefix.empty())
	{
		// Directory only, no file name.
		expanded_filename = prefix;
	}
	else
	{
		expanded_filename.clear();
	}

	return expanded_filename;
}

//static
std::string LLDir::getBaseFileName(const std::string& filepath,
								   bool strip_exten)
{
	std::size_t offset = filepath.find_last_of(LL_DIR_DELIM_CHR);
	offset = offset == std::string::npos ? 0 : offset + 1;
	std::string res = filepath.substr(offset);
	if (strip_exten)
	{
		// If basename STARTS with '.', do not strip
		offset = res.find_last_of('.');
		if (offset != std::string::npos && offset != 0)
		{
			res = res.substr(0, offset);
		}
	}
	return res;
}

//static
std::string LLDir::getDirName(const std::string& filepath)
{
	std::size_t offset = filepath.find_last_of(LL_DIR_DELIM_CHR);
	S32 len = (offset == std::string::npos) ? 0 : offset;
	return filepath.substr(0, len);
}

//static
std::string LLDir::getExtension(const std::string& filepath)
{
	std::string exten;

	if (!filepath.empty())
	{
		std::string basename = getBaseFileName(filepath, false);
		std::size_t offset = basename.find_last_of('.');
		if (offset != std::string::npos && offset != 0)
		{
			exten = basename.substr(offset + 1);
			LLStringUtil::toLower(exten);
		}
	}

	return exten;
}

//static
bool LLDir::isRelativePath(const std::string& path)
{
	if (path.empty()) return false;

	if (path == ".." ||
#if LL_WINDOWS	// Also check for UNIX style paths...
		path.find("../") != std::string::npos ||
		path.find("/..") != std::string::npos ||
#endif
		path.find(".." LL_DIR_DELIM_STR) != std::string::npos ||
		path.find(LL_DIR_DELIM_STR "..") != std::string::npos)
	{
		llwarns << "Skipping relative path: " << path << llendl;
		return true;
	}

	return false;
}

bool LLDir::hasSkin(const char* skin_folder) const
{
	std::string colors = LL_DIR_DELIM_STR;
	colors.append(skin_folder);
	colors.append(LL_DIR_DELIM_STR "colors_base.xml");
	return LLFile::exists(mAppRODataDir + LL_DIR_DELIM_STR "skins" + colors) ||
		   LLFile::exists(mOSUserAppDir + LL_DIR_DELIM_STR "cvlv-skins" +
						  colors);
}

std::string LLDir::findSkinnedFilename(const std::string& filename) const
{
	return findSkinnedFilename(LLStringUtil::null, LLStringUtil::null,
							   filename);
}

std::string LLDir::findSkinnedFilename(const std::string& subdir,
									   const std::string& filename) const
{
	std::string ret;
	if (!isRelativePath(subdir))
	{
		ret = findSkinnedFilename(LLStringUtil::null, subdir, filename);
	}
	return ret;
}

std::string LLDir::findSkinnedFilename(const std::string& subdir1,
									   const std::string& subdir2,
									   const std::string& filename) const
{
	if (isRelativePath(subdir1) || isRelativePath(subdir2))
	{
		return "";
	}

	// Generate subdirectory path fragment, e.g. "/foo/bar", "/foo", ""
	std::string subdirs;
	if (!subdir1.empty())
	{
		subdirs = LL_DIR_DELIM_STR + subdir1;
	}
	if (!subdir2.empty())
	{
		subdirs += LL_DIR_DELIM_STR + subdir2;
	}

	if (mUsingDefaultSkin)
	{
		return findFile(filename,
						mUserSkinDir + subdirs,	// First in user skin override
						mSkinDir + subdirs);	// Then in current skin
	}		

	return findFile(filename,
					mUserSkinDir + subdirs,		// First in user skin override
					mSkinDir + subdirs,			// Then in current skin
					// Then in default user skin override
					mUserDefaultSkinDir + subdirs,
					mDefaultSkinDir + subdirs);	// And last in default skin
}

std::string LLDir::getTempFilename(bool with_extension) const
{
	LLUUID random_uuid;
	random_uuid.generate();
	std::string filename = mTempDir + LL_DIR_DELIM_STR +
						   random_uuid.asString();
	if (with_extension)
	{
		filename += ".tmp";
	}
	return filename;
}

std::string LLDir::getUserFilename(std::string desired_subdir,
								   std::string fallback_subdir,
								   std::string filename)
{
	if (filename.front() == '/' ||
#if LL_WINDOWS
		filename.front() == LL_DIR_DELIM_CHR ||
		filename.back() == LL_DIR_DELIM_CHR ||
#endif
		filename.back() == '/')
	{
		llwarns << "Invalid path separator position for a file name: "
				<< filename << llendl;
		return LLStringUtil::null;
	}

	// Check for sub-directory name(s) in the file name.
	size_t i = filename.rfind('/');
	if (i != std::string::npos)
	{
		if (desired_subdir.back() != '/')
		{
			desired_subdir += '/';
		}
		desired_subdir += filename.substr(0, i);
		if (!fallback_subdir.empty())
		{
			if (fallback_subdir.back() != '/')
			{
				fallback_subdir += '/';
			}
			fallback_subdir += filename.substr(0, i);
		}
		filename = filename.substr(i + 1);
	}

	// Sanitize the filename to remove forbidden characters
	filename = getScrubbedFileName(filename);
	if (filename.empty())
	{
		// No need to proceed farther...
		return filename;
	}

	std::string fullpath, subdir;
	if (!desired_subdir.empty())
	{
		// Sanitize the directory name to remove forbidden characters and paths
		subdir = getScrubbedDirName(desired_subdir);
		// Remove all "current directory" path elements
		LLStringUtil::replaceString(subdir, "." LL_DIR_DELIM_STR, "");

		if (!subdir.empty())
		{
			if (subdir.find("~" LL_DIR_DELIM_STR) == 0)
			{
				// We search in the user home directory
				subdir = mOSUserDir + subdir.substr(1);
				if (subdir.back() != LL_DIR_DELIM_CHR)
				{
					fullpath = subdir + LL_DIR_DELIM_STR + filename;
				}
				else
				{
					fullpath = subdir + filename;
				}
				if (LLFile::exists(fullpath))
				{
					return fullpath;	// Success
				}
				fullpath.clear();	// Failed
				// Sanitize the directory name to remove forbidden characters
				// and paths
				subdir = getScrubbedDirName(fallback_subdir);
				if (!subdir.empty())
				{
					if (subdir.back() != LL_DIR_DELIM_CHR)
					{
						fullpath = subdir + LL_DIR_DELIM_STR + filename;
					}
					else
					{
						fullpath = subdir + filename;
					}
					if (!LLFile::exists(fullpath))
					{
						fullpath.clear();	// Failed
					}
				}
				return fullpath;
			}

			// We search in the Secondlife user settings directory, per account
			// first, then global.
			fullpath = getFullPath(LL_PATH_PER_ACCOUNT, subdir, filename);
			if (!LLFile::exists(fullpath))
			{
				fullpath = getFullPath(LL_PATH_USER_SETTINGS, subdir,
									   filename);
				if (!LLFile::exists(fullpath))
				{
					fullpath.clear();	// Failed
				}
			}
		}
	}
	if (fullpath.empty())
	{
		// Sanitize the directory name to remove forbidden characters and paths
		subdir = getScrubbedDirName(fallback_subdir);
		if (!subdir.empty())
		{
			fullpath = getFullPath(LL_PATH_PER_ACCOUNT, subdir, filename);
			if (!LLFile::exists(fullpath))
			{
				fullpath = getFullPath(LL_PATH_USER_SETTINGS, subdir,
									   filename);
				if (!LLFile::exists(fullpath))
				{
					fullpath.clear();	// Failed
				}
			}
		}
	}

	return fullpath;
}

//static
std::string LLDir::getForbiddenDirChars()
{
	return ":*?\"<>|";
}

//static
std::string LLDir::getForbiddenFileChars()
{
	return "\\/:*?\"<>|";
}

//static
std::string LLDir::getScrubbedDirName(const std::string& dirname)
{
	std::string cleanname = dirname;
	// Use the proper directory delimiter everywhere in name and remove parent
	// directory symbols to forbid navigating upwards in the file system.
#if LL_WINDOWS
	LLStringUtil::replaceChar(cleanname, '/', '\\');
	LLStringUtil::replaceString(cleanname, "..\\", "");
#else
	LLStringUtil::replaceChar(cleanname, '\\', '/');
	LLStringUtil::replaceString(cleanname, "../", "");
#endif

	const std::string illegal_chars = getForbiddenDirChars();
	// Replace any illegal directory chararacter with an underscore '_'
	for (size_t i = 0, count = illegal_chars.length(); i < count; ++i)
	{
		const char& c = illegal_chars[i];
		size_t j = 0;
		while ((j = cleanname.find(c, j)) != std::string::npos)
		{
			cleanname[j++] = '_';
		}
	}
	return cleanname;
}

//static
std::string LLDir::getScrubbedFileName(const std::string& filename)
{
	std::string cleanname = filename;
	const std::string illegal_chars = getForbiddenFileChars();
	// Replace any illegal file chararacter with an underscore '_'
	for (size_t i = 0, count = illegal_chars.length(); i < count; ++i)
	{
		const char& c = illegal_chars[i];
		size_t j = 0;
		while ((j = cleanname.find(c, j)) != std::string::npos)
		{
			cleanname[j++] = '_';
		}
	}
	return cleanname;
}

void LLDir::setLindenUserDir(const std::string& grid, const std::string& first,
							 const std::string& last)
{
	// If both first and last are not set, assume we are grabbing the cached
	// directory
	if (!first.empty() && !last.empty())
	{
		// Some platforms have case-sensitive filesystems, so be utterly
		// consistent with our firstname/lastname case.
		std::string fnlc = first;
		LLStringUtil::toLower(fnlc);
		std::string lnlc = last;
		LLStringUtil::toLower(lnlc);
		mLindenUserDir = mOSUserAppDir + LL_DIR_DELIM_STR + fnlc + "_" + lnlc;
		// Append the name of the grid, but only when not in SL to stay upward
		// compatible with SL viewers.
		if (!grid.empty())
		{
			std::string gridlc = getScrubbedFileName(grid);
			LLStringUtil::toLower(gridlc);
			if (gridlc.find("secondlife") == std::string::npos)
			{
				if (gridlc == "none" || gridlc == "other")
				{
					// Unknown grid name...
					gridlc = "unknown";
				}
				mLindenUserDir += "@" + gridlc;
			}
		}
	}
	else
	{
		llwarns << "Invalid name for User Dir, adopting: unknown_user"
				<< llendl;
		mLindenUserDir = mOSUserAppDir + LL_DIR_DELIM_STR "unknown_user";
	}

	dumpCurrentDirectories();
}

void LLDir::setChatLogsDir(const std::string& path)
{
	if (!path.empty())
	{
		mChatLogsDir = path;
	}
	else
	{
		llwarns << "Invalid (emtpy) path name" << llendl;
	}

	dumpCurrentDirectories();
}

void LLDir::setPerAccountChatLogsDir(const std::string& grid,
									 const std::string& first,
									 const std::string& last)
{
	// If both first and last are not set, assume we are grabbing the cached
	// directory
	if (!first.empty() && !last.empty())
	{
		// some platforms have case-sensitive filesystems, so be
		// utterly consistent with our firstname/lastname case.
		std::string fnlc(first);
		LLStringUtil::toLower(fnlc);
		std::string lnlc(last);
		LLStringUtil::toLower(lnlc);
		mPerAccountChatLogsDir = mChatLogsDir + LL_DIR_DELIM_STR + fnlc + "_" +
								 lnlc;
		// Append the name of the grid, but only when not in SL to stay upward
		// compatible with SL viewers.
		if (!grid.empty())
		{
			std::string gridlc = getScrubbedFileName(grid);
			LLStringUtil::toLower(gridlc);
			if (gridlc.find("secondlife") == std::string::npos)
			{
				if (gridlc == "none" || gridlc == "other")
				{
					// Unknown grid name...
					gridlc = "unknown";
				}
				mPerAccountChatLogsDir += "@" + gridlc;
			}
		}
	}
	else
	{
		llwarns << "Invalid name: " << (first.empty() ? "first" : "last")
				<< " name is empty !" << llendl;
	}

	dumpCurrentDirectories();
}

void LLDir::setSkinFolder(const std::string& skin_folder)
{
	mUsingDefaultSkin = skin_folder == "default";

	// Base skin directory for this skin folder
	// e.g. c:\Program Files\CoolVLViewer-X.Y.Z\skins\silver
	mSkinDir = mAppRODataDir + LL_DIR_DELIM_STR "skins" LL_DIR_DELIM_STR +
			   skin_folder;

	// Base skin directory which is used as a fallback for all skinned files,
	// e.g. c:\Program Files\CoolVLViewer-X.Y.Z\skins\default
	mDefaultSkinDir = mAppRODataDir +
					  LL_DIR_DELIM_STR "skins" LL_DIR_DELIM_STR "default";

	// Base folder for user-made skin overrides e.g. ~/.secondlife/cvlv-skins
	// IMPORTANT: the sub-directory for skin overrides used to be "skins" with
	// Cool VL Viewer versions before 1.32.2.17, but I changed this to a
	// specific "cvlv-skins" sub-directory in order to avoid collisions with
	// other viewers. HB
	std::string user_skin_dir = mOSUserAppDir + LL_DIR_DELIM_STR "cvlv-skins";

	// Skin overrides used to live in a skins/ sub-directory of the user
	// settings directory, but this caused collisions with other viewers.
	// Let's flag the case when the old sub-directory exists and not the new
	// one; this will allow to notify the user about this change, after login.
	// HB
	mHasLegacySkinOverrides = !LLFile::isdir(user_skin_dir) &&
							  LLFile::isdir(mOSUserAppDir +
											LL_DIR_DELIM_STR "skins");
	user_skin_dir += LL_DIR_DELIM_STR;

	// User overrides to current skin e.g. ~/.secondlife/cvlv-skins/silver
	mUserSkinDir = user_skin_dir + skin_folder;

	// User overrides to default skin e.g. ~/.secondlife/cvlv-skins/default
	mUserDefaultSkinDir = user_skin_dir + "default";

	dumpCurrentDirectories();
}

bool LLDir::setCacheDir(const std::string& path)
{
	bool success = false;

	if (path.empty())
	{
		// Reset to default
		mCacheDir.clear();
		success = true;
	}
	else
	{
		LLFile::mkdir(path);
		std::string tempname = path + LL_DIR_DELIM_STR "temp";
		LLFILE* file = LLFile::open(tempname, "wt");
		if (file)
		{
			LLFile::close(file);
			LLFile::remove(tempname);
			mCacheDir = path;
			success = true;
		}
	}

	dumpCurrentDirectories();
	return success;
}

void LLDir::dumpCurrentDirectories()
{
	LL_DEBUGS("AppInit") << "Current Directories:" << LL_ENDL;

	LL_DEBUGS("AppInit") << "  CurPath:               "
						 << getCurPath() << LL_ENDL;
	LL_DEBUGS("AppInit") << "  AppName:               "
						 << mAppName << LL_ENDL;
	LL_DEBUGS("AppInit") << "  ExecutableFilename:    "
						 << mExecutableFilename << LL_ENDL;
	LL_DEBUGS("AppInit") << "  ExecutableDir:         "
						 << mExecutableDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  ExecutablePathAndName: "
						 << mExecutablePathAndName << LL_ENDL;
	LL_DEBUGS("AppInit") << "  LLPluginDir: "
						 << mLLPluginDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  WorkingDir:            "
						 << mWorkingDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  AppRODataDir:          "
						 << mAppRODataDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  OSUserDir:             "
						 << mOSUserDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  OSUserAppDir:          "
						 << mOSUserAppDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  LindenUserDir:         "
						 << mLindenUserDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  ChatLogsDir:           "
						 << mChatLogsDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  PerAccountChatLogsDir: "
						 << mPerAccountChatLogsDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  TempDir:               "
						 << mTempDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  CRTFile:               "
						 << mCRTFile << LL_ENDL;
	LL_DEBUGS("AppInit") << "  SkinDir:               "
						 << mSkinDir << LL_ENDL;
	LL_DEBUGS("AppInit") << "  OSCacheDir:            "
						 << mOSCacheDir << LL_ENDL;
}
