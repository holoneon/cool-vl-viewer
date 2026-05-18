/**
 * @file llgltfscenemanager.cpp
 * @brief LLGLTFSceneManager class implementation.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024, Linden Research, Inc.
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

#include "llgltfscenemanager.h"

#include "lleconomy.h"
#include "llfilesystem.h"
#include "llgltfanimation.h"
#include "llgltfprimitive.h"
#include "llimagej2c.h"
#include "llnotifications.h"
#include "llrender.h"
#include "llrenderutils.h"			// For gl_draw_box_outline()
#include "hbtracy.h"
#include "llvolumeoctree.h"
#include "llworkqueue.h"

#include "llappviewer.h"
#include "llfloaterperms.h"
#include "llpipeline.h"
#include "llspatialpartition.h"		// For renderOctreeRaycast()
#include "llstatusbar.h"			// For usingGLTFScene()
#include "llviewerassetupload.h"
#include "llviewerobjectlist.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"			// For gDebugRaycast*

#define GLTF_SIM_SUPPORT 1

using namespace LLGLTF;

//static
LLGLTFSceneManager::objects_set_t LLGLTFSceneManager::sObjects;
LLGLTFSceneManager::objects_map_t LLGLTFSceneManager::sPendingObjects;
S32 LLGLTFSceneManager::sLastTexture[] = { -2, -2, -2, -2, -2 };
LLPointer<LLViewerObject> LLGLTFSceneManager::sUploadingObject = NULL;
LLPointer<Asset> LLGLTFSceneManager::sUploadingAsset = NULL;
S32 LLGLTFSceneManager::sPendingImageUploads = 0;
S32 LLGLTFSceneManager::sPendingBinaryUploads = 0;
bool LLGLTFSceneManager::sPendingGLTFUpload = false;

// External functions implemented in llfetchedgltfmaterial.cpp, to avoid having
// to deal with LLViewerFetchedTexture from the llgtlf library. HB
extern LLGLTexture* gl_texture_from_fetched(const LLUUID& id);
extern LLGLTexture* gl_texture_from_local_file(const std::string& filename);
// External function implemented in lllocalbitmaps.cpp, to avoid having
// to deal with local textures from the llgtlf library. HB
extern const std::string& get_local_texture_filename(const LLUUID& tex_id);

// Helper function to create a raw image from a memory image data block.
static LLPointer<LLImageRaw> raw_image_from_memory(const U8* datap, U32 size,
												   const std::string& mimetype)
{
	LLPointer<LLImageFormatted> imagep =
		LLImageFormatted::loadFromMemory(datap, size, mimetype);
	if (!imagep)
	{
		return NULL;
	}

	LLPointer<LLImageRaw> raw_imagep = new LLImageRaw();
	imagep->decode(raw_imagep);
	return raw_imagep;
}

// This function is used in the llgltf library to deal with fetched materials
// textures without knowing anything about them excepted that they derive from
// the parent LLGLTexture class... HB
LLGLTexture* gl_texture_from_memory(U8* datap, S32 size,
									const std::string& mime_type)
{
	LLPointer<LLImageRaw> rawp = raw_image_from_memory(datap, size, mime_type);
	if (rawp.isNull())
	{
		return NULL;
	}

	LLViewerFetchedTexture* texp = new LLViewerFetchedTexture(rawp,
															  FTT_LOCAL_FILE,
															  true);
	texp->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
	texp->dontDiscard();
#if !LL_IMPLICIT_SETNODELETE
	texp->setNoDelete();
#endif
	return texp;
}

//static
void LLGLTFSceneManager::init()
{
	// Transmit the hook function pointers to the llgltf library. HB
	Asset::setHooks(gl_texture_from_fetched, gl_texture_from_local_file,
					gl_texture_from_memory, get_local_texture_filename);
}

//static
void LLGLTFSceneManager::cleanup()
{
	sObjects.clear();
	sPendingObjects.clear();
}

//static
std::string LLGLTFSceneManager::load(const std::string& filename,
									 const LLUUID& handle_obj_id)
{
	LLViewerObject* objp = gObjectList.findObject(handle_obj_id);
	if (!objp || objp->isDead() || !objp->asVolume() ||
		objp->mDrawable.isNull() || objp->isAttachment())
	{
		// *TODO: translate
		return "No valid object to act as a handle for the glTF scene.";
	}

	if (filename.empty())
	{
		objp->mGLTFAsset = NULL;
		sObjects.erase(objp);
		sPendingObjects.erase(objp);
		// Ensure the draw info will be regenerated, so to show the handle
		// object. HB
		objp->markForUpdate(true);
		// No error
		return "";
	}

	// Bind a shader to satisfy LLVertexBuffer assertions
	gDebugProgram.bind();

	LLPointer<Asset> assetp = new Asset();
	try
	{
		if (!assetp->load(filename))
		{
			return "Failed to load glTF asset.";
		}
	}
	catch (...)
	{
		return "Failed to load glTF asset (out of memory, file too big ?).";
	}
	assetp->updateTransforms();

	objp->mGLTFAsset = assetp;
	sObjects.emplace(objp);
	// Ensure the draw info will be regenerated, so to hide the handle
	// object. HB
	objp->markForUpdate(true);

	// No error
	return "";
}

//static
std::string LLGLTFSceneManager::save(const std::string& filename,
									 const LLUUID& handle_obj_id)
{
	LLViewerObject* objp = gObjectList.findObject(handle_obj_id);
	if (!objp || objp->isDead() || objp->mGLTFAsset.isNull())
	{
		// *TODO: translate
		return "Not a valid glTF scene handle object.";
	}

	objp->mGLTFAsset->save(filename);
	return "";
}

//static
void LLGLTFSceneManager::addGLTFObject(LLViewerObject* objp,
									   const LLUUID& gltf_id)
{
	// Make sure the object will not vanish on us.
	objp->ref();

	sPendingObjects.erase(objp);

	if (!gAssetStoragep || objp->isDead() ||
		// Do not re-add neither override an existing or missing asset.
		objp->mGLTFAsset.notNull() || objp->mIsGLTFAssetMissing)
	{
		objp->unref();
		return;
	}

	if (gUsePBRShaders)
	{
		// Note: we keep the ref() here; onGLTFLoadComplete will unref()
		gAssetStoragep->getAssetData(gltf_id, LLAssetType::AT_GLTF,
									 onGLTFLoadComplete, (void*)objp);
	}
	else
	{
		sPendingObjects.emplace(objp, gltf_id);
		objp->unref();
	}
}

void LLGLTFSceneManager::onGLTFLoadComplete(const LLUUID& id,
											LLAssetType::EType,
											void* user_data, S32 status,
											LLExtStat)
{
	LLViewerObject* objp = (LLViewerObject*)user_data;
	if (!objp || objp->isDead())
	{
		return;
	}

	if (status != LL_ERR_NOERR || !gAssetStoragep)
	{
		llwarns << "Failed to load glTF asset: " << id << llendl;
		objp->unref();
		return;
	}

	if (!gUsePBRShaders)
	{
		llwarns << "Cannot load glTF asset " << id
				<< " while not in PBR rendering mode. Loading postponed."
				<< llendl;
		sPendingObjects.emplace(objp, id);
		objp->unref();
		return;
	}

	LLFileSystem file(id);
	S32 file_size = file.getSize();
	if (file_size <= 0)
	{
		llwarns << "Failed to read file for glTF asset: " << id << llendl;
		objp->unref();
		return;
	}
	std::string data;
	data.resize(file_size);
	file.read((U8*)data.data(), file_size);
	objp->mGLTFAsset = Asset::createFromJsonData(data);
	if (objp->mGLTFAsset.isNull())
	{
		llwarns << "Failed to decode glTF JSON data for asset: " << id
				<< llendl;
		objp->mIsGLTFAssetMissing = true;
		objp->unref();
		return;
	}

	// For now just assume the buffer is already in the asset cache
	for (auto& buffer : objp->mGLTFAsset->mBuffers)
	{
		LLUUID buffer_id;
		if (!LLUUID::parseUUID(buffer.mUri, &buffer_id))
		{
			llwarns << "Buffer URI is not a valid UUID: " << buffer.mUri
					<< ". Aborted loading for glTF asset: " << id << llendl;
			objp->mIsGLTFAssetMissing = true;
			objp->unref();
			return;
		}
		++objp->mGLTFAsset->mPendingBuffers;
		gAssetStoragep->getAssetData(buffer_id, LLAssetType::AT_GLTF_BIN,
									 onGLTFBinLoadComplete, (void*)objp);
	}
}

void LLGLTFSceneManager::onGLTFBinLoadComplete(const LLUUID& id,
											   LLAssetType::EType,
											   void* user_data, S32 status,
											   LLExtStat)
{
	LLViewerObject* objp = (LLViewerObject*)user_data;
	if (!objp) return;	// Paranoia

	if (status != LL_ERR_NOERR)
	{
		llwarns << "Failed to load glTF asset: " << id << llendl;
		objp->mIsGLTFAssetMissing = true;
		objp->unref();
		return;
	}

	if (objp->mGLTFAsset.isNull() || objp->isDead())
	{
		llwarns << "Asset object is gone for glTF asset: " << id
				<< ". Aborted." << llendl;
		objp->mGLTFAsset = NULL;
		objp->mIsGLTFAssetMissing = true;
		objp->unref();
	}

	if (!gUsePBRShaders)
	{
		sPendingObjects.emplace(objp, id);
		llwarns << "Cannot load glTF asset " << id
				<< " while not in PBR rendering mode. Loading postponed."
				<< llendl;
		objp->mGLTFAsset = NULL;
		objp->unref();
		return;
	}

	if (--objp->mGLTFAsset->mPendingBuffers > 0)
	{
		// Not finished loading buffers...
		return;
	}

	// Make sure we have a bound shader before calling prep(). HB
	if (!LLGLSLShader::sCurBoundShaderPtr)
	{
		gDebugProgram.bind();
	}
	if (!objp->mGLTFAsset->prep())
	{
		llwarns << "Failed to prepare glTF asset: " << id << llendl;
		objp->mGLTFAsset = NULL;
		objp->mIsGLTFAssetMissing = true;
		objp->unref();
		return;
	}

	sObjects.emplace(objp);
	// Ensure the draw info will be regenerated, so to hide the handle
	// object. HB
	objp->markForUpdate(true);
	// Decrement the object's usage count now that we are sure it is referenced
	// in our list. HB
	objp->unref();
}

//static
std::string LLGLTFSceneManager::upload(const LLUUID& handle_obj_id)
{
	if (sUploadingAsset.notNull())
	{
		return "An upload is already in progress.";
	}

	sUploadingObject = gObjectList.findObject(handle_obj_id);
	if (!sUploadingObject || sUploadingObject->isDead() ||
		sUploadingObject->mGLTFAsset.isNull())
	{
		sUploadingObject = NULL;
		sUploadingAsset = NULL;
		// *TODO: translate
		return "Not a valid glTF scene handle object.";
	}

	sUploadingAsset = new Asset();
	*sUploadingAsset = *sUploadingObject->mGLTFAsset;
	Asset& asset = *sUploadingAsset;

	LLEconomy* economyp = LLEconomy::getInstance();
	LLUUID asset_id;
	std::string buffer, name;
	for (auto& image : asset.mImages)
	{
		if (image.mTexture.notNull())
		{
			// Note: this image pointer is stored in Asset as a LLGLTexture
			// pointer but indeed points to a LLViewerFetchedTexture instance;
			// this static cast is therefore legit. HB
			LLViewerFetchedTexture* texp =
				(LLViewerFetchedTexture*)image.mTexture.get();
			LLPointer<LLImageRaw> rawp;
			if (image.mBufferView != INVALID_INDEX)
			{
				BufferView& v = asset.mBufferViews[image.mBufferView];
				Buffer& b = asset.mBuffers[v.mBuffer];
				rawp = raw_image_from_memory(b.mData.data() + v.mByteOffset,
											 v.mByteLength, image.mMimeType);
				image.clearData(asset);
			}
			else
			{
				rawp = texp->getRawImage();
			}
			if (rawp.isNull())
			{
				rawp = texp->getSavedRawImage();
			}
#if 0		// readbackRawImage() is part of the new GPU-side scaling down code
			// which is not used by the Cool VL Viewer due to performances
			// degradation. *TODO: implement readbackRawImage() alone ?  HB
			if (rawp.isNull())
			{
				rawp = texp->readbackRawImage();
			}
#endif
			if (rawp.isNull())
			{
				continue;
			}

			LLPointer<LLImageJ2C> j2cp =
				LLViewerTextureList::convertToUploadFile(rawp);
			if (j2cp.isNull())
			{
				continue;
			}

			buffer.assign((const char*)j2cp->getData(), j2cp->getDataSize());
			asset_id.generate();

			S32 idx = (S32)(&image - &asset.mImages[0]);
			if (image.mName.empty())
			{
				name = llformat("Image_%d", idx);
			}
			else
			{
				name = image.mName;
			}
			S32 cost = economyp->getTextureUploadCost(j2cp->getWidth(),
													  j2cp->getHeight());
			LLNewBufferedResourceUploadInfo::failed_cb_t failure =
				[](const LLUUID&, const LLSD&, std::string)
				{
					if (sUploadingAsset.notNull()) // Not stale ? HB
					{
						--sPendingImageUploads;
					}
				};

			LLNewBufferedResourceUploadInfo::uploaded_cb_t finish =
				[idx, rawp, j2cp](LLUUID new_id, LLSD)
				{
					if (sUploadingAsset.isNull())	// Stale callback ? HB
					{
						return;
					}
					--sPendingImageUploads;
					if (idx >= (S32)sUploadingAsset->mImages.size())
					{
						llwarns << "Image index too large" << llendl;
						return;
					}
					sUploadingAsset->mImages[idx].mUri = new_id.asString();
				};

			++sPendingImageUploads;
			LLResourceUploadInfo::ptr_t info =
				std::make_shared<LLNewBufferedResourceUploadInfo>(
					buffer, asset_id, name, name, 0, LLFolderType::FT_TEXTURE,
					LLInventoryType::IT_TEXTURE, LLAssetType::AT_TEXTURE,
					LLFloaterPerms::getNextOwnerPerms(),
					LLFloaterPerms::getGroupPerms(),
					LLFloaterPerms::getEveryonePerms(), cost, finish, failure);
			upload_new_resource(info);
		}
	}

	for (auto& bin : asset.mBuffers)
	{
		S32 idx = (S32)(&bin - &asset.mBuffers[0]);
		buffer.assign((const char*)bin.mData.data(), bin.mData.size());
		asset_id.generate();

		LLNewBufferedResourceUploadInfo::uploaded_cb_t finish =
			[idx](LLUUID new_id, LLSD)
			{
				if (sUploadingAsset.isNull())	// Stale callback ? HB
				{
					return;
				}
				--sPendingBinaryUploads;
				if (idx >= (S32)sUploadingAsset->mBuffers.size())
				{
					llwarns << "Buffer index too large" << llendl;
					return;
				}
				sUploadingAsset->mBuffers[idx].mUri = new_id.asString();
			};
		
#if GLTF_SIM_SUPPORT		
		LLNewBufferedResourceUploadInfo::failed_cb_t failure =
			[](const LLUUID&, const LLSD& response, std::string reason)
			{
				if (sUploadingAsset.notNull()) // Not stale ? HB
				{
					// Unrecoverable error: abort upload.
					sUploadingObject = NULL;
					sUploadingAsset = NULL;
					sPendingBinaryUploads = 0;
					llwarns << "Failed to upload glTF binary with error: "
							<< reason << " - Response: " << response << llendl;
				}
			};

		++sPendingBinaryUploads;
		constexpr S32 cost = 1;	// Temporary placeholder
		LLResourceUploadInfo::ptr_t info =
			std::make_shared<LLNewBufferedResourceUploadInfo>(
				buffer, asset_id, "glTF binary data", asset_id.asString(), 0,
				LLFolderType::FT_NONE, LLInventoryType::IT_GLTF_BIN,
				LLAssetType::AT_GLTF_BIN, LLFloaterPerms::getNextOwnerPerms(),
				LLFloaterPerms::getGroupPerms(),
				LLFloaterPerms::getEveryonePerms(), cost, finish, failure);
		upload_new_resource(info);
#else
		finish(asset_id, LLSD());
#endif
	}

	return "";
}

//static
bool LLGLTFSceneManager::lineSegmentIntersect(LLVOVolume* volp, Asset* assetp,
											  const LLVector4a& start,
											  const LLVector4a& end,
											  S32 face, bool, bool,
											  S32* node_hitp, S32* prim_hitp,
											  LLVector4a* interp,
											  LLVector2* tcoordp,
											  LLVector4a* normp,
											  LLVector4a* tgtp)
{
	LLVector4a local_start, local_end, p, n, tn;
	LLVector2 tc;
	if (interp)
	{
		p = *interp;
	}
	if (tcoordp)
	{
		tc = *tcoordp;
	}
	if (normp)
	{
		n = *normp;
	}
	if (tgtp)
	{
		tn = *tgtp;
	}

	// Line segment intersection test 'start' and 'end' should be in agent
	// space. Volume space and asset space should be the same coordinate frame.
	// Results should be transformed back to agent space.
	LLMatrix4a asset_to_agent = volp->getGLTFAssetToAgentTransform();
	LLMatrix4a agent_to_asset = asset_to_agent;
	agent_to_asset.invert();
	agent_to_asset.affineTransform(start, local_start);
	agent_to_asset.affineTransform(end, local_end);

	S32 hit_node_index = assetp->lineSegmentIntersect(local_start, local_end,
													  &p, &tc, &n, &tn,
													  prim_hitp);
	if (hit_node_index < 0)
	{
		return false;
	}

	local_end = p;
	if (node_hitp)
	{
		*node_hitp = hit_node_index;
	}
	if (interp)
	{
		asset_to_agent.affineTransform(p, *interp);
	}
	if (normp)
	{
		LLVector3 v_n(n.getF32ptr());
		normp->load3(volp->volumeDirectionToAgent(v_n).mV);
		normp->normalize3fast();
	}
	if (tgtp)
	{
		LLVector3 v_tn(tn.getF32ptr());
		LLVector4a trans_tangent;
		trans_tangent.load3(volp->volumeDirectionToAgent(v_tn).mV);

		LLVector4Logical mask;
		mask.clear();
		mask.setElement<3>();

		tgtp->setSelectWithMask(mask, tn, trans_tangent);
		tgtp->normalize3fast();
	}
	if (tcoordp)
	{
		*tcoordp = tc;
	}

	return true;
}

//static
void LLGLTFSceneManager::update()
{
	LL_TRACY_TIMER(TRC_GLTF_SCENE_UPDATE);

	// Verify we do not hold on dead object pointers in the pending objects
	// map. HB
	for (objects_map_t::iterator it = sPendingObjects.begin(),
								 end = sPendingObjects.end();
		 it != end; )
	{
		if (it->first->isDead())
		{
			it = sPendingObjects.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (gUsePBRShaders)
	{
		// Add pending objects queued while not in PBR rendering mode. HB
		while (!sPendingObjects.empty())
		{
			objects_map_t::iterator it = sPendingObjects.begin();
			addGLTFObject(it->first, it->second);
		}
	}

	for (objects_set_t::iterator it = sObjects.begin(), end = sObjects.end();
		 it != end; )
	{
		LLViewerObject* objp = it->get();
		if (objp->isDead() || objp->mGLTFAsset.isNull())
		{
			it = sObjects.erase(it);
		}
		else
		{
			objp->mGLTFAsset->update();
			++it;
		}
	}

	// When not in PBR rendering mode, make sure the user is made aware they
	// are not seeing everything as they should... HB
	if (gStatusBarp)
	{
		gStatusBarp->usingGLTFScene(!sObjects.empty() ||
									!sPendingObjects.empty());
	}

	// glTF asset upload

	if (!gUsePBRShaders || sUploadingAsset.isNull() || sPendingGLTFUpload ||
		sPendingBinaryUploads > 0 || sPendingImageUploads > 0)
	{
		return;
	}

	std::string buffer;
	sUploadingAsset->serializeToString(buffer);
#if GLTF_SIM_SUPPORT		
	LLNewBufferedResourceUploadInfo::failed_cb_t failure =
		[](const LLUUID&, const LLSD& response, std::string reason)
		{
			if (sUploadingAsset.notNull()) // Not stale ? HB
			{
				// Unrecoverable error: abort upload.
				sUploadingObject = NULL;
				sUploadingAsset = NULL;
				sPendingGLTFUpload = false;
				llwarns << "Failed to upload glTF JSON data with error: "
						<< reason << " - Response: " << response << llendl;
			}
		};
#endif
	LLNewBufferedResourceUploadInfo::uploaded_cb_t finish =
		[buffer](LLUUID new_id, LLSD)
	{
		if (sUploadingAsset.isNull() || !gMainloopWorkp)
		{
			return;
		}
		static LLWorkQueue::weak_t general_queue =
			LLWorkQueue::getNamedInstance("General");

		gMainloopWorkp->postTo(general_queue,
							   // Work done on general queue
							   [new_id, buffer]()
							   {
									// *HACK: save buffer to cache to emulate a
									// successful upload.
									LLFileSystem file(new_id,
													  LLFileSystem::OVERWRITE);
									file.write((const U8*)buffer.c_str(),
											   buffer.size());
							   },
							   // Callback to main thread
							   [new_id]()
							   {
									if (sUploadingAsset.isNull())
									{
										return;	// Stale callback
									}
									sPendingGLTFUpload = false;
									sUploadingAsset = NULL;
									if (sUploadingObject.notNull())
									{
										sUploadingObject->mGLTFAsset = NULL;
										sUploadingObject->mIsGLTFAssetMissing =
											false;
										sUploadingObject->setGLTFAsset(new_id);
										sUploadingObject->markForUpdate();
										sUploadingObject = NULL;
									}
							   });
	};

	sPendingGLTFUpload = true;

	LLUUID asset_id;
	asset_id.generate();
#if GLTF_SIM_SUPPORT		
	constexpr S32 cost = 1;	// Temporary placeholder
	LLResourceUploadInfo::ptr_t info =
		std::make_shared<LLNewBufferedResourceUploadInfo>(
			buffer, asset_id, "glTF asset", asset_id.asString(), 0,
			LLFolderType::FT_NONE, LLInventoryType::IT_GLTF,
			LLAssetType::AT_GLTF, LLFloaterPerms::getNextOwnerPerms(),
			LLFloaterPerms::getGroupPerms(),
			LLFloaterPerms::getEveryonePerms(), cost, finish, failure);
	upload_new_resource(info);
#else
	finish(asset_id, LLSD());
#endif
}

//static
LLDrawable* LLGLTFSceneManager::lineSegmentIntersect(const LLVector4a& start,
													 const LLVector4a& end,
													 bool pick_transparent,
													 bool pick_rigged,
													 S32* node_hitp,
													 S32* prim_hitp,
													 LLVector4a* interp,
													 LLVector2* tcoordp,
													 LLVector4a* normp,
													 LLVector4a* tgtp)
{
	LLDrawable* drawp = NULL;
	LLVector4a local_end = end;
	LLVector4a position;
	for (objects_set_t::iterator it = sObjects.begin(), end = sObjects.end();
		 it != end; )
	{
		LLVOVolume* volp = it->get()->asVolume();
		if (!volp || volp->isDead() || volp->mGLTFAsset.isNull())
		{
			it = sObjects.erase(it);
			continue;
		}

		// Temporary debug: always double check objects that have glTF scenes
		// hanging off of them even if the ray does not intersect the object
		// bounds.
		if (lineSegmentIntersect(volp, volp->mGLTFAsset,start, local_end, -1,
			pick_transparent, pick_rigged, node_hitp, prim_hitp, &position,
			tcoordp, normp, tgtp))
		{
			local_end = position;
			if (interp)
			{
				*interp = position;
			}
			drawp = volp->mDrawable;
		}

		++it;
	}
	return drawp;
}

//static
void LLGLTFSceneManager::render(bool opaque, bool rigged, bool unlit)
{
	LL_TRACY_TIMER(TRC_GLTF_SCENE_RENDER);

	if (!gUsePBRShaders)
	{
		return;
	}

	if (!LLViewerShaderMgr::sCanRenderGLTFScene)
	{
		static bool warned = false;
		if (!warned)
		{
			warned = true;
			gNotifications.add("GLTFNoShaderSupport");
		}
		return;
	}

	U8 variant = 0;
	if (rigged)
	{
		variant |= LLGLSLShader::RIGGED;
	}
	if (!opaque)
	{
		variant |= LLGLSLShader::ALPHA_BLEND;
	}
	if (unlit)
	{
		variant |= LLGLSLShader::UNLIT;
	}
	renderVariant(variant);
}

//static
void LLGLTFSceneManager::renderVariant(U8 variant)
{
	// *HACK: implicitly render multi-UV variant
	if (!(variant & LLGLSLShader::MULTI_UV))
	{
		renderVariant(variant | LLGLSLShader::MULTI_UV);
	}

	for (objects_set_t::iterator it = sObjects.begin(), end = sObjects.end();
		 it != end; ++it)
	{
		LLViewerObject* objp = it->get();
		if (objp->isDead() || objp->mGLTFAsset.isNull())
		{
			continue;
		}
		Asset* assetp = objp->mGLTFAsset.get();
		gGL.pushMatrix();
		// Provide a modelview matrix that goes from asset to camera space
		// (matrix palettes are in asset space).
		LLMatrix4a mat = objp->getGLTFAssetToAgentTransform();
		gGL.loadMatrix(gGLModelView);
		gGL.multMatrix(mat.getF32ptr());
		renderAsset(*assetp, variant);
		gGL.popMatrix();
	}
}

//static
void LLGLTFSceneManager::renderAsset(Asset& asset, U8 v)
{
	if (gGLTFPBRMetallicRoughnessProgram.mGLTFVariants.size() <= v)
	{
		llwarns_once << "Invalid variant value: " << v << llendl;
		return;
	}

	for (U32 ds = 0; ds < 2; ++ds)
	{
		RenderData& rd = asset.mRenderData[ds];
		auto& batches = rd.mBatches[v];
		if (batches.empty())
		{
			return;
		}

		LLGLDisable cull_face(ds == 1 ? GL_CULL_FACE : 0);
		bool opaque = !(v & LLGLSLShader::ALPHA_BLEND);
		bool rigged = v & LLGLSLShader::RIGGED;
		LLGLSLShader* shaderp = NULL;
		for (size_t i = 0, count = batches.size(); i < count; ++i)
		{
			if (batches[i].mPrimitives.empty() ||
				batches[i].mVertexBuffer.isNull())
			{
				continue;
			}
			// Do not bind the shader until we know we have something to render
			if (!shaderp)
			{
				if (opaque)
				{
					shaderp = &gGLTFPBRMetallicRoughnessProgram;
					shaderp->bindVariant(v);
				}
				else
				{
					shaderp =
						&gGLTFPBRMetallicRoughnessProgram.mGLTFVariants[v];
					gPipeline.bindDeferredShader(*shaderp);
				}
				if (!rigged)
				{
					glBindBufferBase(GL_UNIFORM_BUFFER,
									 LLGLSLShader::UB_GLTF_NODES,
									 asset.mNodesUBO);
				}
				glBindBufferBase(GL_UNIFORM_BUFFER,
								 LLGLSLShader::UB_GLTF_MATERIALS,
								 asset.mMaterialsUBO);
				for (U8 j = 0; j < TEXTURE_TYPE_COUNT; ++j)
				{
					sLastTexture[j] = -2;
				}

				gGL.syncMatrices();
			}

			batches[i].mVertexBuffer->setBuffer();

			S32 mat_idx = i - 1;
			if (mat_idx >= 0)
			{
				Material& material = asset.mMaterials[mat_idx];
				bind(asset, material);
			}
			else
			{
				static LLFetchedGLTFMaterial default_mat;
				default_mat.bind(NULL, 256.f);
				shaderp->uniform1i(LLShaderMgr::GLTF_MATERIAL_ID, -1);
			}

			for (auto& pdata : batches[i].mPrimitives)
			{
				Node& node = asset.mNodes[pdata.mNodeIndex];
				Mesh& mesh = asset.mMeshes[node.mMesh];
				Primitive& prim = mesh.mPrimitives[pdata.mPrimitiveIndex];
				if (rigged)
				{
					Skin& skin = asset.mSkins[node.mSkin];
					glBindBufferBase(GL_UNIFORM_BUFFER,
									 LLGLSLShader::UB_GLTF_JOINTS, skin.mUBO);
				}
				else
				{
					shaderp->uniform1i(LLShaderMgr::GLTF_NODE_ID,
									   pdata.mNodeIndex);
				}
				prim.mVertexBuffer->drawRangeFast(prim.mGLMode,
												  prim.mVertexOffset,
												  prim.mVertexOffset +
													prim.getVertexCount() - 1,
												  prim.getIndexCount(),
												  prim.mIndexOffset);
			}
		}
	}
}

//static
void LLGLTFSceneManager::bindTexture(Asset& asset, U8 type, TextureInfo& info,
									 LLViewerFetchedTexture* fallback_texp)
{
	static const S32 uniform[] =
	{
		LLShaderMgr::DIFFUSE_MAP,
		LLShaderMgr::NORMAL_MAP,
		LLShaderMgr::METALLIC_ROUGHNESS_MAP,
		LLShaderMgr::OCCLUSION_MAP,
		LLShaderMgr::EMISSIVE_MAP
	};

	if (info.mIndex == sLastTexture[type])
	{
		return;	// Already bound
	}

	S32 channel =
		LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(uniform[type]);
	if (channel < 0)
	{
		return;
	}

	glActiveTexture(GL_TEXTURE0 + channel);
	if (info.mIndex == INVALID_INDEX)
	{
		glBindTexture(GL_TEXTURE_2D, fallback_texp->getTexName());
		return;
	}

	Texture& texture = asset.mTextures[info.mIndex];
	// Note: this cast is safe since while stored as a LLGLTexture*, mTexture
	// is indeed a LLViewerFetchedTexture*. HB
	LLViewerFetchedTexture* texp =
		(LLViewerFetchedTexture*)asset.mImages[texture.mSource].mTexture.get();
	if (!texp)
	{
		glBindTexture(GL_TEXTURE_2D, fallback_texp->getTexName());
		return;
	}

	glBindTexture(GL_TEXTURE_2D, texp->getTexName());
	if (channel != -1 && texture.mSampler != -1)
	{
		Sampler& sampler = asset.mSamplers[texture.mSampler];
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.mWrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.mWrapT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.mMagFilter);
		// Note: we do not set min filter so to respect client preferences.
	}
	else
	{
		// Set default sampler state
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
}

//static
void LLGLTFSceneManager::bind(Asset& asset, Material& mat)
{
	bindTexture(asset, BASE_COLOR, mat.mPbrMetallicRoughness.mBaseColorTexture,
				LLViewerFetchedTexture::sWhiteImagep);
	if (!LLPipeline::sShadowRender)
	{
		bindTexture(asset, NORMAL, mat.mNormalTexture,
					LLViewerFetchedTexture::sFlatNormalImagep);
		bindTexture(asset, METALLIC_ROUGHNESS,
					mat.mPbrMetallicRoughness.mMetallicRoughnessTexture,
					LLViewerFetchedTexture::sWhiteImagep);
		bindTexture(asset, OCCLUSION, mat.mOcclusionTexture,
					LLViewerFetchedTexture::sWhiteImagep);
		bindTexture(asset, EMISSIVE, mat.mEmissiveTexture,
					LLViewerFetchedTexture::sWhiteImagep);
	}
	LLGLSLShader::sCurBoundShaderPtr->uniform1i(LLShaderMgr::GLTF_MATERIAL_ID,
												(GLint)(&mat -
														&asset.mMaterials[0]));
}

static void draw_asset_debug(LLViewerObject* objp, Asset* assetp, bool bbox,
							 bool ray)
{
	gGL.pushMatrix();
	LLMatrix4a agent_to_asset = objp->getAgentToGLTFAssetTransform();
	gGL.multMatrix(agent_to_asset.getF32ptr());

	LLVector4a t;
	agent_to_asset.affineTransform(gDebugRaycastStart, t);
	glm::vec4 start = glm::make_vec4(t.getF32ptr());
	agent_to_asset.affineTransform(gDebugRaycastEnd, t);
	glm::vec4 end = glm::make_vec4(t.getF32ptr());
	start.w = end.w = 1.f;

	for (auto& node : assetp->mNodes)
	{
		if (node.mMesh < 0)
		{
			continue;
		}
		Mesh& mesh = assetp->mMeshes[node.mMesh];
		gGL.pushMatrix();
		gGL.multMatrix((F32*)glm::value_ptr(node.mAssetMatrix));

		if (bbox)
		{
			// Draw bounding box of mesh primitives
			gGL.color3f(0.f, 1.f, 1.f);
			for (auto& primitive : mesh.mPrimitives)
			{
				LLVolumeOctree* octreep = primitive.mOctree.get();
				if (octreep)
				{
					LLVolumeOctreeListenerNoOwnership* listenerp =
						(LLVolumeOctreeListenerNoOwnership*)octreep->getListener(0);
					if (listenerp)
					{
						LLVector4a center = listenerp->mBounds[0];
						LLVector4a size = listenerp->mBounds[1];
						gl_draw_box_outline(center, size);
					}
				}
			}
		}

		if (ray)
		{
			gGL.flush();
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			// Convert raycast to node local space
			glm::vec4 local_start = node.mAssetMatrixInv * start;
			glm::vec4 local_end = node.mAssetMatrixInv * end;

			LLVector4a s, e;
			for (auto& primitive : mesh.mPrimitives)
			{
				if (primitive.mOctree.notNull())
				{
					s.load3(glm::value_ptr(local_start));
					e.load3(glm::value_ptr(local_end));
					renderOctreeRaycast(s, e, primitive.mOctree);
				}
			}

			gGL.flush();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		gGL.popMatrix();
	}

	gGL.popMatrix();
}

//static
void LLGLTFSceneManager::renderDebug()
{
	LL_TRACY_TIMER(TRC_GLTF_SCENE_RENDER_DEBUG);

	gDebugProgram.bind();

	gGL.pushMatrix();
	gGL.loadMatrix(gGLModelView);

	LLGLDisable cullface(GL_CULL_FACE);
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gPipeline.disableLights();

	bool bbox = gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BBOXES);
	bool ray = gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST);
	if (bbox || ray)
	{
		for (objects_set_t::iterator it = sObjects.begin(),
									 end = sObjects.end();
			 it != end; ++it)
		{
			LLViewerObject* objp = it->get();
			if (!objp->isDead() && objp->mGLTFAsset.notNull())
			{
				draw_asset_debug(objp, objp->mGLTFAsset.get(), bbox, ray);
			}
		}
	}
	if (ray)
	{
		S32 node_hit = -1;
		S32 prim_hit = -1;
		LLVector4a inter;
		LLDrawable* drawp = lineSegmentIntersect(gDebugRaycastStart,
												 gDebugRaycastEnd, true, true,
												 &node_hit, &prim_hit, &inter,
												 NULL, NULL, NULL);
		if (drawp && !drawp->isDead() && drawp->getVObj() &&
			node_hit >= 0 && prim_hit >= 0)
		{
			LLViewerObject* objp = drawp->getVObj();
			Asset* assetp = objp->mGLTFAsset;
			Node* nodep = &assetp->mNodes[node_hit];
			Primitive* primp =
				&assetp->mMeshes[nodep->mMesh].mPrimitives[prim_hit];

			gGL.pushMatrix();
			gGL.multMatrix(objp->getGLTFAssetToAgentTransform().getF32ptr());

			gGL.flush();
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			gGL.color3f(1.f, 0.f, 1.f);
			gl_draw_box_outline(inter, LLVector4a(0.1f, 0.1f, 0.1f, 0.f));
			gGL.multMatrix(glm::value_ptr(nodep->mAssetMatrix));

			LLVolumeOctree* octreep = primp->mOctree.get();
			if (octreep)
			{
				LLVolumeOctreeListenerNoOwnership* listenerp =
					(LLVolumeOctreeListenerNoOwnership*)octreep->getListener(0);
				if (listenerp)
				{
					gl_draw_box_outline(listenerp->mBounds[0],
										listenerp->mBounds[1]);
				}
			}

			gGL.flush();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			gGL.popMatrix();
		}
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_NODES))
	{
		// Render nodes hierarchy
		for (U32 i = 0; i < 2; ++i)
		{
			LLGLDepthTest depth(GL_TRUE, i ? GL_TRUE : GL_FALSE,
								i ? GL_LEQUAL : GL_GREATER);
			LLGLState blend(GL_BLEND, i ? GL_FALSE : GL_TRUE);
			gGL.pushMatrix();
			for (objects_set_t::iterator it = sObjects.begin(),
										 end = sObjects.end();
				 it != end; ++it)
			{
				LLViewerObject* objp = it->get();
				if (objp->isDead() || objp->mGLTFAsset.isNull())
				{
					continue;
				}
				gGL.pushMatrix();
				gGL.multMatrix(objp->getGLTFAssetToAgentTransform().getF32ptr());

				Asset* assetp = objp->mGLTFAsset;
				LLMatrix4a mat = objp->getGLTFAssetToAgentTransform();
				mat.matMul(mat, gGLModelView);
				for (auto& node : assetp->mNodes)
				{
					gGL.pushMatrix();
					gGL.multMatrix(glm::value_ptr(node.mAssetMatrix));

					// Render x-axis red, y-axis green, z-axis blue
					gGL.color4f(1.f, 0.f, 0.f, 0.5f);
					gGL.begin(LLRender::LINES);
					gGL.vertex3f(0.f, 0.f, 0.f);
					gGL.vertex3f(1.f, 0.f, 0.f);
					gGL.end(true);
					gGL.color4f(0.f, 1.f, 0.f, 0.5f);
					gGL.begin(LLRender::LINES);
					gGL.vertex3f(0.f, 0.f, 0.f);
					gGL.vertex3f(0.f, 1.f, 0.f);
					gGL.end(true);
					gGL.color4f(0.f, 0.f, 1.f, 0.5f);
					gGL.begin(LLRender::LINES);
					gGL.vertex3f(0.f, 0.f, 0.f);
					gGL.vertex3f(0.f, 0.f, 1.f);
					gGL.end(true);

					// Render path to child nodes cyan
					gGL.color4f(0.f, 1.f, 1.f, 0.5f);
					gGL.begin(LLRender::LINES);
					for (auto& child_idx : node.mChildren)
					{
						Node& child = assetp->mNodes[child_idx];
						gGL.vertex3f(0.f, 0.f, 0.f);
						gGL.vertex3fv(glm::value_ptr(child.mMatrix[3]));
					}
					gGL.end(true);
					gGL.popMatrix();
				}
				gGL.popMatrix();
			}
			gGL.popMatrix();
		}
	}

	gGL.popMatrix();

	gDebugProgram.unbind();
}
