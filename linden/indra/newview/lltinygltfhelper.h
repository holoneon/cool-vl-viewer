/**
 * @file lltinygltfhelper.h
 * @brief The LLTinyGLTFHelper class declaration
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

#pragma once

#include "tinygltf/tiny_gltf.h"

#include "llgltfmaterial.h"
#include "llpointer.h"

#include "llgltfmateriallist.h"

class LLImageRaw;
class LLViewerFetchedTexture;

// Purely static class
class LLTinyGLTFHelper final
{
protected:
	LOG_CLASS(LLTinyGLTFHelper);

public:
	LLTinyGLTFHelper() = delete;
	~LLTinyGLTFHelper() = delete;

	static LLColor4 getColor(const std::vector<double>& in);
	static const tinygltf::Image* getImageFromTextureIndex(const tinygltf::Model& m,
														   S32 teX_idx);
	static LLImageRaw* getTexture(const std::string& folder,
								  const tinygltf::Model& model, S32 tex_idx,
								  std::string& name, bool flip = true);
	static LLImageRaw* getTexture(const std::string& folder,
								  const tinygltf::Model& model, S32 tex_idx,
								  bool flip = true);

	static bool loadModel(const std::string& filename,
						  tinygltf::Model& model_out);

	static bool getMaterialFromModel(const std::string& filename,
									 const tinygltf::Model& model, S32 mat_idx,
									 LLFetchedGLTFMaterial* materialp,
									 std::string& mat_name, bool flip = true);

	static void initFetchedTextures(tinygltf::Material& materialp,
									LLPointer<LLImageRaw>& basecolor_imgp,
									LLPointer<LLImageRaw>& normal_imgp,
									LLPointer<LLImageRaw>& mr_imgp,
									LLPointer<LLImageRaw>& emissive_imgp,
									LLPointer<LLImageRaw>& occlusion_imgp,
									LLPointer<LLViewerFetchedTexture>& basecolorp,
									LLPointer<LLViewerFetchedTexture>& normalp,
									LLPointer<LLViewerFetchedTexture>& mrp,
									LLPointer<LLViewerFetchedTexture>& emissivep);
};
