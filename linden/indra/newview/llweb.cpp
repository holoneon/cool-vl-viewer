/**
 * @file llweb.cpp
 * @brief Functions dealing with web browsers
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llweb.h"

#include "llnotifications.h"
#include "llsys.h"						// For LLOSInfo
#include "lluri.h"
#include "llversionviewer.h"

#include "llagent.h"
#include "llfloatermediabrowser.h"
#include "llgridmanager.h"
#include "hbviewerautomation.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

using namespace std::placeholders;

//static
void LLWeb::initClass()
{
	LLAlertDialog::setURLLoader(&sAlertURLLoader);
}

//static
void LLWeb::loadURL(const std::string& url)
{
	loadURL(url, "");
}

//static
void LLWeb::loadURL(std::string url, const std::string& target)
{
	// 0 = loading forbidden, 1 = use web plugin, 2 = use external browser. HB
	U32 dispatch = 1;
	if (target != "_internal" &&
		(target == "_external" || gSavedSettings.getBool("UseExternalBrowser")))
	{
		dispatch = 2;
	}
	if (gAutomationp)
	{
		dispatch = gAutomationp->onURLDispatch(url, target, dispatch == 2);
		if (!dispatch)
		{
			// Dispatching this URL has been forbidden by the user's Lua
			// script. HB
			return;
		}
	}

	// Check for URLs pointing to the Cool VL Viewer web site, and use the
	// backup site if configured to do so. HB
	static const char* MAIN_SITE = "http://sldev.free.fr";
	static const char* BACKUP_SITE = "https://sldevel.pages-perso.free.fr";
	static const char* FORUM_SUBFOLDER = "/forum/";
	static LLCachedControl<U32> home_src(gSavedSettings,
										 "CoolVLViewerHomeSiteSource");
	if (home_src && url.find(MAIN_SITE) == 0 &&
		(home_src > 1 || url.find(FORUM_SUBFOLDER) == std::string::npos))
	{
		LLStringUtil::replaceString(url, MAIN_SITE, BACKUP_SITE);
	}

	if (dispatch == 2)
	{
		loadURLExternal(url);
	}
	else
	{
		LLFloaterMediaBrowser::showInstance(url);
	}
}

//static
void LLWeb::loadURLInternal(const std::string& url)
{
	loadURL(url, "_internal");
}

//static
void LLWeb::loadURLExternal(const std::string& url)
{
	loadURLExternal(url, true);
}

bool on_load_url_external_response(const LLSD& notification,
								   const LLSD& response, bool async)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		LLSD payload = notification["payload"];
		std::string url = payload["url"].asString();
		std::string escaped_url = LLWeb::escapeURL(url);
		if (gWindowp)
		{
			gWindowp->spawnWebBrowser(escaped_url, async);
		}
	}
	return false;
}

//static
void LLWeb::loadURLExternal(const std::string& url, bool async)
{
	LLSD payload;
	payload["url"] = url;
	gNotifications.add("WebLaunchExternalTarget", LLSD(), payload,
					   std::bind(on_load_url_external_response, _1, _2,
								 async));
}

//static
std::string LLWeb::escapeURL(const std::string& url)
{
	// The CURL curl_escape() function escapes colons, slashes and all
	// characters but A-Z and 0-9. Do a cheesy mini-escape.
	std::string escaped_url;
	S32 len = url.length();
	for (S32 i = 0; i < len; ++i)
	{
		char c = url[i];
		if (c == ' ')
		{
			escaped_url += "%20";
		}
		else if (c == '\\')
		{
			escaped_url += "%5C";
		}
		else
		{
			escaped_url += c;
		}
	}
	return escaped_url;
}

//static
std::string LLWeb::expandURLSubstitutions(const std::string& url,
										  const LLStringUtil::format_map_t& default_subs)
{
	LLStringUtil::format_map_t substitution = default_subs;
	substitution["[VERSION]"] = llformat("%d.%d.%d.%d", LL_VERSION_MAJOR,
										 LL_VERSION_MINOR, LL_VERSION_BRANCH,
										 LL_VERSION_RELEASE);
	substitution["[VERSION_MAJOR]"] = llformat("%d", LL_VERSION_MAJOR);
	substitution["[VERSION_MINOR]"] = llformat("%d", LL_VERSION_MINOR);
	substitution["[VERSION_PATCH]"] = llformat("%d", LL_VERSION_BRANCH);
	substitution["[VERSION_BUILD]"] = llformat("%d", LL_VERSION_RELEASE);
	substitution["[CHANNEL]"] =  gSavedSettings.getString("VersionChannelName");
	substitution["[GRID]"] = LLGridManager::getInstance()->getGridLabel();
	substitution["[OS]"] = LLOSInfo::getInstance()->getOSStringSimple();
	substitution["[SESSION_ID]"] = gAgentSessionID.asString();
	substitution["[FIRST_LOGIN]"] = llformat("%d", gAgent.isFirstLogin());

	// Work out the current language
	std::string lang = LLUI::getLanguage();
	if (lang == "en-us")
	{
		lang = "en";
	}
	substitution["[LANGUAGE]"] = lang;

	// Find the region ID and name
	LLUUID region_id;
	std::string region_name;
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		region_id = region->getRegionID();
		region_name = LLURI::escape(region->getName());
	}
	substitution["[REGION_ID]"] = region_id.asString();
	substitution["[REGION]"] = region_name;

	// Find the parcel local ID
	S32 parcel_id = 0;
	LLParcel* parcel = gViewerParcelMgr.getAgentParcel();
	if (parcel)
	{
		parcel_id = parcel->getLocalID();
	}
	substitution["[PARCEL_ID]"] = llformat("%d", parcel_id);

	if (!gIsInSecondLife)
	{
		substitution["SLURL_TYPE"] = "hop";
	}

	// Expand all of the substitution strings and escape the url
	std::string expanded_url = url;
	LLStringUtil::format(expanded_url, substitution);

	return escapeURL(expanded_url);
}

//virtual
void LLWeb::URLLoader::load(const std::string& url)
{
	loadURL(url);
}

//static
LLWeb::URLLoader LLWeb::sAlertURLLoader;
