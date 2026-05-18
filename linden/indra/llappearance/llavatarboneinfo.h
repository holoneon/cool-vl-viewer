/**
 * @File llavatarboneinfo.h
 * @brief Trans/Scale/Rot etc. info about each avatar bone. Used both in
 * llavatarappearance.cpp and in llgltfjointdata.cpp.
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

#include "llvector3.h"
#include "llxmltree.h"

class LLAvatarBoneInfo final
{
public:
	LL_INLINE LLAvatarBoneInfo()
	:	mIsJoint(false)
	{
	}

	~LLAvatarBoneInfo();

	bool parseXml(LLXmlTreeNode* nodep);

public:
	typedef std::vector<LLAvatarBoneInfo*> vec_t;
	vec_t			mChildren;
	std::string		mName;
	std::string		mSupport;
	std::string		mAliases;
	std::string		mGroup;
	LLVector3		mPos;
	LLVector3		mEnd;
	LLVector3		mRot;
	LLVector3		mScale;
	LLVector3		mPivot;
	bool			mIsJoint;
};
