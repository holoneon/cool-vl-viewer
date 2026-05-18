/**
 * @file llviewerjointmesh.h
 * @brief Declaration of LLViewerJointMesh class
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

#include "llavatarjointmesh.h"
#include "llpolymesh.h"
#include "llcolor4.h"

#include "llviewerjoint.h"
#include "llviewertexture.h"

#if LL_MSVC
# pragma warning(disable : 4250)	// LLViewerJoint::asViewerJoint inheritance
#endif

class LLCharacter;
class LLDrawable;
class LLFace;
class LLViewerTexLayerSet;

//-----------------------------------------------------------------------------
// class LLViewerJointMesh
//-----------------------------------------------------------------------------

class LLViewerJointMesh final : public LLAvatarJointMesh, public LLViewerJoint
{
protected:
	LOG_CLASS(LLViewerJointMesh);

public:
	LLViewerJointMesh();

	// This lifts any ambiguity since LLAvatarJointMesh is not an LLViewerJoint
	LL_INLINE LLViewerJoint* asViewerJoint() override	{ return this; }
	// This lifts any ambiguity since LLViewerJoint is not an LLAvatarJointMesh
	LL_INLINE LLAvatarJoint* asAvatarJoint() override	{ return this; }

	// Render time method to upload batches of joint matrices
	void uploadJointMatrices();

	// Overloaded from base class
	U32 drawShape(bool first_pass = true, bool is_dummy = false) override;

	// Necessary because MS's compiler warns on function inheritance via
	// dominance in the diamond inheritance here. Warns even though
	// LLViewerJoint holds the only non virtual implementation.
	U32 render(F32 pixelArea, bool first_pass = true,
			   bool is_dummy = false) override
	{
		return LLViewerJoint::render(pixelArea, first_pass, is_dummy);
	}

	void updateFaceSizes(U32& num_vertices, U32& num_indices,
						 F32 pixel_area) override;
	void updateFaceData(LLFace* face, F32 pixel_area, bool damp_wind = false,
						bool terse_update = false) override;
	bool updateLOD(F32 pixel_area, bool activate) override;
	void updateJointGeometry() override;
	void dump() override;

	LL_INLINE bool isAnimatable() const override	{ return false; }

private:
	// Copy mesh into given face's vertex buffer, applying current animation
	// pose
	static void updateGeometry(LLFace* face, LLPolyMesh* mesh);
};
