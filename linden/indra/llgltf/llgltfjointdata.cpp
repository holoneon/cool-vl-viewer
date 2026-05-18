/**
 * @file llgltfjointdata.cpp
 * @brief Class for holding individual joint data and skeleton
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 *
 * Copyright (C) 2025, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llgltfjointdata.h"

#include "llavatarappearance.h"
#include "llavatarboneinfo.h"

using namespace LLGLTF;

static mat4 get_joint_matrix(LLAvatarBoneInfo* infop)
{
	mat4 mat(1.f);
	// 1. Scaling
	mat = glm::scale(mat, vec3(infop->mScale[0], infop->mScale[1],
					 infop->mScale[2]));
	// 2. Rotation (avatar_skeleton.xml stores Euler angles in degrees)
	mat = glm::rotate(mat, glm::radians(infop->mRot[0]),
					  vec3(1.f, 0.f, 0.f));
	mat = glm::rotate(mat, glm::radians(infop->mRot[1]),
					  vec3(0.f, 1.f, 0.f));
	mat = glm::rotate(mat, glm::radians(infop->mRot[2]),
					  vec3(0.f, 0.f, 1.f));
	// 3. Position
	mat = glm::translate(mat, vec3(infop->mPos[0], infop->mPos[1],
						 infop->mPos[2]));
	return mat;
}

//static
void JointData::getJointMatricesAndHierarhy(LLAvatarBoneInfo* infop,
											JointData& data,
											const mat4& parent_mat)
{
	data.mName = infop->mName;
	data.mJointMatrix = get_joint_matrix(infop);
	data.mScale = vec3(infop->mScale[0], infop->mScale[1], infop->mScale[2]);
	data.mRotation = infop->mRot;
	data.mRestMatrix = parent_mat * data.mJointMatrix;
	data.mIsJoint = infop->mIsJoint;
	data.mGroup = infop->mGroup;
	data.setSupport(infop->mSupport);
	for (size_t i = 0, count = infop->mChildren.size(); i < count; ++i)
	{
		JointData& child_data = data.mChildren.emplace_back();
		getJointMatricesAndHierarhy(infop->mChildren[i], child_data,
									data.mRestMatrix);
	}
}

//static
void JointData::getJointMatricesAndHierarhy(std::vector<JointData>& data)
{
	mat4 identity(1.f);
	const LLAvatarBoneInfo::vec_t& bones =
		LLAvatarAppearance::getBoneInfoList();
	for (size_t i = 0, count = bones.size(); i < count; ++i)
	{
		JointData& child_data = data.emplace_back();
		getJointMatricesAndHierarhy(bones[i], child_data, identity);
	}
}
