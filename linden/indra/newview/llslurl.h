/**
 * @file llslurl.h
 * @brief Handles "SLURL fragments" like Ahern/123/45 for
 * startup processing, login screen, prefs, etc.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
 * Copyright (c) 2010-2026, Henri Beauchamp.
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

#include "llerror.h"
#include "llstring.h"

class LLAvatarName;
class LLUUID;
class LLUUID;
class LLTextEditor;
class LLVector3d;

class LLSLURL
{
protected:
	LOG_CLASS(LLSLURL);

public:
	// If you modify this enumeration, update typeName as well
	enum eType {
		INVALID,
		LOCATION,
		HOME_LOCATION,
		LAST_LOCATION,
		APP,
		HELP,
		NUM_SLURL_TYPES // must be last
	};

	LL_INLINE LLSLURL()
	:	mType(INVALID)
	{
	}

	LLSLURL(const std::string& slurl);
	LLSLURL(const std::string& grid, const std::string& region);
	LLSLURL(const std::string& region, const LLVector3& position);
	LLSLURL(const std::string& grid, const std::string& region,
			const LLVector3& position);
	LLSLURL(const std::string& grid, const std::string& region,
			const LLVector3d& global_position);
	LLSLURL(const std::string& region, const LLVector3d& global_position);
	LLSLURL(const std::string& command, const LLUUID& id,
			const std::string& verb);

	LL_INLINE eType getType() const				{ return mType; }

	std::string getSLURLString() const;
	std::string getLocationString() const;

	LL_INLINE std::string getGrid() const		{ return mGrid; }
	LL_INLINE std::string getRegion() const		{ return mRegion; }
	LL_INLINE LLVector3 getPosition() const		{ return mPosition; }
	LL_INLINE std::string getAppCmd() const		{ return mAppCmd; }
	LL_INLINE std::string getAppQuery() const	{ return mAppQuery; }
	LL_INLINE LLSD getAppQueryMap() const		{ return mAppQueryMap; }
	LL_INLINE LLSD getAppPath() const			{ return mAppPath; }

	LL_INLINE bool isValid() const				{ return mType != INVALID; }
	LL_INLINE bool isSpatial() const			{ return mType != INVALID && mType <= LAST_LOCATION; }

	bool operator==(const LLSLURL& rhs);
	LL_INLINE bool operator!=(const LLSLURL& rhs)
	{
		return !(*this == rhs);
	}

    std::string asString() const;

	// Searches for SLURLs that can be translated into avatar, group, object or
	// experience names in either the text editor full text, or in 'new_text'
	// when the latter is not empty, and register the editor for pending SLURLs
	// translations, with special treatment for avatar @mentions when true is
	// passed in 'with_mentions'. The 'echo_to_console' boolean is reserved to
	// the text editor corresponding to the main chat floater. Returns true if
	// SLURLs where indeed found, or false otherwise. HB
	static bool findSLURLs(LLTextEditor* editorp,
						   const std::string& new_text = LLStringUtil::null,
						   bool with_mentions = false,
						   bool echo_to_console = false);
	// Launch SLURLs resolving, by querying for avatar/group/experience Ids.
	static void resolveSLURLs();

private:
	// Get a human-readable version of the type for logging
	static std::string getTypeString(eType type);

	static void avatarNameCallback(const LLUUID& id,
								   const LLAvatarName& avatar_name);
	static void cacheNameCallback(const LLUUID& id, const std::string& name,
								  bool is_group);
	static void experienceNameCallback(const LLSD& experience_details);

public:
	static const char*	SLURL_SECONDLIFE_SCHEME;
	static const char*	SLURL_HOP_SCHEME;
	static const char*	SLURL_X_GRID_LOCATION_INFO_SCHEME;
	static const char*	SLURL_X_GRID_INFO_SCHEME;
	static const char*	SLURL_HTTPS_SCHEME;
	static const char*	SLURL_HTTP_SCHEME;
	static const char*	SLURL_SECONDLIFE_PATH;
	static const char*	SLURL_COM;
	static const char*	WWW_SLURL_COM;
	static const char*	MAPS_SECONDLIFE_COM;
	static const char*	SIM_LOCATION_HOME;
	static const char*	SIM_LOCATION_LAST;
	static const char*	SLURL_APP_PATH;
	static const char*	SLURL_REGION_PATH;

private:
	eType				mType;

	// Used for Apps and Help
	std::string			mAppCmd;
	LLSD				mAppPath;
	LLSD				mAppQueryMap;
	std::string			mAppQuery;

	std::string			mGrid;		// Reference to grid manager grid
	std::string			mRegion;
	LLVector3			mPosition;

	static uuid_list_t	sAvatarUUIDs;
	static uuid_list_t	sGroupUUIDs;
	static uuid_list_t	sExperienceUUIDs;
	static uuid_list_t	sObjectsUUIDs;

	typedef std::map<std::string, LLUUID, std::less<> > slurls_map_t;
	static slurls_map_t	sPendingSLURLs;

	static const std::string typeName[NUM_SLURL_TYPES];
};
