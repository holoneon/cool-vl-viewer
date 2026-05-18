/**
 * @file llpathfindingnavmesh.cpp
 * @brief Implementation of llpathfindingnavmesh
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

#include "llpathfindingnavmesh.h"

#include "llsd.h"
#include "llsdserialize.h"

#include "llpathfindingnavmeshstatus.h"

#define NAVMESH_VERSION_FIELD "navmesh_version"
#define NAVMESH_DATA_FIELD    "navmesh_data"

LLPathfindingNavMesh::LLPathfindingNavMesh(const LLUUID& region_id)
:	mNavMeshStatus(region_id),
	mNavMeshRequestStatus(kNavMeshRequestUnknown),
	mNavMeshSignal(),
	mNavMeshData()
{
}

LLPathfindingNavMesh::navmesh_slot_t LLPathfindingNavMesh::registerNavMeshListener(navmesh_cb_t callback)
{
	return mNavMeshSignal.connect(callback);
}

bool LLPathfindingNavMesh::hasNavMeshVersion(const LLPathfindingNavMeshStatus& status) const
{
	return mNavMeshStatus.getVersion() == status.getVersion() &&
		   (mNavMeshRequestStatus == kNavMeshRequestStarted ||
			mNavMeshRequestStatus == kNavMeshRequestCompleted ||
			(mNavMeshRequestStatus == kNavMeshRequestChecking &&
			 !mNavMeshData.empty()));
}

void LLPathfindingNavMesh::handleNavMeshWaitForRegionLoad()
{
	setRequestStatus(kNavMeshRequestWaiting);
}

void LLPathfindingNavMesh::handleNavMeshCheckVersion()
{
	setRequestStatus(kNavMeshRequestChecking);
}

void LLPathfindingNavMesh::handleRefresh(const LLPathfindingNavMeshStatus& status)
{
	if (mNavMeshStatus.getRegionUUID() != status.getRegionUUID())
	{
		llwarns << "Navmesh status received for another region: ignoring."
				<< llendl;
		return;
	}
	if (mNavMeshStatus.getVersion() != status.getVersion())
	{
		llwarns << "Navmesh status received with bad version: ignoring."
				<< llendl;
		return;
	}
	mNavMeshStatus = status;
	if (mNavMeshRequestStatus == kNavMeshRequestChecking)
	{
		if (!mNavMeshData.empty())
		{
			setRequestStatus(kNavMeshRequestCompleted);
		}
		else
		{
			llwarns << "Empty navmesh data received !" << llendl;
		}
	}
	else
	{
		sendStatus();
	}
}

void LLPathfindingNavMesh::handleNavMeshNewVersion(const LLPathfindingNavMeshStatus& status)
{
	if (mNavMeshStatus.getRegionUUID() != status.getRegionUUID())
	{
		llwarns << "Navmesh version received for another region: ignoring."
				<< llendl;
		return;
	}
	if (mNavMeshStatus.getVersion() == status.getVersion())
	{
		mNavMeshStatus = status;
		sendStatus();
	}
	else
	{
		mNavMeshData.clear();
		mNavMeshStatus = status;
		setRequestStatus(kNavMeshRequestNeedsUpdate);
	}
}

void LLPathfindingNavMesh::handleNavMeshStart(const LLPathfindingNavMeshStatus& status)
{
	if (mNavMeshStatus.getRegionUUID() != status.getRegionUUID())
	{
		llwarns << "Navmesh start signal received for another region: ignoring."
				<< llendl;
		return;
	}
	mNavMeshStatus = status;
	setRequestStatus(kNavMeshRequestStarted);
}

void LLPathfindingNavMesh::handleNavMeshResult(const LLSD& content, U32 version)
{
	if (content.has(NAVMESH_VERSION_FIELD) &&
		content.get(NAVMESH_VERSION_FIELD).isInteger() &&
		content.get(NAVMESH_VERSION_FIELD).asInteger() >= 0)
	{
		U32 advertized = (U32)content.get(NAVMESH_VERSION_FIELD).asInteger();
		if (advertized != version)
		{
			llwarns << "Mismatch between expected and embedded navmesh versions occurred"
					<< llendl;
			version = advertized;
		}
	}
	else
	{
		llwarns << "Malformed navmesh data: missing version" << llendl;
	}

	if (mNavMeshStatus.getVersion() == version)
	{
		ENavMeshRequestStatus status;
		if (content.has(NAVMESH_DATA_FIELD))
		{
			LLSD::Binary value = content.get(NAVMESH_DATA_FIELD).asBinary();
			bool valid = false;
			size_t decomp_size = 0;
			U8* buffer = unzip_llsdNavMesh(valid, decomp_size, value.data(),
										   value.size());
			if (!valid || !buffer)
			{
				llwarns << "Unable to decompress the navmesh llsd." << llendl;
				status = kNavMeshRequestError;
			}
			else
			{
				mNavMeshData.resize(decomp_size);
				memcpy(&mNavMeshData[0], &buffer[0], decomp_size);
				status = kNavMeshRequestCompleted;
			}
			if (buffer)
			{
				free(buffer);
			}
		}
		else
		{
			llwarns << "No mesh data received" << llendl;
			status = kNavMeshRequestError;
		}
		setRequestStatus(status);
	}
}

void LLPathfindingNavMesh::handleNavMeshNotEnabled()
{
	mNavMeshData.clear();
	setRequestStatus(kNavMeshRequestNotEnabled);
}

void LLPathfindingNavMesh::handleNavMeshError()
{
	mNavMeshData.clear();
	setRequestStatus(kNavMeshRequestError);
}

void LLPathfindingNavMesh::handleNavMeshError(U32 version)
{
	if (mNavMeshStatus.getVersion() == version)
	{
		handleNavMeshError();
	}
}

void LLPathfindingNavMesh::setRequestStatus(ENavMeshRequestStatus statusp)
{
	mNavMeshRequestStatus = statusp;
	sendStatus();
}

void LLPathfindingNavMesh::sendStatus()
{
	mNavMeshSignal(mNavMeshRequestStatus, mNavMeshStatus, mNavMeshData);
}
