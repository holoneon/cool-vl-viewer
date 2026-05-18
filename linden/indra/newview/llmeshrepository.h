/**
 * @file llmeshrepository.h
 * @brief Client-side repository of mesh assets.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include <array>
#include <deque>
#include <functional>
#include <map>
#include <queue>

#include "llassettype.h"
#include "llatomic.h"
#include "llcorehttpcommon.h"
#include "llcorehttphandler.h"
#include "llcorehttpheaders.h"
#include "llcorehttpoptions.h"
#include "llcorehttprequest.h"
#include "hbfastmap.h"
#include "hbfastset.h"
#include "llhandle.h"
#include "llmodel.h"
#include "llmutex.h"
#include "llthread.h"

#include "llappviewer.h"		// gFrameTimeSeconds
#include "llviewertexture.h"
#include "llvolume.h"

// Set to 1 to re-enable LL's pending requests sorting code (*), which only
// slows down mesh loading !
// (*) That code relies on drawable radius, which is often unknown or wrong
// with not yet fully loaded meshes anyway... HB
#define LL_PENDING_MESH_REQUEST_SORTING 0

class LLConvexDecomposition;
class LLMeshRepository;
class LLVOVolume;
struct LLCDMeshData;

class LLMeshHeader
{
public:
	LLMeshHeader() noexcept;

	LLMeshHeader(LLMeshHeader&&) noexcept = default;

	void init(const LLSD& header, U32 size);
	void reset();

public:
	U32		mHeaderSize;
	U32		mLodOffset[4];
	U32		mLodSize[4];
	U32		mSkinOffset;
	U32		mSkinSize;
	U32		mPhysicsConvexOffset;
	U32		mPhysicsConvexSize;
	U32		mPhysicsMeshOffset;
	U32		mPhysicsMeshSize;
	bool	mValid;
};

class LLMeshCostData final : public LLRefCount
{
public:
	LLMeshCostData() noexcept;

	LLMeshCostData(LLMeshCostData&&) noexcept = default;

	bool init(LLMeshHeader* header);
	bool init(const LLSD& header);
	bool init(S32 bytes_lowest, S32 bytes_low, S32 bytes_med, S32 bytes_high);

	// Size for given LOD
	LL_INLINE S32 getSizeByLOD(S32 lod)
	{
		return lod >= 0 && lod <= 3 ? mSizeByLOD[lod] : 0;
	}

	// Sum of all LOD sizes.
	LL_INLINE S32 getSizeTotal()			{ return mSizeTotal; }

	// Estimated triangle counts for the given LOD.
	LL_INLINE F32 getEstTrisByLOD(S32 lod)
	{
		return lod >= 0 && lod <= 3 ? mEstTrisByLOD[lod] : 0.f;
	}
	
	// Estimated triangle counts for the largest LOD. Typically this is also
	// the "high" LOD, but not necessarily.
	LL_INLINE F32 getEstTrisMax()			{ return mEstTrisMax; }

	// Triangle count as computed by original streaming cost formula. Triangles
	// in each LOD are weighted based on how frequently they will be seen.
	// This was called "unscaled_value" in the original getStreamingCost()
	// functions.
	F32 getRadiusWeightedTris(F32 radius);

	// Triangle count used by triangle-based cost formula. Based on triangles
	// in highest LOD plus potentially partial charges for lower LODs depending
	// on complexity.
	F32 getEstTrisForStreamingCost();

	// Streaming cost. This should match the server-side calculation for the
	// corresponding volume.
	F32 getRadiusBasedStreamingCost(F32 radius);

	// New streaming cost formula, currently only used for animated objects.
	F32 getTriangleBasedStreamingCost();

private:
	// Note: the first member variable is 32 bits in order to align on 64 bits
	// for the next variables, counting the 32 bits counter from LLRefCount. HB

	// Sum of all LOD sizes.
	S32					mSizeTotal;
	// Estimated triangle counts for the largest LOD. Typically this is also
	// the "high" LOD, but not necessarily.
	F32					mEstTrisMax;
	F32					mChargedTris;
	// From the "size" field of the mesh header. LOD 0=lowest, 3=highest.
	std::vector<S32>	mSizeByLOD;
	// Estimated triangle counts derived from the LOD sizes. LOD 0=lowest,
	// 3=highest.
	std::vector<F32>	mEstTrisByLOD;
};

class LLRequestStats
{
public:
	LL_INLINE LLRequestStats() noexcept
	:	mRetries(0),
		mNextRetryTime(0.f)
	{
	}

	LL_INLINE U32 getRetries()
	{
		return mRetries;
	}

	LL_INLINE void updateTime()
	{
		constexpr F32 DOWNLOAD_RETRY_DELAY = 0.5f; // seconds
		mNextRetryTime = gFrameTimeSeconds +
						 DOWNLOAD_RETRY_DELAY * (F32)(1 << mRetries++);
	}

	LL_INLINE bool canRetry() const
	{
		constexpr U32 DOWNLOAD_RETRY_LIMIT = 8;
		return mRetries < DOWNLOAD_RETRY_LIMIT;
	}

	LL_INLINE bool isDelayed() const
	{
		return gFrameTimeSeconds < mNextRetryTime;
	}

private:
	U32 mRetries;
	F32 mNextRetryTime;
};

class LLPhysicsDecomp : public LLThread
{
protected:
	LOG_CLASS(LLPhysicsDecomp);

public:
	LLPhysicsDecomp();
	~LLPhysicsDecomp() override;

	void shutdown() override;

	void run() override;

	void setMeshData(LLConvexDecomposition* cvxdecompp, LLCDMeshData& mesh,
					 bool vertex_based);
	void doDecomposition(LLConvexDecomposition* cvxdecompp);
	void doDecompositionSingleHull(LLConvexDecomposition* cvxdecompp);
	void notifyCompleted();
	void cancel();

	typedef std::map<std::string, LLSD, std::less<> > decomp_params_t;

	class Request : public LLThreadSafeRefCount
	{
	public:

		// Status message callback, called from decomposition thread
		virtual S32 statusCallback(const char* status, S32 p1, S32 p2) = 0;

		// Completed callback, called from the main thread
		virtual void completed() = 0;

		virtual void setStatusMessage(const std::string& msg);

		LL_INLINE bool isValid() const
		{
			return mPositions.size() > 2 && mIndices.size() > 2;
		}

	protected:
		void assignData(LLModel* modelp);
		void updateTriangleAreaThreshold();
		bool isValidTriangle(U16 idx1, U16 idx2, U16 idx3);

	protected:
		LLVector3 							mBBox[2];
		F32									mTriangleAreaThreshold;

	public:
		// Input params
		S32*								mDecompID;
		std::string							mStage;
		std::vector<U16>					mIndices;
		std::vector<LLVector3>				mPositions;
		decomp_params_t						mParams;

		// Output state
		std::string							mStatusMessage;
		std::vector<LLModel::PhysicsMesh>	mHullMesh;
		LLModel::hull_decomp_t				mHull;
	};

	void submitRequest(Request* requestp);

	static S32 llcdCallback(const char*, S32, S32);

public:
	LLCondition						mSignal;
	LLMutex							mMutex;

	typedef std::map<std::string, S32, std::less<> > stage_map_t;
	stage_map_t						mStageID;

	typedef std::queue<LLPointer<Request> > request_queue_t;
	request_queue_t					mRequestQ;

	LLPointer<Request>				mCurRequest;

	std::queue<LLPointer<Request> >	mCompletedQ;

	bool							mQuitting;
};

class LLMeshRepoThread final : public LLThread
{
protected:
	LOG_CLASS(LLMeshRepoThread);

public:
	LLMeshRepoThread();
	~LLMeshRepoThread() override;

	void run() override;

	void lockAndLoadMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	void loadMeshLOD(const LLVolumeParams& mesh_params, S32 lod);

	bool fetchMeshHeader(const LLVolumeParams& mesh_params,
						 bool can_retry = true);
	bool fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod,
					  bool can_retry = true);
	bool headerReceived(const LLVolumeParams& mesh_params, U8* data,
						S32 data_size);
	bool lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data,
					 S32 data_size);
	bool skinInfoReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool decompositionReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	LLMeshHeader* getMeshHeader(const LLUUID& mesh_id);

	void notifyLoadedMeshes();
	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);

	void loadMeshSkinInfo(const LLUUID& mesh_id);
	void loadMeshDecomposition(const LLUUID& mesh_id);
	void loadMeshPhysicsShape(const LLUUID& mesh_id);

	// Mutex: acquires mMutex
	std::string constructUrl(const LLUUID& mesh_id, U32* version);

	class HeaderRequest final : public LLRequestStats
	{
	public:
		LL_INLINE HeaderRequest(const LLVolumeParams& mesh_params) noexcept
		:	LLRequestStats(),
			mMeshParams(mesh_params)
		{
		}

		LL_INLINE bool operator<(const HeaderRequest& rhs) const
		{
			return mMeshParams < rhs.mMeshParams;
		}

	public:
		LLVolumeParams mMeshParams;
	};

	class LODRequest final : public LLRequestStats
	{
	public:
		LL_INLINE LODRequest(const LLVolumeParams& params, S32 lod) noexcept
		:	LLRequestStats(),
#if LL_PENDING_MESH_REQUEST_SORTING
			mScore(0.f),
#endif
			mMeshParams(params),
			mLOD(lod)
		{
		}

	public:
		LLVolumeParams	mMeshParams;
		S32				mLOD;
#if LL_PENDING_MESH_REQUEST_SORTING
		F32				mScore;
#endif
	};

#if LL_PENDING_MESH_REQUEST_SORTING
	struct CompareScoreGreater
	{
		LL_INLINE bool operator()(const LODRequest& lhs, const LODRequest& rhs)
		{
			return lhs.mScore > rhs.mScore;	// Greatest = first
		}
	};
#endif

	class UUIDBasedRequest final : public LLRequestStats
	{
	public:
		LL_INLINE UUIDBasedRequest(const LLUUID& id) noexcept
		:	LLRequestStats(),
			mId(id)
		{
		}

		// Needed by boost::unordered_set and boost::unordered_map
		LL_INLINE bool operator==(const UUIDBasedRequest& rhs) const
		{
			return mId == rhs.mId;
		}

	public:
		LLUUID mId;
	};

private:
	// Codes returned by the fetch*() methods below. FETCH_PENDING means we are
	// still waiting for the mesh header and the operation should be retried
	// until we get it. HB
	enum { FETCH_PENDING = -1, FETCH_FAILED = 0, FETCH_SUCCESS = 1 };

	// Sends the request for skin info.
	S32 fetchMeshSkinInfo(const LLUUID& mesh_id, bool can_retry);

	// Sends the request for decomposition, returns true if header info exists
	// (should hold onto mesh_id and try again later if header info does not
	// exist)
	S32 fetchMeshDecomposition(const LLUUID& mesh_id);

	// Sends the request for PhysicsShape, returns true if header info exists
	// (should hold onto mesh_id and try again later if header info does not
	// exist).
	S32 fetchMeshPhysicsShape(const LLUUID& mesh_id);

	typedef fast_hset<UUIDBasedRequest> base_requests_set_t;
	// This method adds 'remaining' and 'incomplete' requests back into the
	// mMutex-protected 'dest' requests set. Both 'remaining' and 'incomplete'
	// are empty on method exit.
	void insertRequests(base_requests_set_t& dest,
						base_requests_set_t& remaining,
						base_requests_set_t& incomplete);

	// Issue a GET request to a URL with 'Range' header using the correct
	// policy class and other attributes. If an invalid handle is returned,
	// the request failed and caller must retry or dispose of handler.
	//
	// Threads: Repo thread only
	LLCore::HttpHandle getByteRange(const std::string& url, U32 cap_version,
									size_t offset, size_t len,
									const LLCore::HttpHandler::ptr_t& handler);

	struct LoadedMesh
	{
		LoadedMesh(LLVolume* volume, const LLVolumeParams& mesh_params,
				   S32 lod)
		:	mVolume(volume),
			mMeshParams(mesh_params),
			mLOD(lod)
		{
		}

		LLPointer<LLVolume>	mVolume;
		S32					mLOD;
		LLVolumeParams		mMeshParams;
	};

public:
	LLMutex							mMutex;
	LLMutex							mHeaderMutex;
	LLCondition						mSignal;

	std::string						mGetMeshCapability;
	U32								mGetMeshVersion;

	// Map of known mesh headers
	typedef flat_hmap<LLUUID, LLMeshHeader*> mesh_header_map_t;
	mesh_header_map_t				mMeshHeaders;

	// Queue of requested headers
	typedef std::deque<HeaderRequest> header_req_queue_t;
	header_req_queue_t				mHeaderReqQ;

	// Queue of requested LODs
	typedef std::deque<LODRequest> lod_req_queue_t;
	lod_req_queue_t					mLODReqQ;

	// List of unavailable LODs (either asset does not exist or asset does not
	// have desired LOD)
	typedef std::list<LODRequest> lod_req_list_t;
	lod_req_list_t					mUnavailableLODs;

	// List of unavailable skin infos.
	uuid_vec_t						mUnavailableSkins;

	// List of successfully loaded meshes
	typedef std::list<LoadedMesh> loaded_mesh_list_t;
	loaded_mesh_list_t				mLoadedMeshes;

	// Set of requested skin info
	base_requests_set_t				mSkinRequests;

	// List of completed skin info requests
	typedef std::list<LLMeshSkinInfo*> skin_info_list_t;
	skin_info_list_t				mSkinInfos;

	// Set of requested decompositions
	base_requests_set_t				mDecompositionRequests;

	// Set of requested physics shapes
	base_requests_set_t				mPhysicsShapeRequests;

	// List of completed decomposition info requests
	typedef std::list<LLModel::Decomposition*> decomp_list_t;
	decomp_list_t					mDecompositions;

	// CoreHttp interface objects.
	LLCore::HttpStatus				mHttpStatus;
	LLCore::HttpRequest*			mHttpRequest;
	LLCore::HttpOptions::ptr_t		mHttpOptions;
	LLCore::HttpOptions::ptr_t		mHttpLargeOptions;
	LLCore::HttpHeaders::ptr_t		mHttpHeaders;
	LLCore::HttpRequest::policy_t	mHttpPolicyClass;
	LLCore::HttpRequest::policy_t	mHttpLegacyPolicyClass;
	LLCore::HttpRequest::policy_t	mHttpLargePolicyClass;

	// Outstanding HTTP requests
	typedef fast_hset<LLCore::HttpHandler::ptr_t> http_request_t;
	http_request_t					mHttpRequestSet;

	// Map of pending header requests and currently desired LODs
	typedef fast_hmap<LLUUID,
					  std::array<S32, LLModel::NUM_LODS> > pending_lod_map_t;
	pending_lod_map_t				mPendingLOD;

	static LLAtomicS32				sActiveHeaderRequests;
	static LLAtomicS32				sActiveLODRequests;
	static U32						sMaxConcurrentRequests;
	static S32						sRequestLowWater;
	static S32						sRequestHighWater;
	// Stats-use only, may read outside of thread
	static S32						sRequestWaterLevel;
};

//
// Observers for model uploads
//

class LLWholeModelFeeObserver
{
protected:
	LOG_CLASS(LLWholeModelFeeObserver);

public:
	LLWholeModelFeeObserver()
	{
		mWholeModelFeeObserverHandle.bind(this);
	}

	virtual ~LLWholeModelFeeObserver() = default;

	virtual void onModelPhysicsFeeReceived(const LLSD& result,
										   std::string upload_url) = 0;
	virtual void setModelPhysicsFeeErrorStatus(S32 status,
											   const std::string& reason,
											   const LLSD& result) = 0;

	LL_INLINE LLHandle<LLWholeModelFeeObserver> getWholeModelFeeObserverHandle() const
	{
		return mWholeModelFeeObserverHandle;
	}

protected:
	LLRootHandle<LLWholeModelFeeObserver> mWholeModelFeeObserverHandle;
};

class LLWholeModelUploadObserver
{
protected:
	LOG_CLASS(LLWholeModelUploadObserver);

public:
	LLWholeModelUploadObserver()
	{
		mWholeModelUploadObserverHandle.bind(this);
	}

	virtual ~LLWholeModelUploadObserver() = default;

	virtual void onModelUploadSuccess() = 0;

	virtual void onModelUploadFailure() = 0;

	LL_INLINE LLHandle<LLWholeModelUploadObserver> getWholeModelUploadObserverHandle() const
	{
		return mWholeModelUploadObserverHandle;
	}

protected:
	LLRootHandle<LLWholeModelUploadObserver> mWholeModelUploadObserverHandle;
};

// Class whose instances represent a single upload-type request for meshes: one
// fee query or one actual upload attempt. Yes, it creates a unique thread for
// that single request. As it is 1:1, it can also trivially serve as the
// HttpHandler object for request completion notifications.
class LLMeshUploadThread final : public LLThread, public LLCore::HttpHandler
{
protected:
	LOG_CLASS(LLMeshUploadThread);

public:
	typedef std::vector<LLModelInstance> instance_list_t;

	LLMeshUploadThread(instance_list_t& data, const strings_map_t& lod_sources,
					   LLVector3& scale, bool upload_tex, bool upload_skin,
					   bool upload_joints, bool lock_scale_if_joint_position,
					   const std::string& upload_url, bool do_upload = true,
					   LLHandle<LLWholeModelFeeObserver> fee_observer =
						(LLHandle<LLWholeModelFeeObserver>()),
					   LLHandle<LLWholeModelUploadObserver> upload_observer =
						(LLHandle<LLWholeModelUploadObserver>()));
	~LLMeshUploadThread() override;

	LL_INLINE bool finished() const								{ return mFinished; }
	void run() override;
	void preStart();
	void discard();
	bool isDiscarded();

	void generateHulls();

	void doWholeModelUpload();
	void requestWholeModelFee();

	void wholeModelToLLSD(LLSD& dest, bool include_textures);

	void decomposeMeshMatrix(LLMatrix4& transformation,
							 LLVector3& result_pos,
							 LLQuaternion& result_rot,
							 LLVector3& result_scale);

	LL_INLINE void setFeeObserverHandle(LLHandle<LLWholeModelFeeObserver> obs)
	{
		mFeeObserverHandle = obs;
	}

	LL_INLINE void setUploadObserverHandle(LLHandle<LLWholeModelUploadObserver> obs)
	{
		mUploadObserverHandle = obs;
	}

	// Inherited from LLCore::HttpHandler
	void onCompleted(LLCore::HttpHandle handle,
					 LLCore::HttpResponse* responsep) override;

	LLViewerFetchedTexture* findViewerTexture(const LLImportMaterial& mat);

	class DecompRequest final : public LLPhysicsDecomp::Request
	{
	public:
		DecompRequest(LLModel* modelp, LLModel* base_modelp,
					  LLMeshUploadThread* threadp);

		LL_INLINE S32 statusCallback(const char*, S32, S32) override
		{
			return 1;
		}

		void completed() override;

	public:
		LLPointer<LLModel>	mModel;
		LLPointer<LLModel>	mBaseModel;
		LLMeshUploadThread*	mThread;
	};

private:
	void packModelInst(LLModel* modelp,
					   LLMeshUploadThread::instance_list_t instances,
					   std::string& model_name, LLSD& res,
					   S32& mesh_num, S32& tex_num, S32& inst_num,
					   fast_hset<LLViewerTexture*>& textures,
					   flat_hmap<LLViewerTexture*, S32>& tex_idx_map,
					   flat_hmap<LLModel*, S32>& mesh_idx_map,
					   bool include_textures);

public:
	LLPointer<DecompRequest> mFinalDecomp;
	volatile bool	mPhysicsComplete;

	typedef std::map<LLPointer<LLModel>, std::vector<LLVector3> > hull_map_t;
	hull_map_t		mHullMap;

	instance_list_t	mInstanceList;

	// In order to upload models in a deterministic order, make it so that
	// they get naturally sorted by model name in the map (this saves us from
	// performing costly copy and sorting on upload). HB
	struct LLModelInstanceLess
	{
		LL_INLINE bool operator()(const LLPointer<LLModel>& a,
								  const LLPointer<LLModel>& b) const
		{
			return a.isNull() || b.isNull() || a->mLabel < b->mLabel;
		}
	};
	typedef std::map<LLPointer<LLModel>, instance_list_t,
					 LLModelInstanceLess> instance_map_t;
	instance_map_t	mInstance;

	strings_map_t	mLodSources;

	LLHost			mHost;
	std::string		mWholeModelUploadURL;

	LLMutex			mMutex;
	LLVector3		mOrigin;
	S32				mPendingUploads;
	bool			mFinished;
	bool			mUploadTextures;
	bool			mUploadSkin;
	bool			mUploadJoints;
	bool			mLockScaleIfJointPosition;
	volatile bool	mDiscarded;

private:
	// Maximum time in seconds to execute an uploading request.
	S32										mMeshUploadTimeOut;

	LLHandle<LLWholeModelFeeObserver>		mFeeObserverHandle;
	LLHandle<LLWholeModelUploadObserver>	mUploadObserverHandle;

	// CoreHttp library interface objects.
	LLCore::HttpStatus						mHttpStatus;
	LLCore::HttpRequest*					mHttpRequest;
	LLCore::HttpOptions::ptr_t				mHttpOptions;
	LLCore::HttpHeaders::ptr_t				mHttpHeaders;
	LLCore::HttpRequest::policy_t			mHttpPolicyClass;

	LLSD									mModelData;

	// If false only model data will be requested, otherwise the model will be
	// uploaded
	bool									mDoUpload;
};

class LLMeshRepository
{
protected:
	LOG_CLASS(LLMeshRepository);

public:
	LLMeshRepository();

	void init();
	void shutdown();
	S32 update();

	LLMeshCostData* getCostData(const LLUUID& mesh_id);

	// Estimated triangle count of the largest LOD
	F32 getEstTrianglesMax(const LLUUID& mesh_id);
	F32 getEstTrianglesStreamingCost(const LLUUID& mesh_id);

	// This must be called on destruction by any LLVOVolume that was flagged
	// as referenced by the mesh repository.
	void unregisterVolume(LLVOVolume* volp, bool has_mesh, bool has_skin);

	// Mesh management functions
	S32 loadMesh(LLVOVolume* volume, const LLVolumeParams& mesh_params,
				 S32 detail = 0, S32 last_lod = -1);

	void notifyLoadedMeshes();
	void notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volp);
	void notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod);
	void notifySkinInfoReceived(LLMeshSkinInfo* vobjp);
	void notifySkinInfoUnavailable(const LLUUID& info);
	void notifyDecompositionReceived(LLModel::Decomposition* vobjp);

	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	static S32 getActualMeshLOD(LLMeshHeader* header, S32 lod);
	const LLPointer<LLMeshSkinInfo> getSkinInfo(const LLUUID& mesh_id,
												LLVOVolume* req_objp);

	LLModel::Decomposition* getDecomposition(const LLUUID& mesh_id);
	void fetchPhysicsShape(const LLUUID& mesh_id);
	bool hasPhysicsShape(const LLUUID& mesh_id);

	void buildHull(const LLVolumeParams& params, S32 detail);
	void buildPhysicsMesh(LLModel::Decomposition& decomp);

	bool meshUploadEnabled();

	void uploadModel(LLMeshUploadThread::instance_list_t& data,
					 const strings_map_t& lod_sources, LLVector3& scale,
					 bool upload_textures, bool upload_skin,
					 bool upload_joints, bool lock_scale_if_joint_position,
					 std::string upload_url, bool do_upload = true,
					 LLHandle<LLWholeModelFeeObserver> fee_observer =
						(LLHandle<LLWholeModelFeeObserver>()),
					 LLHandle<LLWholeModelUploadObserver> upload_observer =
						(LLHandle<LLWholeModelUploadObserver>()));

	S32 getMeshSize(const LLUUID& mesh_id, S32 lod);

	struct InventoryData
	{
		InventoryData(const LLSD& data, const LLSD& content)
		:	mPostData(data),
			mResponse(content)
		{
		}

		LLSD mPostData;
		LLSD mResponse;
	};

	void uploadError(const LLSD& args);
	void updateInventory(InventoryData data);

#if !LL_PENDING_MESH_REQUEST_SORTING
	// Called (from llagent.cpp) during a teleport into another region, to push
	// pending requests into a delayed queue so to give priority to the arrival
	// sim mesh objects rezzing over the departure ones.
	void delayCurrentRequests();
#endif

public:
	LLMeshRepoThread*							mThread;
	LLPhysicsDecomp*							mDecompThread;
	std::vector<LLMeshUploadThread*>			mUploads;
	std::vector<LLMeshUploadThread*>			mUploadWaitList;

	LLMutex										mMeshMutex;
	LLMutex										mSkinMapMutex;

	typedef fast_hmap<LLUUID, fast_hset<LLVOVolume*> > mesh_load_map_t;
	mesh_load_map_t								mLoadingMeshes[4];

	typedef flat_hmap<LLUUID, LLPointer<LLMeshSkinInfo> > skin_map_t;
	skin_map_t									mSkinMap;

	typedef flat_hmap<LLUUID, LLModel::Decomposition*> decomp_map_t;
	decomp_map_t								mDecompositionMap;

#if LL_PENDING_MESH_REQUEST_SORTING
	std::vector<LLMeshRepoThread::LODRequest>	mPendingRequests;
#else
	LLMeshRepoThread::lod_req_queue_t			mPendingRequests;
	LLMeshRepoThread::lod_req_queue_t			mDelayedPendingRequests;
#endif

	// List of mesh ids awaiting skin info
	typedef fast_hmap<LLUUID, fast_hset<LLVOVolume*> > skin_load_map_t;
	skin_load_map_t								mLoadingSkins;

	// List of mesh ids that need to send skin info fetch requests
	std::queue<LLUUID>							mPendingSkinRequests;

	// List of mesh ids awaiting decompositions
	uuid_list_t									mLoadingDecompositions;

	// List of mesh ids that need to send decomposition fetch requests
	std::queue<LLUUID>							mPendingDecompositionRequests;

	// List of mesh ids awaiting physics shapes
	uuid_list_t									mLoadingPhysicsShapes;

	// List of mesh ids that need to send physics shape fetch requests
	std::queue<LLUUID>							mPendingPhysicsShapeRequests;

	std::queue<InventoryData>					mInventoryQ;

	std::queue<LLSD>							mUploadErrorQ;

	// Costs data cache; must only be modified while mHeaderMutex is locked
	typedef flat_hmap<LLUUID, LLPointer<LLMeshCostData> > mesh_costs_map_t;
	mesh_costs_map_t							mCostsMap;

	// Metrics

	static LLAtomicU32							sLODPending;
	static LLAtomicU32							sLODProcessing;

	static U32									sBytesReceived;
	// Total request count, http or cached, all component types:
	static U32									sMeshRequestCount;
	// HTTP GETs issued (not large)
	static U32									sHTTPRequestCount;
	// HTTP GETs issued for large requests
	static U32									sHTTPLargeRequestCount;
	// Total request retries whether successful or failed
	static U32									sHTTPRetryCount;
	// Requests that ended in error
	static U32									sHTTPErrorCount;

	static U32									sCacheBytesRead;
	static U32									sCacheBytesWritten;
	static U32									sCacheReads;
	static U32									sCacheWrites;

	// Maximum sequential locking failures
	static U32									sMaxLockHoldoffs;
};

extern LLMeshRepository gMeshRepo;

// For use with boost::unordered_map and boost::unordered_set
LL_INLINE size_t hash_value(const LLMeshRepoThread::UUIDBasedRequest& rhs) noexcept
{
	return *((size_t*)rhs.mId.mData);
}

// std::hash implementation for LLMeshRepoThread::UUIDBasedRequest
namespace std
{
	template<> struct hash<LLMeshRepoThread::UUIDBasedRequest>
	{
		LL_INLINE size_t operator()(const LLMeshRepoThread::UUIDBasedRequest& rhs) const noexcept
		{
			return *((size_t*)rhs.mId.mData);
		}
	};
}

constexpr F32 ANIMATED_OBJECT_BASE_COST = 15.f;
constexpr F32 ANIMATED_OBJECT_COST_PER_KTRI = 1.5f;
