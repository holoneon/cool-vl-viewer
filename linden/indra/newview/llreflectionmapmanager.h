/**
 * @file llreflectionmapmanager.h
 * @brief LLReflectionMap, LLReflectionMapManager and LLHeroProbeManager
 * classes declaration.
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

#include <list>
#include <queue>
#include <vector>

#include "llcubemap.h"
#include "llcubemaparray.h"
#include "hbfastset.h"
#include "llplane.h"
#include "llrendertarget.h"

#include "llvovolume.h"

class LLSpatialGroup;
struct LLReflectionProbeData;

// Number of reflection probes to keep in VRAM
#define LL_MAX_REFLECTION_PROBE_COUNT 256
// Maximum number of hero reflection probes to keep in VRAM
#define LL_MAX_HERO_PROBE_COUNT 8

// Reflection probe resolution
#define LL_IRRADIANCE_MAP_RESOLUTION 16

// Reflection probe mininum scale
#define LL_REFLECTION_PROBE_MINIMUM_SCALE 1.f;

// Change to 1 to reset the HDRI sky on reflection maps reset (instead, the
// Cool VL Viewer got a debug setting allowing to disable/enable HDRI preview
// on demand). HB
#define LL_RESET_HDRI_SKY_ON_REFLECTION_MAP_RESET 0

// Default probe distance
#define LL_REFLECTION_PROBE_DEFAULT_DIST 96.f

///////////////////////////////////////////////////////////////////////////////
// LLReflectionMap class
// This used to be in its own llreflectionmap.h/cpp module, but the header was
// only included by llreflectionmapmanager.h, so... HB
///////////////////////////////////////////////////////////////////////////////

class alignas(16) LLReflectionMap : public LLRefCount
{
protected:
	LOG_CLASS(LLReflectionMap);

public:
	LL_ALIGNED16_NEW_DELETE

	enum DetailLevel : U32
	{
		DISABLED = 0,
		STATIC_ONLY = 1,
		STATIC_AND_DYNAMIC = 2,
		REALTIME = 3
	};

	typedef std::vector<LLReflectionMap*> reflmap_vec_t;

	// Allocates an environment map of the given resolution
	LLReflectionMap();
	~LLReflectionMap();

	// Updates this environment map resolution
	void update(U32 resolution, U32 face, bool force_dynamic = false,
				F32 near_clip = -1.f, bool use_clip_plane = false,
				LLPlane clip_plane = LLPlane(),
				const LLVector3* lookatp = NULL,
				const LLVector3* updirp = NULL);

	// For volume partition probes, tries to place this probe in the best spot
	void autoAdjustOrigin();

	// Returns true if given reflection map's influence volume intersects with
	// this one's.
	bool intersects(LLReflectionMap* otherp);

	// Gets the ambiance value to use for this probe
	F32 getAmbiance();

	// Gets the near clip plane distance to use for this probe
	F32 getNearClip();

	// Returns true if this probe should include avatars in its reflection map
	bool getIsDynamic();

	// Gets the encoded bounding box of this probe's influence volume; will
	// only return a box if this probe is associated with a LLVOVolume with its
	// reflection probe influence volume to to VOLUME_TYPE_BOX. Returns false
	// if no bounding box (treat as sphere influence volume).
	bool getBox(LLMatrix4& box);

	// Returns true if this probe is active for rendering
	LL_INLINE bool isActive()					{ return mCubeIndex != -1; }

	// Performs occlusion query/readback
	void doOcclusion(const LLVector4a& eye);

	// Returns false if this probe is not currently relevant (for example,
	// disabled due to graphics preferences).
	bool isRelevant();

public:
	// Index into array packed by LLReflectionMapManager::getReflectionMaps()
	// WARNING: only valid immediately after call to getReflectionMaps().
	// Note
	S32							mProbeIndex;

	// Spatial group this probe is tracking (if any).
	LLPointer<LLSpatialGroup>	mGroup;

	// Point at which environment map was last generated from (in agent space).
	// Note: placed 16 bytes (counting the LLRefCount S32) from variable
	// members start, to avoid padding issues. HB
	LLVector4a					mOrigin;

	// Viewer object this probe is tracking (if any).
	LLPointer<LLViewerObject>	mViewerObject;

	// Set of any LLReflectionMaps that intersect this map (maintained by
	// LLReflectionMapManager).
	reflmap_vec_t				mNeighbors;

	// Cube map used to sample this environment map.
	LLPointer<LLCubeMapArray>	mCubeArray;
	// Index into cube map array or -1 if not currently stored in a cube map.
	S32							mCubeIndex;

	// Distance from main viewer camera.
	F32							mDistance;

	// Minimum and maximum depth in current render camera.
	F32							mMinDepth;
	F32							mMaxDepth;

	// Radius of this probe's affected area
	F32							mRadius;

	// Last time this probe was updated (or when its update timer got reset).
	F32							mLastUpdateTime;
	// Last time this probe was bound for rendering.
	F32							mLastBindTime;

	// fade in parameter for this probe
	F32							mFadeIn;

	// What priority should this probe have (higher is higher priority)
	// currently only 0 or 1
	// 0 - automatic probe
	// 1 - manual probe
	U32							mPriority;

	// Occlusion culling state
	U32							mOcclusionQuery;
	U32							mOcclusionPendingFrames;
	bool						mOccluded;

	// true when probe has had at least one full update and is ready to render.
	bool						mComplete;
};

///////////////////////////////////////////////////////////////////////////////
// LLReflectionMapManager class
///////////////////////////////////////////////////////////////////////////////

class alignas(16) LLReflectionMapManager
{
	friend class LLHeroProbeManager;
	friend class LLPipeline;

protected:
	LOG_CLASS(LLReflectionMapManager);

public:
	LL_ALIGNED16_NEW_DELETE

	// Allocates an environment map of the given resolution
	LLReflectionMapManager();

	~LLReflectionMapManager();

	// Releases any GL state
	void cleanup();

	// Maintains reflection probes
	void update();

	// Adds a probe for the given spatial group.
	LLReflectionMap* addProbe(LLSpatialGroup* groupp = NULL);

	typedef std::vector<LLPointer<LLReflectionMap> > prmap_vec_t;

	// Populate "maps" with the N most relevant reflection maps where N is no
	// more than maps.size(). If less than maps.size() reflection maps are
	// available, will assign trailing elements to NULL. 'maps' must be an
	// adequately sized array of reflection map pointers.
	void getReflectionMaps(prmap_vec_t& maps);

	// Called by LLSpatialGroup constructor. If spatial group should receive a
	// reflection probe, creates one for the specified spatial group.
	LLReflectionMap* registerSpatialGroup(LLSpatialGroup* groupp);

	// Presently hacked into LLViewerObject::setTE(). Used by LLViewerObjects
	// which are reflection probes. 'vobjp' must not be NULL. Guaranteed to not
	// return NULL.
	LLReflectionMap* registerViewerObject(LLViewerObject* vobjp);

	// Resets all state on the next update. If 'hard' is true, then a cleanup()
	// is done and all probes get re-registered on next frame. HB
	void reset(bool hard = false);

	// Pauses all updates other than the default probe for 'duration' seconds.
	void pause(F32 duration);

	// Called on region crossing to "shift" probes into new coordinate frame.
	void shift(const LLVector4a& offset);

	// Called from LLSpatialPartition when reflection probe debug display is
	// active.
	void renderDebug();

	// Called once at startup to allocate cubemap arrays
	void initReflectionMaps();

	// Returns true if currently updating a radiance map, false if currently
	// updating an irradiance map.
	LL_INLINE bool isRadiancePass()				{ return mRadiancePass; }

	// Performs occlusion culling on all active reflection probes
	void doOcclusion();

	// *HACK: "culls" all reflection probes except the default one. Only call
	// this if you do not intend to call updateUniforms directly. Call again
	// with false when done.
	void forceDefaultProbeAndUpdateUniforms(bool force = true);

	U32 allocateQuery();
	void recycleQuery(U32 query);

private:
	void cleanupQueryPool();

	// Initializes mCubeFree array to default values
	void initCubeFree();

	// Deletes the probe with the given index in mProbes
	void deleteProbe(U32 i);

	// Gets a free cube index
	// returns -1 if allocation failed
	S32 allocateCubeIndex();

	// Updates the neighbors of the given probe
	void updateNeighbors(LLReflectionMap* probep);

	// Updates UBO used for rendering (call only once per render pipe flush)
	void updateUniforms();

	// Binds UBO used for rendering
	void setUniforms();

	// Performs an update on the currently updating probe
	void doProbeUpdate();

	// Updates the specified face of the specified probe
	void updateProbeFace(LLReflectionMap* probep, U32 face);

private:
	LLReflectionProbeData*		mProbeData;
	// Render target for cube snapshots; used to generate mipmaps without
	// doing a copy-to-texture.
	LLRenderTarget				mRenderTarget;

	std::deque<U32>				mQueryPool;

	std::vector<LLRenderTarget>	mMipChain;

	// List of free cubemap indices
	std::list<S32>				mCubeFree;

	// Storage for reflection probe radiance maps (plus two scratch space
	// cubemaps)
	LLPointer<LLCubeMapArray>	mTexture;

	// Vertex buffer for pushing verts to filter shaders
	LLPointer<LLVertexBuffer>	mVertexBuffer;

	// Storage for reflection probe irradiance maps
	LLPointer<LLCubeMapArray>	mIrradianceMaps;

	// Default reflection probe to fall back to for pixels with no probe
	// influences (should always be at cube index 0).
	LLPointer<LLReflectionMap>	mDefaultProbe;

	LLReflectionMap*			mUpdatingProbe;

	// List of maps being used for rendering
	prmap_vec_t					mReflectionMaps;

	// List of active reflection maps
	prmap_vec_t					mProbes;
	// List of reflection maps to kill
	prmap_vec_t					mKillList;
	// List of reflection maps to create
	prmap_vec_t					mCreateList;

	// Handle to UBO
	U32							mUBO;

	U32							mUpdatingFace;

	// Number of reflection probes to use for rendering.
	U32							mReflectionProbeCount;
	U32							mDynamicProbeCount;

	// Resolution of reflection probes
	U32							mProbeResolution;

	// Maximum LoD of reflection probes (mip levels - 1)
	F32							mMaxProbeLOD;

	// Amount to scale local lights during an irradiance map update (set during
	// updateProbeFace() and used by LLPipeline).
	F32							mLightScale;

	F32							mResetFade;

	F32							mResumeTime;

	// If true, we are generating the radiance map for the current probe,
	// otherwise we are generating the irradiance map. Update sequence should
	// be to generate the irradiance map from render of the world that has no
	// irradiance, then generate the radiance map from a render of the world
	// that includes irradiance. This should avoid feedback loops and ensure
	// that the colors in the radiance maps match the colors in the
	// environment.
	bool						mRadiancePass;

	// Same as above, but for the realtime probe. Realtime probes should update
	// all six sides of the irradiance map on "odd" frames and all six sides of
	// the radiance map on "even" frames.
	bool						mRealtimeRadiancePass;

	// If true, reset all probe render state on the next update (for teleports
	// and sky changes).
	bool						mReset;

	// If true, only update the default probe
	bool						mPaused;

public:
	static F32					sDefaultDistance;
	static bool					sNeedsReregister;
};

///////////////////////////////////////////////////////////////////////////////
// LLHeroProbeManager class
///////////////////////////////////////////////////////////////////////////////

struct HeroProbeData
{
	LLMatrix4	heroBox[LL_MAX_HERO_PROBE_COUNT];
	LLVector4	heroSphere[LL_MAX_HERO_PROBE_COUNT];
	GLint		heroMipCount;
	GLint		heroProbeCount;
	// heroParams[i] = { shape, cubeIndex, 0, 0 }
	GLint     heroParams[LL_MAX_HERO_PROBE_COUNT][4];
	LLMatrix4 heroPlaneMatrix[LL_MAX_HERO_PROBE_COUNT];
	LLVector4 heroClipPlane[LL_MAX_HERO_PROBE_COUNT];
};

class alignas(16) LLHeroProbeManager
{
	friend class LLPipeline;
	friend class LLReflectionMapManager;

protected:
	LOG_CLASS(LLHeroProbeManager);

public:
	LL_ALIGNED16_NEW_DELETE

	LLHeroProbeManager();
	~LLHeroProbeManager();

	// Releases any GL state
	void cleanup();

	void update();
	void renderProbes();

	// Called from LLSpatialPartition when reflection probe debug display is
	// active.
	void renderDebug();

	// Called once at startup to allocate cubemap arrays
	void initReflectionMaps();

	// Performs occlusion culling on all active reflection probes
	void doOcclusion();

	LL_INLINE void reset()						{ mReset = true; }

	bool registerViewerObject(LLVOVolume* volp);
	void unregisterViewerObject(LLVOVolume* volp);

	bool hasActiveMirror() const;
	LL_INLINE bool isMirrorPass() const			{ return mRenderingMirror; }

private:
	// Returns true if probe probe_idx should be updated now. HB
	bool shouldUpdate(U32 probe_idx);

	// Updates UBO used for rendering (call only once per render pipe flush)
	void updateUniforms();

	// Updates the specified face of the specified probe
	void updateProbeFace(LLReflectionMap* probep, U32 face, bool is_dynamic,
						 F32 near_clip);
	void generateRadiance(LLReflectionMap* probep);

private:
	// Aligned member
	LLPlane						mCurrentClipPlane;

	LLVector3					mPlanarLookDir;
	LLVector3					mPlanarUpDir;

	LLRenderTarget				mRenderTarget;

	std::vector<LLRenderTarget>	mMipChain;

	// Storage for reflection probe radiance maps (plus two scratch space
	// cubemaps)
	LLPointer<LLCubeMapArray>	mTexture;

	// Vertex buffer for pushing verts to filter shaders
	LLPointer<LLVertexBuffer>	mVertexBuffer;

	// List of active reflection maps
	typedef std::vector<LLPointer<LLReflectionMap> > prmap_vec_t;
	prmap_vec_t					mProbes;

	// Default reflection probe to fall back to for pixels with no probe
	// influences (should always be at cube index 0).
	LLPointer<LLReflectionMap>	mDefaultProbe;

	typedef fast_hset<LLPointer<LLVOVolume> > volp_set_t;
	volp_set_t					mHeroVOList;

	typedef std::vector<LLPointer<LLVOVolume> > volp_vec_t;
	volp_vec_t					mActiveHeroes;

	// Number of reflection probes to use for rendering
	U32							mReflectionProbeCount;

	U32							mProbeResolution;

	F32							mNearestProbeDist;

	// Maximum LoD of reflection probes (mip levels - 1)
	F32							mMaxProbeLOD;

	F32							mHeroProbeStrength;

	bool						mIsPlanar;

	bool						mIsInTransition;

	// If true, reset all probe render state on the next update (for teleports
	// and sky changes).
	bool						mReset;

	bool						mRenderingMirror;

public:
	LLVector3					mMirrorPosition;
	LLVector3					mMirrorNormal;
	HeroProbeData				mHeroData;

	S32							mCurrentRenderingProbeIdx;

	bool						mHeroShadowsComplete[LL_MAX_HERO_PROBE_COUNT];

	static bool					sNeedsReregister;
};
