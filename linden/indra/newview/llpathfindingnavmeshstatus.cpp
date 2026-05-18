/**
 * @file llpathfindingnavmeshstatus.cpp
 * @brief Implementation of llpathfindingnavmeshstatus
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

#include "llviewerprecompiledheaders.h"

#include "llpathfindingnavmeshstatus.h"

#include "llsd.h"

#define REGION_FIELD  "region_id"
#define STATUS_FIELD  "status"
#define VERSION_FIELD "version"

const std::string LLPathfindingNavMeshStatus::sStatusPending("pending");
const std::string LLPathfindingNavMeshStatus::sStatusBuilding("building");
const std::string LLPathfindingNavMeshStatus::sStatusComplete("complete");
const std::string LLPathfindingNavMeshStatus::sStatusRepending("repending");

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus()
:	mIsValid(false),
	mRegionUUID(),
	mVersion(0U),
	mStatus(kComplete)
{
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLUUID& region_id)
:	mIsValid(false),
	mRegionUUID(region_id),
	mVersion(0U),
	mStatus(kComplete)
{
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLUUID& region_id,
													   const LLSD& content)
:	mIsValid(true),
	mRegionUUID(region_id),
	mVersion(0U),
	mStatus(kComplete)
{
	parseStatus(content);
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLSD& content)
:	mIsValid(true),
	mVersion(0U),
	mStatus(kComplete)
{
	llassert(content.has(REGION_FIELD));
	llassert(content.get(REGION_FIELD).isUUID());
	mRegionUUID = content.get(REGION_FIELD).asUUID();

	parseStatus(content);
}

LLPathfindingNavMeshStatus::LLPathfindingNavMeshStatus(const LLPathfindingNavMeshStatus& status)
:	mIsValid(status.mIsValid),
	mRegionUUID(status.mRegionUUID),
	mVersion(status.mVersion),
	mStatus(status.mStatus)
{
}

LLPathfindingNavMeshStatus &LLPathfindingNavMeshStatus::operator=(const LLPathfindingNavMeshStatus& status)
{
	mIsValid = status.mIsValid;
	mRegionUUID = status.mRegionUUID;
	mVersion = status.mVersion;
	mStatus = status.mStatus;

	return *this;
}

void LLPathfindingNavMeshStatus::parseStatus(const LLSD& content)
{
	if (content.has(VERSION_FIELD) &&
		content.get(VERSION_FIELD).isInteger() &&
		content.get(VERSION_FIELD).asInteger() >= 0)
	{
		mVersion = static_cast<U32>(content.get(VERSION_FIELD).asInteger());
	}
	else
	{
		llwarns << "Malformed navmesh status data: missing version"
				<< llendl;
	}

	if (!content.has(STATUS_FIELD) || !content.get(STATUS_FIELD).isString())
	{
		llwarns << "Malformed navmesh status data: missing status. Aborting !"
				<< llendl;
		return;
	}
	std::string status = content.get(STATUS_FIELD).asString();

	if (LLStringUtil::compareStrings(status, sStatusPending) == 0)
	{
		mStatus = kPending;
	}
	else if (LLStringUtil::compareStrings(status, sStatusBuilding) == 0)
	{
		mStatus = kBuilding;
	}
	else if (LLStringUtil::compareStrings(status, sStatusComplete) == 0)
	{
		mStatus = kComplete;
	}
	else if (LLStringUtil::compareStrings(status, sStatusRepending) == 0)
	{
		mStatus = kRepending;
	}
	else
	{
		mStatus = kComplete;
		llwarns << "Malformed navmesh status data: bad status" << llendl;
	}
}
