/**
 * @file llvosurfacepatch.h
 * @brief Description of LLVOSurfacePatch class
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

#include "llstrider.h"

#include "llface.h"
#include "llviewerobject.h"
#include "llviewerregion.h"

class LLDrawPool;
class LLFace;
class LLFacePool;
class LLSurfacePatch;
class LLVector2;

class LLVOSurfacePatch final : public LLStaticViewerObject
{
protected:
	LOG_CLASS(LLVOSurfacePatch);

public:
	enum
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD1)
	};

	LLVOSurfacePatch(const LLUUID& id, LLViewerRegion* regionp);

	void markDead() override;

	static void initClass();

	LL_INLINE U32 getPartitionType() const override		{ return LLViewerRegion::PARTITION_TERRAIN; }

	LLDrawable* createDrawable() override;

	void updateGL() override;
	bool updateGeometry(LLDrawable* drawable) override;
	LL_INLINE bool updateLOD() override					{ return true; }
	void updateFaceSize(S32 idx) override;

	void getTerrainGeometry(LLStrider<LLVector3>& verticesp,
							LLStrider<LLVector3>& normalsp,
							LLStrider<LLVector2>& texCoords0p,
							LLStrider<LLVector2>& texCoords1p,
							LLStrider<U16>& indicesp);

	LL_INLINE void updateTextures() override			{}

	// Generates accurate apparent angle and area:
	void setPixelAreaAndAngle() override;

	void updateSpatialExtents(LLVector4a& new_min,
							  LLVector4a& new_max) override;

	// Whether this object needs to do an idleUpdate:
	LL_INLINE bool isActive() const override			{ return false; }

	void setPatch(LLSurfacePatch* patchp);
	LL_INLINE LLSurfacePatch* getPatch() const			{ return mPatchp; }

	void dirtyPatch();
	void dirtyGeom();

	bool lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
							  // Which face to check, -1=ALL_SIDES
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

protected:
	~LLVOSurfacePatch() override;

	LLFacePool* getPool();

	void getGeomSizesMain(S32 stride, S32& num_vertices, S32& num_indices);
	void getGeomSizesNorth(S32 stride, S32 north_stride, S32& num_vertices,
						   S32& num_indices);
	void getGeomSizesEast(S32 stride, S32 east_stride, S32& num_vertices,
						  S32& num_indices);

	void updateMainGeometry(LLFace* facep,
							LLStrider<LLVector3>& verticesp,
							LLStrider<LLVector3>& normalsp,
							LLStrider<LLVector2>& texCoords0p,
							LLStrider<LLVector2>& texCoords1p,
							LLStrider<U16>& indicesp,
							U32& index_offset);
	void updateNorthGeometry(LLFace* facep,
							 LLStrider<LLVector3>& verticesp,
							 LLStrider<LLVector3>& normalsp,
							 LLStrider<LLVector2>& texCoords0p,
							 LLStrider<LLVector2>& texCoords1p,
							 LLStrider<U16>& indicesp,
							 U32& index_offset);
	void updateEastGeometry(LLFace* facep,
							LLStrider<LLVector3>& verticesp,
							LLStrider<LLVector3>& normalsp,
							LLStrider<LLVector2>& texCoords0p,
							LLStrider<LLVector2>& texCoords1p,
							LLStrider<U16>& indicesp,
							U32& index_offset);

protected:
	LLFacePool*		mPool;
	LLSurfacePatch* mPatchp;
	S32				mBaseComp;

	S32				mLastNorthStride;
	S32				mLastEastStride;
	S32				mLastStride;
	S32				mLastLength;

	bool			mDirtyTexture;
	bool			mDirtyTerrain;

public:
	bool			mDirtiedPatch;
	static F32		sLODFactor;
};
