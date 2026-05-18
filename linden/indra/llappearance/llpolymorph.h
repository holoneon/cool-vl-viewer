/**
 * @file llpolymorph.h
 * @brief Implementation of LLPolyMesh class
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

#include <string>
#include <vector>

#include "llfile.h"
#include "llviewervisualparam.h"

class LLAvatarJointCollisionVolume;
class LLPolyMeshSharedData;
class LLVector2;
class LLWearable;

class alignas(16) LLPolyMorphData
{
protected:
	LOG_CLASS(LLPolyMorphData);

public:
	LL_ALIGNED16_NEW_DELETE

	LLPolyMorphData(const char* morph_name);
	~LLPolyMorphData();
	LLPolyMorphData(const LLPolyMorphData& rhs);

	LL_INLINE bool isSuccesfullyAllocated()				{ return mSuccessfullyAllocated; }

	bool loadBinary(LLFILE* fp, LLPolyMeshSharedData* mesh);

	LL_INLINE const std::string& getName()				{ return mName; }

private:
	void freeData();

public:
	// Average vertex distortion, to infer directionality of the morph
	LLVector4a				mAvgDistortion;

	// Morphology
	LLVector4a*				mCoords;
	LLVector4a*				mNormals;
	LLVector4a*				mBinormals;
	LLVector2*				mTexCoords;
	U32*					mVertexIndices;
	U32						mNumIndices;
	U32						mCurrentIndex;

	LLPolyMeshSharedData*	mMesh;

	// Vertex distortion summed over entire morph
	F32						mTotalDistortion;

	// Maximum single vertex distortion in a given morph
	F32						mMaxDistortion;

	std::string				mName;

private:
	bool					mSuccessfullyAllocated;
};

class LLPolyVertexMask
{
public:
	LLPolyVertexMask(LLPolyMorphData* morph_data);
	LLPolyVertexMask(const LLPolyVertexMask& other);
	~LLPolyVertexMask();

	void generateMask(U8* mask_data, S32 width, S32 height, S32 num_components,
					  bool invert, LLVector4a* clothing_weights);
	F32* getMorphMaskWeights();

protected:
	F32*				mWeights;
	LLPolyMorphData*	mMorphData;
	bool				mWeightsGenerated;
};

struct LLPolyVolumeMorphInfo
{
	LLPolyVolumeMorphInfo(const std::string& name, const LLVector3& scale,
						  const LLVector3& pos)
	:	mName(name),
		mScale(scale),
		mPos(pos)
	{
	}

	std::string			mName;
	LLVector3			mScale;
	LLVector3			mPos;
};

struct LLPolyVolumeMorph
{
	LLPolyVolumeMorph(LLAvatarJointCollisionVolume* volume,
					  const LLVector3& scale, const LLVector3& pos)
	:	mVolume(volume),
		mScale(scale),
		mPos(pos)
	{
	}

	LLAvatarJointCollisionVolume*	mVolume;
	LLVector3						mScale;
	LLVector3						mPos;
};

// Shared information for LLPolyMorphTargets
class LLPolyMorphTargetInfo : public LLViewerVisualParamInfo
{
	friend class LLPolyMorphTarget;

protected:
	LOG_CLASS(LLPolyMorphTargetInfo);

public:
	LLPolyMorphTargetInfo();

	bool parseXml(LLXmlTreeNode* node) override;

protected:
	std::string			mMorphName;
	bool				mIsClothingMorph;
	typedef std::vector<LLPolyVolumeMorphInfo> volume_info_list_t;
	volume_info_list_t	mVolumeInfoList;
};

// A set of vertex data associated with morph target. These morph targets must
// be topologically consistent with a given Polymesh (share face sets)
class alignas(16) LLPolyMorphTarget : public LLViewerVisualParam
{
protected:
	LOG_CLASS(LLPolyMorphTarget);

	LLPolyMorphTarget(const LLPolyMorphTarget& other);

public:
	LLPolyMorphTarget(LLPolyMesh* poly_mesh);
	~LLPolyMorphTarget();

	LL_INLINE LLPolyMorphTarget* asPolyMorphTarget() override
	{
		return this;
	}

	// Special: These functions are overridden by child classes
	LL_INLINE LLPolyMorphTargetInfo* getInfo() const	{ return (LLPolyMorphTargetInfo*)mInfo; }

	// Sets mInfo and calls initialization functions
	bool setInfo(LLPolyMorphTargetInfo* info);

	LLViewerVisualParam* cloneParam(LLWearable* wearable) const override;

	// LLVisualParam Virtual function
	void apply(ESex sex) override;

#if 0	// Unused methods
	// LLViewerVisualParam Virtual functions
	F32 getTotalDistortion() override;
	const LLVector4a& getAvgDistortion() override;
	F32 getMaxDistortion() override;
	LLVector4a getVertexDistortion(S32 index, LLPolyMesh* mesh) override;
	const LLVector4a* getFirstDistortion(U32* idx, LLPolyMesh** mesh) override;
	const LLVector4a* getNextDistortion(U32* idx, LLPolyMesh** mesh) override;
#endif

	void applyMask(U8* mask_data, S32 width, S32 height, S32 num_components,
				   bool invert);

	LL_INLINE void addPendingMorphMask()				{ ++mNumMorphMasksPending; }

	// Also used by LLVOAvatar::resetSkeleton()
	void applyVolumeChanges(F32 delta_weight);

protected:
	LLPolyMorphData*				mMorphData;
	LLPolyMesh*						mMesh;
	LLPolyVertexMask*				mVertMask;
	ESex							mLastSex;

	// Number of morph masks that haven't been generated, must be 0 before
	// this morph is applied
	S32								mNumMorphMasksPending;

	typedef std::vector<LLPolyVolumeMorph> volume_list_t;
	volume_list_t 					mVolumeMorphs;

};
