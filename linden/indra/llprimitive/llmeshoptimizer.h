/**
 * @file llmeshoptimizer.h
 * @brief Wrapper around the meshoptimizer library
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 *
 * Copyright (c) 2021, Linden Research, Inc.
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

#include "stdtypes.h"

class LLVector2;
class LLVector4a;

// Purely static class
class LLMeshOptimizer
{
public:
	LLMeshOptimizer() = delete;
	~LLMeshOptimizer() = delete;

	static void generateShadowIndexBuffer16(U16* dest, const U16* indices,
											U64 idx_count,
											const LLVector4a* vert_pos,
											const LLVector4a* normals,
											const LLVector2* tex_coords,
											U64 vert_count);

	static void generateShadowIndexBuffer32(U32* dest, const U32* indices,
											U64 idx_count,
											const LLVector4a* vert_pos,
											const LLVector4a* normals,
											const LLVector2* tex_coords,
											U64 vert_count);

	// Remap methods welding indentical vertexes together and removing unused
	// vertices if indices were provided.

	static size_t generateRemapMulti16(U32* remap,
									   const U16* indices, U64 index_count,
									   const LLVector4a* vert_pos,
									   const LLVector4a* normals,
									   const LLVector2* tex_coords,
									   U64 vert_count);

	static size_t generateRemapMulti32(U32* remap,
									   const U32* indices, U64 index_count,
									   const LLVector4a* vert_pos,
									   const LLVector4a* normals,
									   const LLVector2* tex_coords,
									   U64 vert_count);

	static void remapIndexBuffer16(U16* dest, const U16* indices,
								   U64 index_count, const U32* remap);

	static void remapIndexBuffer32(U32* dest, const U32* indices,
								   U64 index_count, const U32* remap);

	// Works for both positions and normals vertex buffers.
	static void remapVertsBuffer(LLVector4a* dest, const LLVector4a* verts,
								 U64 count, const U32* remap);

	static void remapTexCoordsBuffer(LLVector2* dest, const LLVector2* tc,
									 U64 tc_count, const U32* remap);

	// Simplification methods returning the amount of indices in 'dest';
	// 'sloppy' engages a variant of an algorithm that does not respect
	// topology as much but is much more effective for simpler models. When
	// not NULL, 'result_error' returns how far from original the model is in
	// percents.

	static size_t simplify16(U16* dest, const U16* indices, U64 idx_count,
							 const LLVector4a* vert_pos, U64 vert_count,
							 U64 vert_pos_stride, U64 target_idx_count,
							 F32 target_error, bool sloppy, F32* result_error);

	static size_t simplify32(U32* dest, const U32* indices, U64 idx_count,
							 const LLVector4a* vert_pos, U64 vert_count,
							 U64 vert_pos_stride, U64 target_idx_count,
							 F32 target_error, bool sloppy, F32* result_error);
};
