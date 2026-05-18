/**
 * @file llfetchedgltfmaterial.h
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

#include <functional>

#include "hbfastset.h"
#include "llgltfmaterial.h"
#include "llpointer.h"

class LLTerrain;
class LLViewerTexture;
class LLViewerFetchedTexture;

class LLFetchedGLTFMaterial : public LLGLTFMaterial
{
	friend class LLGLTFMaterialList;
	friend class LLTerrain;

protected:
	LOG_CLASS(LLFetchedGLTFMaterial);

public:
	LL_INLINE LLFetchedGLTFMaterial()
	:	mExpectedFlushTime(0.f),
		mActive(true),
		mFetching(false),
		mFetchSuccess(false)
	{
	}

	LLFetchedGLTFMaterial& operator=(const LLFetchedGLTFMaterial& rhs);
	bool operator==(const LLGLTFMaterial& rhs) const = delete;

	LLFetchedGLTFMaterial* asFetched() override	{ return this; }

	void clearFetchedTextures();

	void onMaterialComplete(std::function<void()> mat_complete_callback);

	// Bind this material for rendering. media_texp is an optional media
	// texture that may override the base color texture.
	void bind(LLViewerTexture* media_texp, F32 vsize);

	LL_INLINE bool isFetching() const			{ return mFetching; }

	LL_INLINE bool isLoaded() const				{ return !mFetching && mFetchSuccess; }

	void addTextureEntry(LLTextureEntry* tep) override;
	void removeTextureEntry(LLTextureEntry* tep) override;
	bool replaceLocalTexture(const LLUUID& tracking_id, const LLUUID& old_id,
							 const LLUUID& new_id) override;
	void updateTextureTracking() override;

	typedef fast_hset<LLTextureEntry*> te_list_t;
	LL_INLINE const te_list_t& getTexEntries() const
	{
		return mTextureEntries;
	}

	// Handy methods to easily setup a material preview. HB
	LLViewerTexture* getPreview();
	LL_INLINE void clearPreview()				{ mPreview = NULL; }

protected:
	// Lifetime management
	LL_INLINE void materialBegin()				{ mFetching = true; }
	void materialComplete(bool success);

public:
	// Textures used for fetching/rendering
	LLPointer<LLViewerFetchedTexture>	mBaseColorTexture;
	LLPointer<LLViewerFetchedTexture>	mNormalTexture;
	LLPointer<LLViewerFetchedTexture>	mMetallicRoughnessTexture;
	LLPointer<LLViewerFetchedTexture>	mEmissiveTexture;

protected:
	te_list_t							mTextureEntries;
	LLPointer<LLViewerTexture>			mPreview;

	// Lifetime management
	std::vector<std::function<void()> >	mCompleteCallbacks;
	F32									mExpectedFlushTime;
	bool								mActive;
	bool								mFetching;
	bool								mFetchSuccess;
};
