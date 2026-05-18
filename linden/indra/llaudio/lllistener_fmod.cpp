/**
 * @file lllistener_fmod.cpp
 * @brief implementation of LISTENER class abstracting the audio support
 * as a FMOD implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "fmod.hpp"

#include "lllistener_fmod.h"

#include "llaudioengine.h"

LLListener_FMOD::LLListener_FMOD(FMOD::System* system)
:	LLListener(),
	mSystem(system)
{
}

//virtual
void LLListener_FMOD::translate(const LLVector3& offset)
{
	if (mSystem)
	{
		LLListener::translate(offset);
		mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV, NULL,
										 (FMOD_VECTOR*)mListenAt.mV,
										 (FMOD_VECTOR*)mListenUp.mV);
	}
}

//virtual
void LLListener_FMOD::setPosition(const LLVector3& pos)
{
	if (mSystem)
	{
		LLListener::setPosition(pos);
		mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV, NULL,
										 (FMOD_VECTOR*)mListenAt.mV,
										 (FMOD_VECTOR*)mListenUp.mV);
	}
}

//virtual
void LLListener_FMOD::setVelocity(const LLVector3& vel)
{
	if (mSystem)
	{
		LLListener::setVelocity(vel);
		mSystem->set3DListenerAttributes(0, NULL, (FMOD_VECTOR*)mVelocity.mV,
										 (FMOD_VECTOR*)mListenAt.mV,
										 (FMOD_VECTOR*)mListenUp.mV);
	}
}

//virtual
void LLListener_FMOD::orient(const LLVector3& up, const LLVector3& at)
{
	if (mSystem)
	{
		LLListener::orient(up, at);
		mSystem->set3DListenerAttributes(0, NULL, NULL, (FMOD_VECTOR*)at.mV,
										 (FMOD_VECTOR*)up.mV);
	}
}

//virtual
void LLListener_FMOD::setRolloffFactor(F32 factor)
{
	// An internal FMOD Studio optimization skips 3D updates if there have not
	// been changes to the 3D sound environment. Sadly, a change in rolloff is
	// not accounted for, thus we must touch the listener properties as well.
	// In short: Changing the position ticks a dirtyflag inside FMOD Studio,
	// which makes it not skip 3D processing next update call.
	if (mSystem)
	{
		if (mRolloffFactor != factor)
		{
			LLVector3 pos = mPosition;
			pos.mV[VZ] -= 0.1f;
			mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)pos.mV, NULL,
											 NULL, NULL);
			mSystem->set3DListenerAttributes(0, (FMOD_VECTOR*)mPosition.mV,
											 NULL, NULL, NULL);
		}
		mRolloffFactor = factor;
		mSystem->set3DSettings(mDopplerFactor, 1.f, mRolloffFactor);
	}
}

//virtual
void LLListener_FMOD::setDopplerFactor(F32 factor)
{
	if (mSystem)
	{
		mDopplerFactor = factor;
		mSystem->set3DSettings(mDopplerFactor, 1.f, mRolloffFactor);
	}
}

//virtual
void LLListener_FMOD::commitDeferredChanges()
{
	if (mSystem)
	{
		mSystem->update();
	}
}
