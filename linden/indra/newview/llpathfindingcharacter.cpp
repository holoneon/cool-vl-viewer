/**
 * @file llpathfindingcharacter.cpp
 * @brief Definition of a pathfinding character that contains various properties required for havok pathfinding.
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

#include "llpathfindingcharacter.h"

#include "llpathfindingobject.h"
#include "llsd.h"

#define CHARACTER_CPU_TIME_FIELD   "cpu_time"
#define CHARACTER_HORIZONTAL_FIELD "horizontal"
#define CHARACTER_LENGTH_FIELD     "length"
#define CHARACTER_RADIUS_FIELD     "radius"

LLPathfindingCharacter::LLPathfindingCharacter(const LLUUID& id,
											   const LLSD& char_data)
:	LLPathfindingObject(id, char_data),
	mCPUTime(0U),
	mIsHorizontal(false),
	mLength(0.f),
	mRadius(0.f)
{
	parseCharacterData(char_data);
}

LLPathfindingCharacter::LLPathfindingCharacter(const LLPathfindingCharacter& obj)
:	LLPathfindingObject(obj),
	mCPUTime(obj.mCPUTime),
	mIsHorizontal(obj.mIsHorizontal),
	mLength(obj.mLength),
	mRadius(obj.mRadius)
{
}

LLPathfindingCharacter& LLPathfindingCharacter::operator=(const LLPathfindingCharacter& obj)
{
	dynamic_cast<LLPathfindingObject&>(*this) = obj;

	mCPUTime = obj.mCPUTime;
	mIsHorizontal = obj.mIsHorizontal;
	mLength = obj.mLength;
	mRadius = obj.mRadius;

	return *this;
}

void LLPathfindingCharacter::parseCharacterData(const LLSD& char_data)
{
	if (char_data.has(CHARACTER_CPU_TIME_FIELD) &&
		char_data.get(CHARACTER_CPU_TIME_FIELD).isReal())
	{
		mCPUTime = char_data.get(CHARACTER_CPU_TIME_FIELD).asReal();
	}
	else
	{
		llwarns << "Malformed pathfinding character data: no CPU time"
				<< llendl;
	}

	if (char_data.has(CHARACTER_HORIZONTAL_FIELD) &&
		char_data.get(CHARACTER_HORIZONTAL_FIELD).isBoolean())
	{
		mIsHorizontal = char_data.get(CHARACTER_HORIZONTAL_FIELD).asBoolean();
	}
	else
	{
		llwarns << "Malformed pathfinding character data: no horizontal flag"
				<< llendl;
	}

	if (char_data.has(CHARACTER_LENGTH_FIELD) &&
		char_data.get(CHARACTER_LENGTH_FIELD).isReal())
	{
		mLength = char_data.get(CHARACTER_LENGTH_FIELD).asReal();
	}
	else
	{
		llwarns << "Malformed pathfinding character data: no length"
				<< llendl;
	}

	if (char_data.has(CHARACTER_RADIUS_FIELD) &&
		char_data.get(CHARACTER_RADIUS_FIELD).isReal())
	{
		mRadius = char_data.get(CHARACTER_RADIUS_FIELD).asReal();
	}
	else
	{
		llwarns << "Malformed pathfinding character data: no radius"
				<< llendl;
	}
}
