/**
 * @file llviewerjointattachment.h
 * @brief Implementation of LLViewerJointAttachment class
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

#include "llstring.h"
#include "lluuid.h"

#include "llviewerjoint.h"
#include "llviewerobject.h"

class LLDrawable;

class LLViewerJointAttachment final : public LLViewerJoint
{
	friend class LLVOAvatar;

protected:
	LOG_CLASS(LLViewerJointAttachment);

public:
	LLViewerJointAttachment();

	// Returns true if this object is transparent.
	// This is used to determine in which order to draw objects.
	LL_INLINE bool isTransparent() override			{ return false; }

	// Draws the shape attached to a joint.
	// Called by render().
	U32 drawShape(bool first_pass = true, bool is_dummy = false) override;

	bool updateLOD(F32 pixel_area, bool activate) override;

	LL_INLINE void setPieSlice(S32 pie_slice)		{ mPieSlice = pie_slice; }
	LL_INLINE void setVisibleInFirstPerson(bool b)	{ mVisibleInFirst = b; }
	LL_INLINE bool getVisibleInFirstPerson() const	{ return mVisibleInFirst; }
	LL_INLINE void setGroup(S32 group)				{ mGroup = group; }
	void setOriginalPosition(LLVector3 &position);
	void setAttachmentVisibility(bool visible);
	LL_INLINE void setIsHUDAttachment(bool is_hud)	{ mIsHUDAttachment = is_hud; }
	LL_INLINE bool getIsHUDAttachment() const		{ return mIsHUDAttachment; }

	LL_INLINE bool isAnimatable() const override	{ return false; }

	LL_INLINE S32 getGroup() const					{ return mGroup; }
	LL_INLINE S32 getPieSlice() const				{ return mPieSlice; }

	LL_INLINE S32 getNumObjects() const				{ return mAttachedObjects.size(); }
	S32	getNumAnimatedObjects() const;

	void clampObjectPosition();

	// Attachments operations
	bool isObjectAttached(const LLViewerObject* viewer_object) const;
	const LLViewerObject* getAttachedObject(const LLUUID& object_id) const;
	LLViewerObject* getAttachedObject(const LLUUID& object_id);

	LL_INLINE bool hasChanged(const LLVector3& pos,
							  const LLQuaternion& rot) const
	{
		constexpr F32 SMALL_CHANGE_DIST2 = 0.05f * 0.05f;
		constexpr F32 SMALL_CHANGE_ANGL = 0.225f;	// Just shy of 13 degrees
		// Angle choice and LLQuaternion::almost_equal: almost_equal uses the
		// Small Angle Approximation for cos. The approximation diverges more
		// than 1% at around 0.6620 radians. We are under this limit and to be
		// honest, an error of 1% in this case is acceptable.
		return (pos - mLastTrackedPos).lengthSquared() > SMALL_CHANGE_DIST2 ||
			   !LLQuaternion::almost_equal(rot, mLastTrackedRot,
										   SMALL_CHANGE_ANGL);
	}

	LL_INLINE void setLastTracked(const LLVector3& pos,
								  const LLQuaternion& rot)
	{
		mLastTrackedPos = pos;
		mLastTrackedRot = rot;
	}

protected:
	// Unique methods, used exclusively by LLVOAvatar
	bool addObject(LLViewerObject* object, bool is_self);
	void removeObject(LLViewerObject* object, bool is_self);

	void calcLOD();
	void setupDrawable(LLViewerObject* object);

public:
	// List of attachments for this joint
	typedef std::vector<LLPointer<LLViewerObject> > attachedobjs_vec_t;
	attachedobjs_vec_t mAttachedObjects;

protected:
	LLVector3		mOriginalPos;
	LLVector3		mLastTrackedPos;
	LLQuaternion	mLastTrackedRot;
	S32				mGroup;
	S32				mPieSlice;
	bool			mIsHUDAttachment;
	bool			mVisibleInFirst;
};
