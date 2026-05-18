/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
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

#include "llaudioengine.h"

#include "lllistener_openal.h"

// After #include "lllistener_openal.h" so that "AL/al.h" is included
// already. HB
#include "AL/alut.h"
#include "AL/alext.h"

LLListener_OpenAL::LLListener_OpenAL()
:	LLListener()
{
}

//virtual
void LLListener_OpenAL::setDopplerFactor(F32 factor)
{
	mDopplerFactor = factor;
	alDopplerFactor(factor);
}

//virtual
F32 LLListener_OpenAL::getDopplerFactor()
{
	mDopplerFactor = (F32)alGetFloat(AL_DOPPLER_FACTOR);
	return mDopplerFactor;
}

//virtual
void LLListener_OpenAL::commitDeferredChanges()
{
	ALfloat orientation[6];
	orientation[0] = mListenAt.mV[0];
	orientation[1] = mListenAt.mV[1];
	orientation[2] = mListenAt.mV[2];
	orientation[3] = mListenUp.mV[0];
	orientation[4] = mListenUp.mV[1];
	orientation[5] = mListenUp.mV[2];

	ALfloat velocity[3];
	velocity[0] = mVelocity.mV[0];
	velocity[1] = mVelocity.mV[1];
	velocity[2] = mVelocity.mV[2];

	alListenerfv(AL_ORIENTATION, orientation);
	alListenerfv(AL_POSITION, mPosition.mV);
	alListenerfv(AL_VELOCITY, velocity);
}
