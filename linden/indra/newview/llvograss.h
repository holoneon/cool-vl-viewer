/**
 * @file llvograss.h
 * @brief Description of LLVOGrass class
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

#include <map>

#include "llviewerobject.h"

class LLSelectNode;
class LLSurfacePatch;
class LLViewerTexture;

class LLVOGrass final : public LLAlphaObject
{
protected:
	LOG_CLASS(LLVOGrass);

	~LLVOGrass() override = default;

public:
	LLVOGrass(const LLUUID& id, LLViewerRegion* regionp);

	// Initialize data that is only initialized once per class.
	static void initClass();
	static void cleanupClass();

	U32 getPartitionType() const override;

	U32 processUpdateMessage(LLMessageSystem* mesgsys, void** user_data,
							 U32 block_num, EObjectUpdateType upd_type,
							 LLDataPacker* dp) override;
	static void import(LLFILE* file, LLMessageSystem* mesgsys,
					   const LLVector3& pos);
	void exportFile(LLFILE* file, const LLVector3& position);

	void updateDrawable(bool force_damped) override;

	LLDrawable* createDrawable() override;
	bool updateGeometry(LLDrawable* drawable) override;
	void getGeometry(S32 idx, LLStrider<LLVector4a>& verticesp,
					 LLStrider<LLVector3>& normalsp,
					 LLStrider<LLVector2>& texcoordsp,
					 LLStrider<LLColor4U>& colorsp,
					 LLStrider<LLColor4U>& emissivep,
					 LLStrider<U16>& indicesp) override;

	LL_INLINE void updateFaceSize(S32 idx) override		{}
	void updateTextures() override;
	bool updateLOD() override;

	// Generate accurate apparent angle and area
	void setPixelAreaAndAngle() override;

	void plantBlades();

	// Whether this object needs to do an idleUpdate:
	LL_INLINE bool isActive() const override			{ return true; }

	void idleUpdate(F64 time) override;

	bool lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
							  // Which face to check, -1 = ALL_SIDES
							  S32 face = -1,
							  bool pick_transparent = false,
							  bool pick_rigged = false,
							  // Which face was hit
							  S32* face_hit = NULL,
							  // Intersection point
							  LLVector4a* intersection = NULL,
							  // Texture coordinates of the intersection point
							  LLVector2* tex_coord = NULL,
							  // Surface normal at the intersection point
							  LLVector4a* normal = NULL,
							  // Surface tangent at the intersection point
							  LLVector4a* tangent = NULL) override;

	void generateSilhouette(LLSelectNode* nodep);

private:
	void generateSilhouetteVertices(std::vector<LLVector3> &vertices,
									std::vector<LLVector3> &normals,
									const LLVector3& view_vec,
									const LLMatrix4& mat,
									const LLMatrix3& norm_mat);
	void updateSpecies();

public:
	struct GrassSpeciesData
	{
		LLUUID				mTextureID;
		F32					mBladeSizeX;
		F32					mBladeSizeY;
	};

	U64						mLastPatchUpdateTime;

	F32						mBladeSizeX;
	F32						mBladeSizeY;
	F32						mBWAOverlap;

	LLSurfacePatch*			mPatch;		//  Stores the land patch where the grass is centered

	U8						mSpecies;	// Species of grass

	static U8				sMaxGrassSpecies;

	typedef std::map<std::string, U8, std::less<> > species_list_t;
	static species_list_t	sSpeciesNames;

private:
	F32						mLastHeight;		// For cheap update hack
	S32						mNumBlades;

	typedef std::map<U8, GrassSpeciesData*> data_map_t;
	static data_map_t		sSpeciesTable;
};
