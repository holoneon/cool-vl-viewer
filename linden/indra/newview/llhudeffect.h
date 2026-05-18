/** 
 * @file llhudeffect.h
 * @brief LLHUDEffect class definition
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

#include "llhudobject.h"

#include "llcolor4u.h"
#include "llframetimer.h"
#include "lluuid.h"

constexpr F32 LL_HUD_DUR_SHORT = 1.f;

class LLMessageSystem;


class LLHUDEffect : public LLHUDObject
{
	friend class LLHUDManager;

public:
	void setNeedsSendToSim(bool b)			{ mNeedsSendToSim = b; }
	bool getNeedsSendToSim() const			{ return mNeedsSendToSim; }
	void setOriginatedHere(bool b)			{ mOriginatedHere = b; }
	bool getOriginatedHere() const			{ return mOriginatedHere; }

	void setDuration(F32 duration)			{ mDuration = duration; }
	void setColor(const LLColor4U& color)	{ mColor = color; }
	void setID(const LLUUID& id)			{ mID = id; }
	const LLUUID& getID() const				{ return mID; }

	bool isDead() const override			{ return mDead; }

protected:
	LLHUDEffect(U8 type);
	~LLHUDEffect() override = default;

	void render() override;

	virtual void packData(LLMessageSystem* mesgsys);
	virtual void unpackData(LLMessageSystem* mesgsys, S32 blocknum);
	virtual void update()					{}

	static void getIDType(LLMessageSystem* mesgsys, S32 blocknum, LLUUID& uuid,
						  U8& type);

protected:
	LLUUID		mID;
	F32			mDuration;
	LLColor4U	mColor;

	bool		mNeedsSendToSim;
	bool		mOriginatedHere;
};
