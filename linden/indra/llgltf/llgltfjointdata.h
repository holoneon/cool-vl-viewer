/**
 * @file llgltfjointdata.h
 * @brief Class for holding individual joint data and skeleton
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 *
 * Copyright (c) 2025, Linden Research, Inc.
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

#include "llgltfglm.h"
#include "llvector3.h"

class LLAvatarBoneInfo;

namespace LLGLTF
{
	class JointData
	{
	public:
		JointData() = default;

		enum SupportCategory
		{
			SUPPORT_BASE,
			SUPPORT_EXTENDED
		};

		LL_INLINE void setSupport(const std::string& support)
		{
			mSupport = support == "extended" ? SUPPORT_EXTENDED : SUPPORT_BASE;
		}

		static void getJointMatricesAndHierarhy(std::vector<JointData>& data);

	private:
		static void getJointMatricesAndHierarhy(LLAvatarBoneInfo* infop,
												JointData& data,
												const mat4& parent_mat);

	public:
		typedef std::vector<JointData> bones_t;
		bones_t			mChildren;
		std::string		mName;
		std::string		mGroup;
		mat4			mJointMatrix;
		mat4			mRestMatrix;
		vec3			mScale;
		LLVector3		mRotation;
		SupportCategory	mSupport;
		// true for a joint, false for a collision volume
		bool			mIsJoint;
	};
}
