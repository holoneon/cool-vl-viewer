/**
 * @file llmodelloader.h
 * @brief LLModelLoader class definition
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

#include <functional>
#include <list>
#include <queue>

#include "llmodel.h"
#include "llstring.h"
#include "llthread.h"

class LLMatrix4a;
class LLJoint;

constexpr S32 SLM_SUPPORTED_VERSION = 3;
constexpr S32 NUM_LOD = 4;

constexpr U32 LEGACY_RIG_FLAG_INVALID = 1;
constexpr U32 LEGACY_RIG_FLAG_NO_JOINT = 2;
constexpr U32 LEGACY_RIG_FLAG_TOO_MANY_JOINTS = 4;
constexpr U32 LEGACY_RIG_FLAG_UNKNOWN_JOINT = 8;

typedef std::map<std::string, LLMatrix4, std::less<> > JointTransformMap;
typedef JointTransformMap:: iterator JointTransformMapIt;
typedef std::deque<std::string> JointNameSet;

class LLModelLoader : public LLThread
{
protected:
	LOG_CLASS(LLModelLoader);

public:
	typedef std::vector<LLPointer<LLModel> > model_list;
	typedef std::vector<LLModelInstance> model_instance_list_t;
	typedef std::map<LLMatrix4, model_instance_list_t> scene;

	// Callback with loaded model data and loaded LoD
	typedef std::function<void(scene&, model_list&, S32,
							   void*)> load_callback_t;

	// Function to provide joint lookup by name (within preview avi skeleton,
	// for example)
	typedef std::function<LLJoint*(const std::string&,
								   void*)> joint_lookup_func_t;

	// Function to load and associate material with all it's textures. The
	// returned value is the number of textures loaded intentionally non-const
	// so func can modify material to store platform-specific data
	typedef std::function<U32 (LLImportMaterial&, void*)> texture_load_func_t;

	// Callback to inform client of state changes during the loading process
	// (errors are reported as state changes here as well)
	typedef std::function<void (U32, void*)> state_callback_t;

	typedef enum
	{
		STARTING = 0,
		READING_FILE,
		CREATING_FACES,
		GENERATING_VERTEX_BUFFERS,
		GENERATING_LOD,
		DONE,
		WARNING_BIND_SHAPE_ORIENTATION,
		ERROR_PARSING,							// Basically, loading failed
		ERROR_MATERIALS,
		ERROR_PASSWORD_REQUIRED,
		ERROR_NEED_MORE_MEMORY,
		ERROR_INVALID_FILE,
		ERROR_LOADER_SETUP,
		ERROR_INVALID_PARAMETERS,
		ERROR_OUT_OF_RANGE,
		ERROR_FILE_VERSION_INVALID,
		ERROR_LOD_MODEL_MISMATCH,
		ERROR_HIGH_LOD_MODEL_MISSING,
		// This error should always be last in this list, error code is passed
		// as ERROR_MODEL+error_code:
		ERROR_MODEL
	} eLoadState;

	LLModelLoader(const std::string& filename, S32 lod,
				  load_callback_t load_cb,
				  joint_lookup_func_t joint_lookup_func,
				  texture_load_func_t texture_load_func,
				  state_callback_t state_cb, void* userdata,
				  JointTransformMap& joint_transform_map,
				  JointNameSet& joints_from_nodes,
				  strings_map_t& legal_joint_names, U32 max_joints_per_mesh,
				  U32 model_limit);

	~LLModelLoader() override;

	void run() override;

	LL_INLINE virtual void setNoNormalize()			{ mNoNormalize = true; }
	LL_INLINE virtual void setNoOptimize()			{ mNoOptimize = true; }

	static bool getSLMFilename(const std::string& model_filename,
							   std::string& slm_filename);

	// Will try SLM or derived class OpenFile as appropriate
	virtual bool doLoadModel();

	// Derived classes need to provide their parsing of files here
	virtual bool openFile(const std::string& filename) = 0;

	bool loadFromSLM(const std::string& filename);

	void setLoadState(U32 state);
	void loadModelCallback();

	// Methods called in the main thread:
	void loadTextures();

	// Determines the viability of an asset to be used as an avatar rig
	// (w or w/o joint upload caps)
	void critiqueRigForUploadApplicability(const std::vector<std::string>& joints);

	LL_INLINE bool isRigValidForJointPositionUpload() const
	{
		return mRigValidJointUpload;
	}

	LL_INLINE void setRigValidForJointPositionUpload(bool b)
	{
		mRigValidJointUpload = b;
	}

	LL_INLINE bool isLegacyRigValid() const			{ return mLegacyRigFlags == 0; }

	LL_INLINE void setLegacyRigValid(bool b)
	{
		mLegacyRigFlags = b ? LEGACY_RIG_FLAG_INVALID : 0;
	}

	LL_INLINE bool getLegacyRigFlags() const		{ return mLegacyRigFlags; }
	LL_INLINE void setLegacyRigFlags(U32 flags)		{ mLegacyRigFlags = flags; }

	LL_INLINE const LLSD& logOut() const			{ return mWarningsArray; }
	LL_INLINE void clearLog()						{ mWarningsArray.clear(); }

	LL_INLINE bool isNodeAJoint(const char* name)
 	{
		return name && mJointMap.find(name) != mJointMap.end();
	}

protected:
	// Determines if a rig is a legacy from the joint list
	U32 determineRigLegacyFlags(const std::vector<std::string>& joints);

	static bool isAlive(LLModelLoader* loader);

public:
	S32									mLod;
	U32									mState;

	LLMatrix4							mTransform;
	LLVector3							mExtents[2];

	std::string							mFilename;

	model_list							mModelList;
	// This contains pretty much what ends up getting loaded for upload.
	// Basically assign things to this guy if you want something uploaded.
	scene								mScene;

	typedef std::queue<LLPointer<LLModel> > model_queue;
	// queue of models that need a physics rep
	model_queue							mPhysicsQ;

	// Map of avatar joints as named in COLLADA assets to internal joint
	// names. Do not use this for anything other than looking up the name of a
	// joint. This is populated elsewhere.
	strings_map_t						mJointMap;
	// The joint list is what you want to use to actually setup the specific
	// joint transformations.
	JointTransformMap&					mJointList;
	JointNameSet&						mJointsFromNode;
	U32									mMaxJointsPerMesh;

	// Attempt to limit amount of generated submodels
	U32									mGeneratedModelLimit;

	bool								mFirstTransform;
	bool								mTrySLM;
	// ignore cached SLM if it does not contain rig info and we want the latter
	bool								mCacheOnlyHitIfRigged;

	bool								mTexturesNeedScaling;

protected:
	bool								mNoNormalize;
	bool								mNoOptimize;

	bool								mRigValidJointUpload;
	U32									mLegacyRigFlags;

	// The model preview floater pulls logs from this
	LLSD								mWarningsArray;

	LLModelLoader::load_callback_t		mLoadCallback;
	LLModelLoader::joint_lookup_func_t	mJointLookupFunc;
	LLModelLoader::texture_load_func_t	mTextureLoadFunc;
	LLModelLoader::state_callback_t		mStateCallback;
	void*								mUserData;

	JointTransformMap					mJointTransformMap;

	static std::list<LLModelLoader*> 	sActiveLoaderList;
};

void stretch_extents(LLModel* model, const LLMatrix4a& mat, LLVector4a& min,
					 LLVector4a& max, bool& first_transform);
void stretch_extents(LLModel* model, const LLMatrix4& mat, LLVector3& min,
					 LLVector3& max, bool& first_transform);
