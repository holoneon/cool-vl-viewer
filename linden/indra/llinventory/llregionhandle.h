/**
 * @file llregionhandle.h
 * @brief Routines for converting positions to/from region handles.
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

#include "indra_constants.h"
#include "llpreprocessor.h"
#include "llvector3.h"
#include "llvector3d.h"

LL_INLINE U64 to_region_handle(U32 x_origin, U32 y_origin)
{
	U64 region_handle;
	region_handle =  ((U64)x_origin) << 32;
	region_handle |= (U64) y_origin;
	return region_handle;
}

LL_INLINE U64 to_region_handle(const LLVector3d& pos_global)
{
	U32 global_x = (U32)pos_global.mdV[VX];
	global_x -= global_x % 256;

	U32 global_y = (U32)pos_global.mdV[VY];
	global_y -= global_y % 256;

	return to_region_handle(global_x, global_y);
}

LL_INLINE U64 to_region_handle_global(F32 x_global, F32 y_global)
{
	// Round down to the nearest origin
	U32 x_origin = (U32)x_global;
	x_origin -= x_origin % REGION_WIDTH_U32;
	U32 y_origin = (U32)y_global;
	y_origin -= y_origin % REGION_WIDTH_U32;
	U64 region_handle;
	region_handle =  ((U64)x_origin) << 32;
	region_handle |= (U64) y_origin;
	return region_handle;
}

LL_INLINE bool to_region_handle(F32 x_pos, F32 y_pos, U64* region_handle)
{
	U32 x_int, y_int;
	if (x_pos < 0.f)
	{
#if 0
		llwarns << "to_region_handle:Clamping negative x position " << x_pos
				 << " to zero !" << llendl;
#endif
		return false;
	}
	else
	{
		x_int = (U32)ll_roundp(x_pos);
	}
	if (y_pos < 0.f)
	{
#if 0
		llwarns << "to_region_handle:Clamping negative y position " << y_pos
				<< " to zero !" << llendl;
#endif
		return false;
	}
	else
	{
		y_int = (U32)ll_roundp(y_pos);
	}
	*region_handle = to_region_handle(x_int, y_int);
	return true;
}

// Stuffs the word-frame XY location of sim's SouthWest corner in x_pos, y_pos
LL_INLINE void from_region_handle(U64 region_handle, F32* x_pos, F32* y_pos)
{
	*x_pos = (F32)((U32)(region_handle >> 32));
	*y_pos = (F32)((U32)(region_handle & 0xFFFFFFFF));
}

// Stuffs the word-frame XY location of sim's SouthWest corner in x_pos, y_pos
LL_INLINE void from_region_handle(U64 region_handle, U32* x_pos, U32* y_pos)
{
	*x_pos = (U32)(region_handle >> 32);
	*y_pos = (U32)(region_handle & 0xFFFFFFFF);
}

// Returns the word-frame XY location of sim's SouthWest corner in LLVector3d
LL_INLINE LLVector3d from_region_handle(U64 region_handle)
{
	return LLVector3d((U32)(region_handle >> 32),
					  (U32)(region_handle & 0xFFFFFFFF), 0.f);
}

// Grid-based region handle encoding. pass in a grid position (eg: 1000,1000)
// and this will return the region handle.
LL_INLINE U64 grid_to_region_handle(U32 grid_x, U32 grid_y)
{
	return to_region_handle(grid_x * REGION_WIDTH_UNITS,
							grid_y * REGION_WIDTH_UNITS);
}

LL_INLINE void grid_from_region_handle(U64 region_handle, U32* grid_x,
									   U32* grid_y)
{
	from_region_handle(region_handle, grid_x, grid_y);
	*grid_x /= REGION_WIDTH_UNITS;
	*grid_y /= REGION_WIDTH_UNITS;
}
