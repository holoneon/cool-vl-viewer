/**
 * @file hbfloaterbump.h
 * @brief Floater listing bumps, pushes and hits, and allowing to take actions.
 * @author Henri Beauchamp
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 *
 * Copyright (c) 2020, Henri Beauchamp
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

#include "llfloater.h"

class LLButton;
class LLMeanCollisionData;
class LLScrollListCtrl;

typedef enum e_mean_collision_types
{
	MEAN_INVALID,
	MEAN_BUMP,
	MEAN_LLPUSHOBJECT,
	MEAN_SELECTED_OBJECT_COLLIDE,
	MEAN_SCRIPTED_OBJECT_COLLIDE,
	MEAN_PHYSICAL_OBJECT_COLLIDE,
	MEAN_EOF
} EMeanCollisionType;

class HBFloaterBump final : public LLFloater,
							public LLFloaterSingleton<HBFloaterBump>
{
	friend class LLUISingleton<HBFloaterBump, VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBFloaterBump);

public:
	void refresh() override;

	static void cleanup();

	static void addMeanCollision(const LLUUID& id, U32 time,
								 EMeanCollisionType type, F32 mag);

	static std::string getMeanCollisionsStats(const LLUUID& perpetrator_id);

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBFloaterBump(const LLSD&);

	bool postBuild() override;
	void draw() override;

	static void onButtonClear(void*);
	static void onButtonClose(void* data);
	static void onButtonFocus(void* data);
	static void onButtonProfile(void* data);
	static void onButtonReport(void* data);

	static void meanNameCallback(const LLUUID& id, const std::string& fullname,
								 bool);
private:
	LLScrollListCtrl*			mBumpsList;
	LLButton*					mClearButton;
	LLButton*					mFocusButton;
	LLButton*					mProfileButton;
	LLButton*					mReportButton;

	typedef std::list<LLMeanCollisionData> collisions_list_t;
	static collisions_list_t	sMeanCollisionsList;
	static bool					sListUpdated;
};
