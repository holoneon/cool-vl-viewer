/**
 * @file llgltfprimitive.h
 * @brief LL GLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024, Linden Research, Inc.
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

#include "hbfastmap.h"
#include "llgltfaccessor.h"
#include "llmemory.h"			// For LL_ALIGNED16_NEW_DELETE
#include "llvertexbuffer.h"
#include "llvolumeoctree.h"

namespace LLGLTF
{
	class Asset;

	// Note 16-byte aligned because we use vectors of LLVector4a. HB
	class alignas(16) Primitive
	{
	protected:
		LOG_CLASS(LLGLTF::Primitive);

	public:
		LL_ALIGNED16_NEW_DELETE

		enum class Mode : U8
		{
			POINTS,
			LINES,
			LINE_LOOP,
			LINE_STRIP,
			TRIANGLES,
			TRIANGLE_STRIP,
			TRIANGLE_FAN
		};

		Primitive();
		~Primitive();

		// Creates an octree based on vertex buffer; must be called before
		// buffer is unmapped and after buffer is populated with valid data.
		void createOctree();

		// Gets the LLVolumeTriangle that intersects with the given line
		// segment at the point closest to start. Moves end to the point of
		// intersection. Returns NULL if no intersection.
		// Line segment must be in the same coordinate frame as this Primitive.
		const LLVolumeTriangle* lineSegmentIntersect(const LLVector4a& start,
													 const LLVector4a& end,
													 LLVector4a* interp = NULL,
													 LLVector2* tcoordp = NULL,
													 LLVector4a* normp = NULL,
													 LLVector4a* tgtp = NULL);

		void serialize(lljson& dst) const;
		const Primitive& operator=(const lljson& src);

		bool prep(Asset& asset);

		// Uploads geometry to given vertex buffer.
		void upload(LLVertexBuffer* bufferp);

		LL_INLINE U32 getVertexCount() const	{ return mPositions.size(); }
		LL_INLINE U32 getIndexCount() const		{ return mIndexArray.size(); }

	public:
		// Aligned members first...
		// CPU copy of mesh data
		alignas(16) std::vector<LLVector4a>	mNormals;
		alignas(16) std::vector<LLVector4a>	mTangents;
		alignas(16) std::vector<LLVector4a>	mPositions;
		alignas(16) std::vector<LLVector4a>	mWeights;
		// ... then the rest.
		std::vector<U64>					mJoints;
		std::vector<LLVector2>				mTexCoords0;
		std::vector<LLVector2>				mTexCoords1;
		std::vector<LLColor4U>				mColors;
		std::vector<U32>					mIndexArray;

		// Raycast acceleration structure
		std::vector<LLVolumeTriangle>		mOctreeTriangles;
		LLPointer<LLVolumeOctree>			mOctree;

		fast_hmap<std::string, S32>			mAttributes;

		// GPU copy of mesh data
		LLPointer<LLVertexBuffer>			mVertexBuffer;
		U32									mVertexOffset;
		U32									mIndexOffset;
		// Vertex attribute mask
		U32									mAttributeMask;

		S32									mIndices;
		S32									mMaterial;
		U32									mGLMode;
		Mode								mMode;
		// Shader variant according to LLGLSLShader::GLTFVariant flags
		U8									mShaderVariant;
	};
}
