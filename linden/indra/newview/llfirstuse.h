/**
 * @file llfirstuse.h
 * @brief Methods that spawn "first-use" dialogs.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include <string>
#include <vector>

#include "stdtypes.h"

class LLFirstUse
{
public:
	// Sets all controls back to show the dialogs.
	static void disableFirstUse();
	static void resetFirstUse();

	// These methods are called each time the appropriate action is taken. The
	// functions themselves handle only showing the dialog the first time, or
	// subsequent times if the user wishes.
	static void useBalanceIncrease(S32 delta);
	static void useBalanceDecrease(S32 delta);
	static void useSit();
	static void useMap();
	static void useBuild();
	static void useLeftClickNoHit();
	static void useOverrideKeys();
	static void useAppearance();
	static void useInventory();
	static void useSandbox();
	static void useSculptedPrim();
	static void useMedia();
	static void useJellyDoll();
	static void newSkinOverridesFolder();

private:
	// Adds all the config variables to sConfigVariables for use by
	// disableFirstUse() and resetFirstUse()
	static void populate();

	static void simpleNotification(const std::string& name);

private:
	static std::vector<std::string> sConfigVariables;
};
