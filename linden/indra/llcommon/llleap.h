/**
 * @file llleap.h
 * @brief Declaration of the class implementing "LLSD Event API Plugin"
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Linden Research, Inc.
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

#include <stdexcept>

#include "llinstancetracker.h"

// LLSD Event API Plugin class. Because it is managed by LLInstanceTracker, you
// can instantiate LLLeap and forget the instance unless you need it later.
// Each instance manages an LLProcess; when the child process terminates,
// LLLeap deletes itself. We do not require an unique LLInstanceTracker key.
//
// The fact that a given LLLeap instance vanishes when its child process
// terminates makes it problematic to store an LLLeap* anywhere. Any stored
// LLLeap* pointer should be validated before use by LLLeap::getNamedInstance()
// (see LLInstanceTracker).

class LLLeap : public LLInstanceTracker<LLLeap>
{
public:
	// Pass a brief string description, mostly for logging purposes. The desc
	// need not be unique, but obviously the clearer we can make it, the
	// easier these things will be to debug. The strings are the command line
	// used to launch the desired plugin process.
	//
	// Pass exc = false to suppress LLLeap::Error exception. Obviously in that
	// case the caller cannot discover the nature of the error, merely that an
	// error of some kind occurred (because create() returned NULL). Either
	// way, the error is logged.
	static LLLeap* create(const std::string& desc,
						  const std::vector<std::string>& plugin,
						  bool exc = true);

	// Pass a brief string description, mostly for logging purposes. The desc
	// need not be unique, but obviously the clearer we can make it, the
	// easier these things will be to debug. Pass a command-line string
	// to launch the desired plugin process.
	//
	// Pass exc = false to suppress LLLeap::Error exception. Obviously in that
	// case the caller cannot discover the nature of the error, merely that an
	// error of some kind occurred (because create() returned NULL). Either
	// way, the error is logged.
	static LLLeap* create(const std::string& desc, const std::string& plugin,
						  bool exc = true);

	// Pass an LLSD map to specify desc, executable, args et al.
	//
	// Pass exc = false to suppress LLLeap::Error exception. Obviously in that
	// case the caller cannot discover the nature of the error, merely that an
	// error of some kind occurred (because create() returned NULL). Either
	// way, the error is logged.
	//
	// Note (HB): to avoid implementing the LLInitParam class templates (which
	// we do not use at all in the Cool VL Viewer, since they were originally
	// only used by the v2 UI of LL's viewer), I re-implemented this method to
	// accept only a LLSD::Map containing the following fields:
	//  - params["desc"] : optional LLSD::String. The description of this LEAP.
	//  - params["executable"] : mandatory LLSD::String. The file name for the
	//							 program or script to run.
	//  - params["args"] : optional LLSD::Array. The command line options and
	//					   arguments to pass to the "executable" process.
	//  - params["cwd"] : optional LLSD::String. The working directory to set
	//					  for the "executable" process.
	//  - params["attached"] : optional LLSD::Boolean. Set to force-kill the
	//						   process on this LLeap's instance destruction.
	//						   WARNING: depending on the OS, this could kill
	//						   the viewer as well !
	static LLLeap* create(const LLSD& params, bool exc = true);

	// Exception thrown for invalid create() arguments, e.g. no plugin program.
	// This way, the caller can catch LLLeap::Error and try to recover.
	// Note: I also made it so that LLProcess (which we only use with LLLeap
	// in the Cool VL Viewer) uses LLLeap::Error to throw, instead of an
	// internal LLProcessError like in LL's viewer: this allows to catch any
	// process error as well when launching a new LLLeap. HB
	struct Error : public std::runtime_error
	{
		LL_INLINE Error(const std::string& what)
		:	std::runtime_error(what)
		{
		}

		LL_INLINE Error(const char* what)
		:	std::runtime_error(what)
		{
		}
	};

	~LLLeap() override = default;

	// To toggle binary LLSD stream from the viewer to the LEAP plugin
	virtual void enableBinaryOutput(bool enable) = 0;
	// To toggle binary LLSD stream from the LEAP plugin to the viewer (broken)
	virtual void enableBinaryInput(bool enable) = 0;

	// Let's offer some introspection methods for LLLeap and its associated
	// LLProcess. HB
	virtual bool binaryOutputEnabled() const = 0;
	virtual bool binaryInputEnabled() const = 0;
	virtual const std::string& getDesc() const = 0;
	virtual const std::string& getProcDesc() const = 0;
	virtual const std::string& getExecutable() const = 0;
	virtual const std::string& getInterpreter() const = 0;
	virtual const std::string& getCwd() const = 0;
	virtual const std::vector<std::string>& getArgs() const = 0;

protected:
	// Use the static create() methods to instantiate a new LLLeap.
	LLLeap() = default;
};
