/**
 * @file llhudicon.h
 * @brief LLHUDIcon class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

// Renders a 2D icon billboard floating at the location specified.

#pragma once

#include <vector>

#include "llframetimer.h"
#include "lluuid.h"

#include "llhudobject.h"

class LLViewerTexture;

class LLHUDIcon final : public LLHUDObject
{
friend class LLHUDObject;

public:
	void render() override;
	void markDead() override;
	F32 getDistance() const override					{ return mDistance; }

	void setImage(LLViewerTexture* imagep);
	LL_INLINE void setScale(F32 fraction_of_fov)		{ mScale = fraction_of_fov; }

	void restartLifeTimer()								{ mLifeTimer.reset(); }

	static LLHUDIcon* lineSegmentIntersectAll(const LLVector4a& start,
											  const LLVector4a& end,
											  LLVector4a* intersection);

	static void cleanupDeadIcons();
	LL_INLINE static void updateAll()					{ cleanupDeadIcons(); }


	LL_INLINE static S32 getNumInstances()				{ return sIconInstances.size(); }

	bool getHidden() const								{ return mHidden; }
	void setHidden(bool hide)							{ mHidden = hide; }

	bool lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
							  LLVector4a* intersection);

	void setClickedCallback(void (*cb)(const LLUUID&))	{ mClickedCallback = cb; }
	void fireClickedCallback(const LLUUID& id);

public:
	static F32 MAX_VISIBLE_TIME;

protected:
	LLHUDIcon(U8 type);
	~LLHUDIcon() override;

private:
	LLPointer<LLViewerTexture>	mImagep;
	LLFrameTimer				mAnimTimer;
	LLFrameTimer				mLifeTimer;
	F32							mDistance;
	F32							mScale;
	bool						mHidden;
	bool						mIsScriptBugIcon;

	void						(*mClickedCallback)(const LLUUID& id);

	typedef std::vector<LLPointer<LLHUDIcon> > icon_instance_t;
	static icon_instance_t sIconInstances;
};
