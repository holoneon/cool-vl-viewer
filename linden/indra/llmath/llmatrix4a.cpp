/**
 * @file llmatrix4a.cpp
 * @brief Methods for vectorized matrix/vector operations
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (C) 2018, Linden Research, Inc.
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

#include "llmath.h"
#include "llmatrix4a.h"
#include "llvector4logical.h"

bool LLMatrix4a::isIdentity() const
{
	thread_local LLMatrix4a mins;
	thread_local LLMatrix4a maxs;
	thread_local bool needs_init = true;
	if (needs_init)
	{
		const LLVector4a delta(0.0001f);

		mins.setIdentity();
		mins.getRow<0>().sub(delta);
		mins.getRow<1>().sub(delta);
		mins.getRow<2>().sub(delta);
		mins.getRow<3>().sub(delta);
	
		maxs.setIdentity();
		maxs.getRow<0>().sub(delta);
		maxs.getRow<1>().sub(delta);
		maxs.getRow<2>().sub(delta);
		maxs.getRow<3>().sub(delta);
	
		needs_init = false;
	}

	LLVector4a mask1 = _mm_and_ps(_mm_cmpgt_ps(mMatrix[0],
											   mins.getRow<0>()),
								  _mm_cmplt_ps(mMatrix[0],
											   maxs.getRow<0>()));
	LLVector4a mask2 = _mm_and_ps(_mm_cmpgt_ps(mMatrix[1],
											   mins.getRow<1>()),
								  _mm_cmplt_ps(mMatrix[1],
											   maxs.getRow<1>()));
	LLVector4a mask3 = _mm_and_ps(_mm_cmpgt_ps(mMatrix[2],
											   mins.getRow<2>()),
								  _mm_cmplt_ps(mMatrix[2],
											   maxs.getRow<2>()));
	LLVector4a mask4 = _mm_and_ps(_mm_cmpgt_ps(mMatrix[3],
											   mins.getRow<3>()),
								  _mm_cmplt_ps(mMatrix[3],
											   maxs.getRow<3>()));

	mask1 = _mm_and_ps(mask1, mask2);
	mask2 = _mm_and_ps(mask3, mask4);

	return _mm_movemask_epi8(_mm_castps_si128(_mm_and_ps(mask1,
														 mask2))) == 0xFFFF;
}

// Convert a bounding box into other coordinate system. Should give the same
// results as transforming every corner of the bounding box and extracting the
// bounding box of that, although that is not necessarily the fastest way to
// implement.
void LLMatrix4a::matMulBoundBox(const LLVector4a* in_extents,
								LLVector4a* out_extents) const
{
	thread_local LLVector4Logical mask[6];
	thread_local bool needs_init = true;
	if (needs_init)
	{
		needs_init = false;
		for (U32 i = 0; i < 6; ++i)
		{
			mask[i].clear();
		}
		mask[0].setElement<2>();	// 001
		mask[1].setElement<1>();	// 010
		mask[2].setElement<1>();	// 011
		mask[2].setElement<2>();
		mask[3].setElement<0>();	// 100
		mask[4].setElement<0>();	// 101
		mask[4].setElement<2>();
		mask[5].setElement<0>();	// 110
		mask[5].setElement<1>();
	}

	// Get 8 corners of bounding box
	LLVector4a v[8];

	v[6] = in_extents[0];
	v[7] = in_extents[1];

	for (U32 i = 0; i < 6; ++i)
	{
		v[i].setSelectWithMask(mask[i], in_extents[0], in_extents[1]);
	}

	LLVector4a tv[8];

	// Transform bounding box into drawable space
	for (U32 i = 0; i < 8; ++i)
	{
		affineTransform(v[i], tv[i]);
	}
	
	// Find bounding box
	out_extents[0] = out_extents[1] = tv[0];

	for (U32 i = 1; i < 8; ++i)
	{
		out_extents[0].setMin(out_extents[0], tv[i]);
		out_extents[1].setMax(out_extents[1], tv[i]);
	}
}
