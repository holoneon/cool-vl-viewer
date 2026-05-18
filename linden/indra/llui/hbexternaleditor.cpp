/**
 * @file hbexternaleditor.cpp
 * @brief Utility class to launch an external program for editing a file and
 * tracking changes on the latter.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 *
 * Copyright (c) 2019-2026, Henri Beauchamp.
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

#if  LL_LINUX
# include <stdlib.h>					// For getenv()
#endif

#include "hbexternaleditor.h"

#include "lllivefile.h"
#include "llprocesslauncher.h"
#include "lltrans.h"
#include "llui.h"				// For LLUI::sConfigGroup

///////////////////////////////////////////////////////////////////////////////
// HBEditorLiveFile class, for edited file live tracking
///////////////////////////////////////////////////////////////////////////////

class HBEditorLiveFile final : public LLLiveFile
{
public:
	HBEditorLiveFile(HBExternalEditor* editor, const std::string& filename)
	:	LLLiveFile(filename, 1.f),
		mEditor(editor)
	{
	}

protected:
	bool loadFile() override
	{
		if (mEditor)
		{
			mEditor->callChangedCallback(filename());
		}
		return true;
	}

private:
	HBExternalEditor* mEditor;
};

///////////////////////////////////////////////////////////////////////////////
// HBExternalEditor class proper
///////////////////////////////////////////////////////////////////////////////

HBExternalEditor::HBExternalEditor(HBExternalEditorFileChangedCB callback,
								   void* userdata, bool orphanize_on_destroy)
:	mFiledChangedCallback(callback),
	mUserData(userdata),
	mIgnoreNextUpdate(false),
	mEditorIsDetached(false),
	mOrphanizeOnDestroy(orphanize_on_destroy),
	mProcess(NULL),
	mEditedFile(NULL)
{
}

HBExternalEditor::~HBExternalEditor()
{
	if (mEditedFile)
	{
		delete mEditedFile;
	}
	if (mProcess)
	{
		if (mOrphanizeOnDestroy)
		{
			mProcess->orphan();
		}
		delete mProcess;
	}
}

void HBExternalEditor::callChangedCallback(const std::string& filename)
{
	if (!mIgnoreNextUpdate && mFiledChangedCallback)
	{
		mFiledChangedCallback(filename, mUserData);
	}
	mIgnoreNextUpdate = false;
}

bool HBExternalEditor::open(const std::string& filename, std::string cmd)
{
	if (!LLFile::isfile(filename))
	{
		mErrorMessage = LLTrans::getString("file_not_found") + " " + filename;
		llwarns << mErrorMessage << llendl;
		return false;
	}

	mEditorIsDetached = false;
	if (cmd.empty())
	{
		cmd = LLUI::sConfigGroup->getString("ExternalEditor");
	}
	if (cmd.empty())
	{
#if LL_LINUX
		llwarns << "Could not find a configured editor; trying 'xdg-open'. This is suboptimal because the state of the editor it will launch cannot be tracked. Please, consider configuring the \"ExternalEditor\" setting."
				<< llendl;
		// *TODO: try every PATH element, in case xdg-open is not in /usr/bin ?
		cmd = "/usr/bin/xdg-open %s";
		mEditorIsDetached = true;
#elif LL_WINDOWS
		llwarns << "Could not find a configured editor; trying 'explorer.exe' to dispatch to the system editor."
				<< llendl;
		cmd = "%SystemRoot%\\explorer.exe \"%s\"";
#endif
	}
	LLStringUtil::trim(cmd);
	if (cmd.empty())
	{
		mErrorMessage = LLTrans::getString("no_valid_command");
		llwarns << mErrorMessage << llendl;
		return false;
	}

	// Split the command line between program file name and arguments
	std::string prg;
	size_t i;
	if (cmd[0] == '"')
	{
		// Starting with a quoted program name, as often seen under Windows,
		// because of spaces in the path.
		i = cmd.find('"', 1);	// Find the matching closing quote
		if (i == std::string::npos)
		{
			mErrorMessage = LLTrans::getString("bad_quoting");
			llwarns << mErrorMessage << llendl;
			return false;
		}
		prg = cmd.substr(1, i - 1);
		cmd = cmd.substr(i + 1);
	}
	else
	{
		i = cmd.find(' ', 1);	// Find the first space
		if (i == std::string::npos)
		{
			// No argument, just a program...
			prg = cmd;
			cmd.clear();
		}
		else
		{
			prg = cmd.substr(0, i);
			cmd = cmd.substr(i + 1);
		}
	}
	if (cmd.find("%s") == std::string::npos)
	{
		// Add the filename if absent from the arguments
#if LL_WINDOWS
		cmd += " \"%s\"";
#else
		cmd += " %s";
#endif
	}
	LLStringUtil::trimHead(cmd);

	if (!LLFile::isfile(prg))
	{
		mErrorMessage = LLTrans::getString("program_not_found") + " " + prg;
		llwarns << mErrorMessage << llendl;
		return false;
	}

	llinfos << "Using external editor command line: " << prg << " " << cmd
			<< llendl;

	if (mEditedFile)
	{
		delete mEditedFile;
		mEditedFile = NULL;
	}
	// Watch as live file only if we got a "file changed" event callback
	if (mFiledChangedCallback)
	{
		mEditedFile = new HBEditorLiveFile(this, filename);
		mEditedFile->addToEventTimer();
	}

	std::vector<std::string> tokens;
	LLStringUtil::getTokens(cmd, tokens, " ");
	if (mProcess)
	{
		if (mOrphanizeOnDestroy)
		{
			mProcess->orphan();
		}
		else
		{
			mProcess->kill();
		}
		mProcess->clearArguments();
		mProcess->setWorkingDirectory("");
	}
	else
	{
		mProcess = new LLProcessLauncher();
	}
	mProcess->setExecutable(prg);
	for (U32 i = 0, count = tokens.size(); i < count; ++i)
	{
		std::string& parameter = tokens[i];
		if (!parameter.empty())
		{
#if LL_LINUX
			// Under POSIX operating systems, arguments for execv() are passed
			// in the argv array and none need quoting; much to the contrary
			// since quotes would cause the path to be considered relative and
			// be prefixed with the working directory path, which is not what
			// we want here !
			LLStringUtil::replaceString(parameter, "\"%s\"", filename);
#endif
			LLStringUtil::replaceString(parameter, "%s", filename);
			mProcess->addArgument(parameter);
		}
	}

#if LL_LINUX
	// Reset LD_LIBRARY_PATH to the operating system path, as provided before
	// viewer launch, so that we do not pollute the editor with custom versions
	// of the libraries bundled with the viewer and which could prove to be
	// incompatible with the editor. Also, do not preload the libcef library or
	// jemalloc. Instead, use the LD_PRELOAD value found on viewer launch. Note
	// that LD_LIBRARY_PATH, SAVED_LD_LIBRARY_PATH, SAVED_LD_PRELOAD and
	// LD_PRELOAD are set by the Linux wrapper script. HB
	char* envvar = getenv("SAVED_LD_LIBRARY_PATH");
	if (envvar && *envvar)
	{
		mProcess->setEnv("LD_LIBRARY_PATH", envvar);
	}
	else
	{
		mProcess->unsetEnv("LD_LIBRARY_PATH");
	}
	envvar = getenv("SAVED_LD_PRELOAD");
	if (envvar && *envvar)
	{
		mProcess->setEnv("LD_PRELOAD", envvar);
	}
	else
	{
		mProcess->unsetEnv("LD_PRELOAD");
	}
#endif

	if (mProcess->launch() != 0)
	{
		mErrorMessage = LLTrans::getString("command_failed") + " " + prg +
						" " + cmd;
		llwarns << mErrorMessage << llendl;
		kill();
		return false;
	}

	// Opening the file in the external editor caused it to be touched and we
	// do not want to trigger a "file changed" event for this...
	mIgnoreNextUpdate = true;

	return true;
}

void HBExternalEditor::kill()
{
	if (mEditedFile)
	{
		delete mEditedFile;
		mEditedFile = NULL;
	}
	if (mProcess)
	{
		if (mEditorIsDetached)
		{
			llwarns << "Cannot kill a detached editor process..." << llendl;
		}
		delete mProcess;
		mProcess = NULL;
	}
}

bool HBExternalEditor::running()
{
	return mProcess && (mEditorIsDetached || mProcess->isRunning());
}

std::string HBExternalEditor::getFilename()
{
	return mEditedFile ? mEditedFile->filename() : LLStringUtil::null;
}
