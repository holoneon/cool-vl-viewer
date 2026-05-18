/**
 * @file llfirstuse.cpp
 * @brief Methods that spawn "first-use" dialogs
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

#include "llviewerprecompiledheaders.h"

#include "llfirstuse.h"

#include "llnotifications.h"
#include "lltrans.h"

#include "llagent.h"			// For gAgent.inPrelude()
#include "lldir.h"
#include "lltracker.h"
#include "llviewercontrol.h"

// First clean starts at 3 AM
constexpr S32 SANDBOX_FIRST_CLEAN_HOUR = 3;
// Clean every <n> hours
constexpr S32 SANDBOX_CLEAN_FREQ = 12;

//static
std::vector<std::string> LLFirstUse::sConfigVariables;

//static
void LLFirstUse::populate()
{
	sConfigVariables.reserve(16);
	sConfigVariables.emplace_back("FirstAppearance");
	sConfigVariables.emplace_back("FirstBalanceDecrease");
	sConfigVariables.emplace_back("FirstBalanceIncrease");
	sConfigVariables.emplace_back("FirstBuild");
	sConfigVariables.emplace_back("FirstInventory");
	sConfigVariables.emplace_back("FirstJellyDoll");
	sConfigVariables.emplace_back("FirstLeftClickNoHit");
	sConfigVariables.emplace_back("FirstMap");
	sConfigVariables.emplace_back("FirstMedia");
	sConfigVariables.emplace_back("FirstOverrideKeys");
	sConfigVariables.emplace_back("FirstSandbox");
	sConfigVariables.emplace_back("FirstSculptedPrim");
	sConfigVariables.emplace_back("FirstSit");
	sConfigVariables.emplace_back("FirstStreamingMusic");
	sConfigVariables.emplace_back("FirstStreamingVideo");
	sConfigVariables.emplace_back("NewSkinOverridesFolder");
}

//static
void LLFirstUse::disableFirstUse()
{
	if (sConfigVariables.empty())
	{
		populate();
	}

	// Set all first-use warnings to disabled
	for (S32 i = 0, count = sConfigVariables.size(); i < count; ++i)
	{
		gSavedSettings.setWarning(sConfigVariables[i], false);
	}
}

//static
void LLFirstUse::resetFirstUse()
{
	if (sConfigVariables.empty())
	{
		populate();
	}

	// Set all first-use warnings to enabled
	for (S32 i = 0, count = sConfigVariables.size(); i < count; ++i)
	{
		gSavedSettings.setWarning(sConfigVariables[i], true);
	}
}

// Called whenever the viewer detects that your balance went up
void LLFirstUse::useBalanceIncrease(S32 delta)
{
	if (gSavedSettings.getWarning("FirstBalanceIncrease"))
	{
		gSavedSettings.setWarning("FirstBalanceIncrease", false);
		LLSD args;
		args["AMOUNT"] = llformat("%d",delta);
		gNotifications.add("FirstBalanceIncrease", args);
	}
}

// Called whenever the viewer detects your balance went down
void LLFirstUse::useBalanceDecrease(S32 delta)
{
	if (gSavedSettings.getWarning("FirstBalanceDecrease"))
	{
		gSavedSettings.setWarning("FirstBalanceDecrease", false);
		LLSD args;
		args["AMOUNT"] = llformat("%d",-delta);
		gNotifications.add("FirstBalanceDecrease", args);
	}
}

//static
void LLFirstUse::simpleNotification(const std::string& name)
{
	if (gSavedSettings.getWarning(name))
	{
		gSavedSettings.setWarning(name, false);
		gNotifications.add(name);
	}
}

//static
void LLFirstUse::useSit()
{
	// SL's orientation island uses sitting to teach vehicle driving so just
	// never show this messagein prelude
	if (!gAgent.inPrelude())
	{
		simpleNotification("FirstSit");
	}
}

//static
void LLFirstUse::useMap()
{
	simpleNotification("FirstMap");
}

//static
void LLFirstUse::useBuild()
{
	simpleNotification("FirstBuild");
}

//static
void LLFirstUse::useLeftClickNoHit()
{
	simpleNotification("FirstLeftClickNoHit");
}

//static
void LLFirstUse::useOverrideKeys()
{
	// SL's orientation island uses key overrides to teach vehicle driving
	// so don't show this message until you get off OI. JC
	if (!gAgent.inPrelude())
	{
		simpleNotification("FirstOverrideKeys");
	}
}

//static
void LLFirstUse::useAppearance()
{
	simpleNotification("FirstAppearance");
}

//static
void LLFirstUse::useInventory()
{
	simpleNotification("FirstInventory");
}

//static
void LLFirstUse::useSandbox()
{
	if (gSavedSettings.getWarning("FirstSandbox"))
	{
		gSavedSettings.setWarning("FirstSandbox", false);
		LLSD args;
		args["HOURS"] = llformat("%d", SANDBOX_CLEAN_FREQ);
		args["TIME"] = llformat("%d", SANDBOX_FIRST_CLEAN_HOUR);
		gNotifications.add("FirstSandbox", args);
	}
}

//static
void LLFirstUse::useSculptedPrim()
{
	simpleNotification("FirstSculptedPrim");
}

//static
void LLFirstUse::useMedia()
{
	simpleNotification("FirstMedia");
}

//static
void LLFirstUse::useJellyDoll()
{
	// We cache the setting in a static variable, because this method may get
	// called a lot...
	static bool warn = gSavedSettings.getWarning("FirstJellyDoll");
	if (warn)
	{
		gSavedSettings.setWarning("FirstJellyDoll", false);
		gNotifications.add("FirstJellyDoll");
		warn = false;
	}
}

//static
void LLFirstUse::newSkinOverridesFolder()
{
	if (gSavedSettings.getWarning("NewSkinOverridesFolder"))
	{
		gSavedSettings.setWarning("NewSkinOverridesFolder", false);
		LLSD args;
		args["OLD_FOLDER"] = gDirUtil.getOSUserAppDir() +
							 LL_DIR_DELIM_STR "skins";
		args["NEW_FOLDER"] = gDirUtil.getUserDefaultSkinDir();
		gNotifications.add("NewSkinOverridesFolder", args);
	}
}
