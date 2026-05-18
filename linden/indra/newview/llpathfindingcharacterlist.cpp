/**
 * @file llpathfindingcharacterlist.cpp
 * @brief Implementation of llpathfindingcharacterlist
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

#include "llviewerprecompiledheaders.h"

#include "llpathfindingcharacterlist.h"

#include "llpathfindingcharacter.h"
#include "llpathfindingobjectlist.h"
#include "llsd.h"

LLPathfindingCharacterList::LLPathfindingCharacterList()
:	LLPathfindingObjectList()
{
}

LLPathfindingCharacterList::LLPathfindingCharacterList(const LLSD& char_data)
:	LLPathfindingObjectList()
{
	parseCharacterListData(char_data);
}

void LLPathfindingCharacterList::parseCharacterListData(const LLSD& char_data)
{
	LLPathfindingObject::map_t& obj_map = getObjectMap();

	std::string id_str;
	LLUUID id;
	for (LLSD::map_const_iterator iter = char_data.beginMap(),
								  end = char_data.endMap();
		 iter != end; ++iter)
	{
		id_str = iter->first;
		const LLSD& data = iter->second;
		if (!data.size())
		{
			llwarns << "Empty data for path finding character Id: " << id_str
					<< llendl;
			continue;
		}
		if (LLUUID::validate(id_str))
		{
			LLUUID id = LLUUID(id_str);
			obj_map.emplace(id,
							std::make_shared<LLPathfindingCharacter>(id,
																	 data));
		}
		else
		{
			llwarns << "Invalid path finding character Id: " << id_str
					<< llendl;
		}
	}
}
