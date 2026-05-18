/** 
 * @file llpathfindinglinksetlist.cpp
 * @brief Implementation of llpathfindinglinksetlist
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

#include "llpathfindinglinksetlist.h"

#include "llsd.h"

LLPathfindingLinksetList::LLPathfindingLinksetList()
:	LLPathfindingObjectList()
{
}

LLPathfindingLinksetList::LLPathfindingLinksetList(const LLSD& data)
:	LLPathfindingObjectList()
{
	parseLinksetListData(data);
}

LLSD LLPathfindingLinksetList::encodeObjectFields(EUsage use, S32 a, S32 b,
												  S32 c, S32 d) const
{
	LLSD list_data;

	for (const_iterator iter = begin(); iter != end(); ++iter)
	{
		const LLPathfindingObject::ptr_t objectp = iter->second;
		if (!objectp) continue;

		const LLPathfindingLinkset* linksetp = objectp.get()->asLinkset();
		if (linksetp && !linksetp->isTerrain())
		{
			LLSD data = linksetp->encodeAlteredFields(use, a, b, c, d);
			if (!data.isUndefined())
			{
				list_data[iter->first.asString()] = data;
			}
		}
	}

	return list_data;
}

LLSD LLPathfindingLinksetList::encodeTerrainFields(EUsage use, S32 a, S32 b,
												   S32 c, S32 d) const
{
	for (const_iterator iter = begin(); iter != end(); ++iter)
	{
		const LLPathfindingObject::ptr_t objectp = iter->second;
		if (!objectp) continue;

		const LLPathfindingLinkset* linksetp = objectp.get()->asLinkset();
		if (linksetp && linksetp->isTerrain())
		{
			return linksetp->encodeAlteredFields(use, a, b, c, d);
		}
	}
	return LLSD();
}

bool LLPathfindingLinksetList::showUnmodifiablePhantomWarning(EUsage use) const
{
	for (const_iterator iter = begin(); iter != end(); ++iter)
	{
		const LLPathfindingObject::ptr_t objectp = iter->second;
		if (!objectp) continue;

		const LLPathfindingLinkset* linksetp = objectp.get()->asLinkset();
		if (linksetp && linksetp->showUnmodifiablePhantomWarning(use))
		{
			return true;
		}
	}
	return false;
}

bool LLPathfindingLinksetList::showPhantomToggleWarning(EUsage use) const
{
	for (const_iterator iter = begin(); iter != end(); ++iter)
	{
		const LLPathfindingObject::ptr_t objectp = iter->second;
		if (!objectp) continue;

		const LLPathfindingLinkset* linksetp = objectp.get()->asLinkset();
		if (linksetp && linksetp->showPhantomToggleWarning(use))
		{
			return true;
		}
	}
	return false;
}

bool LLPathfindingLinksetList::showCannotBeVolumeWarning(EUsage use) const
{
	for (const_iterator iter = begin(); iter != end(); ++iter)
	{
		const LLPathfindingObject::ptr_t objectp = iter->second;
		if (!objectp) continue;

		const LLPathfindingLinkset* linksetp = objectp.get()->asLinkset();
		if (linksetp && linksetp->showCannotBeVolumeWarning(use))
		{
			return true;
		}
	}
	return false;
}

void LLPathfindingLinksetList::determinePossibleStates(bool& walkable,
													   bool& static_obstacle,
													   bool& dynamic_obstacle,
													   bool& material_volume,
													   bool& exclusion_volume,
													   bool& dynamic_phantom) const
{
	walkable = static_obstacle = dynamic_obstacle = material_volume =
			   exclusion_volume = dynamic_phantom = false;

	for (const_iterator iter = begin();
		 !(walkable && static_obstacle && dynamic_obstacle &&
		   material_volume && exclusion_volume && dynamic_phantom) &&
		 iter != end();
		 ++iter)
	{
		const LLPathfindingObject::ptr_t objectp = iter->second;
		if (!objectp) continue;

		const LLPathfindingLinkset* linksetp = objectp.get()->asLinkset();
		if (!linksetp) continue;

		if (linksetp->isTerrain())
		{
			walkable = true;
		}
		else if (linksetp->isModifiable())
		{
			walkable = true;
			static_obstacle = true;
			dynamic_obstacle = true;
			dynamic_phantom = true;
			if (linksetp->canBeVolume())
			{
				material_volume = true;
				exclusion_volume = true;
			}
		}
		else if (linksetp->isPhantom())
		{
			dynamic_phantom = true;
			if (linksetp->canBeVolume())
			{
				material_volume = true;
				exclusion_volume = true;
			}
		}
		else
		{
			walkable = true;
			static_obstacle = true;
			dynamic_obstacle = true;
		}
	}
}

void LLPathfindingLinksetList::parseLinksetListData(const LLSD& data)
{
	LLPathfindingObject::map_t& obj_map = getObjectMap();

	std::string id_str;
	LLUUID id;
	for (LLSD::map_const_iterator iter = data.beginMap(), end = data.endMap();
		 iter != end; ++iter)
	{
		id_str = iter->first;
		const LLSD& data = iter->second;
		if (!data.size())
		{
			llwarns << "Empty data for path finding linkset Id: " << id_str
					<< llendl;
			continue;
		}
		if (LLUUID::validate(id_str))
		{
			LLUUID id = LLUUID(id_str);
			obj_map.emplace(id,
							std::make_shared<LLPathfindingLinkset>(id, data));
		}
		else
		{
			llwarns << "Invalid path finding linkset Id: " << id_str << llendl;
		}
	}
}
