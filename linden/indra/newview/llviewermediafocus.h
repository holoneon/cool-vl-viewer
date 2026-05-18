/**
 * @file llviewermediafocus.h
 * @brief Declaration for the class governing focus on media prims
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llfocusmgr.h"

#include "llselectmgr.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"		// For LLPickInfo

class LLViewerMediaImpl;
class LLPanelMediaHUD;

class LLViewerMediaFocus : public LLFocusableElement,
						   public LLSingleton<LLViewerMediaFocus>
{
	friend class LLSingleton<LLViewerMediaFocus>;

protected:
	LOG_CLASS(LLViewerMediaFocus);

public:
	LLViewerMediaFocus();
	~LLViewerMediaFocus();

	static void cleanupClass();

	void setFocusFace(bool face_auto_zoom, LLPointer<LLViewerObject> objectp,
					  S32 face, viewer_media_t media_impl,
					  LLVector3 pick_normal = LLVector3::zero);
	void clearFocus();

	// Set/clear the face that has "media hover" (has the mimimal set of
	// controls to zoom in or pop out into a media browser). If a media face
	// has focus, the media hover will be ignored.
	void setHoverFace(LLPointer<LLViewerObject> objectp,
					  S32 face,
					  viewer_media_t media_impl,
					  LLVector3 pick_normal = LLVector3::zero);
	void clearHover();

	LL_INLINE void setPickInfo(LLPickInfo pick_info)	{ mPickInfo = pick_info; }

	bool getFocus();

	// The MOAP objects want keyup and keydown events. Overridden from
	// LLFocusableElement to return true.
	LL_INLINE bool wantsKeyUpKeyDown() const override	{ return true; }
	LL_INLINE bool wantsReturnKey() const override		{ return true; }

	bool handleKey(KEY key, MASK mask, bool called_from_parent) override;

	bool handleKeyUp(KEY key, MASK mask, bool called_from_parent) override;

	bool handleUnicodeChar(llwchar uni_char, bool called_from_parent) override;
	bool handleScrollWheel(S32 x, S32 y, S32 clicks);

	void update();

	void setCameraZoom(LLViewerObject* object, LLVector3 normal,
					   F32 padding_factor, bool zoom_in_only = true);
	void focusZoomOnMedia(const LLUUID& media_id);
	void unZoom();
	bool isZoomed();
	bool isZoomedOnMedia(const LLUUID& media_id);

	F32 getBBoxAspectRatio(const LLBBox& bbox, const LLVector3& normal,
						   F32* height, F32* width, F32* depth);

	bool isFocusedOnFace(LLPointer<LLViewerObject> objectp, S32 face);
	bool isHoveringOverFace(LLPointer<LLViewerObject> objectp, S32 face);

	// These look up (by uuid) and return the values that were set with
	// setFocusFace. They will return null if the objects have been destroyed.
	LLViewerMediaImpl* getFocusedMediaImpl();
	LLViewerObject* getFocusedObject();
	LL_INLINE S32 getFocusedFace()						{ return mFocusedObjectFace; }
	LL_INLINE LLUUID getFocusedObjectID()				{ return mFocusedObjectID; }
	LL_INLINE LLObjectSelectionHandle getSelection()	{ return mSelection; }

	// These look up (by uuid) and return the values that were set with
	// setHoverFace. They will return null if the objects have been destroyed.
	LLViewerMediaImpl* getHoverMediaImpl();
	LLViewerObject* getHoverObject();
	LL_INLINE S32 getHoverFace()						{ return mHoverObjectFace; }

	LL_INLINE bool isFocusableCtrl() const override		{ return false; }

protected:
	void onFocusReceived() override;
	void onFocusLost() override;

private:
	LLHandle<LLPanelMediaHUD>	mMediaHUD;

	LLObjectSelectionHandle		mSelection;

	LLUUID						mFocusedObjectID;
	LLUUID						mFocusedImplID;
	LLUUID						mPrevFocusedImplID;
	LLUUID						mHoverObjectID;
	LLUUID						mHoverImplID;

	LLVector3					mFocusedObjectNormal;
	LLVector3					mHoverObjectNormal;

	S32							mFocusedObjectFace;
	S32							mHoverObjectFace;

	LLPickInfo					mPickInfo;

	bool						mFocusedIsHUDObject;
};
