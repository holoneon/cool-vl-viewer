/**
 * @file lllistener_fmod.h
 * @brief Description of LISTENER class abstracting the audio support as an
 * FMOD implementation
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

#pragma once

#include "lllistener.h"

// Stubs
namespace FMOD
{
	class System;
}

class LLListener_FMOD final : public LLListener
{
public:
	LLListener_FMOD(FMOD::System* system);

	void translate(const LLVector3& offset) override;
	void setPosition(const LLVector3& pos) override;
	void setVelocity(const LLVector3& vel) override;
	void orient(const LLVector3& up, const LLVector3& at) override;
	void setDopplerFactor(F32 factor) override;
	void setRolloffFactor(F32 factor) override;

	void commitDeferredChanges() override;

protected:
	 FMOD::System*	mSystem;
};
