/** 
 * @file llviewertextureanim.h
 * @brief LLViewerTextureAnim class header file
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "lltextureanim.h"
#include "llframetimer.h"

class LLVOVolume;

class LLViewerTextureAnim : public LLTextureAnim
{
protected:
	LOG_CLASS(LLViewerTextureAnim);

private:
	static std::vector<LLViewerTextureAnim*> sInstanceList;
	S32 mInstanceIndex;

public:
	static void initClass();
	static void updateClass();
	static void dumpStats();

	LLViewerTextureAnim(LLVOVolume* vobj);
	virtual ~LLViewerTextureAnim();

	void reset() override;

	S32 animateTextures(F32& off_s, F32& off_t, F32& scale_s, F32& scale_t,
						F32& rotate);
	enum
	{
		TRANSLATE = 0x01 // Result code JUST for animateTextures
	};

public:
	F32				mOffS;
	F32				mOffT;
	F32				mScaleS;
	F32				mScaleT;
	F32				mRot;

protected:
	LLVOVolume*		mVObj;
	LLFrameTimer	mTimer;
	F64				mLastTime;
	F32				mLastFrame;
};
