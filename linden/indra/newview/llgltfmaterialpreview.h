/**
 * @file llgltfmaterialpreview.h
 * @brief The LLAppViewer class definitions
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 *
 * Copyright (c) 2023, Linden Research, Inc.
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

#include "lldynamictexture.h"
#include "llfetchedgltfmaterial.h"

class LLGLTFPreviewTexture final : public LLViewerDynamicTexture
{
protected:
	// Use getPreview()
	LLGLTFPreviewTexture(LLFetchedGLTFMaterial* matp, S32 width = 512);

public:
	// Returns NULL if the material is not loaded yet. Note: the texture should
	// be cached if the same material is being previewed.
	static LLPointer<LLViewerTexture> getPreview(LLFetchedGLTFMaterial* matp);

	bool needsRender() override;
	void preRender(bool clear_depth = true) override;
	bool render() override;
	void postRender(bool success) override;

	class MaterialLoadLevels
	{
	public:
		MaterialLoadLevels();

		bool isFullyLoaded() const;

		LL_INLINE S32& operator[](size_t i)				{ return mLevels[i]; }
		LL_INLINE const S32& operator[](size_t i) const	{ return mLevels[i]; }

		// Less is better. Returns false if lhs is not strictly less or equal
		// for all levels.
		bool operator<(const MaterialLoadLevels& other) const;

		// Less is better. Returns false if lhs is not strictly greater or
		// equal for all levels
		bool operator>(const MaterialLoadLevels& other) const;

	public:
		S32 mLevels[LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT];
	};

private:
	LLPointer<LLFetchedGLTFMaterial>	mGLTFMaterial;
	MaterialLoadLevels					mBestLoad;
	bool								mShouldRender;
};
