/**
 * @file llflexibleobject.h
 * @author JJ Ventrella, Andrew Meadows, Tom Yedwab
 * @brief Flexible object definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

// This is for specifying objects in the world that are animated and rendered
// viewer-side. Flexible Objects are linear arrays of positions, which stay at
// a fixed distance from each other. One position is fixed as an "anchor" and
// is attached to some other object in the world, determined by the server. All
// the other positions are updated according to local physics.

#pragma once

#include "llvovolume.h"

// 10 ms for the whole thing !
constexpr F32 FLEXIBLE_OBJECT_TIMESLICE	= 0.003f;
constexpr U32 FLEXIBLE_OBJECT_MAX_LOD	= 10;

// See llprimitive.h for LLFlexibleObjectData and DEFAULT/MIN/MAX values

struct LLFlexibleObjectSection
{
	// Input parameters
	LLVector2		mScale;
	LLQuaternion	mAxisRotation;
	// Simulated state
	LLVector3		mPosition;
	LLVector3		mVelocity;
	LLVector3		mDirection;
	LLQuaternion	mRotation;
	// Derivatives (Not all currently used, will come back with LLVolume
	// changes to automagically generate normals)
	LLVector3		mdPosition;
	//LLMatrix4		mRotScale;
	//LLMatrix4		mdRotScale;
};

class LLVolumeImplFlexible final : public LLVolumeInterface
{
protected:
	LOG_CLASS(LLVolumeImplFlexible);

public:
	static void initClass();
	static void updateClass();
	static void dumpStats();

	LLVolumeImplFlexible(LLViewerObject* volumep,
						 LLFlexibleObjectData* attributesp);

	~LLVolumeImplFlexible() override;

	// LLVolumeInterface overrides

	LL_INLINE LLVolumeInterfaceType getInterfaceType() const override
	{
		return INTERFACE_FLEXIBLE;
	}

	void doIdleUpdate() override;

	bool doUpdateGeometry(LLDrawable* drawablep) override;

	LLVector3 getPivotPosition() const override;

	LL_INLINE void onSetVolume(const LLVolumeParams&, S32) override
	{
	}

	void onSetScale(const LLVector3& scale, bool damped) override;

	void onParameterChanged(U16 param_type, LLNetworkData* datap, bool in_use,
							bool local_origin) override;

	void onShift(const LLVector4a& shift_vector) override;

	LL_INLINE bool isVolumeUnique() const override			{ return true; }
	LL_INLINE bool isVolumeGlobal() const override			{ return true; }
	LL_INLINE bool isActive() const override				{ return true; }

	const LLMatrix4& getWorldMatrix(LLXformMatrix* xformp) const override;

	void updateRelativeXform(bool force_identity = false) override;

	LL_INLINE U32 getID() const override					{ return mID; }

	void preRebuild() override;

	LLVector3 getFramePosition() const;
	LLQuaternion getFrameRotation() const;

	// New methods

	void updateRenderRes();
	void doFlexibleUpdate(); // Called to update the simulation

	// Called to rebuild the geometry:
	void doFlexibleRebuild(bool rebuild_volume);

	void setParentPositionAndRotationDirectly(LLVector3 p, LLQuaternion r);

	LL_INLINE void setCollisionSphere(LLVector3 position, F32 radius)
	{
		mCollisionSpherePosition = position;
		mCollisionSphereRadius = radius;
	}

	LLVector3 getEndPosition();
	LLQuaternion getEndRotation();
	LLVector3 getNodePosition(S32 node_idx);
	LLVector3 getAnchorPosition() const;

private:
	void setAttributesOfAllSections	(LLVector3* in_scalep = NULL);

	void remapSections(LLFlexibleObjectSection* srcp, S32 source_sections,
					   LLFlexibleObjectSection* dstp, S32 dest_sections);

private:
    // Backlink only; do not make this an LLPointer.
	LLViewerObject*				mVO;

	LLTimer						mTimer;
	LLVector3					mAnchorPosition;
	LLVector3					mParentPosition;
	LLQuaternion				mParentRotation;
	LLQuaternion				mLastFrameRotation;
	LLQuaternion				mLastSegmentRotation;
	LLFlexibleObjectData*		mAttributes;
	LLFlexibleObjectSection		mSection[(1 << FLEXIBLE_OBJECT_MAX_SECTIONS) + 1];
	S32							mInitializedRes;
	S32							mSimulateRes;
	S32							mRenderRes;
	U64							mLastFrameNum;
	U32							mLastUpdatePeriod;
	LLVector3					mCollisionSpherePosition;
	F32							mCollisionSphereRadius;
	U32							mID;
	bool						mInitialized;
	bool						mUpdated;

	S32							mInstanceIndex;

	typedef std::vector<LLVolumeImplFlexible*> instances_list_t;
	static instances_list_t		sInstanceList;

public:
	// Global setting for update rate
	static F32					sUpdateFactor;
};
