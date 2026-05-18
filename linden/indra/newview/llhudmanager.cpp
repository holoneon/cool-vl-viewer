/**
 * @file llhudmanager.cpp
 * @brief LLHUDManager class implementation
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

#include "llhudmanager.h"

#include "llfasttimer.h"
#include "llmessage.h"
#include "object_flags.h"

#include "llagent.h"
#include "llhudeffect.h"
#include "llpipeline.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"

//static
LLHUDManager::effects_list_t LLHUDManager::sHUDEffects;

//static
void LLHUDManager::updateEffects()
{
	LL_FAST_TIMER(FTM_HUD_EFFECTS);
	for (S32 i = 0, count = sHUDEffects.size(); i < count; ++i)
	{
		LLHUDEffect* hep = sHUDEffects[i];
		if (!hep->isDead())
		{
			hep->update();
		}
	}
}

//static
void LLHUDManager::sendEffects()
{
	for (S32 i = 0, count = sHUDEffects.size(); i < count; ++i)
	{
		LLHUDEffect* hep = sHUDEffects[i];
		if (hep->isDead())
		{
			// It does happen (e.g. on TP or logout). Harmless: just ignore. HB
			continue;
		}
		if (hep->mType < LLHUDObject::LL_HUD_EFFECT_BEAM)
		{
			llwarns << "Trying to send effect of unknown type: " << hep->mType
					<< llendl;
			llassert(false);
			continue;
		}
		if (hep->getNeedsSendToSim() && hep->getOriginatedHere())
		{
			LLMessageSystem* msg = gMessageSystemp;
			msg->newMessageFast(_PREHASH_ViewerEffect);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
			msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
			msg->nextBlockFast(_PREHASH_Effect);
			hep->packData(msg);
			hep->setNeedsSendToSim(false);
			gAgent.sendMessage();
		}
	}
}

//static
void LLHUDManager::cleanupClass()
{
	sHUDEffects.clear();
}

//static
void LLHUDManager::cleanupEffects()
{
	effects_list_t::iterator iter = sHUDEffects.begin();
	while (iter != sHUDEffects.end())
	{
		LLHUDEffect* effect = (*iter).get();
		if (!effect || effect->isDead())
		{
			if (iter + 1 != sHUDEffects.end())
			{
				*iter = sHUDEffects.back();
			}
			sHUDEffects.pop_back();
		}
		else
		{
			++iter;
		}
	}
}

//static
LLHUDEffect* LLHUDManager::createEffect(U8 type, bool send_to_sim,
										bool originated_here)
{
	// SJB: DO NOT USE addHUDObject !  Not all LLHUDObjects are LLHUDEffects !
	LLHUDEffect* effectp = LLHUDObject::addHUDEffect(type);
	if (effectp)
	{
		LLUUID tmp;
		tmp.generate();
		effectp->setID(tmp);
		effectp->setNeedsSendToSim(send_to_sim);
		effectp->setOriginatedHere(originated_here);

		sHUDEffects.push_back(effectp);
	}
	return effectp;
}

//static
void LLHUDManager::processViewerEffect(LLMessageSystem* mesgsys, void**)
{
	LLUUID effect_id;
	U8 effect_type = 0;
	S32 number_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_Effect);
	for (S32 k = 0; k < number_blocks; ++k)
	{
		LLHUDEffect* effectp = NULL;
		LLHUDEffect::getIDType(mesgsys, k, effect_id, effect_type);
		effects_list_t::iterator iter = sHUDEffects.begin();
		while (iter != sHUDEffects.end())
		{
			LLHUDEffect* cur_effectp = (*iter).get();
			if (!cur_effectp || cur_effectp->isDead())
			{
				LL_DEBUGS("HudManager") << (cur_effectp ? "Dead" : "NULL")
										<< " effect in manager list; removed."
										<< LL_ENDL;
				if (iter + 1 != sHUDEffects.end())
				{
					*iter = sHUDEffects.back();
				}
				sHUDEffects.pop_back();
				continue;
			}
			if (cur_effectp->getID() == effect_id)
			{
				if (cur_effectp->getType() != effect_type)
				{
					llwarns << "Viewer effect " << effect_id
							<< " update does not match effect type (effect type: "
							<< cur_effectp->getType() << " - update type: "
							<< effect_type << ")" << llendl;
				}
				effectp = cur_effectp;
				break;
			}
			++iter;
		}

		if (effect_type)
		{
			if (!effectp)
			{
				effectp = LLHUDManager::createEffect(effect_type, false,
													 false);
			}
			if (effectp)
			{
				effectp->unpackData(mesgsys, k);
			}
		}
		else
		{
			llwarns << "Received viewer effect " << effect_id
					<< " without type; skipped." << llendl;
		}
	}
}
