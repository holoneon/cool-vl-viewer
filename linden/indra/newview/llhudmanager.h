/**
 * @file llhudmanager.h
 * @brief LLHUDManager class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

// Responsible for managing all HUD elements.

#include <vector>

#include "llhudobject.h"

class LLHUDEffect;
class LLMessageSystem;

// Purely static class
class LLHUDManager
{
	LLHUDManager() = delete;
	~LLHUDManager() = delete;

protected:
	LOG_CLASS(LLHUDManager);

public:
	static LLHUDEffect* createEffect(U8 type, bool send_to_sim = true,
									 bool originated_here = true);

	static void updateEffects();
	static void sendEffects();
	static void cleanupEffects();

	static void cleanupClass();

	static void processViewerEffect(LLMessageSystem* mesgsys, void**);

protected:
	typedef std::vector<LLPointer<LLHUDEffect> > effects_list_t;
	static effects_list_t sHUDEffects;
};
