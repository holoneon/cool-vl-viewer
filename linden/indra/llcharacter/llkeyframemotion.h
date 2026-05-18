/**
 * @file llkeyframemotion.h
 * @brief Implementation of LLKeframeMotion class.
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

#include "llassetstorage.h"
#include "llbboxlocal.h"
#include "llbvhconsts.h"
#include "hbfastmap.h"
#include "llhandmotion.h"
#include "lljointstate.h"
#include "llmotion.h"
#include "llquaternion.h"
#include "lluuid.h"
#include "llvector3d.h"
#include "llvector3.h"

class LLKeyframeDataCache;
class LLDataPacker;

#define MIN_REQUIRED_PIXEL_AREA_KEYFRAME (40.f)
#define MAX_CHAIN_LENGTH (4)

constexpr S32 KEYFRAME_MOTION_VERSION = 1;
constexpr S32 KEYFRAME_MOTION_SUBVERSION = 0;

class LLKeyframeMotion : public LLMotion
{
	friend class LLKeyframeDataCache;

protected:
	LOG_CLASS(LLKeyframeMotion);

public:
	LLKeyframeMotion(const LLUUID& id);
	~LLKeyframeMotion() override;

	LL_INLINE LLKeyframeMotion* asKeyframeMotion() override
	{
		return this;
	}

	LL_INLINE static LLMotion* create(const LLUUID& id)
	{
		return new LLKeyframeMotion(id);
	}

	// Motions must specify whether or not they loop
	LL_INLINE bool getLoop() override
	{
		return mJointMotionList && mJointMotionList->mLoop;
	}

	// Motions must report their total duration
	LL_INLINE F32 getDuration() override
	{
		return mJointMotionList ? mJointMotionList->mDuration : 0.f;
	}

	// Motions must report their "ease in" duration
	LL_INLINE F32 getEaseInDuration() override
	{
		return mJointMotionList ? mJointMotionList->mEaseInDuration : 0.f;
	}

	// Motions must report their "ease out" duration.
	LL_INLINE F32 getEaseOutDuration() override
	{
		return mJointMotionList ? mJointMotionList->mEaseOutDuration : 0.f;
	}

	// Motions must report their priority
	LL_INLINE LLJoint::JointPriority getPriority() override
	{
		return mJointMotionList ? mJointMotionList->mBasePriority
								: LLJoint::LOW_PRIORITY;
	}

	LL_INLINE LLMotionBlendType getBlendType() override	{ return NORMAL_BLEND; }

	// Called to determine when a motion should be activated/deactivated based
	// on avatar pixel coverage
	LL_INLINE F32 getMinPixelArea() override			{ return MIN_REQUIRED_PIXEL_AREA_KEYFRAME; }

	// Run-time (post constructor) initialization, called after parameters
	// have been set; must return true to indicate success and be available
	// for activation
	LLMotionInitStatus onInitialize(LLCharacter* character) override;

	// Called when a motion is activated; must return true to indicate success,
	// or else it will be deactivated
	bool onActivate() override;

	// Called per time step; must return true while it is active, and must
	// return false when the motion is completed.
	bool onUpdate(F32 time, U8* joint_mask) override;

	// Called when a motion is deactivated
	void onDeactivate() override;

	void setStopTime(F32 time) override;

	LL_INLINE void setCharacter(LLCharacter* character)	{ mCharacter = character; }

	static void onLoadComplete(const LLUUID& asset_uuid, LLAssetType::EType,
							   void* user_data, S32 status, LLExtStat);

	U32 getFileSize();

	bool serialize(LLDataPacker& dp) const;
	bool deserialize(LLDataPacker& dp, const LLUUID& asset_id,
					 bool allow_invalid_joints = true);

	LL_INLINE bool isLoaded()							{ return mJointMotionList != NULL; }

	bool dumpToFile(const std::string& name);

	// setters for modifying a keyframe animation
	void setLoop(bool loop);

	LL_INLINE F32 getLoopIn()
	{
		return mJointMotionList ? mJointMotionList->mLoopInPoint : 0.f;
	}

	LL_INLINE F32 getLoopOut()
	{
		return mJointMotionList ? mJointMotionList->mLoopOutPoint : 0.f;
	}

	void setLoopIn(F32 in_point);

	void setLoopOut(F32 out_point);

	LL_INLINE void setHandPose(LLHandMotion::eHandPose pose)
	{
		if (mJointMotionList)
		{
			mJointMotionList->mHandPose = pose;
		}
	}

	LL_INLINE LLHandMotion::eHandPose getHandPose()
	{
		return mJointMotionList ? mJointMotionList->mHandPose
								: LLHandMotion::HAND_POSE_RELAXED;
	}

	void setPriority(S32 priority);

	void setEmote(const LLUUID& emote_id);

	void setEaseIn(F32 ease_in);

	void setEaseOut(F32 ease_in);

	LL_INLINE F32 getLastUpdateTime()					{ return mLastLoopedTime; }

	const LLBBoxLocal& getPelvisBBox();

	static void flushKeyframeCache();

private:
	// Private helper functions to wrap some asserts
	LLPointer<LLJointState>& getJointState(U32 index);
	LLJoint* getJoint(U32 index);

protected:
	class JointConstraintSharedData
	{
	public:
		JointConstraintSharedData()
		:	mChainLength(0),
			mEaseInStartTime(0.f),
			mEaseInStopTime(0.f),
			mEaseOutStartTime(0.f),
			mEaseOutStopTime(0.f),
			mUseTargetOffset(false),
			mConstraintType(CONSTRAINT_TYPE_POINT),
			mConstraintTargetType(CONSTRAINT_TARGET_TYPE_BODY),
			mSourceConstraintVolId(-1),
			mTargetConstraintVolId(-1),
			mJointStateIndices(NULL)
		{
		}

		LL_INLINE ~JointConstraintSharedData()			{ delete[] mJointStateIndices; }

	public:
		S32						mSourceConstraintVolId;
		S32						mTargetConstraintVolId;
		LLVector3				mSourceConstraintOffset;
		LLVector3				mTargetConstraintOffset;
		LLVector3				mTargetConstraintDir;
		S32						mChainLength;
		S32*					mJointStateIndices;
		F32						mEaseInStartTime;
		F32						mEaseInStopTime;
		F32						mEaseOutStartTime;
		F32						mEaseOutStopTime;
		EConstraintType			mConstraintType;
		bool					mUseTargetOffset;
		EConstraintTargetType	mConstraintTargetType;
	};

	class JointConstraint
	{
	public:
		JointConstraint(JointConstraintSharedData* shared_data);

	public:
		JointConstraintSharedData*	mSharedData;
		LLJoint*					mSourceVolume;
		LLJoint*					mTargetVolume;
		F32							mWeight;
		F32							mTotalLength;
		F32							mFixupDistanceRMS;
		F32							mJointLengths[MAX_CHAIN_LENGTH];
		F32							mJointLengthFractions[MAX_CHAIN_LENGTH];
		LLVector3					mPositions[MAX_CHAIN_LENGTH];
		LLVector3					mGroundNorm;
		LLVector3d					mGroundPos;
		bool						mActive;
	};

	void applyKeyframes(F32 time);

	void applyConstraints(F32 time, U8* joint_mask);

	void activateConstraint(JointConstraint* constraintp);

	void initializeConstraint(JointConstraint* constraint);

	void deactivateConstraint(JointConstraint *constraintp);

	void applyConstraint(JointConstraint* constraintp, F32 time,
						 U8* joint_mask);

	bool setupPose();

public:
	enum AssetStatus {
		ASSET_LOADED,
		ASSET_FETCHED,
		ASSET_NEEDS_FETCH,
		ASSET_FETCH_FAILED,
		ASSET_UNDEFINED
	};

	enum InterpolationType { IT_STEP, IT_LINEAR, IT_SPLINE };

	class ScaleKey
	{
	public:
		LL_INLINE ScaleKey()							{ mTime = 0.f; }

		LL_INLINE ScaleKey(F32 time, const LLVector3& scale)
		{
			mTime = time;
			mScale = scale;
		}

	public:
		F32			mTime;
		LLVector3	mScale;
	};

	class RotationKey
	{
	public:
		LL_INLINE RotationKey()							{ mTime = 0.f; }

		LL_INLINE RotationKey(F32 time, const LLQuaternion& rot)
		{
			mTime = time;
			mRotation = rot;
		}

	public:
		F32				mTime;
		LLQuaternion	mRotation;
	};

	class PositionKey
	{
	public:
		LL_INLINE PositionKey()							{ mTime = 0.f; }

		LL_INLINE PositionKey(F32 time, const LLVector3& pos)
		{
			mTime = time;
			mPosition = pos;
		}

	public:
		F32			mTime;
		LLVector3	mPosition;
	};

	class ScaleCurve
	{
	protected:
		LOG_CLASS(LLKeyframeMotion::ScaleCurve);

	public:
		ScaleCurve();
		~ScaleCurve();
		LLVector3 getValue(F32 time, F32 duration);
		LLVector3 interp(F32 u, ScaleKey& before, ScaleKey& after);

	public:
		InterpolationType	mInterpolationType;
		S32					mNumKeys;
		typedef std::map<F32, ScaleKey> key_map_t;
		key_map_t 			mKeys;
		ScaleKey			mLoopInKey;
		ScaleKey			mLoopOutKey;
	};

	class RotationCurve
	{
	protected:
		LOG_CLASS(LLKeyframeMotion::RotationCurve);

	public:
		RotationCurve();
		~RotationCurve();
		LLQuaternion getValue(F32 time, F32 duration);
		LLQuaternion interp(F32 u, RotationKey& before, RotationKey& after);

	public:
		InterpolationType	mInterpolationType;
		S32					mNumKeys;
		typedef std::map<F32, RotationKey> key_map_t;
		key_map_t			mKeys;
		RotationKey			mLoopInKey;
		RotationKey			mLoopOutKey;
	};

	class PositionCurve
	{
	protected:
		LOG_CLASS(LLKeyframeMotion::PositionCurve);

	public:
		PositionCurve();
		~PositionCurve();
		LLVector3 getValue(F32 time, F32 duration);
		LLVector3 interp(F32 u, PositionKey& before, PositionKey& after);

	public:
		InterpolationType	mInterpolationType;
		S32					mNumKeys;
		typedef std::map<F32, PositionKey> key_map_t;
		key_map_t			mKeys;
		PositionKey			mLoopInKey;
		PositionKey			mLoopOutKey;
	};

	class JointMotion
	{
	public:
		void update(LLJointState* joint_state, F32 time, F32 duration);

	public:
		PositionCurve			mPositionCurve;
		RotationCurve			mRotationCurve;
		ScaleCurve				mScaleCurve;
		U32						mJointKey;
		U32						mUsage;
		LLJoint::JointPriority	mPriority;
		std::string				mJointName;
	};

	class JointMotionList
	{
	protected:
		LOG_CLASS(LLKeyframeMotion::JointMotionList);

	public:
		JointMotionList();
		~JointMotionList();
		U32 dumpDiagInfo();

		LL_INLINE JointMotion* getJointMotion(U32 index) const
		{
			return index < mJointMotionArray.size() ? mJointMotionArray[index]
													: NULL;
		}

		LL_INLINE U32 getNumJointMotions() const
		{
			return mJointMotionArray.size();
		}

	public:
		std::vector<JointMotion*>	mJointMotionArray;
		F32							mDuration;
		F32							mLoopInPoint;
		F32							mLoopOutPoint;
		F32							mEaseInDuration;
		F32							mEaseOutDuration;
		LLJoint::JointPriority		mBasePriority;
		LLHandMotion::eHandPose 	mHandPose;
		LLJoint::JointPriority  	mMaxPriority;
		LLBBoxLocal					mPelvisBBox;
		typedef std::list<JointConstraintSharedData*> constraint_list_t;
		constraint_list_t			mConstraints;
		LLUUID						mEmoteID;
		// mEmoteName is a facial motion, but it's necessary to appear here so
		// that it's cached.
		// *TODO: LLKeyframeDataCache::getKeyframeData should probably return a
		// class containing JointMotionList and mEmoteName, see
		// LLKeyframeMotion::onInitialize().
		std::string					mEmoteName;
		bool						mLoop;
	};

protected:
	JointMotionList*	mJointMotionList;
	std::vector<LLPointer<LLJointState> > mJointStates;
	LLJoint*			mPelvisp;
	LLCharacter*		mCharacter;
	typedef std::list<JointConstraint*>	constraint_list_t;
	constraint_list_t	mConstraints;
	U32					mLastSkeletonSerialNum;
	F32					mLastUpdateTime;
	F32					mLastLoopedTime;
	AssetStatus			mAssetStatus;

	static std::string	sStaticAnimsDir;
};

// Purely static class
class LLKeyframeDataCache
{
	LLKeyframeDataCache() = delete;
	~LLKeyframeDataCache() = delete;

protected:
	LOG_CLASS(LLKeyframeDataCache);

public:
	LL_INLINE static void addKeyframeData(const LLUUID& id,
										  LLKeyframeMotion::JointMotionList* ml)
	{
		sKeyframeDataMap[id] = ml;
	}

	static LLKeyframeMotion::JointMotionList* getKeyframeData(const LLUUID& id);

	static void removeKeyframeData(const LLUUID& id);

	// Prints out diagnostic info
	static void dumpDiagInfo();

	static void clear();

private:
	typedef fast_hmap<LLUUID, LLKeyframeMotion::JointMotionList*> data_map_t;
	static data_map_t sKeyframeDataMap;
};
