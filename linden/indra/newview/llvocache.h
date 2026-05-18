/**
 * @file llvocache.h
 * @brief Cache of objects on the viewer.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2016 (original implementation), Linden Research, Inc.
 * Copyright (c) 2022 (PBR extra data implementation), Linden Research, Inc.
 * Copyright (c) 2016-2023, Henri Beauchamp (debugging, cache parameters
 * biasing based on memory usage, removal of APR dependency, threading).
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

#include <set>
#include <string>

#include "lldatapacker.h"
#include "lldir.h"
#include "hbfastmap.h"
#include "llfile.h"
#include "llgltfmaterial.h"
#include "llpointer.h"
#include "llsingleton.h"
#include "llthreadpool.h"
#include "lluuid.h"

#include "llvieweroctree.h"

#define HB_AJUSTED_VOCACHE_PARAMETERS 1

class LLCamera;

//---------------------------------------------------------------------------
// Cache entries

class LLGLTFOverrideCacheEntry
{
protected:
	LOG_CLASS(LLGLTFOverrideCacheEntry);

public:
	LL_INLINE LLGLTFOverrideCacheEntry()
	:	mRegionHandle(0),
		mLocalId(0)
	{
	}

	bool fromLLSD(const LLSD& data);
	LLSD toLLSD() const;

public:
	// LLSD per side
	typedef fast_hmap<S32, LLSD> data_map_t;
	data_map_t	mSides;

	// glTF material per side
	typedef fast_hmap<S32, LLPointer<LLGLTFMaterial> > mat_map_t;
	mat_map_t	mGLTFMaterial;

	U64			mRegionHandle;
	U32			mLocalId;
};

class alignas(16) LLVOCacheEntry : public LLViewerOctreeEntryData
{
protected:
	LOG_CLASS(LLVOCacheEntry);

	~LLVOCacheEntry();

public:
	LL_VOLUME_AREANA_NEW_DELETE

	enum
	{
		// Low 16-bit state
		INACTIVE = 0x00000000,		// Not visible
		IN_QUEUE = 0x00000001,		// In visible queue, object to be created
		WAITING  = 0x00000002,		// Object creation request sent
		ACTIVE   = 0x00000004,		// Object created and in rendering pipeline

		// High 16-bit state
		IN_VO_TREE = 0x00010000,	// Entry is in the object cache tree

		LOW_BITS  = 0x0000ffff,
		HIGH_BITS = 0xffff0000
	};

	struct CompareVOCacheEntry
	{
		LL_INLINE bool operator()(const LLVOCacheEntry* const& lhs,
								  const LLVOCacheEntry* const& rhs) const
		{
			F32 lpa = lhs->getSceneContribution();
			F32 rpa = rhs->getSceneContribution();
			// Larger pixel area first
#if 1
			return lpa > rpa || (lpa == rpa && lhs < rhs);
#else
			if (lpa > rpa)
			{
				return true;
			}
			if (lpa < rpa)
			{
				return false;
			}
			return lhs < rhs;
#endif
		}
	};

	LLVOCacheEntry(U32 local_id, U32 crc, LLDataPackerBinaryBuffer& dp);
	LLVOCacheEntry(LLFile* infilep);
	LLVOCacheEntry();

	void updateEntry(U32 crc, LLDataPackerBinaryBuffer& dp);

	void setState(U32 state);
	LL_INLINE void clearState(U32 state)					{ mState &= ~state; }
	LL_INLINE bool hasState(U32 state)						{ return (mState & state) != 0; }
	LL_INLINE bool isState(U32 state)						{ return (mState & LOW_BITS) == state; }
	LL_INLINE U32 getState() const							{ return mState & LOW_BITS; }

	bool isAnyVisible(const LLVector4a& camera_origin,
					  const LLVector4a& local_camera_origin,
					  F32 dist_threshold);

	LL_INLINE U32 getLocalID() const						{ return mLocalID; }
	LL_INLINE U32 getCRC() const							{ return mCRC; }
	LL_INLINE S32 getHitCount() const						{ return mHitCount; }
	LL_INLINE S32 getCRCChangeCount() const					{ return mCRCChangeCount; }

	void calcSceneContribution(const LLVector4a& camera_origin,
							   bool needs_update, U32 last_update,
							   F32 dist_threshold);
	LL_INLINE void setSceneContribution(F32 contrib)		{ mSceneContrib = contrib;}
	LL_INLINE F32 getSceneContribution() const				{ return mSceneContrib; }

	void dump() const;
	bool writeToFile(LLFile* outfilep) const;
	LLDataPackerBinaryBuffer* getDP();
	void recordHit()										{ ++mHitCount; }
	LL_INLINE void recordDupe()								{ ++mDupeCount; }

	void setOctreeEntry(LLViewerOctreeEntry* entry) override;

	void setParentID(U32 id);
	LL_INLINE U32 getParentID() const						{ return mParentID; }
	LL_INLINE bool isChild() const							{ return mParentID > 0; }

	void addChild(LLVOCacheEntry* entry);
	void removeChild(LLVOCacheEntry* entry);
	void removeAllChildren();

	// Removes the first child and returns it.
	LLVOCacheEntry* getChild();

	LL_INLINE S32 getNumOfChildren()						{ return mChildrenList.size(); }

	// Called from processing object update message:
	void setBoundingInfo(const LLVector3& pos, const LLVector3& scale);

	void updateParentBoundingInfo();
	void saveBoundingSphere();

	LL_INLINE void setValid(bool valid = true)				{ mValid = valid; }
	LL_INLINE bool isValid() const							{ return mValid; }

	LL_INLINE void setUpdateFlags(U32 flags)				{ if (flags != 0xffffffff) mUpdateFlags = flags; }
	LL_INLINE U32 getUpdateFlags() const					{ return mUpdateFlags; }

	// Used to set the entry as always visible
	void keepVisible();

	static void updateSettings();
	static F32 getSquaredPixelThreshold(bool is_front);

	typedef fast_hmap<U32, LLPointer<LLVOCacheEntry> > map_t;
	typedef std::set<LLVOCacheEntry*> set_t;
	typedef std::set<LLVOCacheEntry*, CompareVOCacheEntry> prio_list_t;
	typedef fast_hmap<U32, LLGLTFOverrideCacheEntry> emap_t;

private:
	void updateParentBoundingInfo(const LLVOCacheEntry* child);

protected:
	LLVector4a      			mBSphereCenter;		// Bounding sphere center
	F32                         mBSphereRadius;		// Bounding sphere radius

	U32							mLocalID;
	U32							mParentID;
	U32							mCRC;
	U32							mUpdateFlags;		// Received from sim
	S32							mHitCount;
	S32							mDupeCount;
	S32							mCRCChangeCount;
	U8*							mBuffer;
	LLDataPackerBinaryBuffer	mDP;

	// Projected scene contributuion of this object:
	F32							mSceneContrib;
	// High 16 bits reserved for special use:
	U32							mState;

	// Children entries in a linked set.
	set_t						mChildrenList;

	// If set, this entry is cannot be made invisible. HB
	bool						mKeepVisible;

	// If set, this entry is valid, otherwise it is invalid:
	bool						mValid;

public:
	S32							mLastCameraUpdated;

	static U32					sMinFrameRange;
	static F32					sNearRadius;
	static F32					sRearFarRadius;
	static F32					sFrontPixelThreshold;
	static F32					sRearPixelThreshold;
#if HB_AJUSTED_VOCACHE_PARAMETERS
	static bool					sBiasedRetention;
#endif
};

class LLVOCacheGroup : public LLOcclusionCullingGroup
{
protected:
	LOG_CLASS(LLVOCacheGroup);

public:
	LLVOCacheGroup(OctreeNode* node, LLViewerOctreePartition* part)
	:	LLOcclusionCullingGroup(node, part)
	{
	}

	// virtual
	void handleChildAddition(const OctreeNode* parent, OctreeNode* child);

protected:
	virtual ~LLVOCacheGroup();
};

class LLVOCachePartition : public LLViewerOctreePartition
{
protected:
	LOG_CLASS(LLVOCachePartition);

public:
	LLVOCachePartition(LLViewerRegion* regionp);
	~LLVOCachePartition() override;

	bool addEntry(LLViewerOctreeEntry* entry);
	void removeEntry(LLViewerOctreeEntry* entry);
	S32 cull(LLCamera& camera, bool do_occlusion = false) override;
	void addOccluders(LLViewerOctreeGroup* gp);
	void resetOccluders();
	void processOccluders(LLCamera* camera);
	void removeOccluder(LLVOCacheGroup* group);

	void setCullHistory(bool has_new_object);

	LL_INLINE bool isFrontCull() const						{ return mFrontCull; }

private:
	// Selects objects behind camera.
	void selectBackObjects(LLCamera& camera, F32 projection_area_cutoff,
						   bool use_occlusion);

public:
	static bool sNeedsOcclusionCheck;

private:
	U32		mCullHistory;
	U32		mCulledTime[LLViewerCamera::NUM_CAMERAS];
	S32		mBackSlectionEnabled; // enable to select back objects if > 0.
	U32		mIdleHash;
	// The view frustum cull if set, otherwise is back sphere cull:
	bool	mFrontCull;

	std::set<LLVOCacheGroup*> mOccludedGroups;
};

// Note: LLVOCache is not thread-safe
class LLVOCache : public LLSingleton<LLVOCache>
{
    friend class LLSingleton<LLVOCache>;

protected:
	LOG_CLASS(LLVOCache);

private:
	struct HeaderEntryInfo
	{
		HeaderEntryInfo()
		:	mIndex(0),
			mHandle(0),
			mTime(0)
		{
		}

		S32 mIndex;
		U64 mHandle;
		U32 mTime;
	};

	struct HeaderMetaInfo
	{
		HeaderMetaInfo()
		:	mVersion(0),
			mAddressSize(0)
		{
		}

		U32 mVersion;
		U32 mAddressSize;
	};

	struct header_entry_less
	{
		LL_INLINE bool operator()(const HeaderEntryInfo* lhs,
								  const HeaderEntryInfo* rhs) const
		{
			if (lhs->mTime == rhs->mTime)
			{
				return lhs < rhs;
			}

			// older entry in front of queue (set)
			return lhs->mTime < rhs->mTime;
		}
	};

	typedef std::set<HeaderEntryInfo*, header_entry_less> header_entry_queue_t;
	typedef fast_hmap<U64, HeaderEntryInfo*> handle_entry_map_t;

private:
	LLVOCache();

public:
	~LLVOCache();

	void initCache(ELLPath location, U32 size);
	void removeCache(ELLPath location);

	void readFromCache(U64 handle, const std::string& region_name,
					   const LLUUID& id);
	// IMPORTANT: entry_map and extras_map may be wiped out by this method. HB
	void writeToCache(U64 handle, const std::string& region_name,
					  const LLUUID& id, LLVOCacheEntry::map_t& entry_map,
					  bool dirty_cache, LLVOCacheEntry::emap_t& extras_map,
					  bool removal_enabled);

	void removeEntry(U64 handle);

	void setReadOnly(bool read_only)						{ mReadOnly = read_only; }
	bool isReadOnly() const									{ return mReadOnly; }

	bool isEnabled() const									{ return mEnabled; }

private:
	class ReadWorker
	{
	protected:
		LOG_CLASS(LLVOCache::ReadWorker);

	public:
		ReadWorker(U64 handle, const LLUUID& id,
				   const std::string& region_name);
		
		void readCacheFile();

	private:
		LLUUID								mId;
		U64									mHandle;
		std::string							mRegionName;
	};

	class WriteWorker
	{
	protected:
		LOG_CLASS(LLVOCache::WriteWorker);

	public:
		WriteWorker(U64 handle, const LLUUID& id,
					const std::string& region_name,
					LLVOCacheEntry::map_t& entry_map,
					LLVOCacheEntry::emap_t& extras_map,
					bool removal_enabled);
		
		void writeCacheFile();

	private:
		LLUUID					mId;
		U64						mHandle;
		std::string				mRegionName;
		LLVOCacheEntry::map_t	mEntryMap;
		LLVOCacheEntry::emap_t	mExtraMap;
		bool					mRemovalEnabled;
	};

	void setDirNames(ELLPath location);
	// Determine the cache filename for the region from the region handle
	void getObjectCacheFilename(U64 handle, std::string& filename,
								bool extra_entries = false);
	void removeFromCache(HeaderEntryInfo* entry);
	void readCacheHeader();
	void writeCacheHeader();
	void clearCacheInMemory();
	void removeCache();
	void removeEntry(HeaderEntryInfo* entry);
	void purgeEntries(U32 size);
	bool updateEntry(const HeaderEntryInfo* entry);

private:
	typedef std::unique_ptr<LLThreadPool> thread_pool_ptr_t;
	thread_pool_ptr_t		mThreadPoolp;
	LLMutex					mMutex;
	HeaderMetaInfo			mMetaInfo;
	U32						mCacheSize;
	U32						mNumEntries;
	std::string				mHeaderFileName;
	std::string				mObjectCacheDirName;
	header_entry_queue_t	mHeaderEntryQueue;
	handle_entry_map_t		mHandleEntryMap;
	bool					mEnabled;
	bool					mInitialized;
	bool					mReadOnly;
};
