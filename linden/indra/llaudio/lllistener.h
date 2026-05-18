/**
 * @file lllistener.h
 * @brief Description of LISTENER base class abstracting the audio support.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

class LLListener
{
public:
	LLListener();
	virtual ~LLListener() = default;

	virtual void set(const LLVector3& pos, const LLVector3& vel,
					 const LLVector3& up, const LLVector3& at);

	LL_INLINE virtual void setPosition(const LLVector3& pos)
	{
		mPosition = pos;
	}

	LL_INLINE virtual void setVelocity(const LLVector3& vel)
	{
		mVelocity = vel;
	}

	LL_INLINE virtual void orient(const LLVector3& up, const LLVector3& at)
	{
		mListenUp = up;
		mListenAt = at;
	}

	LL_INLINE virtual void translate(const LLVector3& offset)
	{
		mPosition += offset;
	}

	LL_INLINE virtual void setDopplerFactor(F32 factor)
	{
		mDopplerFactor = factor;
	}

	LL_INLINE virtual void setRolloffFactor(F32 factor)
	{
		mRolloffFactor = factor;
	}

	LL_INLINE virtual F32 getDopplerFactor()		{ return mDopplerFactor; }
	LL_INLINE virtual F32 getRolloffFactor()		{ return mRolloffFactor; }

	// No need for virtual methods here.
	LL_INLINE LLVector3 getPosition()				{ return mPosition; }
	LL_INLINE LLVector3 getAt()						{ return mListenAt; }
	LL_INLINE LLVector3 getUp()						{ return mListenUp; }

	LL_INLINE virtual void commitDeferredChanges()	{}

protected:
	LLVector3	mPosition;
	LLVector3	mVelocity;
	LLVector3	mListenAt;
	LLVector3	mListenUp;
	F32			mDopplerFactor;
	F32			mRolloffFactor;
};
