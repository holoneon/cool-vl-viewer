/**
 * @file llvowlsky.h
 * @brief LLVOWLSky class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llsettingssky.h"		// For LL_VARIABLE_SKY_DOME_SIZE

#include "llviewerobject.h"

class LLVOWLSky final : public LLStaticViewerObject
{
protected:
	LOG_CLASS(LLVOWLSky);

public:
	LLVOWLSky(const LLUUID& id, LLViewerRegion* regionp);

	// Nothing to do.
	LL_INLINE void idleUpdate(F64) override				{}

	LL_INLINE bool isActive() const override			{ return false; }

	LLDrawable* createDrawable() override;
	bool updateGeometry(LLDrawable* drawable) override;

	void drawStars();
	void drawDome();

	void resetVertexBuffers() override;

	void cleanupGL();
	void restoreGL();

	static void initClass();
	static void updateSettings();
	static void cleanupClass();

private:
	LL_INLINE static U32 getNumStacks()
	{
		return sWLSkyDetail;
	}

	LL_INLINE static U32 getNumSlices()
	{
		return 2 * sWLSkyDetail;
	}

	LL_INLINE static U32 getFanNumVerts()
	{
		return getNumSlices() + 1;
	}

	LL_INLINE static U32 getFanNumIndices()
	{
		return getNumSlices() * 3;
	}

	// Gets the dome radius, based on whether we render Windlight or extended
	// environment settings.
#if LL_VARIABLE_SKY_DOME_SIZE
	static F32 getDomeRadius();
#else
	// In fact, Windlight always had it fixed to 15000m, and it is also the
	// value for the current extended environment code... So, why bothering ?
	LL_INLINE static F32 getDomeRadius()				{ return 15000.f; }
#endif

	// A tiny helper method for controlling the sky dome tesselation.
	static F32 calcPhi(U32 i);

	// Helper method for initializing the stars.
	void initStars();

	// Helper method for building the strips vertex buffer. Note: begin_stack
	// and end_stack follow stl iterator conventions, begin_stack is the first
	// stack to be included, end_stack is the first stack not to be included.
	static void buildStripsBuffer(U32 begin_stack, U32 end_stack,
								  LLStrider<LLVector3>& vertices,
								  LLStrider<LLVector2>& texCoords,
								  LLStrider<U16>& indices);

	// Helper method for updating the stars colors.
	void updateStarColors();

	// Helper method for updating the stars geometry.
	bool updateStarGeometry(LLDrawable* drawable);

private:
	LLPointer<LLVertexBuffer>	mStarsVerts;

	typedef std::vector<LLPointer<LLVertexBuffer> > strips_verts_vec_t;
	strips_verts_vec_t			mStripsVerts;

	std::vector<LLVector3>		mStarVertices;
	std::vector<LLColor4>		mStarColors;
	std::vector<F32>			mStarIntensities;

	U32							mLastWLSkyDetail;

	static U32					sWLSkyDetail;
};
