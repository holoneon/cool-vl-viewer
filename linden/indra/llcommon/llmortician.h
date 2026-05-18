/**
 * @file llmortician.h
 * @brief Base class for delayed deletions.
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

#pragma once

#include <list>
#include <string>

#include "llpreprocessor.h"

class LLMortician
{
public:
	LL_INLINE LLMortician()
	:	mIsDead(false)
	{
	}

	virtual ~LLMortician();

	void die();
	LL_INLINE bool isDead()						{ return mIsDead; }

	static void updateClass();

	LL_INLINE static void setZealous(bool b)	{ sDestroyImmediate = b; }

private:
	bool							mIsDead;

	static std::list<LLMortician*>	sGraveyard;
	static bool						sDestroyImmediate;
};
