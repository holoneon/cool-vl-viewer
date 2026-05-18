/**
 * @file hbfloaterteleporthistory.h
 * @author Henri Beauchamp
 * @brief HBFloaterTeleportHistory class definition
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * 
 * Copyright (c) 2018, Henri Beauchamp
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

#include "linden_common.h"

#include "boost/signals2.hpp"

#include "llfloater.h"

class LLButton;
class LLFlyoutButton;
class LLScrollListCtrl;
class LLScrollListItem;
class LLSearchEditor;
class LLTabContainer;

class HBFloaterTeleportHistory final : public LLFloater
{
protected:
	LOG_CLASS(HBFloaterTeleportHistory);

public:
	HBFloaterTeleportHistory();
	~HBFloaterTeleportHistory() override;

	// Toggles the floater on and off (i.e. shown and hidden)
	void toggle();

	// Loads the saved teleport history
	void loadEntries();
	// Adds the pending teleport destination
	void addPendingEntry(const std::string& region_name, LLVector3 pos);
	// Adds a source teleport SLURL to the visited places.
	void addSourceEntry(const std::string& source_slurl,
						const std::string& parcel_name);

private:
	// Reimplemented to check for selection changes in the TP history list
	// scrolllist
	void onFocusReceived() override;

	// Reimplemented to allow initial resize
	void onOpen() override;
	// Reimplemented to make the menu toggle work
	void onClose(bool app_quitting) override;

	// Reimplemented to prevent this floater from closing while the viewer is
	// shutting down
	bool canClose() override;

	bool postBuild() override;
	void draw() override;

	enum HISTORY_COLUMN_ORDER
	{
		LIST_TYPE,
		LIST_PARCEL,
		LIST_REGION,
		LIST_POSITION,
		LIST_TIMESTAMP
	};

	enum FAVORITES_COLUMN_ORDER
	{
		FAV_PARCEL,
		FAV_REGION,
		FAV_POSITION,
		FAV_VISITS
	};

	enum RESULTS_COLUMN_ORDER
	{
		RES_PARCEL,
		RES_REGION,
		RES_POSITION,
		RES_TIMESTAMP
	};

	// Returns the history file name. If fallback is 'true' (useful for reads),
	// also use the old history file name if the new one does not correspond to
	// and existing file.
	std::string getHistoryFileName(bool fallback = false) const;

	void addPlacesListComment();
	void removePlacesListComment();

	void populateLists(const LLSD& file_data);
	void updateSearchResults();

	void saveList();

	// Adds the destination to the list of visited places
	void addEntry(std::string parcel_name, bool departure = false);

	// Enables/disables or shows/hides the buttons depending on selected tab
	// and selected list entry.
	void setButtonsStatus();

	bool getSelectedLocation(std::string& region, LLVector3& pos);

	// Teleport callbacks
	static void onTeleportArriving();
	static void onTeleportFinished(const LLVector3d& pos, bool local);
	static void onTeleportFailed();

	// UI callbacks
	static void onTabChanged(void* data, bool);
	static void onPlacesSelected(LLUICtrl* ctrl, void* data);
	static void onTeleport(void* data);
	static void onShowOnMap(void* data);
	static void onCopySLURL(void* data);
	static void onRefresh(void* data);
	static void onButtonClose(void* data);
	static void onRemove(LLUICtrl* ctrl, void* data);
	static void onSearchEdit(const std::string& search_string, void* data);

private:
	LLTabContainer*				mTabContainer;

	LLScrollListCtrl*			mPlacesList;
	LLScrollListCtrl*			mFavoritesList;
	LLScrollListCtrl*			mResultsList;

	LLScrollListItem*			mPlacesListComment;

	LLSearchEditor*				mSearchEditor;

	LLButton*					mTeleportBtn;
	LLButton*					mShowOnMapBtn;
	LLButton*					mCopySLURLBtn;
	LLButton*					mRefreshBtn;

	LLFlyoutButton*				mRemoveFlyoutBtn;

	S32							mCount;

	std::string					mNumEntriesStr;
	std::string					mNoEntryStr;
	std::string					mPendingRegionName;
	std::string					mPendingPosition;
	std::string					mPendingTimeString;
	std::string					mSearchString;

	LLSD						mTPlist;

	boost::signals2::connection	mTeleportArrivingConnection;
	boost::signals2::connection	mTeleportFinishConnection;
	boost::signals2::connection	mTeleportFailedConnection;

	bool						mFirstOpen;
	bool						mCanTeleport;
};

// Global
extern HBFloaterTeleportHistory* gFloaterTeleportHistoryp;
