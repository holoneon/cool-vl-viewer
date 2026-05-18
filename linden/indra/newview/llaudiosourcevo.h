/**
 * @file llaudiosourcevo.h
 * @author Douglas Soo, James Cook
 * @brief Audio sources attached to viewer objects
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

#pragma once

#include "llaudioengine.h"
#include "llpointer.h"

#include "llviewerobject.h"

class LLViewerObject;

class LLAudioSourceVO final : public LLAudioSource
{
public:
	LLAudioSourceVO(const LLUUID& sound_id, const LLUUID& owner_id,
					F32 gain, LLViewerObject* objectp);

	~LLAudioSourceVO() override;

	void update() override;
	void setGain(F32 gain) override;

	void checkCutOffRadius();

	LL_INLINE LLPointer<LLViewerObject> getObject()	{ return mObjectp; }

private:
	bool isInCutOffRadius(const LLVector3d& pos_global, F32 cutoff) const;
	void updateMute();

private:
	LLPointer<LLViewerObject>	mObjectp;
	F32							mLastUpdate;
};
