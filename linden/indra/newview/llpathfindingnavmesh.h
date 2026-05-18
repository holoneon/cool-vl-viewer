/**
 * @file llpathfindingnavmesh.h
 * @brief Header file for llpathfindingnavmesh
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

#include <functional>
#include <memory>
#include <string>

#include "boost/signals2.hpp"

#include "llsd.h"

#include "llpathfindingnavmeshstatus.h"

class LLUUID;

class LLPathfindingNavMesh
{
protected:
	LOG_CLASS(LLPathfindingNavMesh);

public:
	typedef std::shared_ptr<LLPathfindingNavMesh> ptr_t;

	typedef enum {
		kNavMeshRequestUnknown,
		kNavMeshRequestWaiting,
		kNavMeshRequestChecking,
		kNavMeshRequestNeedsUpdate,
		kNavMeshRequestStarted,
		kNavMeshRequestCompleted,
		kNavMeshRequestNotEnabled,
		kNavMeshRequestError
	} ENavMeshRequestStatus;

	typedef std::function<void(ENavMeshRequestStatus,
							   const LLPathfindingNavMeshStatus&,
							   const LLSD::Binary&)> navmesh_cb_t;
	typedef boost::signals2::signal<void (ENavMeshRequestStatus,
										  const LLPathfindingNavMeshStatus&,
										  const LLSD::Binary&)> navmesh_signal_t;
	typedef boost::signals2::connection navmesh_slot_t;

	LLPathfindingNavMesh(const LLUUID& region_id);

	navmesh_slot_t registerNavMeshListener(navmesh_cb_t callback);

	bool hasNavMeshVersion(const LLPathfindingNavMeshStatus& status) const;

	void handleNavMeshWaitForRegionLoad();
	void handleNavMeshCheckVersion();
	void handleRefresh(const LLPathfindingNavMeshStatus& status);
	void handleNavMeshNewVersion(const LLPathfindingNavMeshStatus& status);
	void handleNavMeshStart(const LLPathfindingNavMeshStatus& status);
	void handleNavMeshResult(const LLSD& content, U32 version);
	void handleNavMeshNotEnabled();
	void handleNavMeshError();
	void handleNavMeshError(U32 version);

protected:

private:
	void setRequestStatus(ENavMeshRequestStatus statusp);
	void sendStatus();

	LLPathfindingNavMeshStatus mNavMeshStatus;
	ENavMeshRequestStatus      mNavMeshRequestStatus;
	navmesh_signal_t           mNavMeshSignal;
	LLSD::Binary               mNavMeshData;
};
