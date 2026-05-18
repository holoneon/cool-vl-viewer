/**
 * @file llhoverview.h
 * @brief LLHoverView class definition
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

#include <list>
#include <string>

#include "llframetimer.h"
#include "llview.h"

#include "llviewerobject.h"
#include "llviewerwindow.h"		// For LLPickInfo

class LLFontGL;
class LLParcel;
class LLTool;

class LLHoverView final : public LLView
{
public:
	LLHoverView(const LLRect& rect);
	~LLHoverView() override;

	void draw() override;

	void updateHover(LLTool* current_tool);
	void cancelHover();

	// The last hovered object is retained even after the hover is cancelled,
	// so allow it to be specifically reset. JC
	void resetLastHoverObject();

	void setHoverActive(bool active);

	// We do not do hover picks while the user is typing. In fact, we stop
	// until the mouse is moved.
	LL_INLINE void setTyping(bool b)			{ mTyping = b; }

	LL_INLINE bool isHoveringObject() const		{ return mLastHoverObject.notNull() && !mLastHoverObject->isDead(); }
	LL_INLINE bool isHoveringLand() const		{ return !mHoverLandGlobal.isExactlyZero(); }
	LL_INLINE bool isHovering() const			{ return isHoveringLand() || isHoveringObject(); }

	LLViewerObject* getLastHoverObject() const;
	LL_INLINE LLPickInfo getPickInfo()			{ return mLastPickInfo; }

	static void pickCallback(const LLPickInfo& info);

protected:
	void	updateText();

protected:
	// If not null and not dead, we are over an object.
	LLPointer<LLViewerObject>	mLastHoverObject;
	LLViewerObject*				mLastObjectWithFullText;
	LLParcel*					mLastParcelWithFullText;

	LLPickInfo					mLastPickInfo;

	LLCoordGL					mHoverPos;

	// If not LLVector3d::ZERO, we are over land.
	LLVector3d					mHoverLandGlobal;
	LLVector3					mHoverOffset;

	LLUIImagePtr				mShadowImage;

	const LLFontGL*				mFont;

	// How long has the hover popup been visible ?
	LLFrameTimer				mHoverTimer;
	LLFrameTimer				mStartHoverTimer;

	bool						mStartHoverPickTimer;
	bool						mDoneHoverPick;
	bool						mHoverActive;
	bool						mUseHover;
	bool						mTyping;

	std::string					mRetrievingData;
	std::string					mTooltipPerson;
	std::string					mTooltipNoName;
	std::string					mTooltipOwner;
	std::string					mTooltipPublic;
	std::string					mTooltipIsGroup;
	std::string					mTooltipFlagScript;
	std::string					mTooltipFlagCharacter;
	std::string					mTooltipFlagPhysics;
	std::string					mTooltipFlagPermanent;
	std::string					mTooltipFlagTouch;
	std::string					mTooltipFlagMoney;
	std::string					mTooltipFlagDropInventory;
	std::string					mTooltipFlagPhantom;
	std::string					mTooltipFlagTemporary;
	std::string					mTooltipFlagRightClickMenu;
	std::string					mTooltipFreeToCopy;
	std::string					mTooltipForSaleMsg;
	std::string					mTooltipLand;
	std::string					mTooltipFlagGroupBuild;
	std::string					mTooltipFlagNoBuild;
	std::string					mTooltipFlagNoEdit;
	std::string					mTooltipFlagNotSafe;
	std::string					mTooltipFlagNoFly;
	std::string					mTooltipFlagGroupScripts;
	std::string					mTooltipFlagNoScripts;

	typedef std::list<std::string> text_list_t;
	text_list_t					mText;

public:
	// Show in-world hover tips. Allow to turn off for movie making, game
	// playing. Public so menu can directly toggle.
	static bool					sShowHoverTips;
};

extern LLHoverView* gHoverViewp;
