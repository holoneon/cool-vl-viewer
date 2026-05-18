/**
 * @file llhudeffect.cpp
 * @brief LLHUDEffect class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llhudeffect.h"

#include "llmessage.h"

#include "llagent.h"

LLHUDEffect::LLHUDEffect(U8 type)
:	LLHUDObject(type),
	mDuration(1.f),
	mNeedsSendToSim(false),
	mOriginatedHere(false)
{
}

void LLHUDEffect::render()
{
	llerrs << "Never call this!" << llendl;
}

void LLHUDEffect::packData(LLMessageSystem* mesgsys)
{
	mesgsys->addUUIDFast(_PREHASH_ID, mID);
	mesgsys->addUUIDFast(_PREHASH_AgentID, gAgentID);
	mesgsys->addU8Fast(_PREHASH_Type, mType);
	mesgsys->addF32Fast(_PREHASH_Duration, mDuration);
	mesgsys->addBinaryData(_PREHASH_Color, mColor.mV, 4);
}

void LLHUDEffect::unpackData(LLMessageSystem* mesgsys, S32 blocknum)
{
	mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_ID, mID, blocknum);
	mesgsys->getU8Fast(_PREHASH_Effect, _PREHASH_Type, mType, blocknum);
	mesgsys->getF32Fast(_PREHASH_Effect, _PREHASH_Duration, mDuration, blocknum);
	mesgsys->getBinaryDataFast(_PREHASH_Effect,_PREHASH_Color, mColor.mV, 4, blocknum);
}

//static
void LLHUDEffect::getIDType(LLMessageSystem* mesgsys, S32 blocknum, LLUUID& id,
							U8& type)
{
	mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_ID, id, blocknum);
	mesgsys->getU8Fast(_PREHASH_Effect, _PREHASH_Type, type, blocknum);
}
