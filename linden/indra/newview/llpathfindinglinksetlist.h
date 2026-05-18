/** 
 * @file llpathfindinglinksetlist.h
 * @brief Header file for llpathfindinglinksetlist
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

#include "llpathfindinglinkset.h"
#include "llpathfindingobjectlist.h"

class LLSD;

class LLPathfindingLinksetList : public LLPathfindingObjectList
{
protected:
	LOG_CLASS(LLPathfindingLinksetList);

public:
	LLPathfindingLinksetList();
	LLPathfindingLinksetList(const LLSD& data);

	LL_INLINE LLPathfindingLinksetList* asLinksetList() override
	{
		return this;
	}

	LL_INLINE const LLPathfindingLinksetList* asLinksetList() const override
	{
		return this;
	}

	typedef LLPathfindingLinkset::ELinksetUse EUsage;

	LLSD encodeObjectFields(EUsage use, S32 a, S32 b, S32 c, S32 d) const;
	LLSD encodeTerrainFields(EUsage use, S32 a, S32 b, S32 c, S32 d) const;

	bool showUnmodifiablePhantomWarning(EUsage use) const;
	bool showPhantomToggleWarning(EUsage use) const;
	bool showCannotBeVolumeWarning(EUsage use) const;

	void determinePossibleStates(bool& walkable, bool& static_obstacle,
								 bool& dynamic_obstacle, bool& material_volume,
								 bool& exclusion_volume,
								 bool& dynamic_phantom) const;

private:
	void parseLinksetListData(const LLSD& data);
};
