/**
 * @file llhudobject.h
 * @brief LLHUDObject class definition
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

/**
 * Base class and manager for in-world 2.5D non-interactive objects
 */

#include <list>

#include "llpointer.h"
#include "llrefcount.h"
#include "llvector3.h"
#include "llvector3d.h"
#include "llcolor4.h"

class LLHUDEffect;
class LLViewerObject;

class LLHUDObject : public LLRefCount
{
protected:
	LOG_CLASS(LLHUDObject);

public:
	virtual void markDead();
	LL_INLINE virtual bool isDead() const				{ return mDead; }
	LL_INLINE virtual F32 getDistance() const			{ return 0.f; }
	virtual void setSourceObject(LLViewerObject* objectp);
	virtual void setTargetObject(LLViewerObject* objectp);

	LL_INLINE virtual LLViewerObject* getSourceObject()
	{
		return mSourceObject;
	}

	LL_INLINE virtual LLViewerObject* getTargetObject()	{ return mTargetObject; }

	void setPositionGlobal(const LLVector3d& position_global);
	void setPositionAgent(const LLVector3& position_agent);

	LL_INLINE bool isVisible() const					{ return mVisible; }

	LL_INLINE U8 getType() const						{ return mType; }

	LL_INLINE LLVector3d getPositionGlobal() const		{ return mPositionGlobal; }

	static LLHUDObject* addHUDObject(U8 type);
	static LLHUDEffect* addHUDEffect(U8 type);
	static void updateAll();
	static void renderAll();
	static void removeExpired();

	static void cleanupHUDObjects();

	enum
	{
		LL_HUD_TEXT,
		LL_HUD_ICON,
		LL_HUD_CONNECTOR,
		LL_HUD_FLEXIBLE_OBJECT,
		LL_HUD_ANIMAL_CONTROLS,
		LL_HUD_LOCAL_ANIMATION_OBJECT,
		LL_HUD_CLOTH,
		LL_HUD_EFFECT_BEAM,
		LL_HUD_EFFECT_GLOW,
		LL_HUD_EFFECT_POINT,
		LL_HUD_EFFECT_TRAIL,
		LL_HUD_EFFECT_SPHERE,
		LL_HUD_EFFECT_SPIRAL,
		LL_HUD_EFFECT_EDIT,
		LL_HUD_EFFECT_LOOKAT,
		LL_HUD_EFFECT_POINTAT,
		LL_HUD_EFFECT_VOICE_VISUALIZER
	};

protected:
	static void sortObjects();

	LLHUDObject(U8 type);
	// Do not declare = default here: because of includes circular dependencies
	// this would cause compilation failures. HB
	// *TODO: solve the circular dependency issue.
	~LLHUDObject() override;

	virtual void render() = 0;

	static void renderObjects();

protected:
	LLVector3d					mPositionGlobal;
	LLPointer<LLViewerObject>	mSourceObject;
	LLPointer<LLViewerObject>	mTargetObject;
	U8							mType;
	bool						mDead;
	bool						mVisible;
	bool						mOnHUDAttachment;

private:
	typedef std::list<LLPointer<LLHUDObject> > hud_object_list_t;
	static hud_object_list_t	sHUDObjects;
};
