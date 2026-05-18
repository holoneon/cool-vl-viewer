/**
 * @file llpreviewanim.cpp
 * @brief LLPreviewAnim class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llpreviewanim.h"

#include "llbutton.h"
#include "llkeyframemotion.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llvoavatarself.h"

LLPreviewAnim::LLPreviewAnim(const std::string& name, const LLRect& rect,
							 const std::string& title, const LLUUID& item_uuid,
							 S32 activate, const LLUUID& object_uuid)
:	LLPreview(name, rect, title, item_uuid, object_uuid)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_preview_animation.xml");

	childSetAction("Anim play btn", playAnim, this);
	childSetAction("Anim audition btn", auditionAnim, this);

	const LLInventoryItem* item = getItem();

	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);

	setTitle(title);

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}

	switch (activate)
	{
		case 1:
			refreshFromItem(); // Pre-load the animation immediately
			playAnim((void*)this);
			break;

		case 2:
			refreshFromItem(); // Pre-load the animation immediately
			auditionAnim((void*)this);
			break;

		default:
			break;
	}
}

//virtual
void LLPreviewAnim::refreshFromItem()
{
	const LLInventoryItem* item = getItem();
	if (!item || !isAgentAvatarValid())
	{
		return;
	}

	// Preload the animation
	LLMotion* motionp = gAgentAvatarp->createMotion(item->getAssetUUID());
	if (motionp)
	{
		motionp->setName(item->getName());
	}

	LLPreview::refreshFromItem();
}

//static
void LLPreviewAnim::endAnimCallback(void* userdata)
{
	LLHandle<LLFloater>* handlep = ((LLHandle<LLFloater>*)userdata);
	LLFloater* self = handlep->get();
	delete handlep; // Done with the handle

	if (self)
	{
		self->childSetValue("Anim play btn", false);
		self->childSetValue("Anim audition btn", false);
	}
}

//static
void LLPreviewAnim::playAnim(void* userdata)
{
	LLPreviewAnim* self = (LLPreviewAnim*)userdata;
	if (!self || !isAgentAvatarValid())
	{
		return;
	}

	const LLInventoryItem* item = self->getItem();
	if (!item)
	{
		return;
	}

	LLButton* btn = self->getChild<LLButton>("Anim play btn");
	if (btn)
	{
		btn->toggleState();
	}

	const LLUUID& id = item->getAssetUUID();

	if (!self->childGetValue("Anim play btn").asBoolean())
	{
		gAgentAvatarp->stopMotion(id);
		gAgent.sendAnimationRequest(id, ANIM_REQUEST_STOP);
		return;
	}

	self->mPauseRequest = NULL;
	gAgent.sendAnimationRequest(id, ANIM_REQUEST_START);

	LLMotion* motion = gAgentAvatarp->findMotion(id);
	if (!motion)
	{
		return;
	}
	motion->setDeactivateCallback(&endAnimCallback,
								  (void*)(new LLHandle<LLFloater>(self->getHandle())));
}

//static
void LLPreviewAnim::auditionAnim(void* userdata)
{
	LLPreviewAnim* self = (LLPreviewAnim*)userdata;
	if (!self || !isAgentAvatarValid())
	{
		return;
	}

	const LLInventoryItem* item = self->getItem();
	if (!item)
	{
		return;
	}

	LLButton* btn = self->getChild<LLButton>("Anim audition btn");
	if (btn)
	{
		btn->toggleState();
	}

	const LLUUID& id = item->getAssetUUID();

	if (!self->childGetValue("Anim audition btn").asBoolean())
	{
		gAgentAvatarp->stopMotion(id);
		gAgent.sendAnimationRequest(id, ANIM_REQUEST_STOP);
		return;
	}

	self->mPauseRequest = NULL;
	gAgentAvatarp->startMotion(id);

	LLMotion* motion = gAgentAvatarp->findMotion(id);
	if (!motion)
	{
		return;
	}
	motion->setDeactivateCallback(&endAnimCallback,
								  (void*)(new LLHandle<LLFloater>(self->getHandle())));
}

void LLPreviewAnim::onClose(bool app_quitting)
{
	const LLInventoryItem* item = getItem();
	if (item && isAgentAvatarValid())
	{
		const LLUUID& id = item->getAssetUUID();
		gAgentAvatarp->stopMotion(id);
		gAgent.sendAnimationRequest(id, ANIM_REQUEST_STOP);
	}
	LLFloater::onClose(app_quitting);
}
