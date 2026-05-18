/**
 * @file llviewerassetstorage.cpp
 * @brief Subclass capable of loading asset data to/from an external source.
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

#include "llviewerprecompiledheaders.h"

#include "llviewerassetstorage.h"

#include "llcoproceduremanager.h"
#include "llfilesystem.h"
#include "lltransfersourceasset.h"
#include "lltransfertargetvfile.h"
#include "llmessage.h"

#include "llagent.h"
#include "llappviewer.h"				// For gAppViewerp and LLAppCoreHttp
#include "llgridmanager.h"				// For gIsInSecondLife
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"			// For LLTerrain::isAsset()

LLViewerAssetStorage::LLViewerAssetStorage(LLMessageSystem* msg,
										   LLXferManager* xfer)
:	LLAssetStorage(msg, xfer)
{
	LLCoprocedureManager::getInstance()->initializePool("AssetStorage");
	LLAppCoreHttp& app_core_http = gAppViewerp->getAppCoreHttp();
	mHttpPolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_ASSETS);
}

//virtual
LLViewerAssetStorage::~LLViewerAssetStorage()
{
	while (!mCoroWaitList.empty())
	{
		CoroWaitList& request = mCoroWaitList.front();
		// Clean up pending downloads, delete request and trigger callbacks
		removeAndCallbackPendingDownloads(request.mId, request.mType,
										  request.mId, request.mType,
										  LL_ERR_NOERR, LLExtStat::NONE);
		mCoroWaitList.pop_front();
	}
}

//virtual
void LLViewerAssetStorage::storeAssetData(const LLTransactionID& tid,
										  LLAssetType::EType asset_type,
										  LLStoreAssetCallback callback,
										  void* user_data,
										  bool temp_file,
										  bool is_priority,
										  bool store_local,
										  bool user_waiting,
										  F64 timeout)
{
	// Verify that gMessageSystemp (== mMessageSys) has not been destroyed. HB
	if (!gMessageSystemp)
	{
		llwarns << "Message system no longer exists. Failed to store asset: "
				<< temp_file << llendl;
		return;
	}

	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	llinfos << "Legacy call for " << tid << "."
			<< LLAssetType::lookup(asset_type) << " - Asset ID: " << asset_id
			<< llendl;

	if (mUpstreamHost.isOk())
	{
		if (LLFileSystem::getExists(asset_id))
		{
			// Pack data into this packet if we can fit it.
			U8 buffer[MTUBYTES];
			buffer[0] = 0;

			LLFileSystem vfile(asset_id);
			S32 asset_size = vfile.getSize();

			LLAssetRequest* req = new LLAssetRequest(asset_id, asset_type);
			req->mUpCallback = callback;
			req->mUserData = user_data;

			if (asset_size < 1)
			{
				// This can happen if there's a bug in our code or if the cache
				// has been corrupted.
				llwarns << "Data  for asset " << asset_id << "."
						<< LLAssetType::lookup(asset_type)
						<< "_should_ already be in the cache, but it is not !"
						<< llendl;
				delete req;
				if (callback)
				{
					callback(asset_id, user_data, LL_ERR_ASSET_REQUEST_FAILED,
							 LLExtStat::CACHE_CORRUPT);
				}
				return;
			}
			else
			{
				if (is_priority)
				{
					mPendingUploads.push_front(req);
				}
				else
				{
					mPendingUploads.push_back(req);
				}
			}

			// Read the data from the cache if it will fit in this packet.
			if (asset_size + 100 < MTUBYTES)
			{
				bool res = vfile.read(buffer, asset_size);
				S32 bytes_read = res ? vfile.getLastBytesRead() : 0;

				if (bytes_read == asset_size)
				{
					req->mDataSentInFirstPacket = true;
				}
				else
				{
					llwarns << "Probable corruption in cached file, aborting store asset data."
							<< llendl;
					if (callback)
					{
						callback(asset_id, user_data,
								 LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE,
								 LLExtStat::CACHE_CORRUPT);
					}
					return;
				}
			}
			else
			{
				// Too big, do an xfer
				buffer[0] = 0;
				asset_size = 0;
			}
			mMessageSys->newMessageFast(_PREHASH_AssetUploadRequest);
			mMessageSys->nextBlockFast(_PREHASH_AssetBlock);
			mMessageSys->addUUIDFast(_PREHASH_TransactionID, tid);
			mMessageSys->addS8Fast(_PREHASH_Type, (S8)asset_type);
			mMessageSys->addBoolFast(_PREHASH_Tempfile, temp_file);
			mMessageSys->addBoolFast(_PREHASH_StoreLocal, store_local);
			mMessageSys->addBinaryDataFast(_PREHASH_AssetData, buffer,
										   asset_size);
			mMessageSys->sendReliable(mUpstreamHost);
		}
		else
		{
			llwarns << "AssetStorage: attempt to upload non-existent vfile "
					<< asset_id << "." << LLAssetType::lookup(asset_type)
					<< llendl;
			if (callback)
			{
				callback(asset_id, user_data,
						 LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE,
						 LLExtStat::NONEXISTENT_FILE);
			}
		}
	}
	else
	{
		llwarns << "Attempt to move asset store request upstream without valid upstream provider"
				<< llendl;
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CIRCUIT_GONE,
					 LLExtStat::NO_UPSTREAM);
		}
	}
}

void LLViewerAssetStorage::storeAssetData(const std::string& filename,
										  const LLTransactionID& tid,
										  LLAssetType::EType asset_type,
										  LLStoreAssetCallback callback,
										  void* user_data,
										  bool temp_file,
										  bool is_priority,
										  bool user_waiting,
										  F64 timeout)
{
	// Verify that gMessageSystemp (== mMessageSys) has not been destroyed. HB
	if (!gMessageSystemp)
	{
		llwarns << "Message system no longer exists. Failed to store asset: "
				<< filename << llendl;
		return;
	}

	if (filename.empty())
	{
		llerrs << "No filename specified" << llendl;
		return;
	}

	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	llinfos << "Legacy storeAssetData call for asset" << asset_id << "."
			<< LLAssetType::lookup(asset_type) << llendl;

	S32 size = 0;
	LLFILE* fp = LLFile::open(filename, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}
	if (size)
	{
		LLLegacyAssetRequest* legacyp = new LLLegacyAssetRequest;
		legacyp->mUpCallback = callback;
		legacyp->mUserData = user_data;

		LLFileSystem file(asset_id, LLFileSystem::APPEND);

		constexpr S32 buf_size = 65536;
		U8 copy_buf[buf_size];
		while ((size = (S32)fread(copy_buf, 1, buf_size, fp)))
		{
			file.write(copy_buf, size);
		}
		LLFile::close(fp);

		// If this upload fails, the caller needs to setup a new tempfile for
		// us
		if (temp_file)
		{
			LLFile::remove(filename);
		}

		// LLAssetStorage metric: Success not needed; handled in the
		// overloaded method here:

		storeAssetData(tid, asset_type, legacyStoreDataCallback,
					   (void**)legacyp, temp_file, is_priority);
	}
	else // size == 0 (but previous block changes size)
	{
		if (fp)
		{
			LLFile::close(fp);
		}
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CANNOT_OPEN_FILE,
					 LLExtStat::BLOCKED_FILE);
		}
	}
}

//virtual
void LLViewerAssetStorage::checkForTimeouts()
{
	// If no agent region: wait for it to come up (required for "terrain
	// assets" in the agent region on login). HB
	LLViewerRegion* aregionp = gAgent.getRegion();
	if (!aregionp || !aregionp->capabilitiesReceived() || LLApp::isExiting() ||
		!gAssetStoragep)
	{
		return;
	}

	LLAssetStorage::checkForTimeouts();

	// Restore requests
	LLCoprocedureManager* cpmgr = LLCoprocedureManager::getInstance();
	while (!mCoroWaitList.empty())
	{
		CoroWaitList& req = mCoroWaitList.front();
		LLUUID id =
			cpmgr->enqueueCoprocedure("AssetStorage",
									  "LLViewerAssetStorage::assetRequestCoro",
									  std::bind(&LLViewerAssetStorage::assetRequestCoro,
												this, req.mUrl, req.mId,
												req.mType, req.mCallback,
												req.mUserData));
		if (id.isNull())	// Failed to enqueue...
		{
			llinfos << "Will retry: " << req.mId << llendl;
			break;
		}
		mCoroWaitList.pop_front();
	}
}

//virtual
void LLViewerAssetStorage::queueDataRequest(const LLUUID& uuid,
											LLAssetType::EType atype,
											LLGetAssetCallback callback,
											void* user_data, bool duplicate,
											bool is_priority)
{
	static LLCachedControl<bool> use_viewerasset(gSavedSettings,
												 "UseViewerAssetCap");
	if (gIsInSecondLife ||
		(use_viewerasset && gAgent.hasRegionCapability("ViewerAsset")))
	{
		queueHttpRequest(uuid, atype, callback, user_data, duplicate,
						 is_priority);
		return;
	}
	// Legacy, UDP fetch, for OpenSim
	queueUdpRequest(uuid, atype, callback, user_data, duplicate, is_priority);
}

void LLViewerAssetStorage::queueUdpRequest(const LLUUID& uuid,
										   LLAssetType::EType atype,
										   LLGetAssetCallback callback,
										   void* user_data, bool duplicate,
										   bool is_priority)
{
	if (mUpstreamHost.isOk())
	{
		// Stash the callback info so we can find it after we get the response
		// message
		LLAssetRequest* req = new LLAssetRequest(uuid, atype);
		req->mDownCallback = callback;
		req->mUserData = user_data;
		req->mIsPriority = is_priority;
		mPendingDownloads.push_back(req);

		if (!duplicate)
		{
			// Send request message to our upstream data provider.
			// Create a new asset transfer.
			LLTransferSourceParamsAsset spa;
			spa.setAsset(uuid, atype);

			// Set our destination file, and the completion callback.
			LLTransferTargetParamsVFile tpvf;
			tpvf.setAsset(uuid, atype);
			tpvf.setCallback(downloadCompleteCallback, *req);

			LL_DEBUGS("AssetStorage") << "Starting transfer for " << uuid
									  << LL_ENDL;
			LLTransferTargetChannel* ttcp =
				gTransferManager.getTargetChannel(mUpstreamHost, LLTCT_ASSET);
			if (ttcp)
			{
				ttcp->requestTransfer(spa, tpvf,
									  100.f + (is_priority ? 1.f : 0.f));
			}
			else
			{
				llwarns << "Cannot find transfer manager channel for upstream host: "
						<< mUpstreamHost.getIPandPort() << ". Aborted."
						<< llendl;
			}
		}
	}
	else
	{
		// Uh-oh, we should not have gotten here
		llwarns << "Attempt to move asset data request upstream without valid upstream provider"
				<< llendl;
		if (callback)
		{
			callback(uuid, atype, user_data, LL_ERR_CIRCUIT_GONE,
					 LLExtStat::NO_UPSTREAM);
		}
	}
}

void LLViewerAssetStorage::queueHttpRequest(const LLUUID& asset_id,
											LLAssetType::EType atype,
											LLGetAssetCallback callback,
											void* user_data, bool duplicate,
											bool is_priority)
{
	if (LLApp::isExiting() || !gAssetStoragep)
	{
		return;
	}

	LLAssetRequest* req = new LLAssetRequest(asset_id, atype);
	req->mDownCallback = callback;
	req->mUserData = user_data;
	req->mIsPriority = is_priority;
	mPendingDownloads.push_back(req);

	if (duplicate)
	{
		return;
	}

	std::string query = "?";
	query += LLAssetType::lookup(atype);
	query += "_id=" + asset_id.asString();

	// If no agent region or capabilities not yet received for it, wait. HB
	LLViewerRegion* aregionp = gAgent.getRegion();
	if (!aregionp || !aregionp->capabilitiesReceived())
	{
		LL_DEBUGS("AssetStorage") << "Agent region ";
		if (aregionp)
		{
			LL_CONT << "capabilities not yet received";
		}
		else
		{
			LL_CONT << "not yet set";
		}
		LL_CONT << ". Delaying: " << asset_id << LL_ENDL;
		mCoroWaitList.emplace_back(query, asset_id, atype, callback,
								   user_data);
		return;
	}

	LLCoprocedureManager* cpmgr = LLCoprocedureManager::getInstance();
	LLUUID id =
		cpmgr->enqueueCoprocedure("AssetStorage",
								  "LLViewerAssetStorage::assetRequestCoro",
								  std::bind(&LLViewerAssetStorage::assetRequestCoro,
											this, query, asset_id, atype,
											callback, user_data));
	if (id.isNull())	// Failed to enqueue...
	{
		mCoroWaitList.emplace_back(query, asset_id, atype, callback,
								   user_data);
		llinfos << "Will retry: " << asset_id << llendl;
	}
}

static void cap_received_for_region(std::string pump_name)
{
	gEventPumps.obtain(pump_name).post(LLSD());
}

void LLViewerAssetStorage::assetRequestCoro(std::string query, LLUUID uuid,
											LLAssetType::EType atype,
 											void (*callback)(const LLUUID&,
															 LLAssetType::EType,
															 void*, S32,
															 LLExtStat),
											void* user_data)
{
	if (!gAssetStoragep)
	{
		llwarns << "Asset storage no longer exists. Failed to fetch asset: "
				<< uuid << llendl;
		return;
	}

	S32 result_code = LL_ERR_NOERR;
	LLExtStat ext_status = LLExtStat::NONE;

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp)
	{
		llwarns << "No agent region !  Failed to fetch asset: " << uuid
				<< llendl;
		result_code = LL_ERR_ASSET_REQUEST_FAILED;
		ext_status = LLExtStat::NONE;
		removeAndCallbackPendingDownloads(uuid, atype, uuid, atype,
										  result_code, ext_status);
		return;
	}

	if (!regionp->capabilitiesReceived())
	{
		llinfos_once << "Waiting for capabilities in region: "
					 << regionp->getName() << llendl;
		LLEventStream caps_recv("waitForCaps", true);
		auto connection =
			regionp->setCapsReceivedCB(std::bind(&cap_received_for_region,
												 caps_recv.getName()));
		llcoro::suspendUntilEventOn(caps_recv);
		connection.disconnect();
	}

	if (LLApp::isExiting() || !gAssetStoragep)
	{
		// Bail out if capabilities arrive after shutdown has been started.
		return;
	}

	const std::string& cap = gAgent.getRegionCapability("ViewerAsset");
	if (cap.empty())
	{
		if (regionp != gAgent.getRegion())
		{
			llwarns << "Region gone. Failed to fetch asset: " << uuid
					<< llendl;
		}
		else
		{
			llwarns << "Capabilities received but no ViewerAsset cap found. Failed to fetch asset: "
					<< uuid << llendl;
		}
		result_code = LL_ERR_ASSET_REQUEST_FAILED;
		ext_status = LLExtStat::NONE;
		removeAndCallbackPendingDownloads(uuid, atype, uuid, atype,
										  result_code, ext_status);
		return;
	}
	
	LL_DEBUGS("AssetStorage") << "Starting transfer for " << uuid
							  << " - Request URL: " << cap + query << LL_ENDL;

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("assetRequestCoro",
												 mHttpPolicyClass);
	LLSD result = adapter.getRawAndSuspend(cap + query);

	if (LLApp::isExiting() || !gAssetStoragep)
	{
		// Bail out if result arrives after shutdown has been started.
		return;
	}

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (status)
	{
		const LLSD::Binary& raw =
			result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
		S32 size = raw.size();
		if (size > 0)
		{
			LLFileSystem vf(uuid, LLFileSystem::OVERWRITE);
			if (!vf.write(raw.data(), size))
			{
				// *TODO asset-http: handle error
				llwarns << "Failure to write data in cache for asset: " << uuid
						<< llendl;
				result_code = LL_ERR_ASSET_REQUEST_FAILED;
				ext_status = LLExtStat::CACHE_CORRUPT;
			}
			else
			{
				LL_DEBUGS("AssetStorage") << "Transfer successful for " << uuid
										  << LL_ENDL;
			}
		}
		else
		{
			llwarns << "Bad size (" << size
					<< ") in response to fetch request for asset: " << uuid
					<< llendl;
			result_code = LL_ERR_ASSET_REQUEST_FAILED;
			ext_status = LLExtStat::NONE;
			// *TODO: implement UDP fallback path ?
		}
	}
	else
	{
		// A failure is "normal" if that asset Id was actually a terrain
		// texture Id and not a PBR material, so do not log an error. HB
		if (!LLTerrain::isAsset(uuid))
		{
			llwarns << "Request failed for asset: " << uuid << " - Reason: "
					<< status.toString() << llendl;
		}
		result_code = LL_ERR_ASSET_REQUEST_FAILED;
		ext_status = LLExtStat::NONE;
		// *TODO: implement UDP fallback path (for OpenSIM only now) ?
	}

	removeAndCallbackPendingDownloads(uuid, atype, uuid, atype, result_code,
									  ext_status);
}
