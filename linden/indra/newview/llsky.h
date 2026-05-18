/**
 * @file llsky.h
 * @brief It's, uh, the sky!
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

#include "llmath.h"
#include "llpointer.h"
#include "llvector3.h"
#include "llvector4.h"
#include "llcolor4.h"
#include "llcolor4u.h"
#include "llvosky.h"

class LLVOWLSky;

class LLSky
{
public:
	LLSky();

	void init();

	void cleanup();

	void destroyGL();
	void restoreGL();
	void resetVertexBuffers();

	// *TODO: do culling for WL sky properly -Brad
	LL_INLINE void updateCull()						{}

	void updateSky();

	void addSunMoonBeacons();

	void setCloudDensityAtAgent(F32 cloud_density);
	void setWind(const LLVector3& wind);

	void updateFog(F32 distance);

	// Windlight specific methods

	void setSunDirection(const LLVector3& sun_direction,
						 const LLVector3& sun_ang_velocity);

	void setOverrideSun(bool override_sun);
	LL_INLINE bool getOverrideSun()					{ return mOverrideSimSunPosition; }

	LL_INLINE void setSunTargetDirection(const LLVector3& sun_direction,
										 const LLVector3& sun_ang_velocity)
	{
		mSunTargDir = sun_direction;
	}

	void propagateHeavenlyBodies(F32 dt);	// dt = seconds

	LLVector3 getSunDirection() const;
	LLVector3 getMoonDirection() const;
	bool sunUp() const;

	// Extended environment specific methods

	void setSunScale(F32 sun_scale);
	void setMoonScale(F32 moon_scale);

	// These directions should be in CFR coord sys (+x at, +z up, +y right)
	void setSunAndMoonDirectionsCFR(const LLVector3& sun_direction,
									const LLVector3& moon_direction);
	void setSunDirectionCFR(const LLVector3& sun_direction);
	void setMoonDirectionCFR(const LLVector3& moon_direction);

	void setSunTextures(const LLUUID& sun_tex1,
						const LLUUID& sun_tex2 = LLUUID::null);
	void setMoonTextures(const LLUUID& moon_tex1,
						 const LLUUID& moon_tex2 = LLUUID::null);
	void setCloudNoiseTextures(const LLUUID& cld_tex1,
							   const LLUUID& cld_tex2 = LLUUID::null);
	void setBloomTextures(const LLUUID& bloom_tex1,
						  const LLUUID& bloom_tex2 = LLUUID::null);

public:
	// Pointer to the LLVOSky object (only one, ever !)
	LLPointer<LLVOSky>		mVOSkyp;
	LLPointer<LLVOWLSky>	mVOWLSkyp;

	LLVector3				mSunTargDir;

	S32						mLightingGeneration;

	bool					mUpdatedThisFrame;

protected:
	bool					mOverrideSimSunPosition;

	LLVector3				mLastSunDirection;
};

extern LLSky gSky;
