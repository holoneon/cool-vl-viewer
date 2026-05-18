/**
 * @file llcommandlineparser.h
 * @brief LLCommandLineParser class declaration
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

#include <functional>

#include "boost/program_options.hpp"

#include "llstring.h"

// LLCommandLineParser handles defining and parsing the command line.

class LLCommandLineParser
{
protected:
	LOG_CLASS(LLCommandLineParser);

public:
	// Adds a value-less option to the command line description.
	// 'option_name' is the long name of the cmd-line option and 'description'
	// is the text description of the option usage.
	void addOptionDesc(const std::string& option_name,
					   std::function<void(const strings_vec_t&)> notify_cb = NULL,
					   unsigned int num_tokens = 0,
					   const std::string& description = LLStringUtil::null,
					   const std::string& short_name = LLStringUtil::null,
					   bool composing = false, bool positional = false,
					   bool last_option = false);

	// Parses the command line given by argc/argv.
	bool parseCommandLine(int argc, char **argv);

	// Parses the command line contained by the given file.
	bool parseCommandLineString(const std::string& str);

	// Parses the command line contained by the given file.
	bool parseCommandLineFile(const std::basic_istream<char>& file);

	// Calls the callbacks associated with option descriptions; use this to
	// handle the results of parsing.
	void notify();

	// Prints a description of the configured options to the given ostream.
	// Useful for displaying usage info.
	std::ostream& printOptionsDesc(std::ostream& os) const;

	// Use these to retrieve get the values set for an option.

	bool hasOption(const std::string& name) const;
	// Returns an empty value if the option is not set.
	const strings_vec_t& getOption(const std::string& name) const;

	// Prints the list of configured options.
	void printOptions() const;

	// Gets the error message, if it exists.
	LL_INLINE const std::string& getErrorMessage() const	{ return mErrorMsg; }

	// parser_func takes an input string, and should return a name/value pair
	// as the result.
	typedef std::function<std::pair<std::string,
									std::string>(const std::string&)> parser_func;

	// Adds a custom parser func to the parser.
	// Use this method to add a custom parser for parsing values that the
	// simple parser may not handle. It will be applied to each parameter
	// before the default parser gets a chance.
	LL_INLINE void setCustomParser(parser_func f)			{ mExtraParser = f; }

private:
	bool parseAndStoreResults(boost::program_options::command_line_parser& clp);

private:
	std::string mErrorMsg;
	parser_func mExtraParser;
};

LL_INLINE std::ostream& operator<<(std::ostream& out, const LLCommandLineParser& clp)
{
    return clp.printOptionsDesc(out);
}
