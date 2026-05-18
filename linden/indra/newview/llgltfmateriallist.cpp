/**
 * @file llgltfmateriallist.cpp
 * @brief The LLGLTFMaterialList class implementation
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 *
 * Copyright (c) 2022, Linden Research, Inc.
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

#include "boost/iostreams/device/array.hpp"
#include "boost/iostreams/stream.hpp"

#include "tinygltf/tiny_gltf.h"

#include "llgltfmateriallist.h"

#include "indra_constants.h"				// For BLANK_MATERIAL_ASSET_ID
#include "llassetstorage.h"
#include "llcorehttputil.h"
#include "llfilesystem.h"
#include "llsdserialize.h"
#include "llworkqueue.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llpipeline.h"
#include "lltinygltfhelper.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"				// For LLTerrain::isAsset()
#include "llvocache.h"
#include "llworld.h"
#if HB_DEBUG_OBJECT
# include "hbfloaterdebugtags.h"
#endif

LLGLTFMaterialList gGLTFMaterialList;

LLGLTFMaterialList::modify_queue_t LLGLTFMaterialList::sModifyQueue;
LLGLTFMaterialList::apply_queue_t LLGLTFMaterialList::sApplyQueue;
LLSD LLGLTFMaterialList::sUpdates;
LLGLTFMaterialList::selection_cb_list_t LLGLTFMaterialList::sSelectionCallbacks;

///////////////////////////////////////////////////////////////////////////////
// LLGLTFMaterialList class
///////////////////////////////////////////////////////////////////////////////

void LLGLTFMaterialList::applyQueuedOverrides(LLViewerObject* objp)
{
	if (objp)
	{
		// The override cache is the authoritative source of the most recent
		// override data
		LLViewerRegion* regionp = objp->getRegion();
		if (regionp)
		{
			regionp->applyCacheMiscExtras(objp);
		}
	}
}

void LLGLTFMaterialList::queueModify(const LLViewerObject* objp, S32 side,
									 const LLGLTFMaterial* matp)
{
	if (!objp || objp->getRenderMaterialID(side).isNull())
	{
		llwarns << "Cannot modify: NULL " << (objp ? "material" : "object")
				<< llendl;
		return;
	}

	const LLUUID& obj_id = objp->getID();

	if (matp)
	{
#if HB_DEBUG_OBJECT
		if (objp->getDebugUpdateMsg())
		{
			llinfos << "Queuing material modification for face " << side
					<< " of object " << obj_id << llendl;
		}
#endif
		sModifyQueue.emplace_back(obj_id, *matp, side, true);
		return;
	}

#if HB_DEBUG_OBJECT
	if (objp->getDebugUpdateMsg())
	{
		llinfos << "Queuing material reset for face " << side << " of object "
				<< obj_id << llendl;
	}
#endif
	sModifyQueue.emplace_back(obj_id, LLGLTFMaterial(), side, false);
}

void LLGLTFMaterialList::queueApply(const LLViewerObject* objp, S32 side,
									const LLUUID& asset_id)
{
	const LLUUID& obj_id = objp->getID();
	const LLGLTFMaterial* matp = objp->getTE(side)->getGLTFMaterialOverride();
	if (matp)
	{
#if HB_DEBUG_OBJECT
		if (objp->getDebugUpdateMsg())
		{
			llinfos << "Queuing material applying for face " << side
					<< " of object " << obj_id << llendl;
		}
#endif
		LLGLTFMaterial* cleared_matp = new LLGLTFMaterial(*matp);
		cleared_matp->setBaseMaterial();
		sApplyQueue.emplace_back(obj_id, asset_id, cleared_matp, side);
		return;
	}
#if HB_DEBUG_OBJECT
	if (objp->getDebugUpdateMsg())
	{
		llinfos << "Queuing material reset for face " << side << " of object "
				<< obj_id << llendl;
	}
#endif
	sApplyQueue.emplace_back(obj_id, asset_id, nullptr, side);
}

void LLGLTFMaterialList::queueApply(const LLViewerObject* objp, S32 side,
									const LLUUID& asset_id,
									const LLGLTFMaterial* matp)
{
	if (!matp || asset_id.isNull())
	{
		queueApply(objp, side, asset_id);
		return;
	}
	sApplyQueue.emplace_back(objp->getID(), asset_id,
							 new LLGLTFMaterial(*matp), side);
}

void LLGLTFMaterialList::queueUpdate(const LLSD& data)
{
	if (!sUpdates.isArray())
	{
		sUpdates = LLSD::emptyArray();
	}
	sUpdates[sUpdates.size()] = data;
}

void LLGLTFMaterialList::flushUpdates(done_cb_t callback)
{
	LLSD& data = sUpdates;

	S32 i = data.size();

#if HB_DEBUG_OBJECT
	const LLUUID& debugged_id = LLViewerObject::getDebuggedObjectId();
#endif

	for (ModifyMaterialData& e : sModifyQueue)
	{
		data[i]["object_id"] = e.object_id;
		data[i]["side"] = e.side;

		if (e.has_override)
		{
			data[i]["gltf_json"] = e.override_data.asJSON();
		}
		else
		{
			// Clear all overrides
			data[i]["gltf_json"] = "";
		}

#if HB_DEBUG_OBJECT
		if (e.object_id == debugged_id)
		{
			llinfos << "Sending modifications for object " << debugged_id
					<< " on side " << e.side << ". JSON data: "
					<< data[i]["gltf_json"] << llendl;
		}
#endif

		++i;
	}
	sModifyQueue.clear();

	for (auto& e : sApplyQueue)
	{
		data[i]["object_id"] = e.object_id;
		data[i]["side"] = e.side;
		data[i]["asset_id"] = e.asset_id;
		if (e.override_data.notNull())
		{
			data[i]["gltf_json"] = e.override_data->asJSON();
		}
		else
		{
			// Clear all overrides
			data[i]["gltf_json"] = "";
		}

#if HB_DEBUG_OBJECT
		if (e.object_id == debugged_id)
		{
			llinfos << "Applying material to object " << debugged_id
					<< " on side " << e.side << " asset Id " << e.asset_id
					<< ". JSON data: " << data[i]["gltf_json"] << llendl;
		}
#endif

		++i;
	}
	sApplyQueue.clear();

	if (data.size() == 0)
	{
		return;
	}

	const std::string& cap_url =
		gAgent.getRegionCapability("ModifyMaterialParams");
	if (cap_url.empty())
	{
		LL_DEBUGS("GLTF") << "No ModifyMaterialParams capability. Aborted"
						  << LL_ENDL;
		return;
	}

	gCoros.launch("modifyMaterialCoro",
				  std::bind(&LLGLTFMaterialList::modifyMaterialCoro, cap_url,
							data, callback));
	sUpdates = LLSD::emptyArray();
}

//static
void LLGLTFMaterialList::addSelectionUpdateCallback(update_cb_t callback)
{
	sSelectionCallbacks.insert(callback);
}

//static
void LLGLTFMaterialList::doSelectionCallbacks(const LLUUID& obj_id, S32 side)
{
	for (auto& callback : sSelectionCallbacks)
	{
		callback(obj_id, side);
	}
}

struct GLTFAssetLoadUserData
{
	GLTFAssetLoadUserData() = default;

	LL_INLINE GLTFAssetLoadUserData(tinygltf::Model model,
									LLFetchedGLTFMaterial* matp)
	:	mModelIn(model),
		mMaterial(matp)
	{
	}

	tinygltf::Model						mModelIn;
	LLPointer<LLFetchedGLTFMaterial>	mMaterial;
};

// Work done via the general queue thread pool.
//static
bool LLGLTFMaterialList::decodeAsset(const LLUUID& id,
									 GLTFAssetLoadUserData* asset_datap)
{
	LLFileSystem file(id);
	S32 size = file.getSize();
	if (!size)
	{
		llwarns << "Cannot read asset cache file for " << id << llendl;
		return false;
	}

	std::string buffer(size + 1, '\0');
	file.read((U8*)buffer.data(), size);

	// Read file into buffer
	std::stringstream istream(buffer);
	LLSD asset;
	if (!LLSDSerialize::deserialize(asset, istream, -1))
	{
		llwarns << "Failed to deserialize material LLSD for " << id << llendl;
		return false;
	}
	if (!asset.has("version"))
	{
		llwarns << "Missing glTF version in material LLSD for " << id
				<< llendl;
		return false;
	}
	std::string data = asset["version"].asString();
	if (!LLGLTFMaterial::isAcceptedVersion(data))
	{
		llwarns << "Unsupported glTF version " << data << " for " << id
				<< llendl;
		return false;
	}
	if (!asset.has("type"))
	{
		llwarns << "Missing glTF asset type in material LLSD for " << id
				<< llendl;
		return false;
	}
	data = asset["type"].asString();
	if (data != LLGLTFMaterial::ASSET_TYPE)
	{
		llwarns << "Incorrect glTF asset type '" << data << "' for " << id
				<< llendl;
		return false;
	}
	if (!asset.has("data") || !asset["data"].isString())
	{
		llwarns << "Invalid glTF asset data for " << id << llendl;
		return false;
	}
	data = asset["data"].asString();
	std::string warn_msg, error_msg;
	tinygltf::TinyGLTF gltf;
	if (!gltf.LoadASCIIFromString(&asset_datap->mModelIn, &error_msg, &warn_msg,
								  data.c_str(), data.length(), ""))
	{
		llwarns << "Failed to decode material asset " << id
				<< ". tinygltf reports: \n" << warn_msg << "\n"
				<< error_msg << llendl;
		return false;
	}
	return true;
}

// Work on the main thread via the main loop work queue.
//static
void LLGLTFMaterialList::decodeAssetCallback(const LLUUID& id,
											 GLTFAssetLoadUserData* asset_datap,
											 bool result)
{
	if (asset_datap->mMaterial.isNull())	// Paranoia ?  HB
	{
		LL_DEBUGS("GLTF") << "NULL material returned for " << id << LL_ENDL;
		return;
	}

	if (result)
	{
		asset_datap->mMaterial->setFromModel(asset_datap->mModelIn, 0);
	}
	else
	{
		LL_DEBUGS("GLTF") << "Failed to get material " << id << LL_ENDL;
	}
	asset_datap->mMaterial->materialComplete(true);
	delete asset_datap;
}

void LLGLTFMaterialList::onAssetLoadComplete(const LLUUID& id,
											 LLAssetType::EType asset_type,
											 void* userdatap, S32 status,
											 LLExtStat ext_status)
{
	GLTFAssetLoadUserData* asset_datap = (GLTFAssetLoadUserData*)userdatap;
	if (status != LL_ERR_NOERR)
	{
		// A failure is "normal" if that asset Id was actually a terrain
		// texture Id and not a PBR material, so do not log an error. HB
		if (!LLTerrain::isAsset(id))
		{
			llwarns << "Error getting material asset data: "
					<< LLAssetStorage::getErrorString(status)
					<< " (" << status << ")" << llendl;
		}
		asset_datap->mMaterial->materialComplete(false);
		delete asset_datap;
		return;
	}

	if (!gMainloopWorkp)
	{
		// We are likely shutting down... HB
		return;
	}

	static LLWorkQueue::weak_t general_queue =
		LLWorkQueue::getNamedInstance("General");

	gMainloopWorkp->postTo(general_queue,
						   // Work done on general queue
						   [id, asset_datap]()
						   {
								return decodeAsset(id, asset_datap);
						   },
						   // Callback to main thread
						   [id, asset_datap](bool result)
						   {
								decodeAssetCallback(id, asset_datap, result);
						   });
}

LLFetchedGLTFMaterial* LLGLTFMaterialList::getMaterial(const LLUUID& id)
{
	id_mat_map_t::iterator iter = mList.find(id);
	if (iter != mList.end())
	{
		return iter->second;
	}

	LLFetchedGLTFMaterial* matp = new LLFetchedGLTFMaterial();
	mList[id] = matp;

	matp->materialBegin();

	GLTFAssetLoadUserData* datap = new GLTFAssetLoadUserData();
	datap->mMaterial = matp;

	if (gAssetStoragep)	// In case this would trigger after logoff... HB
	{
		gAssetStoragep->getAssetData(id, LLAssetType::AT_MATERIAL,
									 onAssetLoadComplete, (void*)datap);
	}

	return matp;
}

void LLGLTFMaterialList::flushMaterials()
{
	// Similar variant to what textures use (TextureFetchUpdateMinCount in LL's
	// PBR viewer code).
	static LLCachedControl<U32> min_update_count(gSavedSettings,
												 "TextureFetchUpdateMinMediumPriority");
	// Update min_update_count or 5% of materials, whichever is greater
	U32 update_count = llmax((U32)min_update_count, mList.size() / 20);
	update_count = llmin(update_count, (U32)mList.size());

	constexpr F32 TIMEOUT = 30.f;
	F32 cur_time = gFrameTimeSeconds;

	// Advance iter one past the last key we updated
	id_mat_map_t::iterator iter = mList.find(mLastUpdateKey);
	if (iter != mList.end())
	{
		++iter;
	}

	while (update_count-- > 0)
	{
		if (iter == mList.end())
		{
			iter = mList.begin();
		}

		LLPointer<LLFetchedGLTFMaterial> material = iter->second;
		if (material->getNumRefs() == 2) // This one plus one from the list
		{

			if (!material->mActive && cur_time > material->mExpectedFlushTime)
			{
				iter = mList.erase(iter);
			}
			else
			{
				if (material->mActive)
				{
					material->mExpectedFlushTime = cur_time + TIMEOUT;
					material->mActive = false;
				}
				++iter;
			}
		}
		else
		{
			material->mActive = true;
			++iter;
		}
	}

	if (iter != mList.end())
	{
		mLastUpdateKey = iter->first;
	}
	else
	{
		mLastUpdateKey.setNull();
	}
}

//static
void LLGLTFMaterialList::modifyMaterialCoro(const std::string& cap_url,
											LLSD overrides, done_cb_t callback)
{
	LL_DEBUGS("GLTF") << "Applying override via ModifyMaterialParams cap: "
					  << overrides << LL_ENDL;

	LLCore::HttpOptions::ptr_t options(new LLCore::HttpOptions);
	options->setFollowRedirects(true);

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("modifyMaterialCoro");
	LLSD result = adapter.postAndSuspend(cap_url, overrides, options);

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);

	bool success = true;
	if (!status)
	{
		llwarns << "Failed to modify material." << llendl;
		success = false;
	}
	else if (!result["success"].asBoolean())
	{
		llwarns << "Failed to modify material: " << result["message"] << LL_ENDL;
		success = false;
	}

	if (callback)
	{
		callback(success);
	}
}

void LLGLTFMaterialList::applyOverrideMessage(LLMessageSystem* msg,
											  const std::string& data_in)
{
	if (!msg) return;	// Paranoia

	const LLHost& host = msg->getSender();
	LLViewerRegion* regionp = gWorld.getRegion(host);
	if (!regionp)
	{
		return;
	}

	boost::iostreams::stream<boost::iostreams::array_source>
		istream(data_in.data(), data_in.size());
	LLSD data;
	LLSDSerialize::fromNotation(data, istream, data_in.size());

	const LLSD& tes = data["te"];
	if (!tes.isArray())
	{
		llwarns_once << "Malformed message: no 'te' array." << llendl;
		return;
	}

	U32 local_id = data.get("id").asInteger();
	LLUUID obj_id;
	gObjectList.getUUIDFromLocal(obj_id, local_id, host.getAddress(),
								 host.getPort());
	LLViewerObject* objp = NULL;
#if HB_DEBUG_OBJECT
	bool set_debug_tag = false;
#endif
	if (obj_id.notNull())
	{
#if HB_DEBUG_OBJECT
		set_debug_tag = obj_id == LLViewerObject::getDebuggedObjectId() &&
						!HBFloaterDebugTags::debugTagActive("GLTF");
		if (set_debug_tag)
		{
			HBFloaterDebugTags::setTag("GLTF", true);
		}
#endif
		LL_DEBUGS("GLTF") << "Received PBR material data for object " << obj_id
						  << ": " << data_in << LL_ENDL;

		objp = gObjectList.findObject(obj_id);
		// Note: objp may be NULL  if the viewer has not heard about the object
		// yet...
		if (objp && gShowObjectUpdates)
		{
			// Display a cyan blip for override updates when "Show objects
			// updates" is enabled.
			gPipeline.addDebugBlip(objp->getPositionAgent(), LLColor4::cyan);
		}
	}

	bool has_te[MAX_TES] = { false };
	fast_hset<S32> selected_tes;

	LLGLTFOverrideCacheEntry entry;
	entry.mLocalId = local_id;
	entry.mRegionHandle = regionp->getHandle();

	const LLSD& od = data["od"];
	for (U32 i = 0, count = llmin(tes.size(), MAX_TES); i < count; ++i)
	{
		LL_DEBUGS("GLTF") << "Face " << i << ", material override: ";
		if (od[i].isDefined())
		{
			LL_CONT << od[i];
		}
		else
		{
			LL_CONT << "none.";
		}
		LL_CONT << LL_ENDL;

		// Note: setTEGLTFMaterialOverride() and cache will take ownership.
		LLGLTFMaterial* matp = new LLGLTFMaterial();
		matp->applyOverrideLLSD(od[i]);
		S32 te = tes[i].asInteger();
		has_te[te] = true;
		entry.mSides[te] = od[i];
		entry.mGLTFMaterial[te] = matp;
		if (objp)
		{
			objp->setTEGLTFMaterialOverride(te, matp);
			LLTextureEntry* tep = objp->getTE(te);
			if (tep && tep->isSelected())
			{
				selected_tes.insert(te);
			}
		}
	}

	if (objp)
	{
		// Null out overrides on TEs that should not have them
		for (U32 te = 0, count = llmin(objp->getNumTEs(), MAX_TES); te < count;
			 ++te)
		{
			if (!has_te[te])
			{
				LLTextureEntry* tep = objp->getTE(te);
				if (tep && tep->getGLTFMaterialOverride())
				{
					objp->setTEGLTFMaterialOverride(te, NULL);
					selected_tes.insert(te);
				}
			}
		}
	}

	regionp->cacheFullUpdateGLTFOverride(entry);

	if (!selected_tes.empty())
	{
		for (fast_hset<S32>::iterator it = selected_tes.begin(),
									  end = selected_tes.end();
			 it != end; ++it)
		{
			doSelectionCallbacks(obj_id, *it);
		}
	}

#if HB_DEBUG_OBJECT
	if (set_debug_tag)
	{
		HBFloaterDebugTags::setTag("GLTF", false);
	}
#endif
}
