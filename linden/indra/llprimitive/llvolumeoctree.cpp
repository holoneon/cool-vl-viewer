/** 
 * @file llvolumeoctree.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * 
 * Copyright (c) 2002-2010, Linden Research, Inc.
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

#include "llvolumeoctree.h"

bool LLLineSegmentBoxIntersect(const LLVector4a& start,
							   const LLVector4a& end,
							   const LLVector4a& center,
							   const LLVector4a& size)
{
	LLVector4a fAWdU;
	LLVector4a dir;
	LLVector4a diff;

	dir.setSub(end, start);
	dir.mul(0.5f);

	diff.setAdd(end,start);
	diff.mul(0.5f);
	diff.sub(center);
	fAWdU.setAbs(dir); 

	LLVector4a rhs;
	rhs.setAdd(size, fAWdU);

	LLVector4a lhs;
	lhs.setAbs(diff);

	U32 grt = lhs.greaterThan(rhs).getGatheredBits();

	if (grt & 0x7)
	{
		return false;
	}
	
	LLVector4a f;
	f.setCross3(dir, diff);
	f.setAbs(f);

	LLVector4a v0, v1;

	v0 = _mm_shuffle_ps(size, size,_MM_SHUFFLE(3, 0, 0, 1));
	v1 = _mm_shuffle_ps(fAWdU, fAWdU, _MM_SHUFFLE(3, 1, 2, 2));
	lhs.setMul(v0, v1);

	v0 = _mm_shuffle_ps(size, size, _MM_SHUFFLE(3, 1, 2, 2));
	v1 = _mm_shuffle_ps(fAWdU, fAWdU, _MM_SHUFFLE(3, 0, 0, 1));
	rhs.setMul(v0, v1);
	rhs.add(lhs);
	
	grt = f.greaterThan(rhs).getGatheredBits();

	return (grt & 0x7) == 0;
}

template<typename T_PTR>
_LLVolumeOctreeListener<T_PTR>::_LLVolumeOctreeListener(_LLOctreeNode<LLVolumeTriangle,
														T_PTR>* nodep)
{
	if (nodep)
	{
		nodep->addListener(this);
	}
}

template<typename T_PTR>
_LLVolumeOctreeListener<T_PTR>::~_LLVolumeOctreeListener()
{
}

template<typename T_PTR>
void _LLVolumeOctreeListener<T_PTR>::handleChildAddition(const _LLOctreeNode<LLVolumeTriangle,
																			 T_PTR>* parentp,
														 _LLOctreeNode<LLVolumeTriangle,
																	   T_PTR>* childp)
{
	new _LLVolumeOctreeListener<T_PTR>(childp);
}

template class _LLVolumeOctreeListener<LLVolumeTriangle*>;
template class _LLVolumeOctreeListener<LLPointer<LLVolumeTriangle> >;

template<typename T_PTR>
_LLOctreeTriangleRayIntersect<T_PTR>::_LLOctreeTriangleRayIntersect(const LLVector4a& start,
																	const LLVector4a& dir,
																	LLVolumeFace* facep,
																	F32* closest_tp,
																	LLVector4a* intersectp,
																	LLVector2* tcoordp,
																	LLVector4a* normalp,
																	LLVector4a* tangentp)
:	mFace(facep),
	mStart(start),
	mDir(dir),
	mIntersection(intersectp),
	mTexCoord(tcoordp),
	mNormal(normalp),
	mTangent(tangentp),
	mClosestT(closest_tp),
	mHitTriangle(NULL),
	mHitFace(false)
{
	mEnd.setAdd(mStart, mDir);
}

template<typename T_PTR>
void _LLOctreeTriangleRayIntersect<T_PTR>::traverse(const _LLOctreeNode<LLVolumeTriangle,
																		T_PTR>* nodep)
{
	if (!nodep) return;

	_LLVolumeOctreeListener<T_PTR>* vl =
		(_LLVolumeOctreeListener<T_PTR>*)nodep->getListener(0);
	if (vl && LLLineSegmentBoxIntersect(mStart, mEnd, vl->mBounds[0],
										vl->mBounds[1]))
	{
		nodep->accept(this);
		for (U32 i = 0; i < nodep->getChildCount(); ++i)
		{
			traverse(nodep->getChild(i));
		}
	}
}

template<typename T_PTR>
void _LLOctreeTriangleRayIntersect<T_PTR>::visit(const _LLOctreeNode<LLVolumeTriangle,
																	 T_PTR>* nodep)
{
	if (!nodep) return;

	for (typename _LLOctreeNode<LLVolumeTriangle, T_PTR>::const_element_iter
			iter = nodep->getDataBegin();
		 iter != nodep->getDataEnd(); ++iter)
	{
		const LLVolumeTriangle* trip = *iter;
		if (!trip) continue;		// Paranoia ?

		F32 a, b, t;
		if (LLTriangleRayIntersect(*trip->mV[0], *trip->mV[1], *trip->mV[2],
								   mStart, mDir, a, b, t))
		{
			if (t >= 0.f &&		// if hit is after start
				t <= 1.f &&		// and before end
				t < *mClosestT)	// and this hit is closer
			{
				*mClosestT = t;
				mHitFace = true;
				mHitTriangle = trip;
				if (mIntersection)
				{
					LLVector4a intersect = mDir;
					intersect.mul(*mClosestT);
					intersect.add(mStart);
					*mIntersection = intersect;
				}

				U32 idx0 = trip->mIndex[0];
				U32 idx1 = trip->mIndex[1];
				U32 idx2 = trip->mIndex[2];

				if (mTexCoord && mFace->mTexCoords)
				{
					LLVector2* tc = (LLVector2*)mFace->mTexCoords;
					*mTexCoord = (1.f - a - b) * tc[idx0] + a * tc[idx1] +
								 b * tc[idx2];

				}

				if (mNormal && mFace->mNormals)
				{
					LLVector4a* norm = mFace->mNormals;

					LLVector4a n1 = norm[idx0];
					n1.mul(1.f - a - b);
					LLVector4a n2 = norm[idx1];
					n2.mul(a);
					LLVector4a n3 = norm[idx2];
					n3.mul(b);
					n1.add(n2);
					n1.add(n3);

					*mNormal = n1;
				}

				if (mTangent && mFace->mTangents)
				{
					LLVector4a* tangents = mFace->mTangents;

					LLVector4a t1 = tangents[idx0];
					t1.mul(1.f - a - b);
					LLVector4a t2 = tangents[idx1];
					t2.mul(a);
					LLVector4a t3 = tangents[idx2];
					t3.mul(b);
					t1.add(t2);
					t1.add(t3);

					*mTangent = t1;
				}
			}
		}
	}
}

template class _LLOctreeTriangleRayIntersect<LLVolumeTriangle*>;
template class _LLOctreeTriangleRayIntersect<LLPointer<LLVolumeTriangle> >;

const LLVector4a& LLVolumeTriangle::getPositionGroup() const
{
	return mPositionGroup;
}

const F32& LLVolumeTriangle::getBinRadius() const
{
	return mRadius;
}

// TEST CODE

template<typename T_PTR>
void _LLVolumeOctreeValidate<T_PTR>::visit(const _LLOctreeNode<LLVolumeTriangle,
															   T_PTR>* branchp)
{
	if (!branchp) return;

	_LLVolumeOctreeListener<T_PTR>* nodep =
		(_LLVolumeOctreeListener<T_PTR>*)branchp->getListener(0);
	if (!nodep) return;

	// Make sure bounds matches extents
	LLVector4a& min = nodep->mExtents[0];
	LLVector4a& max = nodep->mExtents[1];

	LLVector4a& center = nodep->mBounds[0];
	LLVector4a& size = nodep->mBounds[1];

	LLVector4a test_min, test_max;
	test_min.setSub(center, size);
	test_max.setAdd(center, size);

	if (!test_min.equals3(min, 0.001f) || !test_max.equals3(max, 0.001f))
	{
		llerrs << "Bad bounding box data found." << llendl;
	}

	test_min.sub(LLVector4a(0.001f));
	test_max.add(LLVector4a(0.001f));

	for (U32 i = 0; i < branchp->getChildCount(); ++i)
	{
		_LLVolumeOctreeListener<T_PTR>* childp =
			(_LLVolumeOctreeListener<T_PTR>*)branchp->getChild(i)->getListener(0);
		if (!childp) continue;
		// Make sure all children fit inside this node
		if (childp->mExtents[0].lessThan(test_min).areAnySet(LLVector4Logical::MASK_XYZ) ||
			childp->mExtents[1].greaterThan(test_max).areAnySet(LLVector4Logical::MASK_XYZ))
		{
			llerrs << "Child protrudes from bounding box." << llendl;
		}
	}

	// Children fit, check data
	for (typename _LLOctreeNode<LLVolumeTriangle, T_PTR>::const_element_iter
			iter = branchp->getDataBegin(); 
		 iter != branchp->getDataEnd(); ++iter)
	{
		const LLVolumeTriangle* trip = *iter;
		if (!trip) continue;

		// Validate triangle
		for (U32 i = 0; i < 3; i++)
		{
			if (trip->mV[i]->greaterThan(test_max).areAnySet(LLVector4Logical::MASK_XYZ) ||
				trip->mV[i]->lessThan(test_min).areAnySet(LLVector4Logical::MASK_XYZ))
			{
				llerrs << "Triangle protrudes from node." << llendl;
			}
		}
	}
}

template class _LLVolumeOctreeValidate<LLPointer<LLVolumeTriangle> >;
template class _LLVolumeOctreeValidate<LLVolumeTriangle*>;
