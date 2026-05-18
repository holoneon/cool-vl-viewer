/**
 * @file llvlcomposition.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llvlcomposition.h"

#include "imageids.h"
#include "llmutex.h"
#include "llnoise.h"
#include "llregionhandle.h"			// For from_region_handle()

#include "llgltfmateriallist.h"
#include "llstartup.h"
#include "llsurface.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"

constexpr S32 BASE_SIZE = 128;

// Helper functions

// Not sure if this is the right math... Takes the weighted average of all four
// points (bilinear interpolation)
static F32 bilinear(F32 v00, F32 v01, F32 v10, F32 v11, F32 x_frac, F32 y_frac)
{
	F32 inv_x_frac = 1.f - x_frac;
	F32 inv_y_frac = 1.f - y_frac;
	return inv_x_frac * inv_y_frac * v00 + x_frac * inv_y_frac * v10 +
		   inv_x_frac * y_frac * v01 + x_frac * y_frac * v11;
}

static void boost_terrain_texture(LLViewerFetchedTexture* texp,
								  bool mega_texture = true)
{
	constexpr F32 area_1k = 1024.f * 1024.f;
	constexpr F32 area_2k = 2048.f * 2048.f;
	if (texp)
	{
		texp->setBoostLevel(LLGLTexture::BOOST_TERRAIN);
		if (mega_texture)
		{
			texp->addTextureStats(area_2k);
			texp->setMegaTexture(true);
		}
		else
		{
			texp->addTextureStats(area_1k);
		}
	}
}

static void unboost_terrain_texture(LLViewerFetchedTexture* texp)
{
	if (texp)
	{
		texp->setBoostLevel(LLGLTexture::BOOST_NONE);
		texp->setMinDiscardLevel(MAX_DISCARD_LEVEL + 1);
		texp->forceActive();
	}
}

static void boost_terrain_material(LLFetchedGLTFMaterial* matp)
{
	if (matp)
	{
		boost_terrain_texture(matp->mBaseColorTexture);
		boost_terrain_texture(matp->mNormalTexture);
		boost_terrain_texture(matp->mMetallicRoughnessTexture);
		// Do not set emissive as a mega texture. HB
		boost_terrain_texture(matp->mEmissiveTexture, false);
	}
}

static void unboost_terrain_material(LLFetchedGLTFMaterial* matp)
{
	if (matp)
	{
		unboost_terrain_texture(matp->mBaseColorTexture);
		unboost_terrain_texture(matp->mNormalTexture);
		unboost_terrain_texture(matp->mMetallicRoughnessTexture);
		unboost_terrain_texture(matp->mEmissiveTexture);
	}
}

static LLPointer<LLViewerFetchedTexture> fetch_terrain_tex(const LLUUID& id)
{
    if (id.isNull())
    {
        return nullptr;
    }

    LLPointer<LLViewerFetchedTexture> texp =
		LLViewerTextureManager::getFetchedTexture(id);
	// Non-loading mini-map textures fixes follow...
	// We need the maximum resolution (lowest discard level) to avoid partly
	// loaded textures that would never complete (probably a race condition
	// issue in the fetcher, with loading textures and changing the discard
	// level while they load): the textures will get appropriately discarded
	// anyway, once the composition will have been created from them. HB.
	texp->setMinDiscardLevel(0);
	// We also need to give this kind of textures the highest (and appropriate)
	// priority from the start... HB.
	texp->setBoostLevel(LLGLTexture::BOOST_TERRAIN);
#if !LL_IMPLICIT_SETNODELETE
	texp->setNoDelete();
#endif
    return texp;
}

///////////////////////////////////////////////////////////////////////////////
// LLViewerLayer class
///////////////////////////////////////////////////////////////////////////////

LLViewerLayer::LLViewerLayer(S32 width, F32 scale)
:	mWidth(width),
	mScale(scale),
	mScaleInv(1.f / scale)
{
	mDatap = new F32[width * width];
	for (S32 i = 0; i < width * width; ++i)
	{
		*(mDatap + i) = 0.f;
	}
}

LLViewerLayer::~LLViewerLayer()
{
	delete[] mDatap;
	mDatap = NULL;
}

F32 LLViewerLayer::getValueScaled(F32 x, F32 y) const
{
	F32 x_frac = x * mScaleInv;
	S32 x1 = llfloor(x_frac);
	S32 x2 = x1 + 1;
	x_frac -= x1;

	F32 y_frac = y * mScaleInv;
	S32 y1 = llfloor(y_frac);
	S32 y2 = y1 + 1;
	y_frac -= y1;

	S32 max = mWidth - 1;
	x1 = llclamp(x1, 0, max);
	x2 = llclamp(x2, 0, max);
	y1 = llclamp(y1, 0, max);
	y2 = llclamp(y2, 0, max);

	// Take weighted average of all four points (bilinear interpolation)
	S32 row1 = y1 * mWidth;
	S32 row2 = y2 * mWidth;

	// Access in squential order in memory, and do not use immediately.
	F32 row1_left  = mDatap[row1 + x1];
	F32 row1_right = mDatap[row1 + x2];
	F32 row2_left  = mDatap[row2 + x1];
	F32 row2_right = mDatap[row2 + x2];

	F32 row1_interp = row1_left - x_frac * (row1_left - row1_right);
	F32 row2_interp = row2_left - x_frac * (row2_left - row2_right);

	return row1_interp - y_frac * (row1_interp - row2_interp);
}

///////////////////////////////////////////////////////////////////////////////
// LLTerrain class
///////////////////////////////////////////////////////////////////////////////

// We need a mutex to protect sGlobalAssets since while the latter is modified
// only from the main thread, it is queried from the main thread and worker
// threads. HB
static LLMutex sAssetListMutex;
// Since the same asset Id can be used in multiple regions, we must count the
// number of times we have it in use to know when we can remove that Id from
// the global list, thus the map... HB
static fast_hmap<LLUUID, S32> sGlobalAssets;
// *HACK: on login in main SL land, sim servers send a first set of terrain
// textures Ids (estate terrains, perhaps ?) inside an inital "RegionHandshake"
// message for the agent region in STATE_WORLD_INIT, then a second identical
// message in STATE_AGENT_WAIT, but with the actual terrain assets... It means
// that the fetch for the first set of assets will have started (and will fail
// for the glTF fetcher, since these are texture assets), but that the fetch
// may not have yet completed when receiving the new set of Ids, at which point
// we would remove the old Ids from our sGlobalAssets map, causing the glTF
// material assets fetcher to warn on fetch completion failure about missing
// assets while this was expected and that this pointless warning was supposed
// to be avoided... So, let's use a second list, for early asset fetches !  HB
static uuid_list_t sEarlyAssets;

//static
void LLTerrain::addGlobalAsset(const LLUUID& id)
{
	if (id.isNull())
	{
		return;
	}
	sAssetListMutex.lock();
	fast_hmap<LLUUID, S32>::iterator it = sGlobalAssets.find(id);
	if (it == sGlobalAssets.end())
	{
		LL_DEBUGS("RegionTexture") << "Registering new terrain Id: " << id
								   << LL_ENDL;
		sGlobalAssets.emplace(id, 1);
	}
	else
	{
		++(it->second);
		LL_DEBUGS("RegionTexture") << "Incremented usage (" << it->second
								   <<") for terrain Id: " << id << LL_ENDL;
	}
	// *HACK: see the above comment for sEarlyAssets. HB
	if (LLStartUp::getStartupState() < EStartupState::STATE_AGENT_WAIT)
	{
		LL_DEBUGS("RegionTexture") << "Registering early terrain Id: " << id
								   << LL_ENDL;
		sEarlyAssets.emplace(id);
	}
	sAssetListMutex.unlock();
}

//static
void LLTerrain::removeGlobalAsset(const LLUUID& id)
{
	if (id.isNull())
	{
		return;
	}
	sAssetListMutex.lock();
	fast_hmap<LLUUID, S32>::iterator it = sGlobalAssets.find(id);
	if (it != sGlobalAssets.end())
	{
		if (--(it->second) <= 0)
		{
			LL_DEBUGS("RegionTexture") << "Removing terrain Id: " << id
									   << LL_ENDL;
			sGlobalAssets.erase(it);
		}
	}
	sAssetListMutex.unlock();
}

//static
bool LLTerrain::isAsset(const LLUUID& id)
{
	sAssetListMutex.lock();
	bool in_list = sGlobalAssets.count(id);
#if 0	// Ideally, we should clear this easly assets list, after "a while",
		// but we cannot know, a priori, how long "a while" will actually
		// take; it depends on the asset fetch duration, and has been seen
		// happening even some time after login has completed, so... HB
	if (LLStartUp::isLoggedIn())
	{
		sEarlyAssets.clear();
	}
	else
#endif
	if (!in_list)
	{
		in_list = sEarlyAssets.count(id);
		if (in_list)
		{
			LL_DEBUGS("RegionTexture") << "Early terrain Id recognized: " << id
									   << LL_ENDL;
		}
	}
	sAssetListMutex.unlock();
	return in_list;
}

LLTerrain::LLTerrain()
:	mTerrainType(0)
{
}

//virtual
LLTerrain::~LLTerrain()
{
	for (U32 i = 0; i < ASSET_COUNT; ++i)
	{
		unboost_terrain_texture(mDetailTextures[i]);
		unboost_terrain_material(mDetailRenderMaterials[i]);
		removeGlobalAsset(mAssetIds[i]);
	}
}

//virtual
void LLTerrain::setDetailAssetID(U32 asset, const LLUUID& id)
{
	if (asset >= ASSET_COUNT)
	{
		llassert(false);
		return;
	}

	if (mAssetIds[asset] == id)
	{
		return;	// Nothing to do !  HB
	}

	// This must be reset, in case the terrain would have changed from textures
	// to PBR materials or vice versa. The next call to isPBR() will resync it
	// accordingly. HB
	mTerrainType = 0;

	// Tell the texture and material fetchers that we do not know, a priori,
	// whether this asset Id corresponds to a texture or a material, and that
	// the fetch may therefore fail (and will indeed fail for one of the two
	// fetchers)... So dirty !  I wish LL had added a flag in the RegionInfo
	// UDP message block to indicate a PBR material terrain type... HB
	addGlobalAsset(id);
	// And unregister any old asset Id (will be ignored if a null UUID).
	removeGlobalAsset(mAssetIds[asset]);

	// Keep track of our new asset Id.
	mAssetIds[asset] = id;

	mDetailTextures[asset] = fetch_terrain_tex(id);
	if (id.isNull())
	{
		mDetailMaterials[asset] = NULL;
	}
	else
	{
		mDetailMaterials[asset] = gGLTFMaterialList.getMaterial(id);
	}
	mDetailRenderMaterials[asset] = NULL;
}

void LLTerrain::setMaterialOverride(U32 asset, LLGLTFMaterial* mat_overridep)
{
	if (asset < ASSET_COUNT)
	{
		mDetailMaterialOverrides[asset] = mat_overridep;
		mDetailRenderMaterials[asset] = NULL;
	}
}

void LLTerrain::apply(const LLTerrain* terrainp)
{
	if (!terrainp) return;	// Paranoia

	for (U32 i = 0; i < ASSET_COUNT; ++i)
	{
		const LLGLTFMaterial* matp = terrainp->getMaterialOverride(i);
		if (matp)
		{
			LLGLTFMaterial* omatp = new LLGLTFMaterial(*matp);
			setMaterialOverride(i, omatp);
		}
		else
		{
			setMaterialOverride(i, NULL);
		}
	}
}

bool LLTerrain::isPBR()
{
	constexpr U32 TT_TEXTURES = 1;
	constexpr U32 TT_MATERIALS = 2;

	if (mTerrainType)
	{
		return mTerrainType == TT_MATERIALS;
	}
	if (texturesReady(false, false))
	{
		mTerrainType = TT_TEXTURES;
		return false;
	}
	if (materialsReady(false, false))
	{
		mTerrainType = TT_MATERIALS;
		return true;
	}

	return false;
}

bool LLTerrain::texturesReady(bool boost_it, bool strict)
{
	bool one_ready = false;
	bool all_ready = true;
	// Note: calls to textureReady may boost textures. Do not early-return.
	for (U32 i = 0; i < ASSET_COUNT; i++)
	{
		if (mDetailTextures[i].notNull() &&
			textureReady(mDetailTextures[i], boost_it))
		{
			one_ready = true;
		}
		else
		{
			all_ready = false;
		}
	}
	return strict ? all_ready : one_ready;
}

bool LLTerrain::materialsReady(bool boost_it, bool strict)
{
	bool one_ready = false;
	bool all_ready = true;
	// Note: calls to textureReady may boost textures. Do not early-return.
	for (U32 i = 0; i < ASSET_COUNT; i++)
	{
		LLFetchedGLTFMaterial* matp = mDetailMaterials[i].get();
		if (!matp || !matp->isLoaded())
		{
			all_ready = false;
			continue;
		}
		bool render_material_textures_set = true;
		LLFetchedGLTFMaterial* rmatp = mDetailRenderMaterials[i].get();
		if (!rmatp)
		{
			render_material_textures_set = false;
			mDetailRenderMaterials[i] = rmatp = new LLFetchedGLTFMaterial();
			*rmatp = *matp;
			rmatp->materialBegin();
			rmatp->materialComplete(true);
			LLGLTFMaterial* omatp = mDetailMaterialOverrides[i].get();
			if (omatp)
			{
				rmatp->applyOverride(*omatp);
			}
		}
		if (materialReady(rmatp, render_material_textures_set, boost_it, 
						  strict))
		{
			one_ready = true;
		}
		else
		{
			all_ready = false;
		}
	}
	return strict ? all_ready : one_ready;
}

//static
bool LLTerrain::textureReady(LLViewerFetchedTexture* texp, bool boost_it)
{
	if (!texp || texp->getComponents() == 0)
	{
		return false;
	}

	S32 discard = texp->getDiscardLevel();
	if (discard < 0)
	{
		if (boost_it)
		{
			texp->setBoostLevel(LLGLTexture::BOOST_TERRAIN);
			texp->addTextureStats(BASE_SIZE * BASE_SIZE);
		}
		return false;
	}
	if (discard != 0 &&
		(texp->getWidth() < BASE_SIZE || texp->getHeight() < BASE_SIZE))
	{
		if (boost_it)
		{
			texp->setBoostLevel(LLGLTexture::BOOST_TERRAIN);

			S32 min_dim = llmin(texp->getFullWidth(), texp->getFullHeight());
			S32 ddiscard = 0;
			while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
			{
				++ddiscard;
				min_dim /= 2;
			}
			texp->setBoostLevel(LLGLTexture::BOOST_TERRAIN);
			texp->setMinDiscardLevel(ddiscard);
			texp->addTextureStats(BASE_SIZE * BASE_SIZE);
		}

		return false;
	}

	return true;
}

//static
bool LLTerrain::materialReady(LLFetchedGLTFMaterial* matp, bool& textures_set,
							  bool boost_it, bool strict)
{
	// Material is loaded, but textures may not be
	if (!textures_set)
	{
		textures_set = true;
		// Note: these can sometimes be set to to NULL due to what happens in
		// updateTEMaterialTextures(). For the sake of robustness, we emulate
		// that fetching behavior by setting null UUID textures to NULL.
		const LLGLTFMaterial::uuid_array_t& texids = matp->mTextureId;
		matp->mBaseColorTexture = fetch_terrain_tex(texids[BASECOLIDX]);
		matp->mNormalTexture = fetch_terrain_tex(texids[NORMALIDX]);
		matp->mMetallicRoughnessTexture = fetch_terrain_tex(texids[MROUGHIDX]);
		matp->mEmissiveTexture = fetch_terrain_tex(texids[EMISSIVEIDX]);
	}

	// Note: calls to textureReady may boost textures; do not early-return.
	bool all_ready = true;
	if (matp->mTextureId[BASECOLIDX].notNull() &&
		!textureReady(matp->mBaseColorTexture, boost_it))
	{
		all_ready = false;
	}
	if (matp->mTextureId[NORMALIDX].notNull() &&
		!textureReady(matp->mNormalTexture, boost_it))
	{
		all_ready = false;
	}
	if (matp->mTextureId[MROUGHIDX].notNull() &&
		!textureReady(matp->mMetallicRoughnessTexture, boost_it))
	{
		all_ready = false;
	}
	if (matp->mTextureId[EMISSIVEIDX].notNull() &&
		!textureReady(matp->mEmissiveTexture, boost_it))
	{
		all_ready = false;
	}
	return all_ready || !strict;
}

void LLTerrain::getTextures(std::vector<LLViewerTexture*>& textures)
{
	textures.resize(ASSET_COUNT);
	if (isPBR())
	{
		LLViewerTexture* default_texp = LLViewerFetchedTexture::sDefaultImagep;
		for (U32 i = 0; i < ASSET_COUNT; ++i)
		{
			LLFetchedGLTFMaterial* matp = mDetailRenderMaterials[i].get();
			textures[i] = matp ? matp->mBaseColorTexture : default_texp;
			// It may happen there is not even a base color texture... HB
			if (!textures[i])
			{
				textures[i] = default_texp;
			}
		}
	}
	else
	{
		for (U32 i = 0; i < ASSET_COUNT; ++i)
		{
			textures[i] = mDetailTextures[i];
		}
	}
}

void LLTerrain::getGLTFMaterials(std::vector<LLGLTFMaterial*>& materials)
{
	static LLGLTFMaterial default_mat(LLGLTFMaterial::sDefault);
	materials.resize(ASSET_COUNT);
	for (U32 i = 0; i < ASSET_COUNT; ++i)
	{
		LLGLTFMaterial* matp = mDetailRenderMaterials[i].get();
		if (matp)
		{
			materials[i] = matp;
		}
		else
		{
			materials[i] = &default_mat;
		}
	}	
}

void LLTerrain::boostTextures()
{
	if (isPBR())
	{
		for (U32 i = 0; i < ASSET_COUNT; ++i)
		{
			boost_terrain_material(mDetailRenderMaterials[i].get());
		}
	}
	else
	{
		for (U32 i = 0; i < ASSET_COUNT; ++i)
		{
			boost_terrain_texture(mDetailTextures[i].get());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLVLComposition class
///////////////////////////////////////////////////////////////////////////////

LLVLComposition::LLVLComposition(LLSurface* surfacep, U32 width, F32 scale)
:	LLTerrain(),
	LLViewerLayer(width, scale),
	mSurfacep(surfacep),
	mTexScaleX(16.f),
	mTexScaleY(16.f),
	mParamsReady(false)
{
	// Load terrain textures - Original ones
	static const LLUUID default_textures[ASSET_COUNT] =
	{
		TERRAIN_DIRT_DETAIL,
		TERRAIN_GRASS_DETAIL,
		TERRAIN_MOUNTAIN_DETAIL,
		TERRAIN_ROCK_DETAIL
	};
	for (U32 i = 0; i < ASSET_COUNT; ++i)
	{
		setDetailAssetID(i, default_textures[i]);
	}

	static LLCachedControl<F32> color_start_height(gSavedSettings,
												   "TerrainColorStartHeight");
	static LLCachedControl<F32> color_height_range(gSavedSettings,
												   "TerrainColorHeightRange");
	F32 start_height = llmax(0.f, color_start_height);
	F32 height_range = llmax(0.f, color_height_range);
	// Initialize the texture matrix to defaults.
	for (U32 i = 0; i < CORNER_COUNT; ++i)
	{
		mStartHeight[i] = start_height;
		mHeightRange[i] = height_range;
	}
}

//virtual
void LLVLComposition::setDetailAssetID(U32 asset, const LLUUID& id)
{
	if (id.notNull())
	{
		mRawImages[asset] = NULL;
		LLTerrain::setDetailAssetID(asset, id);
	}
}

void LLVLComposition::forceRebuild()
{
	if (isPBR())
	{
		for (U32 i = 0; i < ASSET_COUNT; ++i)
		{
			LLFetchedGLTFMaterial* matp = mDetailRenderMaterials[i].get();
			if (!matp)
			{
				continue;
			}
			LLViewerFetchedTexture* texp = matp->mBaseColorTexture.get();
			if (texp)
			{
				texp->forceRefetch();
			}
			texp = matp->mNormalTexture.get();
			if (texp)
			{
				texp->forceRefetch();
			}
			texp = matp->mMetallicRoughnessTexture.get();
			if (texp)
			{
				texp->forceRefetch();
			}
			texp = matp->mEmissiveTexture.get();
			if (texp)
			{
				texp->forceRefetch();
			}
		}
	}
	else
	{
		for (U32 i = 0; i < ASSET_COUNT; ++i)
		{
			LLViewerFetchedTexture* texp = mDetailTextures[i];
			if (texp)
			{
				texp->forceRefetch();
			}
		}
	}
}

bool LLVLComposition::generateHeights(F32 x, F32 y, F32 width, F32 height)
{
	if (!mParamsReady)
	{
		// All the parameters have not been set yet (we did not get the message
		// from the sim)
		return false;
	}

	if (!mSurfacep || !mSurfacep->getRegion())
	{
		llassert(mSurfacep);
		// We do not always have the region yet here....
		return false;
	}

	S32 x_begin, y_begin, x_end, y_end;

	x_begin = ll_round(x * mScaleInv);
	y_begin = ll_round(y * mScaleInv);
	x_end = ll_round((x + width) * mScaleInv);
	y_end = ll_round((y + width) * mScaleInv);

	if (x_end > mWidth)
	{
		x_end = mWidth;
	}
	if (y_end > mWidth)
	{
		y_end = mWidth;
	}

	LLVector3d origin_global =
		from_region_handle(mSurfacep->getRegion()->getHandle());

	// For perlin noise generation...
	constexpr F32 slope_squared = 1.5f * 1.5f;

	// Degree to which noise modulates composition layer (versus simple height)
	constexpr F32 noise_magnitude = 2.f;

	// Heights map into textures as 0-1 = first, 1-2 = second, etc.
	// So we need to compress heights into this range.
	constexpr S32 NUM_TEXTURES = 4;

	constexpr F32 xy_scale_inv = 1.f / 4.9215f;
	constexpr F32 z_scale_inv = 1.f / 4.f;
	const F32 inv_width = 1.f / (F32)mWidth;

	// OK, for now, just have the composition value equal the height at the
	// point
	for (S32 j = y_begin; j < y_end; ++j)
	{
		for (S32 i = x_begin; i < x_end; ++i)
		{

			F32 vec[3];
			F32 vec1[3];
			F32 twiddle;

			// Bilinearly interpolate the start height and height range of the
			// textures
			F32 start_height = bilinear(mStartHeight[SOUTHWEST],
										mStartHeight[SOUTHEAST],
										mStartHeight[NORTHWEST],
										mStartHeight[NORTHEAST],
										// These will be bilinearly interpolated
										i * inv_width, j * inv_width);
			F32 height_range = bilinear(mHeightRange[SOUTHWEST],
										mHeightRange[SOUTHEAST],
										mHeightRange[NORTHWEST],
										mHeightRange[NORTHEAST],
										// These will be bilinearly interpolated
										i * inv_width, j * inv_width);

			LLVector3 location(i * mScale, j * mScale, 0.f);

			F32 height = mSurfacep->resolveHeightRegion(location);

			// Step 0: Measure the exact height at this texel
			//  Adjust to non-integer lattice
			vec[0] = (F32)(origin_global.mdV[VX] + location.mV[VX]) *
					 xy_scale_inv;
			vec[1] = (F32)(origin_global.mdV[VY] + location.mV[VY]) *
					 xy_scale_inv;
			vec[2] = height * z_scale_inv;

			// Choose material value by adding to the exact height a random
			// value

			vec1[0] = vec[0] * 0.2222222222f;
			vec1[1] = vec[1] * 0.2222222222f;
			vec1[2] = vec[2] * 0.2222222222f;

			// Low freq component for large divisions
			twiddle = noise2(vec1) * 6.5f;

			// High frequency component
			twiddle += turbulence2(vec, 2) * slope_squared;

			twiddle *= noise_magnitude;

			F32 scaled_noisy_height = (height + twiddle - start_height) *
									   F32(NUM_TEXTURES) / height_range;

			scaled_noisy_height = llmax(0.f, scaled_noisy_height);
			scaled_noisy_height = llmin(3.f, scaled_noisy_height);
			*(mDatap + i + j * mWidth) = scaled_noisy_height;
		}
	}
	return true;
}

bool LLVLComposition::generateComposition()
{
	return mParamsReady && LLTerrain::generateMaterials();
}

static bool set_desired_discard(LLViewerFetchedTexture* texp,
								S32& ddiscard, bool& delete_raw)
{
	ddiscard = 0;
	S32 width = texp->getFullWidth();
	S32 height = texp->getFullHeight();
	S32 min_dim = llmin(width, height);
	while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
	{
		++ddiscard;
		min_dim /= 2;
	}

	// Read back a raw image for this discard level, if it exists
	delete_raw = texp->reloadRawImage(ddiscard) != NULL;
	S32 cur_discard = texp->getRawImageLevel();
	if ((width == height && cur_discard > ddiscard) ||
		// *FIXME: for some reason, rectangular textures always get stuck one
		// discard level too high... HB
		(width != height && cur_discard > ddiscard + 1))
	{
		// Raw image is not detailed enough...
		LL_DEBUGS("RegionTexture") << "Cached raw data for terrain detail texture is not ready yet: "
								   << texp->getID()
								   << " - Discard level: " << cur_discard
								   << " - Desired discard level: " << ddiscard
								   << " - Full size: " << texp->getFullWidth()
								   << "x" << texp->getFullHeight()
								   << " - Current size: " << texp->getWidth()
								   << "x" << texp->getHeight()
								   << " - Shared raw image: "
								   << (delete_raw ? "false" : "true")
								   << LL_ENDL;
		if (texp->getDecodePriority() <= 0.f && !texp->hasSavedRawImage())
		{
			texp->setBoostLevel(LLGLTexture::BOOST_TERRAIN);
			texp->forceToRefetchTexture(ddiscard);
		}
		if (delete_raw)
		{
			texp->destroyRawImage();
		}
		return false;
	}

	return true;
}

bool LLVLComposition::generateLandTile(F32 x, F32 y, F32 width, F32 height)
{
	if (!mParamsReady)
	{
		// All the parameters have not been set yet (we did not get the message
		// from the sim)...
		return false;
	}

	if (!mSurfacep || x < 0.f || y < 0.f)
	{
		llwarns << "Invalid surface: mSurfacep = "  << std::hex
				<< (intptr_t)mSurfacep << std::dec << " - x = "  << x
					<< " - y = " << y << llendl;
		llassert(false);
		return false;
	}

	bool use_textures = true;
	if (isPBR())
	{
		if (!materialsReady(true, true))
		{
			return false;
		}
		use_textures = false;
	}
	else if (!texturesReady(true, true))
	{
		return false;
	}

	LLTimer gen_timer;

	// Generate raw data arrays for surface textures

	U8* st_data[ASSET_COUNT];
	S32 st_data_size[ASSET_COUNT];
	LLColor3 base_color, emissive_color;
	LLViewerFetchedTexture* texp;
	// These are the defaults and never change when using textures. HB
	LLViewerFetchedTexture* etexp = NULL;
	bool has_base_color = false;
	bool has_emissive_color = false;
	bool has_alpha = false;
	S32 ddiscard, eddiscard;
	bool delete_raw, edelete_raw;
	for (U32 i = 0; i < ASSET_COUNT; ++i)
	{
		if (mRawImages[i].isNull())
		{
			if (use_textures)
			{
				texp = mDetailTextures[i];
			}
			else
			{
				LLFetchedGLTFMaterial* matp = mDetailRenderMaterials[i].get();
				if (!matp)	// Paranoia
				{
					return false;	// Material not ready
				}
				texp = matp->mBaseColorTexture;
				if (!texp)
				{
					texp = LLViewerFetchedTexture::sWhiteImagep;
				}
				etexp = matp->mEmissiveTexture;
				base_color = LLColor3(matp->mBaseColor);
				// *HACK: treat alpha as black
				base_color *= matp->mBaseColor.mV[VW];
				emissive_color = matp->mEmissiveColor;
				has_base_color = base_color.mV[VX] != 1.f ||
								 base_color.mV[VY] != 1.f ||
								 base_color.mV[VZ] != 1.f;
				has_emissive_color = emissive_color.mV[VX] != 1.f ||
									 emissive_color.mV[VY] != 1.f ||
									 emissive_color.mV[VZ] != 1.f;
				has_alpha = matp->mAlphaMode !=
								LLGLTFMaterial::ALPHA_MODE_OPAQUE;
			}

			// Compute the desired discard
			if (!set_desired_discard(texp, ddiscard, delete_raw))
			{
				return false;	// Texture not ready
			}
			if (etexp && !set_desired_discard(etexp, eddiscard, edelete_raw))
			{
				return false;	// Texture not ready
			}

			mRawImages[i] = texp->getRawImage(); // Deletes previous raw
			if (delete_raw)
			{
				texp->destroyRawImage();
			}

			LLPointer<LLImageRaw> eraw;
			if (etexp)
			{
				eraw = etexp->getRawImage();
				if (has_emissive_color ||
					etexp->getWidth(eddiscard) < BASE_SIZE ||
					etexp->getHeight(eddiscard) < BASE_SIZE ||
					etexp->getComponents() != 4)
				{
					LLPointer<LLImageRaw> enewraw = new LLImageRaw(BASE_SIZE,
																   BASE_SIZE,
																   4);
					// Copy RGB, leave alpha alone (set to opaque by default)
					enewraw->copy(mRawImages[i]);
					if (has_emissive_color)
					{
						enewraw->tint(emissive_color);
					}
					eraw = enewraw; // Deletes previous raw
				}
			}

			if (has_base_color || has_alpha || eraw.notNull() ||
				texp->getWidth(ddiscard) < BASE_SIZE ||
				texp->getHeight(ddiscard) < BASE_SIZE ||
				texp->getComponents() != 3)
			{
				LLPointer<LLImageRaw> newraw = new LLImageRaw(BASE_SIZE,
															  BASE_SIZE, 3);
				if (has_alpha)
				{
					// Approximate the water underneath terrain alpha with
					// solid water color.
					newraw->clear(MAX_WATER_COLOR.mV[VX],
								  MAX_WATER_COLOR.mV[VY],
								  MAX_WATER_COLOR.mV[VZ], 255);
				}
				newraw->composite(mRawImages[i]);
				if (has_base_color)
				{
					newraw->tint(base_color);
				}
				if (eraw.notNull())
				{
					newraw->composite(eraw);
				}
				mRawImages[i] = newraw; // Deletes previous raw
			}

			if (etexp && edelete_raw)
			{
				etexp->destroyRawImage();
			}
		}

		st_data[i] = mRawImages[i]->getData();
		st_data_size[i] = mRawImages[i]->getDataSize();
	}

	// Generate and clamp x/y bounding box.

	S32 x_begin = (S32)(x * mScaleInv);
	S32 y_begin = (S32)(y * mScaleInv);
	S32 x_end = ll_round((x + width) * mScaleInv);
	S32 y_end = ll_round((y + width) * mScaleInv);

	if (x_end > mWidth)
	{
		llwarns << "x end > width" << llendl;
		x_end = mWidth;
	}
	if (y_end > mWidth)
	{
		llwarns << "y end > width" << llendl;
		y_end = mWidth;
	}

	// Generate target texture information, stride ratios.

	LLViewerTexture* texturep = mSurfacep->getSTexture();
	S32 tex_width = texturep->getWidth();
	S32 tex_height = texturep->getHeight();
	S32 tex_comps = texturep->getComponents();
	S32 tex_stride = tex_width * tex_comps;

	S32 st_comps = 3;
	S32 st_width = BASE_SIZE;
	S32 st_height = BASE_SIZE;

	if (tex_comps != st_comps)
	{
		llwarns_sparse << "Base texture comps != input texture comps"
					   << llendl;
		return false;
	}

	F32 tex_x_scalef = (F32)tex_width / (F32)mWidth;
	F32 tex_y_scalef = (F32)tex_height / (F32)mWidth;
	F32 tex_x_begin = (S32)((F32)x_begin * tex_x_scalef);
	F32 tex_y_begin = (S32)((F32)y_begin * tex_y_scalef);
	F32 tex_x_end = (S32)((F32)x_end * tex_x_scalef);
	F32 tex_y_end = (S32)((F32)y_end * tex_y_scalef);

	F32 tex_x_ratiof = (F32)mWidth * mScale / (F32)tex_width;
	F32 tex_y_ratiof = (F32)mWidth * mScale / (F32)tex_height;

	LLPointer<LLImageRaw> raw = new LLImageRaw(tex_width, tex_height,
											   tex_comps);
	U8* rawp = raw->getData();

	F32 st_x_stride = ((F32)st_width / (F32)mTexScaleX) *
					  ((F32)mWidth / (F32)tex_width);
	F32 st_y_stride = ((F32)st_height / (F32)mTexScaleY) *
					  ((F32)mWidth / (F32)tex_height);

	llassert(st_x_stride > 0.f && st_y_stride > 0.f);

	// Iterate through the target texture, striding through the sub-textures
	// and interpolating appropriately.

	F32 sti = tex_x_begin * st_x_stride -
			  st_width * llfloor(tex_x_begin * st_x_stride / st_width);
	F32 stj = tex_y_begin * st_y_stride -
			  st_height * llfloor(tex_y_begin * st_y_stride / st_height);

	S32 st_offset = (llfloor(stj * st_width) + llfloor(sti)) * st_comps;
	for (S32 j = tex_y_begin; j < tex_y_end; ++j)
	{
		U32 offset = j * tex_stride + tex_x_begin * tex_comps;
		sti = tex_x_begin * st_x_stride -
			  st_width * ((U32)(tex_x_begin * st_x_stride) / st_width);
		for (S32 i = tex_x_begin; i < tex_x_end; ++i)
		{
			F32 composition = getValueScaled(i * tex_x_ratiof,
											 j * tex_y_ratiof);

			S32 tex0 = llfloor(composition);
			tex0 = llclamp(tex0, 0, 3);
			composition -= tex0;

			S32 tex1 = tex0 + 1;
			tex1 = llclamp(tex1, 0, 3);

			st_offset = (S32(sti) + S32(stj) * st_width) * st_comps;
			for (S32 k = 0; k < tex_comps; ++k)
			{
				// Linearly interpolate based on composition.
				if (st_offset < st_data_size[tex0] &&
					st_offset < st_data_size[tex1])
				{
					F32 a = *(st_data[tex0] + st_offset);
					F32 b = *(st_data[tex1] + st_offset);
					rawp[offset] = U8(a + composition * (b - a));
				}
				++offset;
				++st_offset;
			}

			sti += st_x_stride;
			if (sti >= st_width)
			{
				sti -= st_width;
			}
		}

		stj += st_y_stride;
		if (stj >= st_height)
		{
			stj -= st_height;
		}
	}

	if (!texturep->hasGLTexture())
	{
		texturep->createGLTexture(0, raw);
	}
	texturep->setSubImage(raw, tex_x_begin, tex_y_begin,
						  tex_x_end - tex_x_begin, tex_y_end - tex_y_begin);
	LLSurface::sTextureUpdateTime += gen_timer.getElapsedTimeF32();
	LLSurface::sTexelsUpdated += (tex_x_end - tex_x_begin) *
								 (tex_y_end - tex_y_begin);

	// Un-boost detail textures (will get re-boosted if rendering in high
	// detail)
	for (U32 i = 0; i < ASSET_COUNT; ++i)
	{
		unboost_terrain_texture(mDetailTextures[i]);
		unboost_terrain_material(mDetailRenderMaterials[i]);
	}

	return true;
}
