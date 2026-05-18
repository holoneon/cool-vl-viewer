/**
 * @file llurlhistory.cpp
 * @brief Manages a list of recent URLs
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

#include "linden_common.h"

#include "llurlhistory.h"

#include "lldir.h"
#include "llsdserialize.h"
#include "lluri.h"

LLSD LLURLHistory::sHistorySD;

constexpr S32 MAX_URL_COUNT = 10;

/////////////////////////////////////////////////////////////////////////////

//static
bool LLURLHistory::loadFile(const std::string& filename)
{
	std::string temp_str = gDirUtil.getLindenUserDir() + LL_DIR_DELIM_STR +
						   filename;
	llifstream file(temp_str.c_str());
	if (file.is_open())
	{
		llinfos << "Loading URL history: " << temp_str << llendl;
		LLSD data;
		LLSDSerialize::fromXML(data, file);
		if (data.isUndefined())
		{
			llinfos << temp_str << " ill-formed or empty; loading aborted."
					<< llendl;
			sHistorySD.clear();
		}
		else
		{
			sHistorySD = data;
			return true;
		}
	}

	return false;
}

//static
bool LLURLHistory::saveFile(const std::string& filename)
{
	std::string temp_str = gDirUtil.getLindenUserDir();
	if (temp_str.empty())
	{
		llwarns << "Can't save URL history. No user directory set." << llendl;
		return false;
	}

	temp_str += LL_DIR_DELIM_STR + filename;
	llofstream out(temp_str.c_str());
	if (!out.is_open())
	{
		llwarns << "Unable to open '" << temp_str << "' for writing."
				<< llendl;
		return false;
	}

	LLSDSerialize::toXML(sHistorySD, out);
	out.close();
	return true;
}

// This function returns a portion of the history llsd that contains the
// collected url history
//static
LLSD LLURLHistory::getURLHistory(const std::string& collection)
{
	if (sHistorySD.has(collection))
	{
		return sHistorySD[collection];
	}
	return LLSD();
}

//static
void LLURLHistory::addURL(const std::string& collection,
						  const std::string& url)
{
	if (!url.empty())
	{
		LLURI uri(url);
		std::string simplified_url = uri.scheme() + "://" + uri.authority() +
									 uri.path();
		sHistorySD[collection].insert(0, simplified_url);
		LLURLHistory::limitSize(collection);
	}
}

//static
void LLURLHistory::removeURL(const std::string& collection,
							 const std::string& url)
{
	if (url.empty())
	{
		return;
	}

	LLURI uri(url);
	std::string simplified_url = uri.scheme() + "://" + uri.authority() +
								 uri.path();
	for (size_t index = 0; index < sHistorySD[collection].size(); ++index)
	{
		if (sHistorySD[collection].get(index).asString() == simplified_url)
		{
			sHistorySD[collection].erase(index);
		}
	}
}

//static
void LLURLHistory::clear(const std::string& collection)
{
	sHistorySD[collection] = LLSD();
}

void LLURLHistory::limitSize(const std::string& collection)
{
	while ((S32)sHistorySD[collection].size() > MAX_URL_COUNT)
	{
		sHistorySD[collection].erase(MAX_URL_COUNT);
	}
}
