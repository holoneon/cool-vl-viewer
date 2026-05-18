/**
 * @file llmessagetemplateparser.h
 * @brief Classes to parse message template.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include <string>

#include "llmessagetemplate.h"

class LLTemplateTokenizer
{
protected:
	LOG_CLASS(LLTemplateTokenizer);

public:
	LLTemplateTokenizer(const std::string & contents);

	U32 line() const;
	bool atEOF() const;
	std::string next();

	bool want(const std::string & token);
	bool wantEOF();
private:
	void inc();
	void dec();
	std::string get() const;
	void error(std::string message = "generic") const;

	struct positioned_token
	{
		std::string str;
		U32 line;
	};

	bool mStarted;
	std::list<positioned_token> mTokens;
	std::list<positioned_token>::const_iterator mCurrent;
};

class LLTemplateParser
{
protected:
	LOG_CLASS(LLTemplateParser);

public:
	typedef std::list<LLMessageTemplate *>::const_iterator message_iterator;

	static LLMessageTemplate * parseMessage(LLTemplateTokenizer & tokens);
	static LLMessageBlock * parseBlock(LLTemplateTokenizer & tokens);
	static LLMessageVariable * parseVariable(LLTemplateTokenizer & tokens);

	LLTemplateParser(LLTemplateTokenizer & tokens);
	message_iterator getMessagesBegin() const;
	message_iterator getMessagesEnd() const;
	F32 getVersion() const;

private:
	F32 mVersion;
	std::list<LLMessageTemplate *> mMessages;
};
