/** 
 * @file lldictionary.h
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

#pragma once

#include <map>
#include <string>

#include "llpreprocessor.h"

struct LLDictionaryEntry
{
	LLDictionaryEntry(const std::string& name);
	virtual ~LLDictionaryEntry() = default;

	const std::string mName;
	std::string mNameCapitalized;
};

// This is to avoid inlining llerrs...
LL_NO_INLINE void errorDictionaryEntryAlreadyAdded();

template <class Index, class Entry>
class LLDictionary : public std::map<Index, Entry*>
{
public:
	typedef std::map<Index, Entry*> map_t;
	typedef typename map_t::iterator iterator_t;
	typedef typename map_t::const_iterator const_iterator_t;
	
	LL_INLINE LLDictionary()					{}

	virtual ~LLDictionary()
	{
		for (iterator_t iter = map_t::begin(); iter != map_t::end(); ++iter)
		{
			delete iter->second;
		}
	}

	LL_INLINE const Entry* lookup(Index index) const
	{
		const_iterator_t iter = map_t::find(index);
		return iter != map_t::end() ? iter->second : NULL;
	}

	const Index lookup(const std::string& name) const 
	{
		for (const_iterator_t iter = map_t::begin(), end = map_t::end();
			 iter != end; ++iter)
		{
			const Entry* entry = iter->second;
			if (entry && entry->mName == name)
			{
				return iter->first;
			}
		}
		return notFound();
	}

protected:
	// g++ v10.n (with n <= 2, at least) chokes on the LL_INLINE (which
	// translates into a force inline attribute for release builds), when
	// asked to compile with LTO. This is quite obviously a gcc bug... HB
#if defined(GCC_VERSION) && GCC_VERSION >= 100000
	virtual Index notFound() const				{ return Index(-1); }
#else
	LL_INLINE virtual Index notFound() const	{ return Index(-1); }
#endif

	LL_INLINE void addEntry(Index index, Entry* entry)
	{
		if (lookup(index))
		{
			errorDictionaryEntryAlreadyAdded();
		}
		(*this)[index] = entry;
	}
};
