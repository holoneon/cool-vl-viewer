/**
 * @file llemote.h
 * @brief Definition of LLEmote class
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

#include "llmotion.h"

#define MIN_REQUIRED_PIXEL_AREA_EMOTE 2000.f

#define EMOTE_MORPH_FADEIN_TIME 0.3f
#define EMOTE_MORPH_IN_TIME 1.1f
#define EMOTE_MORPH_FADEOUT_TIME 1.4f

class LLVisualParam;

class LLEmote final : public LLMotion
{
protected:
	LOG_CLASS(LLEmote);

public:
	LLEmote(const LLUUID& id);

	LL_INLINE static LLMotion* create(const LLUUID& id)		{ return new LLEmote(id); }

	// Motions must specify whether or not they loop
	LL_INLINE bool getLoop() override						{ return false; }

	// Motions must report their total duration
	LL_INLINE F32 getDuration() override					{ return EMOTE_MORPH_FADEIN_TIME +
																	 EMOTE_MORPH_IN_TIME +
																	 EMOTE_MORPH_FADEOUT_TIME; }

	// Motions must report their "ease in" duration
	LL_INLINE F32 getEaseInDuration() override				{ return EMOTE_MORPH_FADEIN_TIME; }

	// Motions must report their "ease out" duration.
	LL_INLINE F32 getEaseOutDuration() override				{ return EMOTE_MORPH_FADEOUT_TIME; }

	// Called to determine when a motion should be activated/deactivated based
	// on avatar pixel coverage.
	LL_INLINE F32 getMinPixelArea() override				{ return MIN_REQUIRED_PIXEL_AREA_EMOTE; }

	// Motions must report their priority
	LL_INLINE LLJoint::JointPriority getPriority() override	{ return LLJoint::MEDIUM_PRIORITY; }

	LL_INLINE LLMotionBlendType getBlendType() override		{ return NORMAL_BLEND; }

	// Run-time (post constructor) initialization, called after parameters have
	// been set. Must return true to indicate success and be available for
	// activation.
	LLMotionInitStatus onInitialize(LLCharacter* character) override;

	// Called when a motion is activated must return true to indicate success,
	// or else it will be deactivated.
	bool onActivate() override;

	// Called per time step. Must return true while it is active, and must
	// return false when the motion is completed.
	bool onUpdate(F32 time, U8* joint_mask) override;

	// Called when a motion is deactivated
	void onDeactivate() override;

	LL_INLINE bool canDeprecate() override					{ return false; }

protected:
	LLCharacter*	mCharacter;
	LLVisualParam*	mParam;
};
