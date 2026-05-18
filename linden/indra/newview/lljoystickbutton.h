/**
 * @file lljoystickbutton.h
 * @brief LLJoystick class definition
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

#include "llbutton.h"
#include "llcoord.h"

#include "llviewertexture.h"

typedef enum e_joystick_quadrant
{
	JQ_ORIGIN,
	JQ_UP,
	JQ_DOWN,
	JQ_LEFT,
	JQ_RIGHT
} EJoystickQuadrant;

class LLJoystick : public LLButton
{
public:
	LLJoystick(const std::string& name, LLRect rect,
			   const std::string& default_image,
			   const std::string& selected_image, EJoystickQuadrant initial);

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;
	bool handleMouseUp(S32 x, S32 y, MASK mask) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;

	virtual void onMouseUp()							{}
	virtual void onHeldDown() = 0;

	F32 getElapsedHeldDownTime();

	// Called by llbutton callback handler
	static void onHeldDown(void* userdata);

	LL_INLINE void setInitialQuadrant(EJoystickQuadrant initial)
	{
		mInitialQuadrant = initial;
	}

	LLXMLNodePtr getXML(bool save_children = true) const override;

	static std::string nameFromQuadrant(const EJoystickQuadrant quadrant);
	static EJoystickQuadrant quadrantFromName(const std::string& name);
	static EJoystickQuadrant selectQuadrant(LLXMLNodePtr node);

protected:
	virtual void updateSlop();						// Recomputes slop margins

protected:
	EJoystickQuadrant	mInitialQuadrant;			// mousedown = click in this quadrant
	LLCoordGL			mInitialOffset;				// pretend mouse started here
	LLCoordGL			mLastMouse;					// where was mouse on last hover event
	LLCoordGL			mFirstMouse;				// when mouse clicked, where was it
	S32					mVertSlopNear;				// where the slop regions end
	S32					mVertSlopFar;				// where the slop regions end
	S32					mHorizSlopNear;				// where the slop regions end
	S32					mHorizSlopFar;				// where the slop regions end
	LLFrameTimer		mHeldDownTimer;
	bool				mHeldDown;
};

// Turn agent left and right, move forward and back
class LLJoystickAgentTurn : public LLJoystick
{
public:
	LLJoystickAgentTurn(const std::string& name, LLRect rect,
						const std::string& default_image,
						const std::string& selected_image,
						EJoystickQuadrant initial)
	:	LLJoystick(name, rect, default_image, selected_image, initial)
	{
	}

	void onHeldDown() override;

	const std::string& getTag() const override;

	LLXMLNodePtr getXML(bool save_children = true) const override;

	static LLView* fromXML(LLXMLNodePtr node, LLView* parent,
						   LLUICtrlFactory* factory);
};

// Slide left and right, move forward and back
class LLJoystickAgentSlide : public LLJoystick
{
public:
	LLJoystickAgentSlide(const std::string& name, LLRect rect,
						 const std::string& default_image,
						 const std::string& selected_image,
						 EJoystickQuadrant initial)
	:	LLJoystick(name, rect, default_image, selected_image, initial)
	{
	}

	void onHeldDown() override;
	void onMouseUp() override;

	const std::string& getTag() const override;

	LLXMLNodePtr getXML(bool save_children = true) const override;

	static LLView* fromXML(LLXMLNodePtr node, LLView* parent,
						   LLUICtrlFactory* factory);
};

// Rotate camera around the focus point
class LLJoystickCameraRotate : public LLJoystick
{
public:
	LLJoystickCameraRotate(const std::string& name, LLRect rect,
						   const std::string& out_img,
						   const std::string& in_img);

	void setToggleState(bool left, bool top, bool right, bool bottom);

	void draw() override;
	bool handleMouseDown(S32 x, S32 y, MASK mask) override;

	void onHeldDown() override;

protected:
	F32 getOrbitRate();
	void updateSlop() override;
	void drawRotatedImage(LLGLTexture* image, S32 rotations);

protected:
	bool			mInLeft;
	bool			mInTop;
	bool			mInRight;
	bool			mInBottom;
};

// Track the camera focus point forward/backward and side to side
class LLJoystickCameraTrack : public LLJoystickCameraRotate
{
public:
	LLJoystickCameraTrack(const std::string& name, LLRect rect,
						  const std::string& out_img,
						  const std::string& in_img)
	:	LLJoystickCameraRotate(name, rect, out_img, in_img)
	{
	}

	void onHeldDown() override;
};

// Zoom the camera in and out
class LLJoystickCameraZoom : public LLJoystick
{
public:
	LLJoystickCameraZoom(const std::string& name, LLRect rect,
						 const std::string& out_img,
						 const std::string& plus_in_img,
						 const std::string& minus_in_img);

	void setToggleState(bool top, bool bottom);

	void draw() override;

	bool handleMouseDown(S32 x, S32 y, MASK mask) override;

	void onHeldDown() override;

protected:
	F32 getOrbitRate();
	void updateSlop() override;

protected:
	LLUIImagePtr	mPlusInImage;
	LLUIImagePtr	mMinusInImage;
	bool			mInTop;
	bool			mInBottom;
};
