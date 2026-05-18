/**
 * @file llmeshoptimizer.cpp
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

#include "linden_common.h"

#include "meshoptimizer.h"

#include "llmeshoptimizer.h"

#include "llvolume.h"

//static
void LLMeshOptimizer::generateShadowIndexBuffer16(U16* dest,
												  const U16* indices,
												  U64 idx_count,
												  const LLVector4a* vert_pos,
												  const LLVector4a* normals,
												  const LLVector2* tex_coords,
												  U64 vert_count)
{
	meshopt_Stream streams[3];
	S32 index = 0;
	if (vert_pos)
	{
		streams[index].data = (const F32*)vert_pos;
		// Despite being LLVector4a, only x, y and z are in use
		streams[index].size = 3 * sizeof(F32);
		streams[index++].stride = 4 * sizeof(F32);
	}
	if (normals)
	{
		streams[index].data = (const F32*)normals;
		// Despite being LLVector4a, only x, y and z are in use
		streams[index].size = 3 * sizeof(F32);
		streams[index++].stride = 4 * sizeof(F32);
	}
	if (tex_coords)
	{
		streams[index].data = (const F32*)tex_coords;
		streams[index].size = 2 * sizeof(F32);
		streams[index++].stride = 2 * sizeof(F32);
	}
	if (!index)
	{
		return;	// Invalid. Abort.
	}
	meshopt_generateShadowIndexBufferMulti<U16>(dest, indices, idx_count,
												vert_count, streams, index);
}

//static
void LLMeshOptimizer::generateShadowIndexBuffer32(U32* dest,
												  const U32* indices,
												  U64 idx_count,
												  const LLVector4a* vert_pos,
												  const LLVector4a* normals,
												  const LLVector2* tex_coords,
												  U64 vert_count)
{
	meshopt_Stream streams[3];
	S32 index = 0;
	if (vert_pos)
	{
		streams[index].data = (const F32*)vert_pos;
		// Despite being LLVector4a, only x, y and z are in use
		streams[index].size = 3 * sizeof(F32);
		streams[index++].stride = 4 * sizeof(F32);
	}
	if (normals)
	{
		streams[index].data = (const F32*)normals;
		// Despite being LLVector4a, only x, y and z are in use
		streams[index].size = 3 * sizeof(F32);
		streams[index++].stride = 4 * sizeof(F32);
	}
	if (tex_coords)
	{
		streams[index].data = (const F32*)tex_coords;
		streams[index].size = 2 * sizeof(F32);
		streams[index++].stride = 2 * sizeof(F32);
	}
	if (!index)
	{
		return;	// Invalid. Abort.
	}
	meshopt_generateShadowIndexBufferMulti<U32>(dest, indices, idx_count,
												vert_count, streams, index);
}

//static
size_t LLMeshOptimizer::generateRemapMulti16(U32* remap,
											 const U16* indices,
											 U64 index_count,
											 const LLVector4a* vert_pos,
											 const LLVector4a* normals,
											 const LLVector2* tex_coords,
											 U64 vert_count)
{
	U32* indices_u32 = NULL;
	// Remap can function without indices, but providing indices helps with
	// removing unused vertices.
	if (indices)
	{
		indices_u32 = (U32*)allocate_volume_mem(index_count * sizeof(U32));
		if (!indices_u32)
		{
			LLMemory::allocationFailed();
			llwarns << "Out of memory trying to convert indices" << llendl;
			return 0;
		}
		S32 out_of_range_count = 0;
		for (U64 i = 0; i < index_count; ++i)
		{
			if (indices[i] < vert_count)
			{
				indices_u32[i] = indices[i];
			}
			else
			{
				++out_of_range_count;
				indices_u32[i] = 0;
			}
		}
		if (out_of_range_count)
		{
			llwarns << out_of_range_count
					<< " indices were out of range (now zeroed)." << llendl;
		}
	}
	size_t unique = generateRemapMulti32(remap, indices_u32, index_count,
										 vert_pos, normals, tex_coords,
										 vert_count);
	free_volume_mem(indices_u32);
	return unique;
}

//static
size_t LLMeshOptimizer::generateRemapMulti32(U32* remap,
											 const U32* indices,
											 U64 index_count,
											 const LLVector4a* vert_pos,
											 const LLVector4a* normals,
											 const LLVector2* tex_coords,
											 U64 vert_count)
{
	meshopt_Stream streams[] =
	{
		{ (const F32*)vert_pos, sizeof(F32) * 3, sizeof(F32) * 4 },
		{ (const F32*)normals, sizeof(F32) * 3, sizeof(F32) * 4 },
		{ (const F32*)tex_coords, sizeof(F32) * 2, sizeof(F32) * 2 },
	};
	constexpr size_t streams_elements = LL_ARRAY_SIZE(streams);

	// Remap can function without indices, but providing indices helps with
	// removing unused vertices.
	U64 indices_cnt = indices ? index_count : vert_count;
	// Note: this will fail on assert should indices[i] >= vert_count happen.
	return meshopt_generateVertexRemapMulti(remap, indices, indices_cnt,
											vert_count, streams,
											streams_elements);
}

//static
void LLMeshOptimizer::remapIndexBuffer16(U16* dest, const U16* indices,
										 U64 index_count, const U32* remap)
{
	meshopt_remapIndexBuffer<U16>(dest, indices, index_count, remap);
}

//static
void LLMeshOptimizer::remapIndexBuffer32(U32* dest, const U32* indices,
										 U64 index_count, const U32* remap)
{
	meshopt_remapIndexBuffer<U32>(dest, indices, index_count, remap);
}

//static
void LLMeshOptimizer::remapVertsBuffer(LLVector4a* dest,
									   const LLVector4a* verts, U64 count,
									   const U32* remap)
{
	meshopt_remapVertexBuffer((F32*)dest, (const F32*)verts, count,
							  sizeof(LLVector4a), remap);
}

//static
void LLMeshOptimizer::remapTexCoordsBuffer(LLVector2* dest, const LLVector2* tc,
										   U64 tc_count, const U32* remap)
{
	meshopt_remapVertexBuffer((F32*)dest, (const F32*)tc, tc_count,
							  sizeof(LLVector2), remap);
}

//static
size_t LLMeshOptimizer::simplify16(U16* dest, const U16* indices, U64 idx_count,
								   const LLVector4a* vert_pos, U64 vert_count,
								   U64 vert_pos_stride, U64 target_idx_count,
								   F32 target_error, bool sloppy,
								   F32* result_error)
{
	if (sloppy)
	{
		return meshopt_simplifySloppy<U16>(dest, indices, idx_count,
										   (const F32*)vert_pos, vert_count,
										   vert_pos_stride, target_idx_count,
										   target_error, result_error);
	}

	return meshopt_simplify<U16>(dest, indices, idx_count, (const F32*)vert_pos,
								 vert_count, vert_pos_stride, target_idx_count,
								 target_error, meshopt_SimplifyLockBorder,
								 result_error);
}

//static
size_t LLMeshOptimizer::simplify32(U32* dest, const U32* indices,
								   U64 idx_count, const LLVector4a* vert_pos,
								   U64 vert_count, U64 vert_pos_stride,
								   U64 target_idx_count, F32 target_error,
								   bool sloppy, F32* result_error)
{
	if (sloppy)
	{
		return meshopt_simplifySloppy<U32>(dest, indices, idx_count,
										   (const F32*)vert_pos, vert_count,
										   vert_pos_stride, target_idx_count,
										   target_error, result_error);
	}

	return meshopt_simplify<U32>(dest, indices, idx_count, (const F32*)vert_pos,
								 vert_count, vert_pos_stride, target_idx_count,
								 target_error, meshopt_SimplifyLockBorder,
								 result_error);
}
