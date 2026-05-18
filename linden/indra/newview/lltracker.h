/**
 * @file lltracker.h
 * @brief Container for objects user is tracking.
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

// A singleton class for tracking stuff.
//
// TODO -- LLAvatarTracker functionality should probably be moved
// to the LLTracker class.

#pragma once

#include "llpointer.h"
#include "llstring.h"
#include "lluuid.h"
#include "llvector3d.h"

class LLHUDText;

class LLTracker
{
protected:
	LOG_CLASS(LLTracker);

public:
	enum ETrackingStatus
	{
		TRACKING_NOTHING = 0,
		TRACKING_AVATAR = 1,
		TRACKING_LANDMARK = 2,
		TRACKING_LOCATION = 3,
	};

	enum ETrackingLocationType
	{
		LOCATION_NOTHING,
		LOCATION_EVENT,
		LOCATION_ITEM,
	};

	LLTracker();
	~LLTracker();

	// These are static so that they can be used a callbacks
	LL_INLINE ETrackingStatus getTrackingStatus()				{ return mTrackingStatus; }
	LL_INLINE ETrackingLocationType getTrackedLocationType()	{ return mTrackingLocationType; }
	LL_INLINE bool isTracking()									{ return mTrackingStatus != TRACKING_NOTHING; }
	LL_INLINE void clearFocus()									{ mTrackingStatus = TRACKING_NOTHING; }

	LL_INLINE const LLUUID& getTrackedLandmarkAssetID()			{ return mTrackedLandmarkAssetID; }
	LL_INLINE const LLUUID& getTrackedLandmarkItemID()			{ return mTrackedLandmarkItemID; }

	void trackAvatar(const LLUUID& avatar_id, const std::string& name);
	void trackLandmark(const LLUUID& landmark_asset_id,
					   const LLUUID& landmark_item_id,
					   const std::string& name);
	void trackLocation(const LLVector3d& pos, const std::string& full_name,
					   const std::string& tooltip,
					   ETrackingLocationType location_type = LOCATION_NOTHING);
	void stopTracking(bool clear_ui = false);

	// Returns global pos of tracked thing
	LLVector3d getTrackedPositionGlobal();

	bool hasLandmarkPosition();
	LL_INLINE const std::string& getTrackedLocationName()		{ return mTrackedLocationName; }

	void drawHUDArrow();

	// Draw in-world 3D tracking stuff
	void render3D();

	bool handleMouseDown(S32 x, S32 y);

	LL_INLINE const std::string& getLabel()						{ return mLabel; }
	LL_INLINE const std::string& getToolTip()					{ return mToolTip; }

protected:
	static void renderBeacon(const LLVector3d& pos_global,
							 const LLColor4& color, LLHUDText* hud_textp,
							 const std::string& label);

	void stopTrackingAvatar(bool clear_ui = false);
	void stopTrackingLocation(bool clear_ui = false);
	void stopTrackingLandmark(bool clear_ui = false);

	void drawMarker(const LLVector3d& pos_global, const LLColor4& color);
	void setLandmarkVisited();
	void cacheLandmarkPosition();
	void purgeBeaconText();

protected:
	ETrackingStatus 		mTrackingStatus;
	ETrackingLocationType	mTrackingLocationType;

	LLPointer<LLHUDText>	mBeaconText;

	S32						mHUDArrowCenterX;
	S32						mHUDArrowCenterY;

	LLVector3d				mTrackedPositionGlobal;

	LLUUID					mTrackedLandmarkAssetID;
	LLUUID					mTrackedLandmarkItemID;

	std::string				mLabel;
	std::string				mToolTip;
	std::string				mTrackedLandmarkName;
	std::string				mTrackedLocationName;

	uuid_vec_t				mLandmarkAssetIDList;
	uuid_vec_t				mLandmarkItemIDList;

	bool					mIsTrackingLocation;
	bool					mHasReachedLandmark;
	bool 					mHasLandmarkPosition;
	bool					mLandmarkHasBeenVisited;
};

extern LLTracker gTracker;
