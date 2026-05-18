/**
 * @file llpathfindingnavmeshstatus.h
 * @brief Header file for llpathfindingnavmeshstatus
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Linden Research, Inc.
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

#include "lluuid.h"

class LLSD;

class LLPathfindingNavMeshStatus
{
protected:
	LOG_CLASS(LLPathfindingNavMeshStatus);

public:
	typedef enum
	{
		kPending,
		kBuilding,
		kComplete,
		kRepending
	} ENavMeshStatus;

	LLPathfindingNavMeshStatus();
	LLPathfindingNavMeshStatus(const LLUUID& region_id);
	LLPathfindingNavMeshStatus(const LLUUID& region_id, const LLSD& content);
	LLPathfindingNavMeshStatus(const LLSD& content);
	LLPathfindingNavMeshStatus(const LLPathfindingNavMeshStatus& status);

	LLPathfindingNavMeshStatus& operator=(const LLPathfindingNavMeshStatus& status);

	LL_INLINE bool isValid() const					{ return mIsValid; }
	LL_INLINE const LLUUID& getRegionUUID() const	{ return mRegionUUID; }
	LL_INLINE U32 getVersion() const				{ return mVersion; }
	LL_INLINE ENavMeshStatus getStatus() const		{ return mStatus; }

private:
	void parseStatus(const LLSD& content);

private:
	bool						mIsValid;
	LLUUID						mRegionUUID;
	U32							mVersion;
	ENavMeshStatus				mStatus;

	static const std::string	sStatusPending;
	static const std::string	sStatusBuilding;
	static const std::string	sStatusComplete;
	static const std::string	sStatusRepending;
};
