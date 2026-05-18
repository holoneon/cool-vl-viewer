/**
 * @file llmortician.cpp
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 *
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llmortician.h"

std::list<LLMortician*> LLMortician::sGraveyard;

bool LLMortician::sDestroyImmediate = false;

//virtual
LLMortician::~LLMortician()
{
	sGraveyard.remove(this);
}

void LLMortician::die()
{
	// It is valid to call die() more than once on something that has not died
	// yet
	if (sDestroyImmediate)
	{
		// *NOTE: This is a hack to ensure destruction order on shutdown
		// (relative to non-mortician controlled classes).
		mIsDead = true;
		delete this;
		return;
	}
	if (!mIsDead)
	{
		mIsDead = true;
		sGraveyard.push_back(this);
	}
}

//static
void LLMortician::updateClass()
{
	while (!sGraveyard.empty())
	{
		LLMortician* dead = sGraveyard.front();
		delete dead;
	}
}
