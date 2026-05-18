/**
 * @file lljointsolverrp3.h
 * @brief Implementation of LLJointSolverRP3 class
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

#include "lljoint.h"

/* ----------------------------------------------------------------------------
// class LLJointSolverRP3
//
// This class is a "poor man's" IK for simple 3 joint kinematic chains.
// It is modeled after the 'ikRPSolver' in Maya.
// This class takes 4 LLJoints:
//   joint_a
//   joint_b
//   joint_c
//   joint_goal
//
// Such that joint_a is the parent of joint_b, joint_b is the parent of joint_c.
// When invoked, this class modifies the rotations of joint_a and joint_b such
// that the position of the joint_c attempts to reach the position of joint_goal.
//
// At object initialization time, the distances between joint_a - joint_b and
// joint_b - joint_c are cached.  During evaluation these bone lengths are
// preserved.
//
//  A          A
//  |          |
//  |          |
//  B          B---CG     A---B---C...G
//   \
//    \
//     CG
//
//
// In addition a "pole_vec" is specified that does two things:
//
// a) defines the plane in which the solution occurs, thus
//    reducing an infinite number of solutions, down to 2.
//
// b) disambiguates the resulting two solutions as follows:
//
//  A             A            A--->pole_vec
//  |              \            \
//  |               \            \
//  B       vs.      B   ==>      B
//   \               |            |
//    \              |            |
//     CG            CG           CG
//
// A "twist" setting allows the solution plane to be rotated about the
// line between A and C.  A handy animation feature.
//
// For "smarter" results for non-coplanar limbs, specify the joints axis
// of bend in the B's local frame (see setBAxis())
// ------------------------------------------------------------------------- */

class LLJointSolverRP3
{
public:
	LLJointSolverRP3();
	virtual ~LLJointSolverRP3() = default;

	// This must be called one time to setup the solver. This must be called
	// AFTER the skeleton has been created, all parent/child relationships are
	// established, and after the joints are placed in a valid configuration
	// (as distances between them will be cached).
	void setupJoints(LLJoint* joint_a, LLJoint* joint_b, LLJoint* joint_c,
					 LLJoint* joint_goal);

	// Returns the current pole vector.
	LL_INLINE const LLVector3& getPoleVector()	{ return mPoleVector; }

	// Sets the pole vector. The pole vector is defined relative to (in the
	// space of) joint_a's parent. The default pole vector is (1,0,0), and this
	// is used if this function/ is never called. This vector is normalized
	// when set.
	LL_INLINE void setPoleVector(const LLVector3& pole_vec)
	{
		mPoleVector = pole_vec;
		mPoleVector.normalize();
	}

	// Sets the joint axis in B's local frame, and enable "smarter" solve().
	// This allows for smarter IK when for twisted limbs.
	LL_INLINE void setBAxis(const LLVector3& b_axis)
	{
		mBAxis = b_axis;
		mBAxis.normalize();
		mUseBAxis = true;
	}

	// Returns the current twist in radians.
	LL_INLINE F32 getTwist()					{ return mTwist; }

	// Sets the twist value. The default is 0.f.
	LL_INLINE void setTwist(F32 twist)			{ mTwist = twist; }

	// This is the "work" function. When called, the rotations of joint_a and
	// joint_b will be modified such that joint_c attempts to reach joint_goal.
	void solve();

protected:
	LLJoint*		mJointA;
	LLJoint*		mJointB;
	LLJoint*		mJointC;
	LLJoint*		mJointGoal;

	F32				mLengthAB;
	F32				mLengthBC;
	F32				mTwist;

	LLVector3		mPoleVector;
	LLVector3		mBAxis;

	LLMatrix4		mSavedJointAMat;
	LLMatrix4		mSavedInvPlaneMat;

	LLQuaternion	mJointABaseRotation;
	LLQuaternion	mJointBBaseRotation;

	bool			mUseBAxis;
};
