/**
 * @file llvlcomposition.h
 * @brief Viewer-side representation of a composition layer...
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewertexture.h"

class LLFetchedGLTFMaterial;
class LLGLTFMaterial;
class LLSurface;

// Viewer-side representation of a layer...
class LLViewerLayer
{
public:
	LLViewerLayer(S32 width, F32 scale = 1.f);
	virtual ~LLViewerLayer();

	F32 getValueScaled(F32 x, F32 y) const;

protected:
	LL_INLINE F32 getValue(S32 x, S32 y) const
	{
		return *(mDatap + x + y * mWidth);
	}

protected:
	F32*	mDatap;
	S32		mWidth;
	F32		mScale;
	F32		mScaleInv;
};

class LLTerrain
{
protected:
	LOG_CLASS(LLTerrain);

public:
	LLTerrain();
	virtual ~LLTerrain();

	enum EAssetCount : U32
	{
		ASSET_COUNT = 4
	};

	virtual void setDetailAssetID(U32 asset, const LLUUID& id);

	LL_INLINE const LLUUID& getDetailAssetID(U32 asset) const
	{
		return asset < ASSET_COUNT ? mAssetIds[asset] : LLUUID::null;
	}

	bool isPBR();

	bool texturesReady(bool boost_it, bool strict);
	// strict = true -> all materials must be sufficiently loaded
	// strict = false -> at least one material must be loaded
	bool materialsReady(bool boost_it, bool strict);

	LL_INLINE bool generateMaterials()
	{
		return texturesReady(true, true) || materialsReady(true, true);
	}

	void getTextures(std::vector<LLViewerTexture*>& textures);

	LL_INLINE LLViewerFetchedTexture* getFetchedTexture(U32 asset) const
	{
		return asset < ASSET_COUNT ? mDetailTextures[asset].get() : NULL;
	}

	void getGLTFMaterials(std::vector<LLGLTFMaterial*>& materials);

	LL_INLINE LLFetchedGLTFMaterial* getFetchedMaterial(U32 i) const
	{
		return i < ASSET_COUNT ? mDetailRenderMaterials[i].get() : NULL;
	}

	LL_INLINE const LLGLTFMaterial* getMaterialOverride(U32 i) const
	{
		return i < ASSET_COUNT ? mDetailMaterialOverrides[i].get() : NULL;
	}

	void setMaterialOverride(U32 asset, LLGLTFMaterial* mat_overridep);

	void apply(const LLTerrain* terrainp);

	void boostTextures();

	// *HACK: for PBR terrain support, these allow to avoid to log warnings
	// when it is "normal" that a texture or material corresponding to a
	// terrain asset Id is not found (since for each of our assets, either
	// a texture fetch or a PBR material asset fetch will invariably fail). HB
	static bool isAsset(const LLUUID& id);

protected:
	static bool textureReady(LLViewerFetchedTexture* texp, bool boost_it);
	static bool materialReady(LLFetchedGLTFMaterial* matp, bool& textures_set,
							  bool boost_it, bool strict);

private:
	static void addGlobalAsset(const LLUUID& id);
	static void removeGlobalAsset(const LLUUID& id);

protected:
	LLPointer<LLViewerFetchedTexture>	mDetailTextures[ASSET_COUNT];
	// Note: unlike mDetailRenderMaterials, the textures in this are not
	// guaranteed to be set or loaded after a true return from materialReady().
	LLPointer<LLFetchedGLTFMaterial>	mDetailMaterials[ASSET_COUNT];
	LLPointer<LLFetchedGLTFMaterial>	mDetailRenderMaterials[ASSET_COUNT];
	LLPointer<LLGLTFMaterial>			mDetailMaterialOverrides[ASSET_COUNT];
	LLUUID								mAssetIds[ASSET_COUNT];
	U32									mTerrainType;
};

class LLVLComposition final : public LLTerrain, public LLViewerLayer
{
	friend class LLVOSurfacePatch;

protected:
	LOG_CLASS(LLVLComposition);

public:
	LLVLComposition(LLSurface* surfacep, U32 width, F32 scale);

	LL_INLINE void setSurface(LLSurface* s)		{ mSurfacep = s; }

	void forceRebuild();

	// Viewer side hack to generate composition values
	bool generateHeights(F32 x, F32 y, F32 width, F32 height);

	bool generateComposition();

	// GenerateS texture from composition values.
	bool generateLandTile(F32 x, F32 y, F32 width, F32 height);

	LL_INLINE LLViewerFetchedTexture* getDetailTexture(U32 terrain)
	{
		return mDetailTextures[terrain];
	}

	void setDetailAssetID(U32 asset, const LLUUID& id) override;

	LL_INLINE void setParamsReady()				{ mParamsReady = true; }
	LL_INLINE bool getParamsReady() const		{ return mParamsReady; }

	// Use these as indices into the get/setters below
	enum ECorner : U32
	{
		SOUTHWEST = 0,
		SOUTHEAST = 1,
		NORTHWEST = 2,
		NORTHEAST = 3,
		CORNER_COUNT = 4
	};

	LL_INLINE F32 getStartHeight(U32 c)			{ return mStartHeight[c]; }
	LL_INLINE void setStartHeight(U32 c, F32 h)	{ mStartHeight[c] = h; }

	LL_INLINE F32 getHeightRange(U32 c)			{ return mHeightRange[c]; }
	LL_INLINE void setHeightRange(U32 c, F32 r)	{ mHeightRange[c] = r; }

protected:
	LLSurface*							mSurfacep;

	LLPointer<LLImageRaw>				mRawImages[ASSET_COUNT];

	F32									mStartHeight[CORNER_COUNT];
	F32									mHeightRange[CORNER_COUNT];

	F32									mTexScaleX;
	F32									mTexScaleY;

	bool								mParamsReady;
	bool								mTexturesLoaded;
};
