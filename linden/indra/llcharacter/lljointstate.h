/**
 * @file lljointstate.h
 * @brief Implementation of LLJointState class.
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

#include "llerror.h"
#include "lljoint.h"
#include "llpreprocessor.h"
#include "llrefcount.h"

class LLJointState : public LLRefCount
{
public:
	enum BlendPhase
	{
		INACTIVE,
		EASE_IN,
		ACTIVE,
		EASE_OUT
	};

	// Constructors

	LLJointState()
	:	mUsage(0),
		mJoint(NULL),
		mWeight(0.f),
		mPriority(LLJoint::USE_MOTION_PRIORITY)
	{
	}

	LLJointState(LLJoint* joint)
	:	mUsage(0),
		mJoint(joint),
		mWeight(0.f),
		mPriority(LLJoint::USE_MOTION_PRIORITY)
	{
	}

	// joint that this state is applied to
	LL_INLINE LLJoint* getJoint()							{ return mJoint; }
	LL_INLINE const LLJoint* getJoint() const				{ return mJoint; }
	LL_INLINE bool setJoint(LLJoint* joint)					{ mJoint = joint; return mJoint != NULL; }

	// Transform type (bitwise flags can be combined). Note that these are set
	// automatically when various member setPos/setRot/setScale functions are
	// called.
	enum Usage
	{
		POS		= 1,
		ROT		= 2,
		SCALE	= 4,
	};

	LL_INLINE U32 getUsage() const							{ return mUsage; }
	LL_INLINE void setUsage(U32 usage)						{ mUsage = usage; }
	LL_INLINE F32 getWeight() const							{ return mWeight; }
	LL_INLINE void setWeight(F32 weight)					{ mWeight = weight; }

	// get/set position
	LL_INLINE const LLVector3& getPosition() const			{ return mPosition; }
	LL_INLINE void setPosition(const LLVector3& pos)		{ llassert(mUsage & POS); mPosition = pos; }

	// get/set rotation
	LL_INLINE const LLQuaternion& getRotation() const		{ return mRotation; }
	LL_INLINE void setRotation(const LLQuaternion& rot)		{ llassert(mUsage & ROT); mRotation = rot; }

	// get/set scale
	LL_INLINE const LLVector3& getScale() const				{ return mScale; }
	LL_INLINE void setScale(const LLVector3& scale)			{ llassert(mUsage & SCALE); mScale = scale; }

	// get/set priority
	LL_INLINE LLJoint::JointPriority getPriority() const	{ return mPriority; }
	LL_INLINE void setPriority(LLJoint::JointPriority p)	{ mPriority = p; }

protected:
	~LLJointState() override = default;

protected:
	// Note: the first member variable is 32 bits in order to align on 64 bits
	// for the next variables, counting the 32 bits counter from LLRefCount. HB

	// Indicates which members are used
	U32						mUsage;

	// Associated joint
	LLJoint*				mJoint;

	// Transformation members
	LLQuaternion			mRotation;	// Joint rotation relative to parent
	LLVector3				mPosition;	// Position relative to parent joint
	LLVector3				mScale;		// Scale relative to rotated frame

	// Indicates weighted effect of this joint
	F32						mWeight;

	// How important this joint state is relative to others
	LLJoint::JointPriority	mPriority;
};
