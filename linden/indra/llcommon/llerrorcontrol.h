/**
 * @file llerrorcontrol.h
 * @brief Error message system control functions declarations
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * Copyright (c) 2009-2023, Henri Beauchamp.
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

#include "llerror.h"
#include "llstring.h"

class LLSD;

// Note: the abort() call is normally redundant, but better safe than sorry. HB
#if LL_MSVC
# define LL_ERROR_CRASH	__debugbreak(); abort()
#else
# define LL_ERROR_CRASH	__builtin_trap(); abort()
#endif

// This is the part of the LLError namespace that manages the messages produced
// by the logging. The logging support is defined in llerror.h. Most files do
// not need to include this. These implementations are in llerror.cpp.

// Line buffer interface
class LLLineBuffer
{
public:
	LLLineBuffer() = default;
	virtual ~LLLineBuffer() = default;

	virtual void clear() = 0; // Clear the buffer, and reset it.

	virtual void addLine(const std::string& utf8line) = 0;
};

namespace LLError
{
	// Resets all logging settings to defaults needed by application logs to
	// stderr and windows debug log sets up log configuration from the file
	// logcontrol.xml in dir
	void initForApplication(const std::string& dir);

	//
	// Settings that control what is logged.
	// Setting a level means log messages at that level or above.
	//

	void setPrintLocation(bool print);
	void setDefaultLevel(ELevel level);
	void setFunctionLevel(const std::string& function_name, ELevel level);
	void setClassLevel(const std::string& class_name, ELevel level);
	void setFileLevel(const std::string& file_name, ELevel level);
	void setTagLevel(const std::string& file_name, ELevel level);
	ELevel getTagLevel(const std::string& tag_name);

	strings_set_t getTagsForLevel(ELevel level);

	// The LLSD can configure all of the settings usually read automatically
	// from the live errorlog.xml file
	void configure(const LLSD&);

	//
	// Control functions.
	//

	// The fatal function will be called when an message of LEVEL_ERROR is
	// logged. Note: supressing a LEVEL_ERROR message from being logged (by,
	// for example, setting a class level to LEVEL_NONE), will keep the that
	// message from causing the fatal funciton to be invoked.
	typedef void (*fatal_func_t)(const std::string& message);
	void setFatalFunction(fatal_func_t func);

	// Returns the UTC time stamp with milliseconds when 'print_ms' is true. HB
	std::string utcTime(bool print_ms);

	// This function is used to return the current time, formatted for display
	// by those error recorders that want the time included.
	// Note: 'print_ms' must be true to get milliseconds in time stamp, false
	// otherwise. HB
	typedef std::string (*time_func_t)(bool print_ms);
	void setTimeFunction(time_func_t);

	// An object that handles the actual output or error messages.
	class Recorder
	{
	public:
		virtual ~Recorder();

		// Uses the level for better display, not for filtering
		virtual void recordMessage(ELevel,
								   const std::string& message) = 0;

		// Overrides and returns true if the recorder wants the time string
		// included in the text of the message
		virtual bool wantsTime(); // default returns false
	};

	// Each error message is passed to each recorder via recordMessage()
	void addRecorder(Recorder*);
	void removeRecorder(Recorder*);

	// Utilities to add recorders for logging to a file or a fixed buffer.
	// A second call to the same function will remove the logger added with
	// the first. Passing the empty string or NULL to just removes any prior.
	void logToFile(const std::string& filename);
	void logToFixedBuffer(LLLineBuffer*);

	// Returns name of current logging file, empty string if none
	std::string logFileName();

	// Sets the name of current logging file (used in llappviewer.cpp to keep
	// track of which log is in use, depending on other running instances).
	void setLogFileName(std::string filename);

	void logToFile(const std::string& filename);

	//
	// Utilities for use by the unit tests of LLError itself.
	//

	class Settings;
	Settings* saveAndResetSettings();
	void restoreSettings(Settings*);

	std::string abbreviateFile(const std::string& filePath);
};
