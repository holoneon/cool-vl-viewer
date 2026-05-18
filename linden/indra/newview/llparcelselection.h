/**
 * @file llparcelselection.h
 * @brief Information about the currently selected parcel
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llrefcount.h"
#include "llsafehandle.h"

class LLParcel;

class LLParcelSelection : public LLRefCount
{
	friend class LLViewerParcelMgr;
	friend class LLSafeHandle<LLParcelSelection>;

protected:
	~LLParcelSelection() override = default;

public:
	LLParcelSelection(LLParcel* parcel);
	LLParcelSelection();

	// This can return NULL at any time, as parcel selection might have been
	// invalidated.
	LL_INLINE LLParcel* getParcel()				{ return mParcel; }

	// Return the number of grid units that are owned by you within the
	// selection (computed by server).
	LL_INLINE S32 getSelfCount() const			{ return mSelectedSelfCount; }

	// Returns area that will actually be claimed in meters squared.
	S32 getClaimableArea() const;

	LL_INLINE bool hasOthersSelected() const	{ return mSelectedOtherCount != 0; }

	// Does the selection have multiple land owners in it ?
	LL_INLINE bool getMultipleOwners() const	{ return mSelectedMultipleOwners; }

	// Is the entire parcel selected, or just a part ?
	LL_INLINE bool getWholeParcelSelected() const
	{
		return mWholeParcelSelected;
	}

private:
	LL_INLINE void setParcel(LLParcel* parcel)	{ mParcel = parcel; }

private:
	LLParcel*	mParcel;
	S32			mSelectedSelfCount;
	S32			mSelectedOtherCount;
	S32			mSelectedPublicCount;
	bool		mSelectedMultipleOwners;
	bool		mWholeParcelSelected;
};

typedef LLSafeHandle<LLParcelSelection> LLParcelSelectionHandle;
