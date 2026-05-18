/**
 * @file llmultigesture.h
 * @brief Gestures that are asset-based and can have multiple steps.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include <set>
#include <string>
#include <vector>

#include "llframetimer.h"
#include "llpreprocessor.h"
#include "lluuid.h"

class LLDataPacker;
class LLGestureStep;

class LLMultiGesture
{
protected:
	LOG_CLASS(LLMultiGesture);

public:
	LLMultiGesture();
	virtual ~LLMultiGesture();

	// Maximum number of bytes this could hold once serialized.
	S32 getMaxSerialSize() const;

	bool serialize(LLDataPacker& dp) const;
	bool deserialize(LLDataPacker& dp);

	void dump();

	void reset();

	LL_INLINE const std::string& getTrigger() const	{ return mTrigger; }

protected:
	LLMultiGesture(const LLMultiGesture& gest);
	const LLMultiGesture& operator=(const LLMultiGesture& rhs);

public:
	// name is stored at asset level
	// desc is stored at asset level
	KEY mKey;
	MASK mMask;

	// String, like "/foo" or "hello" that makes it play
	std::string mTrigger;

	// Replaces the trigger substring with this text
	std::string mReplaceText;

	std::vector<LLGestureStep*> mSteps;

	// Is the gesture currently playing?
	bool mPlaying;

	// We're waiting for triggered animations to stop playing
	bool mWaitingAnimations;

	// We're waiting a fixed amount of time
	bool mWaitingTimer;

	// Waiting after the last step played for all animations to complete
	bool mWaitingAtEnd;

	// "instruction pointer" for steps
	S32 mCurrentStep;

	// Timer for waiting
	LLFrameTimer mWaitTimer;

	void (*mDoneCallback)(LLMultiGesture* gesture, void* data);
	void* mCallbackData;

	// Animations that we requested to start
	uuid_list_t mRequestedAnimIDs;

	// Once the animation starts playing (sim says to start playing) the ID is
	// moved from mRequestedAnimIDs to here.
	uuid_list_t mPlayingAnimIDs;
};

// Order must match the library_list in floater_preview_gesture.xml !

enum EStepType
{
	STEP_ANIMATION = 0,
	STEP_SOUND = 1,
	STEP_CHAT = 2,
	STEP_WAIT = 3,

	STEP_EOF = 4
};

class LLGestureStep
{
protected:
	LOG_CLASS(LLGestureStep);

public:
	LLGestureStep() = default;
	virtual ~LLGestureStep() = default;

	virtual EStepType getType() = 0;

	// Return a user-readable label for this step
	virtual std::string getLabel() const = 0;

	virtual S32 getMaxSerialSize() const = 0;
	virtual bool serialize(LLDataPacker& dp) const = 0;
	virtual bool deserialize(LLDataPacker& dp) = 0;

	virtual void dump() = 0;
};

// By default, animation steps start animations.
// If the least significant bit is 1, it will stop animations.
constexpr U32 ANIM_FLAG_STOP = 0x01;

class LLGestureStepAnimation final : public LLGestureStep
{
protected:
	LOG_CLASS(LLGestureStepAnimation);

public:
	LLGestureStepAnimation();

	LL_INLINE EStepType getType() override			{ return STEP_ANIMATION; }

	std::string getLabel() const override;

	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;

	void dump() override;

public:
	std::string	mAnimName;
	LLUUID		mAnimAssetID;
	U32			mFlags;
};

class LLGestureStepSound final : public LLGestureStep
{
protected:
	LOG_CLASS(LLGestureStepSound);

public:
	LLGestureStepSound();

	LL_INLINE EStepType getType() override			{ return STEP_SOUND; }

	std::string getLabel() const override;

	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;

	void dump() override;

public:
	std::string	mSoundName;
	LLUUID		mSoundAssetID;
	U32			mFlags;
};

class LLGestureStepChat final : public LLGestureStep
{
protected:
	LOG_CLASS(LLGestureStepChat);

public:
	LLGestureStepChat();

	LL_INLINE EStepType getType() override			{ return STEP_CHAT; }

	std::string getLabel() const override;

	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;

	void dump() override;

public:
	std::string	mChatText;
	U32			mFlags;
};

constexpr U32 WAIT_FLAG_TIME		= 0x01;
constexpr U32 WAIT_FLAG_ALL_ANIM	= 0x02;

class LLGestureStepWait final : public LLGestureStep
{
protected:
	LOG_CLASS(LLGestureStepWait);

public:
	LLGestureStepWait();

	EStepType getType() override					{ return STEP_WAIT; }

	std::string getLabel() const override;

	S32 getMaxSerialSize() const override;
	bool serialize(LLDataPacker& dp) const override;
	bool deserialize(LLDataPacker& dp) override;

	void dump() override;

public:
	F32 mWaitSeconds;
	U32 mFlags;
};
