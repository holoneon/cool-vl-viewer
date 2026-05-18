/**
 * @file hbfloatergrouptitles.h
 * @brief HBFloaterGroupTitles class definition
 *
 * This class implements a floater where all available group titles are
 * listed, allowing the user to activate any via simple double-click.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 *
 * Copyright (c) 2010, Henri Beauchamp.
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

#include "llevent.h"
#include "llfloater.h"
#include "llgroupmgr.h"

class HBFloaterGroupTitles;
class LLScrollListCtrl;

class HBFloaterGroupTitlesObserver final : public LLGroupMgrObserver
{
public:
	HBFloaterGroupTitlesObserver(HBFloaterGroupTitles* instance,
								 const LLUUID& group_id);
	~HBFloaterGroupTitlesObserver() override;

protected:
	void changed(LLGroupChange gc) override;

private:
	HBFloaterGroupTitles* mFloaterInstance;
};

class HBFloaterGroupTitles final
:	public LLFloater,
	public LLFloaterSingleton<HBFloaterGroupTitles>,
	public LLOldEvents::LLSimpleListener
{
	friend class LLUISingleton<HBFloaterGroupTitles,
							   VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBFloaterGroupTitles);

public:
	~HBFloaterGroupTitles() override;

	void setDirty()								{ mIsDirty = true; }

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBFloaterGroupTitles(const LLSD&);

	bool postBuild() override;
	void draw() override;

	// LLSimpleListener interface
	bool handleEvent(LLPointer<LLOldEvents::LLEvent> event,
					 const LLSD& userdata) override;

	static void onActivate(void* data);
	static void onRefreshButtonPressed(void* data);
	static void onCloseButtonPressed(void* data);

	enum TITLES_COLUMN_ORDER
	{
		LIST_TITLE = 0,
		LIST_GROUP_NAME,
		LIST_GROUP_ID
	};

private:
	typedef fast_hmap<LLUUID, HBFloaterGroupTitlesObserver*> observers_map_t;
	observers_map_t		mObservers;
	LLScrollListCtrl*	mTitlesList;
	bool				mIsDirty;
};
