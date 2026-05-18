/**
 * @file listener.cpp
 * @brief Implementation of LISTENER class abstracting the audio support
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

#include "linden_common.h"

#include "lllistener.h"

LLListener::LLListener()
:	mListenAt(0.f, 0.f, -1.f),
	mListenUp(0.f, 1.f, 0.f)
{
	setDopplerFactor(1.f);
	setRolloffFactor(1.f);
}

//virtual
void LLListener::set(const LLVector3& pos, const LLVector3& vel,
					 const LLVector3& up, const LLVector3& at)
{
	mPosition = pos;
	mVelocity = vel;

	setPosition(pos);
	setVelocity(vel);
	orient(up, at);
}
