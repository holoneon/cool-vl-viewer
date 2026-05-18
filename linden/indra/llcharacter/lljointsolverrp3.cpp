/**
 * @file lljointsolverrp3.cpp
 * @brief Implementation of LLJointSolverRP3 class.
 * Joint solver in Real Projective 3D space (RP3).
 * See: https://en.wikipedia.org/wiki/Real_projective_space
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

#include "linden_common.h"

#include "lljointsolverrp3.h"

#include "llmath.h"

#define F_EPSILON 0.00001f

LLJointSolverRP3::LLJointSolverRP3()
:	mJointA(NULL),
	mJointB(NULL),
	mJointC(NULL),
	mJointGoal(NULL),
	mLengthAB(1.f),
	mLengthBC(1.f),
	mUseBAxis(false),
	mTwist(0.f)
{
	mPoleVector.set(1.f, 0.f, 0.f);
}

void LLJointSolverRP3::setupJoints(LLJoint* joint_a, LLJoint* joint_b,
								   LLJoint* joint_c, LLJoint* joint_goal)
{
	mJointA = joint_a;
	mJointB = joint_b;
	mJointC = joint_c;
	mJointGoal = joint_goal;

	mLengthAB = mJointB->getPosition().length();
	mLengthBC = mJointC->getPosition().length();

	mJointABaseRotation = joint_a->getRotation();
	mJointBBaseRotation = joint_b->getRotation();
}

void LLJointSolverRP3::solve()
{
	// Setup joints in their base rotations
	mJointA->setRotation(mJointABaseRotation);
	mJointB->setRotation(mJointBBaseRotation);

	// Get joint positions in world space
	LLVector3 a_pos = mJointA->getWorldPosition();
	LLVector3 b_pos = mJointB->getWorldPosition();
	LLVector3 c_pos = mJointC->getWorldPosition();
	LLVector3 g_pos = mJointGoal->getWorldPosition();

	// Get the pole_vector in world space
	LLMatrix4 world_joint_a_parent_mat;
	if (mJointA->getParent())
	{
		world_joint_a_parent_mat = mJointA->getParent()->getWorldMatrix();
	}
	LLVector3 pole_vec = rotate_vector(mPoleVector, world_joint_a_parent_mat);

	LLVector3 ab_vec = b_pos - a_pos;	// vector from A to B
	LLVector3 bc_vec = c_pos - b_pos;	// vector from B to C
	LLVector3 ac_vec = c_pos - a_pos;	// vector from A to C
	LLVector3 ag_vec = g_pos - a_pos;	// vector from A to G (goal)

	// Compute needed lengths of those vectors
	F32 ab_len = ab_vec.length();
	F32 bc_len = bc_vec.length();
	F32 ag_len = ag_vec.length();

	// Compute component vector of (A->B) orthogonal to (A->C)
	LLVector3 abac_comp_ortho_vec = ab_vec - ac_vec * ((ab_vec * ac_vec) /
									(ac_vec * ac_vec));

	// Compute the normal of the original ABC plane (and store for later)
	LLVector3 abc_norm;
	if (!mUseBAxis)
	{
		if (are_parallel(ab_vec, bc_vec, 0.001f))
		{
			// The current solution is maxed out, so we use the axis that is
			// orthogonal to both pole_vec and A->B
			if (are_parallel(pole_vec, ab_vec, 0.001f))
			{
				// ACK !  The problem is singular
				if (are_parallel(pole_vec, ag_vec, 0.001f))
				{
					// The solution is also singular
					return;
				}
				else
				{
					abc_norm = pole_vec % ag_vec;
				}
			}
			else
			{
				abc_norm = pole_vec % ab_vec;
			}
		}
		else
		{
			abc_norm = ab_vec % bc_vec;
		}
	}
	else
	{
		abc_norm = mBAxis * mJointB->getWorldRotation();
	}

	// Compute rotation of B.

	// Angle between A->B and B->C
	F32 abbc_ang = angle_between(ab_vec, bc_vec);

	// Vector orthogonal to A->B and B->C
	LLVector3 abbc_ortho_vec = ab_vec % bc_vec;
	if (abbc_ortho_vec.lengthSquared() < 0.001f)
	{
		abbc_ortho_vec = pole_vec % ab_vec;
		abac_comp_ortho_vec = pole_vec;
	}
	abbc_ortho_vec.normalize();

	F32 ag_len_qq = ag_len * ag_len;

	// Angle arm for extension
	F32 cos_theta = (ag_len_qq - ab_len * ab_len - bc_len * bc_len) /
					(2.f * ab_len * bc_len);
	if (cos_theta > 1.f)
	{
		cos_theta = 1.f;
	}
	else if (cos_theta < -1.f)
	{
		cos_theta = -1.f;
	}

	F32 theta = acosf(cos_theta);

	LLQuaternion b_rot(theta - abbc_ang, abbc_ortho_vec);

	// Compute rotation that rotates new A->C to A->G
	bc_vec = bc_vec * b_rot;	// rotate B->C by b_rot

	// Update A->C
	ac_vec = ab_vec + bc_vec;

	LLQuaternion cg_rot;
	cg_rot.shortestArc(ac_vec, ag_vec);

	// Update A->B and B->C with rotation from C to G
	ab_vec = ab_vec * cg_rot;
	bc_vec = bc_vec * cg_rot;
	abc_norm = abc_norm * cg_rot;
	ac_vec = ab_vec + bc_vec;

	// Compute the normal of the APG plane
	if (are_parallel(ag_vec, pole_vec, 0.001f))
	{
		// The solution plane is undefined ==> we are done
		return;
	}
	LLVector3 apg_norm = pole_vec % ag_vec;
	apg_norm.normalize();

	// Compute the normal of the new ABC plane (only necessary if we are NOT
	// using mBAxis
	if (!mUseBAxis)
	{
		if (!are_parallel(ab_vec, bc_vec, 0.001f))
		{
			abc_norm = ab_vec % bc_vec;
		}
#if 0
		else
		{
			// G is either too close or too far away we will use the old
			// ABCnormal
		}
#endif
		abc_norm.normalize();
	}

	// Calcuate plane rotation
	LLQuaternion p_rot;
	if (are_parallel(abc_norm, apg_norm, 0.001f))
	{
		if (abc_norm * apg_norm < 0.f)
		{
			// We must be PI radians off ==> rotate by PI around ag_vec
			p_rot.setAngleAxis(F_PI, ag_vec);
		}
#if 0
		else
		{
			// We are done
		}
#endif
	}
	else
	{
		p_rot.shortestArc(abc_norm, apg_norm);
	}

	// Compute twist rotation
	LLQuaternion twist_rot(mTwist, ag_vec);

	// Compute rotation of A
	LLQuaternion a_rot = cg_rot * p_rot * twist_rot;

	// Apply the rotations
	mJointB->setWorldRotation(mJointB->getWorldRotation() * b_rot);
	mJointA->setWorldRotation(mJointA->getWorldRotation() * a_rot);
}
