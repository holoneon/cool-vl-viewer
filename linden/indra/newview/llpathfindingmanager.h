/**
 * @file llpathfindingmanager.h
 * @brief Header file for llpathfindingmanager
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
#include <string>

#include "boost/signals2.hpp"

#include "llcorehttpheaders.h"
#include "llcorehttpoptions.h"
#include "hbfastmap.h"
#include "llsingleton.h"

#include "llpathfindinglinkset.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingobjectlist.h"

// We do not have access to the closed source Havok library for path-finding GL
// drawing. *TODO: find an Open Source equivalent.
#define HAVE_PATHINGLIB 0

class LinksetsResponder;
class LLPathfindingNavMeshStatus;
class LLViewerRegion;

class LLPathfindingManager : public LLSingleton<LLPathfindingManager>
{
	friend class LLSingleton<LLPathfindingManager>;
	friend class LLNavMeshSimStateChangeNode;
	friend class NavMeshStatusResponder;
	friend class LLAgentStateChangeNode;
	friend class AgentStateResponder;

protected:
	LOG_CLASS(LLPathfindingManager);

public:
	typedef enum {
		kRequestStarted,
		kRequestCompleted,
		kRequestNotEnabled,
		kRequestError
	} ERequestStatus;

	LLPathfindingManager();
	virtual ~LLPathfindingManager();

	bool isPathfindingEnabledForCurrentRegion() const;
	bool isPathfindingEnabledForRegion(LLViewerRegion* region) const;
#if HAVE_PATHINGLIB
	bool isPathfindingViewEnabled() const;
#endif

	bool isAllowViewTerrainProperties() const;

	LLPathfindingNavMesh::navmesh_slot_t registerNavMeshListenerForRegion(LLViewerRegion* region,
																		  LLPathfindingNavMesh::navmesh_cb_t cb);
	void requestGetNavMeshForRegion(LLViewerRegion* region,
									bool get_status_only);

	typedef U32 request_id_t;
	typedef std::function<void(request_id_t, ERequestStatus,
							   LLPathfindingObjectList::ptr_t)> object_request_cb_t;

	void requestGetLinksets(request_id_t req_id, object_request_cb_t cb) const;

	void requestSetLinksets(request_id_t req_id,
							const LLPathfindingObjectList::ptr_t& pobjects,
							LLPathfindingLinkset::ELinksetUse use,
							S32 pA, S32 pB, S32 pC, S32 pD,
							object_request_cb_t cb) const;

	void requestGetCharacters(request_id_t req_id,
							  object_request_cb_t cb) const;

	typedef std::function<void(bool)>			agent_state_cb_t;
	typedef boost::signals2::signal<void(bool)>	agent_state_signal_t;
	typedef boost::signals2::connection			agent_state_slot_t;

	agent_state_slot_t registerAgentStateListener(agent_state_cb_t cb);
	void requestGetAgentState();

	typedef std::function<void(bool)> rebake_navmesh_cb_t;
	void requestRebakeNavMesh(rebake_navmesh_cb_t cb);

private:
	void handleDeferredGetAgentStateForRegion(const LLUUID& region_id);
	void handleDeferredGetNavMeshForRegion(const LLUUID& region_id,
										   bool get_status_only);
	void handleDeferredGetLinksetsForRegion(const LLUUID& region_id,
											request_id_t req_id,
											object_request_cb_t cb) const;
	void handleDeferredGetCharactersForRegion(const LLUUID& region_id,
											  request_id_t req_id,
											  object_request_cb_t cb) const;

	void navMeshStatusRequestCoro(const std::string& url, U64 region_handle,
								  bool get_status_only);
	void navAgentStateRequestCoro(const std::string& url);
	void navMeshRebakeCoro(const std::string& url, rebake_navmesh_cb_t cb);
	void linksetObjectsCoro(const std::string& url,
							std::shared_ptr<LinksetsResponder> responder,
							LLSD put_data) const;
	void linksetTerrainCoro(const std::string& url,
							std::shared_ptr<LinksetsResponder> responder,
							LLSD put_data) const;
	void charactersCoro(const std::string& url, request_id_t req_id,
						object_request_cb_t cb) const;

	void handleNavMeshStatusUpdate(const LLPathfindingNavMeshStatus& status);

	void handleAgentState(bool can_rebake_region);

	LLPathfindingNavMesh::ptr_t getNavMeshForRegion(const LLUUID& region_id);
	LLPathfindingNavMesh::ptr_t getNavMeshForRegion(LLViewerRegion* region);

	const std::string getNavMeshStatusURLForCurrentRegion() const;
	const std::string getNavMeshStatusURLForRegion(LLViewerRegion* r) const;
	const std::string getRetrieveNavMeshURLForRegion(LLViewerRegion* r) const;
	const std::string getRetrieveObjectLinksetsURLForCurrentRegion() const;
	const std::string getChangeObjectLinksetsURLForCurrentRegion() const;
	const std::string getTerrainLinksetsURLForCurrentRegion() const;
	const std::string getCharactersURLForCurrentRegion() const;
	const std::string getAgentStateURLForRegion(LLViewerRegion* region) const;
	const std::string getRegionCapability(LLViewerRegion* region,
										  const char* cap_name) const;

private:
	LLCore::HttpOptions::ptr_t	mHttpOptions;
	LLCore::HttpHeaders::ptr_t	mHttpHeaders;

	typedef fast_hmap<LLUUID, LLPathfindingNavMesh::ptr_t> map_t;
	map_t						mNavMeshMap;

	agent_state_signal_t		mAgentStateSignal;
};
