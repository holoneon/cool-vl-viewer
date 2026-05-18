/**
 * @file llphysshapebuilderutil.h
 * @brief Generic system to convert LL(Physics)VolumeParams to physics shapes
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "indra_constants.h"
#include "llpreprocessor.h"
#include "llvolume.h"

#define USE_SHAPE_QUANTIZATION 0

#define SHAPE_BUILDER_DEFAULT_VOLUME_DETAIL 1

#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_HOLLOW 0.10f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_HOLLOW_SPHERES 0.90f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT 0.05f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_TAPER 0.05f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_TWIST 0.09f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_SHEAR 0.05f

constexpr F32 COLLISION_TOLERANCE = 0.1f;

constexpr F32 SHAPE_BUILDER_ENTRY_SNAP_SCALE_BIN_SIZE = 0.15f;
constexpr F32 SHAPE_BUILDER_ENTRY_SNAP_PARAMETER_BIN_SIZE = 0.01f;
constexpr F32 SHAPE_BUILDER_MIN_GEOMETRY_SIZE = 0.5f * COLLISION_TOLERANCE;
constexpr F32 SHAPE_BUILDER_CONVEXIFICATION_SIZE = 2.f * COLLISION_TOLERANCE;
constexpr F32 SHAPE_BUILDER_CONVEXIFICATION_SIZE_MESH = 0.5f;

class LLPhysicsVolumeParams : public LLVolumeParams
{
public:

	LL_INLINE LLPhysicsVolumeParams(const LLVolumeParams& params,
									bool force_convex)
	:	LLVolumeParams(params),
		mForceConvex(force_convex)
	{
	}

	LL_INLINE bool operator==(const LLPhysicsVolumeParams& params) const
	{
		return (LLVolumeParams::operator==(params) &&
				mForceConvex == params.mForceConvex);
	}

	LL_INLINE bool operator!=(const LLPhysicsVolumeParams& params) const
	{
		return !operator==(params);
	}

	LL_INLINE bool operator<(const LLPhysicsVolumeParams& params) const
	{
		if (LLVolumeParams::operator!=(params))
		{
			return LLVolumeParams::operator<(params);
		}
		return !params.mForceConvex && mForceConvex;
	}

	LL_INLINE bool shouldForceConvex() const	{ return mForceConvex; }

private:
	bool mForceConvex;
};

// Purely static class
class LLPhysShapeBuilderUtil
{
public:
	LLPhysShapeBuilderUtil() = delete;
	~LLPhysShapeBuilderUtil() = delete;

	class ShapeSpec
	{
		friend class LLPhysShapeBuilderUtil;

	public:
		enum ShapeType
		{
			// Primitive types
			BOX,
			SPHERE,
			CYLINDER,

			// User specified they wanted the convex hull of the volume
			USER_CONVEX,

			// Either a volume that is inherently convex but not a primitive
			// type, or a shape with dimensions such that will convexify it
			// anyway.
			PRIM_CONVEX,

			// Special case for traditional sculpts--they are the convex hull
			// of a single particular set of volume params
 			SCULPT,

			// A user mesh. May or may not contain a convex decomposition.
			USER_MESH,

			// A non-convex volume which we have to represent accurately
			PRIM_MESH,

			INVALID
		};

		LL_INLINE ShapeSpec()
		:	mType(INVALID),
			mScale(0.f, 0.f, 0.f),
			mCenter(0.f, 0.f, 0.f)
		{
		}

		LL_INLINE bool isConvex()
		{
			return mType != USER_MESH && mType != PRIM_MESH &&
				   mType != INVALID;
		}

		LL_INLINE bool isMesh()
		{
			return mType == USER_MESH || mType == PRIM_MESH;
		}

		LL_INLINE ShapeType getType()			{ return mType; }
		LL_INLINE const LLVector3& getScale()	{ return mScale; }
		LL_INLINE const LLVector3& getCenter()	{ return mCenter; }

	private:
		ShapeType mType;

		// Dimensions of an AABB around the shape
		LLVector3 mScale;

		// Offset of shape from origin of primitive's reference frame
		LLVector3 mCenter;
	};

	static void getPhysShape(const LLPhysicsVolumeParams& vol_params,
							 const LLVector3& scale, bool has_decomp,
							 ShapeSpec& spec_out);
};
