/**
 * @file llvolumeoctree.h
 * @brief LLVolume octree classes.
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

#pragma once

#include "lloctree.h"
#include "llvolume.h"

class alignas(16) LLVolumeTriangle : public LLRefCount
{
public:
	LL_VOLUME_AREANA_NEW_DELETE

	LLVolumeTriangle()
	{
		mBinIndex = -1;
	}

	LLVolumeTriangle(const LLVolumeTriangle& rhs)
	{
		*this = rhs;
	}

	virtual const LLVector4a& getPositionGroup() const;
	virtual const F32& getBinRadius() const;

	S32 getBinIndex() const				{ return mBinIndex; }
	void setBinIndex(S32 idx) const		{ mBinIndex = idx; }

public:
	// Note: before these variables, we find the 32 bits counter from
	// LLRefCount... Since mPositionGroup will be 16-bytes aligned, fill-up
	// the gap and align in the cache line with other member variables. HB
	F32					mRadius;
	const LLVector4a*	mV[3];

	LLVector4a			mPositionGroup;

	mutable S32			mBinIndex;
	U16					mIndex[3];
};

template <typename T_PTR>
class alignas(16) _LLVolumeOctreeListener
:	public _LLOctreeListener<LLVolumeTriangle, T_PTR>
{
public:
	LL_VOLUME_AREANA_NEW_DELETE

	_LLVolumeOctreeListener(_LLOctreeNode<LLVolumeTriangle, T_PTR>* node);

	_LLVolumeOctreeListener(const _LLVolumeOctreeListener& rhs)
	{
		*this = rhs;
	}

	~_LLVolumeOctreeListener() override;

	 // LISTENER FUNCTIONS
	void handleChildAddition(const _LLOctreeNode<LLVolumeTriangle, T_PTR>* parent,
							 _LLOctreeNode<LLVolumeTriangle, T_PTR>* child) override;

	void handleStateChange(const LLTreeNode<LLVolumeTriangle>* node) override
	{
	}

	void handleChildRemoval(const _LLOctreeNode<LLVolumeTriangle, T_PTR>* parent,
							const _LLOctreeNode<LLVolumeTriangle, T_PTR>* child) override
	{
	}

	void handleInsertion(const LLTreeNode<LLVolumeTriangle>* node,
						 LLVolumeTriangle* tri) override
	{
	}

	void handleRemoval(const LLTreeNode<LLVolumeTriangle>* node,
					   LLVolumeTriangle* tri) override
	{
	}

	void handleDestruction(const LLTreeNode<LLVolumeTriangle>* node) override
	{
	}

public:
	// Bounding box (center, size) of this node and all its children (tight fit
	// to objects)
	alignas(16) LLVector4a mBounds[2];
	// Extents (min, max) of this node and all its children
	alignas(16) LLVector4a mExtents[2];
};

using LLVolumeOctreeListener = _LLVolumeOctreeListener<LLPointer<LLVolumeTriangle> >;
using LLVolumeOctreeListenerNoOwnership = _LLVolumeOctreeListener<LLVolumeTriangle*>;

template<typename T_PTR>
class alignas(16) _LLOctreeTriangleRayIntersect
:	public _LLOctreeTraveler<LLVolumeTriangle, T_PTR>
{
public:
	_LLOctreeTriangleRayIntersect(const LLVector4a& start,
								  const LLVector4a& dir,
								  LLVolumeFace* face,
								  F32* closest_t,
								  LLVector4a* intersection,
								  LLVector2* tex_coord,
								  LLVector4a* normal,
								  LLVector4a* tangent);

	void traverse(const _LLOctreeNode<LLVolumeTriangle, T_PTR>* node) override;
	void visit(const _LLOctreeNode<LLVolumeTriangle, T_PTR>* node) override;

public:
	LLVector4a				mStart;
	LLVector4a				mDir;
	LLVector4a				mEnd;
	LLVector4a*				mIntersection;
	LLVector2*				mTexCoord;
	LLVector4a*				mNormal;
	LLVector4a*				mTangent;
	F32*					mClosestT;
	LLVolumeFace*			mFace;
	const LLVolumeTriangle*	mHitTriangle;
	bool					mHitFace;
};

using LLOctreeTriangleRayIntersect = _LLOctreeTriangleRayIntersect<LLPointer<LLVolumeTriangle> >;
using LLOctreeTriangleRayIntersectNoOwnership = _LLOctreeTriangleRayIntersect<LLVolumeTriangle*>;

template <typename T_PTR>
class _LLVolumeOctreeValidate
:	public _LLOctreeTraveler<LLVolumeTriangle, T_PTR>
{
	void visit(const _LLOctreeNode<LLVolumeTriangle, T_PTR>* branch) override;
};

using LLVolumeOctreeValidate = _LLVolumeOctreeValidate<LLPointer<LLVolumeTriangle> >;
using LLVolumeOctreeValidateNoOwnership = _LLVolumeOctreeValidate<LLVolumeTriangle*>;

class LLVolumeOctreeRebound
:	public LLOctreeTravelerDepthFirstNoOwnership<LLVolumeTriangle>
{
protected:
	LOG_CLASS(LLVolumeOctreeRebound);

public:
	LLVolumeOctreeRebound() = default;

	void visit(const LLOctreeNodeNoOwnership<LLVolumeTriangle>* branch) override
	{
		if (!branch) return;	// Paranoia ?

		// This is a depth first traversal, so it's safe to assume all children
		// have complete bounding data

		LLVolumeOctreeListenerNoOwnership* node =
			(LLVolumeOctreeListenerNoOwnership*)branch->getListener(0);
		if (!node) return;	// Paranoia ?

		LLVector4a& min = node->mExtents[0];
		LLVector4a& max = node->mExtents[1];

		if (!branch->isEmpty())
		{
			// Node has data, find AABB that binds data set
			const LLVolumeTriangle* tri = *(branch->getDataBegin());
			if (!tri)
			{
				llwarns << "NULL volume triangle found." << llendl;
				return;
			}

			// Initialize min/max to first available vertex
			min = *(tri->mV[0]);
			max = *(tri->mV[0]);

			// For each triangle in node stretch by triangles in node
			for (LLOctreeNodeNoOwnership<LLVolumeTriangle>::const_element_iter
					iter = branch->getDataBegin();
				 iter != branch->getDataEnd(); ++iter)
			{
				tri = *iter;

				min.setMin(min, *tri->mV[0]);
				min.setMin(min, *tri->mV[1]);
				min.setMin(min, *tri->mV[2]);

				max.setMax(max, *tri->mV[0]);
				max.setMax(max, *tri->mV[1]);
				max.setMax(max, *tri->mV[2]);
			}
		}
		else if (branch->getChildCount())
		{
			// No data, but child nodes exist
			LLVolumeOctreeListenerNoOwnership* child =
				(LLVolumeOctreeListenerNoOwnership*)branch->getChild(0)->getListener(0);

			// Initialize min/max to extents of first child
			min = child->mExtents[0];
			max = child->mExtents[1];
		}
		else if (branch->isLeaf())
		{
			llwarns << "Empty leaf" << llendl;
			return;
		}

		for (S32 i = 0, count = branch->getChildCount(); i < count; ++i)
		{
			// Stretch by child extents
			LLVolumeOctreeListenerNoOwnership* child =
				(LLVolumeOctreeListenerNoOwnership*)branch->getChild(i)->getListener(0);
			min.setMin(min, child->mExtents[0]);
			max.setMax(max, child->mExtents[1]);
		}

		node->mBounds[0].setAdd(min, max);
		node->mBounds[0].mul(0.5f);

		node->mBounds[1].setSub(max, min);
		node->mBounds[1].mul(0.5f);
	}
};

class LLVolumeOctree : public LLOctreeRootNoOwnership<LLVolumeTriangle>,
					   public LLRefCount
{
public:
	LL_INLINE LLVolumeOctree(const LLVector4a& center, const LLVector4a& size)
	:	LLOctreeRootNoOwnership<LLVolumeTriangle>(center, size, NULL)
	{
		new LLVolumeOctreeListenerNoOwnership(this);
	}

	LL_INLINE LLVolumeOctree()
	:	LLOctreeRootNoOwnership<LLVolumeTriangle>(LLVector4a::getZero(),
												  LLVector4a(1.f, 1.f, 1.f),
												  NULL)
	{
		new LLVolumeOctreeListenerNoOwnership(this);
	}
};
