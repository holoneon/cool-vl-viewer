/**
 * @file llviewerjoystick.h
 * @brief Viewer joystick / NDOF device functionality.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "ndofdev_external.h"

#include "llcontrol.h"

typedef enum e_joystick_driver_state
{
	JDS_UNINITIALIZED,
	JDS_INITIALIZED,
	JDS_INITIALIZING
} EJoystickDriverState;

class LLViewerJoystick final : public LLSingleton<LLViewerJoystick>
{
	friend class LLSingleton<LLViewerJoystick>;

protected:
	LOG_CLASS(LLViewerJoystick);

public:
	LLViewerJoystick();
	~LLViewerJoystick() override;

	void init(bool autoenable);
	void terminate();

	void updateStatus();
	void scanJoystick();
	void moveObjects(bool reset = false);
	void moveAvatar(bool reset = false);
	void moveFlycam(bool reset = false);

	std::string getDescription();
	F32 getJoystickAxis(S32 axis) const;
	bool getJoystickButton(S32 button) const;

	LL_INLINE bool isJoystickInitialized() const	{ return mDriverState == JDS_INITIALIZED; }
	bool isLikeSpaceNavigator() const;

	LL_INLINE void setNeedsReset(bool reset = true)	{ mResetFlag = reset; }

	LL_INLINE void setCameraNeedsUpdate(bool b)		{ mCameraUpdated = b; }
	LL_INLINE bool getCameraNeedsUpdate() const		{ return mCameraUpdated; }

	LL_INLINE bool getOverrideCamera()				{ return mOverrideCamera; }
	void setOverrideCamera(bool val);
	bool toggleFlycam();

	void setToDefaults();
	void setSNDefaults();

protected:
	void updateEnabled(bool autoenable);
	void handleRun(F32 inc);
	void agentSlide(F32 inc);
	void agentPush(F32 inc);
	void agentFly(F32 inc);
	void agentRotate(F32 pitch_inc, F32 turn_inc);
    void agentJump();
	void resetDeltas(S32 axis[]);

	static NDOF_HotPlugResult hotPlugAddCallback(NDOF_Device* dev);
	static void hotPlugRemovalCallback(NDOF_Device* dev);

private:
	LLCachedControl<bool>	mJoystickEnabled;
	LLCachedControl<bool>	mJoystickAvatarEnabled;
	LLCachedControl<bool>	mJoystickFlycamEnabled;
	LLCachedControl<bool>	mJoystickBuildEnabled;

	LLCachedControl<bool>	mCursor3D;

	LLCachedControl<S32>	mJoystickAxis0;
	LLCachedControl<S32>	mJoystickAxis1;
	LLCachedControl<S32>	mJoystickAxis2;
	LLCachedControl<S32>	mJoystickAxis3;
	LLCachedControl<S32>	mJoystickAxis4;
	LLCachedControl<S32>	mJoystickAxis5;
	LLCachedControl<S32>	mJoystickAxis6;

	NDOF_Device*			mNdofDev;

	EJoystickDriverState	mDriverState;

	F32						mPerfScale;
	U32						mJoystickRun;

	F32						mAxes[6];
	bool					mBtn[16];

	bool					mResetFlag;
	bool					mCameraUpdated;
	bool 					mOverrideCamera;

	static F32				sLastDelta[7];
	static F32				sDelta[7];
};
