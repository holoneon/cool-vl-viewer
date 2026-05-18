/**
 * @file llpathfindingcharacterlist.h
 * @brief Header file for llpathfindingcharacterlist
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

#include "llpathfindingobjectlist.h"

class LLSD;

class LLPathfindingCharacterList : public LLPathfindingObjectList
{
protected:
	LOG_CLASS(LLPathfindingCharacterList);

public:
	LLPathfindingCharacterList();
	LLPathfindingCharacterList(const LLSD& char_data);

	LL_INLINE LLPathfindingCharacterList* asCharacterList() override
	{
		return this;
	}

	LL_INLINE const LLPathfindingCharacterList* asCharacterList() const override
	{
		return this;
	}

private:
	void parseCharacterListData(const LLSD& char_data);
};
