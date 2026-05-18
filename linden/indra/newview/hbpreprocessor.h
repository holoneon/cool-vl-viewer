/**
 * @file hbpreprocessor.h
 * @brief HBPreprocessor class definition
 *
 * HBPreprocessor is a simple sources pre-processor with support for #include,
 * #define (plain defines, no macros)/#undef/#if/#ifdef/#ifndef/#elif/#else/
 * #endif/#warning/#error directives (it also got special #pragma's).
 * It is of course not as complete as boost::wave, but it is about 20 times
 * smaller (when comparing stripped binaries).
 * It can be used to preprocess Lua or LSL source files, for example.
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

#pragma once

#include <stack>
#include <vector>

#include "llerror.h"
#include "llstring.h"

struct lua_State;

class HBPreprocessor
{
protected:
	LOG_CLASS(HBPreprocessor);

public:
	enum { PAUSED = -1, FAILURE = 0, SUCCESS = 1 };

	// The #include callback gets passed the include name, mDefaultIncludePath,
	// as well as the 'userdata' that was set on HBPreprocessor construction,
	// and must fill 'buffer' with the contents of the corresponding file or
	// asset. The callback should also modify include_name to prepend it with
	// the full path (so that it can properly be reported in __FILE__). The
	// callback shall return HBPreprocessor::FAILURE on failure to find or load
	// the include, HBPreprocessor::PAUSED if the include asset is not yet
	// available, or HBPreprocessor::SUCCESS on success.
	typedef S32 (*HBPPIncludeCB)(std::string& include_name,
								 const std::string& default_path,
								 std::string& buffer, void* userdata);

	HBPreprocessor(const std::string& file_name, HBPPIncludeCB callback,
				   void* userdata = NULL);
	~HBPreprocessor();

	// Resets the preprocessed data
	void clear();

	// Preprocesses 'sources'. Returns HBPreprocessor::FAILURE on failure,
	// HBPreprocessor::PAUSED when the preprocessing was paused due to a not
	// yet loaded include asset, or HBPreprocessor::SUCCESS on success.
	// The pre-processed sources (up to the last valid line when an error
	// occurred) can be retrieved with getResult() and any error message can be
	// retrieved with getError().
	S32 preprocess(const std::string& sources);

	// This method is to be called after a pause and the include asset that
	// caused it is available. It returns the same results as preprocess()
	// above.
	S32 resume();

	LL_INLINE const std::string& getResult() const	{ return mPreprocessed; }
	LL_INLINE const std::string& getError() const	{ return mErrorMessage; }

	// Returns the line number in the original, non-preprocessed sources, that
	// corresponds to the 'line' in the preprocessed sources, or 0 if there is
	// no match or no mapping.
	LL_INLINE S32 getOriginalLine(S32 line) const
	{
		if (line < 1 || line > (S32)mLineMapping.size())
		{
			return 0;
		}
		return mLineMapping[line - 1];
	}

	// Used to change the preprocessed file name when needed.
	LL_INLINE void setFilename(const std::string& file_name)
	{
		mFilename = file_name;
	}

	// This is an optional callback, for the caller to be informed of warning
	// and error messsages. The callback gets passed the 'userdata' that was
	// set on HBPreprocessor construction.
	typedef void (*HBPPMessageCB)(const std::string& message, bool is_warning,
								  void* userdata);

	// This allows to set the optional error and warning message callback
	LL_INLINE void setMessageCallback(HBPPMessageCB callback)
	{
		mMessageCallback = callback;
	}

	// This is for language-specifc needs, when you want to prevent the use
	// of #define or #undef with special language tokens (e.g. "_G" for Lua).
	LL_INLINE void addForbiddenToken(const char* token)
	{
		mForbiddenTokens.emplace(token);
	}

	// This returns any <path> in the last '#pragma include-from: <path>'
	// directive encountered in the unprocessed sources, or an empty string
	// when no such directive was encountered.
	LL_INLINE const std::string& getDefaultIncludePath() const
	{
		return mDefaultIncludePath;
	}

	// Returns the value for 'name' if the latter is defined, else returns
	// 'name' itself.
	std::string getDefine(const std::string& name) const;

	// Returns true when 'sources' contains preprocessor directives
	static bool needsPreprocessing(const std::string& sources);

	// Tries and evaluates a mathematical 'expression' using any provided Lua
	// state, or creating a temporary one when none is provided; returns 'true'
	// and the result in 'expression' on success, or 'false' (and a tab-clean,
	// space-trimmed 'expression') on failure.
	// Valid operators for the math expression are: +, -, *, /, %, ^
	// Valid math functions are: acos(), cos(), asin(), sin(), atan(), tan(),
	// abs() (or fabs()), mod() (or fmod()), max(), min(), ceil(), floor(),
	// int(), deg(), rad(), sqrt(), exp(), log(), rand().
	// You may also use 'PI' as a constant in the expression.
	static bool evaluate(std::string& expression, lua_State* statep = NULL);

private:
	void parsingError(const std::string& error_msg, bool is_warning = false);

	bool isValidToken(const std::string& token);

	// Replaces 'defined(TOKEN)' expressions in 'expr' with either 1 when TOKEN
	// is defined, or 0 when not, then returns the result.
	std::string replaceDefinedInExpr(std::string expr);

	// Replaces in 'line' all defined tokens with their value and returns the
	// result.
	std::string replaceDefinesInLine(const std::string& line);

	// Replaces 'eval(MATH_EPRESSION)' expressions in 'expr' with the result of
	// the evaluation of MATH_EPRESSION, then returns the result. When the
	// 'replace_tokens' boolean is true, each #defined tokens *inside* the each
	// eval() expression is also replaced with their value.
	std::string replaceEvalInExpr(std::string expr,
								  bool replace_tokens = false);

	bool skipToElseOrEndif(const std::string& buffer, size_t& pos);

	// Method used to evaluate a logical expression using Lua. Note that the
	// "!=", "||", "&&", "!" and "^" operators are automatically translated
	// into their Lua equivalent. This method returns a bool, which is true if
	// the result is not zero or false otherwise (including in case of error).
	// In case of error, the mErrorMessage variables is set.
	bool isExpressionTrue(std::string expression);

	// Used to pre-populate the defines table with the default (and constant
	// during preprocess() execution) defines, namely __DATE__, __TIME__,
	// __AGENT_ID__ and __AGENT_NAME__.
	void setDefaultDefines();

private:
	S32						(*mIncludeCallback)(std::string& include_name,
												const std::string& path,
												std::string& buffer,
												void* userdata);
	void 					(*mMessageCallback)(const std::string& message,
												bool is_warning,
												void* userdata);
	void*					mCallbackUserData;

	lua_State*				mLuaState;

	S32						mCurrentLine;
	S32						mSavedPos;
	// This is the line of the root #include directive in the unprocessed
	// sources. It is used to map #included sources preprocessed lines with
	// unprocessed sources lines.
	S32						mRootIncludeLine;

	std::string				mFilename;
	std::string				mErrorMessage;
	std::string				mSourcesBuffer;
	std::string				mIncludeBuffer;
	std::string				mPreprocessed;
	std::string				mDefaultIncludePath;

	strings_map_t			mDefines;
	strings_set_t			mIncludes;
	strings_set_t			mForbiddenTokens;
	std::stack<std::string>	mFilenames;
	std::stack<bool>		mIfClauses;
	// This is the line mapping: each entry in this vector corresponds to a
	// line of the preprocessed sources (with line 1 at index 0 of the vector)
	// and contains the corresponding line number in the unprocessed sources
	// (for include files, it is the line number of the root #include directive
	// in the unprocessed sources).
	typedef std::vector<S32> line_map_vec_t;
	line_map_vec_t			mLineMapping;
};

// Extracts one line of text from a buffer: the line shall always be terminated
// with a line feed (true for Linux and even Windows that also got a carriage
// return before the line feed).
// The line feed (and possible carriage return) is part of the returned 'line'.
// On return, 'pos' is updated to point to the start of the next line in the
// buffer.
// Used also in llpreviewscript.cpp
std::string get_one_line(const std::string& buffer, size_t& pos);
