/**
 * @file hbfloaterdebugtags.h
 * @brief The HBFloaterDebugTags class declaration
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Henri Beauchamp
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

#include "llfloater.h"

class LLScrollListCtrl;

class HBFloaterDebugTags final : public LLFloater,
								 public LLFloaterSingleton<HBFloaterDebugTags>
{
	friend class LLUISingleton<HBFloaterDebugTags,
							   VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBFloaterDebugTags);

public:
	LL_INLINE static bool hasActiveDebugTags()
	{
		return !sAddedTagsList.empty();
	}

	LL_INLINE static bool debugTagActive(const std::string& tag)
	{
		return sAddedTagsList.count(tag) != 0;
	}

	static void primeTagsFromLogControl();
	static void setTag(const std::string& tag, bool enable);

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBFloaterDebugTags(const LLSD&);

	bool postBuild() override;
	void draw() override;

	void refreshList();

	static void onSelectLine(LLUICtrl* ctrl, void* data);

private:
	LLScrollListCtrl*		mDebugTagsList;
	bool					mIsDirty;

	static strings_set_t	sDefaultTagsList;
	static strings_set_t	sAddedTagsList;
};
