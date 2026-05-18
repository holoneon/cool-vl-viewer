/**
 * @file llcoord.h
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

#include "llpreprocessor.h"

// A two-dimensional pixel value
class LLCoord
{
public:
	LL_INLINE LLCoord() noexcept
	:	mX(0),
		mY(0)
	{
	}

	LL_INLINE LLCoord(S32 x, S32 y) noexcept
	:	mX(x),
		mY(y)
	{
	}

	// Allow the use of the default C++11 move constructor and assignation
	LLCoord(LLCoord&& other) noexcept = default;
	LLCoord& operator=(LLCoord&& other) noexcept = default;

	LLCoord(const LLCoord& other) = default;
	LLCoord& operator=(const LLCoord& other) = default;

	virtual ~LLCoord() = default;

	LL_INLINE virtual void set(S32 x, S32 y)	{ mX = x; mY = y; }

public:
	S32 mX;
	S32 mY;
};

// GL coordinates start in the client region of a window, with origin on bottom
// left of the screen.
class LLCoordGL : public LLCoord
{
public:
	LL_INLINE LLCoordGL() noexcept
	:	LLCoord()
	{
	}

	LL_INLINE LLCoordGL(S32 x, S32 y) noexcept
	:	LLCoord(x, y)
	{
	}

	// Allow the use of the default C++11 move constructor and assignation
	LLCoordGL(LLCoordGL&& other) noexcept = default;
	LLCoordGL& operator=(LLCoordGL&& other) noexcept = default;

	LLCoordGL(const LLCoordGL& other) = default;
	LLCoordGL& operator=(const LLCoordGL& other) = default;

	LL_INLINE bool operator==(const LLCoordGL& other) const
	{
		return mX == other.mX && mY == other.mY;
	}

	LL_INLINE bool operator!=(const LLCoordGL& other) const
	{
		return mX != other.mX || mY != other.mY;
	}
};

//bool operator ==(const LLCoordGL& a, const LLCoordGL& b);

// Window coords include things like window borders, menu regions, etc.
class LLCoordWindow : public LLCoord
{
public:
	LL_INLINE LLCoordWindow() noexcept
	:	LLCoord()
	{
	}

	LL_INLINE LLCoordWindow(S32 x, S32 y) noexcept
	:	LLCoord(x, y)
	{
	}

	// Allow the use of the default C++11 move constructor and assignation
	LLCoordWindow(LLCoordWindow&& other) noexcept = default;
	LLCoordWindow& operator=(LLCoordWindow&& other) noexcept = default;

	LLCoordWindow(const LLCoordWindow& other) = default;
	LLCoordWindow& operator=(const LLCoordWindow& other) = default;

	LL_INLINE bool operator==(const LLCoordWindow& other) const
	{
		return mX == other.mX && mY == other.mY;
	}

	LL_INLINE bool operator!=(const LLCoordWindow& other) const
	{
		return mX != other.mX || mY != other.mY;
	}
};

// Screen coords start at left, top = 0, 0
class LLCoordScreen : public LLCoord
{
public:
	LL_INLINE LLCoordScreen() noexcept
	:	LLCoord()
	{
	}

	LL_INLINE LLCoordScreen(S32 x, S32 y) noexcept
	:	LLCoord(x, y)
	{
	}

	// Allow the use of the default C++11 move constructor and assignation
	LLCoordScreen(LLCoordScreen&& other) noexcept = default;
	LLCoordScreen& operator=(LLCoordScreen&& other) noexcept = default;

	LLCoordScreen(const LLCoordScreen& other) = default;
	LLCoordScreen& operator=(const LLCoordScreen& other) = default;

	LL_INLINE bool operator==(const LLCoordScreen& other) const
	{
		return mX == other.mX && mY == other.mY;
	}

	LL_INLINE bool operator!=(const LLCoordScreen& other) const
	{
		return mX != other.mX || mY != other.mY;
	}
};
