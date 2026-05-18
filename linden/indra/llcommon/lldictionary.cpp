/**
 * @file lldictionary.cpp
 * @brief Lldictionary class header file
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "lldictionary.h"

#include "llstring.h"

LLDictionaryEntry::LLDictionaryEntry(const std::string& name)
:	mName(name)
{
	mNameCapitalized = mName;
	LLStringUtil::replaceChar(mNameCapitalized, '-', ' ');
	LLStringUtil::replaceChar(mNameCapitalized, '_', ' ');
	for (U32 i = 0, count = mNameCapitalized.size(); i < count; ++i)
	{
		if (i == 0 || mNameCapitalized[i - 1] == ' ')
		{
			mNameCapitalized[i] = toupper(mNameCapitalized[i]);
		}
	}
}

void errorDictionaryEntryAlreadyAdded()
{
	llerrs << "Dictionary entry already added (attempted to add duplicate entry)"
		   << llendl;
}
