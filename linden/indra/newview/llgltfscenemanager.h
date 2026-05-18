/**
 * @file llgltfscenemanager.h
 * @brief LLGLTFSceneManager class declaration.
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

#pragma once

#include "llassettype.h"
#include "llextendedstatus.h"
#include "hbfastset.h"
#include "hbfastmap.h"
#include "llgltfasset.h"

class LLDrawable;
class LLVector2;
class LLVector4a;
class LLViewerObject;
class LLViewerFetchedTexture;
class LLVOVolume;

// Purely static class (instead of a LLSimpleton in LL's code). HB
class LLGLTFSceneManager
{
protected:
	LOG_CLASS(LLGLTFSceneManager);

public:
	LLGLTFSceneManager() = delete;
	~LLGLTFSceneManager() = delete;

	// Called at viewer initialization from LLAppViewer::initWindow().
	// Initializes the newview function hooks needed by the llgltf library. HB
	static void init();

	// Called on viewer shutdown, before clearing objects list, from
	// LLAppViewer::cleanup(). HB
	static void cleanup();

	// The two following methods return an empty string on success, or the
	// reason for a failure. HB
	static std::string load(const std::string& filename,
							const LLUUID& handle_obj_id);
	static std::string save(const std::string& filename,
							const LLUUID& handle_obj_id);

	static std::string upload(const LLUUID& handle_obj_id);

	LL_INLINE static bool isUploading()	{ return sUploadingAsset.notNull(); }

	static bool lineSegmentIntersect(LLVOVolume* volp, LLGLTF::Asset* assetp,
									 const LLVector4a& start,
									 const LLVector4a& end, S32 face,
									 bool pick_transparent, bool pick_rigged,
									 S32* node_hitp, S32* prim_hitp,
									 LLVector4a* interp,LLVector2* tcoordp,
									 LLVector4a* normp, LLVector4a* tgtp);

	static LLDrawable* lineSegmentIntersect(const LLVector4a& start,
											const LLVector4a& end,
											bool pick_transparent,
											bool pick_rigged, S32* node_hitp,
											S32* prim_hitp, LLVector4a* interp,
											LLVector2* tcoordp,
											LLVector4a* normp,
											LLVector4a* tgtp);

	static void update();

	static void render(bool opaque, bool rigged = false, bool unlit = false);

	LL_INLINE static void renderOpaque(bool rigged = false)
	{
		render(true, rigged);
	}

	LL_INLINE static void renderAlpha(bool rigged = false)
	{
		render(false, rigged);
	}

	static void renderDebug();

	static void addGLTFObject(LLViewerObject* objp, const LLUUID& gltf_id);

private:
	static void onGLTFLoadComplete(const LLUUID& id,
								   LLAssetType::EType asset_type,
								   void* user_data, S32 status,
								   LLExtStat ext_status);
	static void onGLTFBinLoadComplete(const LLUUID& id,
									  LLAssetType::EType asset_type,
									  void* user_data, S32 status,
									  LLExtStat ext_status);

	enum TextureType : U8
	{
		BASE_COLOR = 0,
		NORMAL,
		METALLIC_ROUGHNESS,
		OCCLUSION,
		EMISSIVE,
		TEXTURE_TYPE_COUNT
	};

	static void renderVariant(U8 variant);
	static void renderAsset(LLGLTF::Asset& asset, U8 variant);
	static void bindTexture(LLGLTF::Asset& asset, U8 type,
							LLGLTF::TextureInfo& info,
							LLViewerFetchedTexture* fallback_texp);
	static void bind(LLGLTF::Asset& asset, LLGLTF::Material& mat);

private:
	typedef fast_hset<LLPointer<LLViewerObject> > objects_set_t;
	static objects_set_t			 sObjects;

	typedef fast_hmap<LLPointer<LLViewerObject>, LLUUID> objects_map_t;
	static objects_map_t			 sPendingObjects;

	static S32						 sLastTexture[TEXTURE_TYPE_COUNT];

	// glTF asset upload
	static LLPointer<LLViewerObject> sUploadingObject;
	static LLPointer<LLGLTF::Asset>	 sUploadingAsset;
	static S32						 sPendingImageUploads;
	static S32						 sPendingBinaryUploads;
	static bool						 sPendingGLTFUpload;
};
