/**
 * @file llpreviewsound.cpp
 * @brief LLPreviewSound class implementation
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

#include "llpreviewsound.h"

#include "llaudioengine.h"
#include "llbutton.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"	// send_sound_trigger()

constexpr F32 SOUND_GAIN = 1.f;

//static
S32 LLPreviewSound::sPreviewSoundCount = 0;

LLPreviewSound::LLPreviewSound(const std::string& name, const LLRect& rect,
							   const std::string& title,
							   const LLUUID& item_uuid,
							   const LLUUID& object_uuid)
:	LLPreview(name, rect, title, item_uuid, object_uuid)
{
	++sPreviewSoundCount;

	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_preview_sound.xml");

	childSetAction("Sound play btn", playSound, this);
	childSetAction("Sound audition btn", auditionSound, this);

	LLButton* button = getChild<LLButton>("Sound play btn");
	button->setSoundFlags(LLView::SILENT);

	button = getChild<LLButton>("Sound audition btn");
	button->setSoundFlags(LLView::SILENT);

	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);

	const LLInventoryItem* item = getItem();
	if (item)	// May be null (e.g. during prim contents fetches)...
	{
		childSetText("desc", item->getDescription());
		if (gAudiop)
		{
			// preload the sound
			gAudiop->preloadSound(item->getAssetUUID());
		}
	}
	else
	{
		childSetText("desc", std::string("(loading...)"));
	}

	setTitle(title);

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}
}

LLPreviewSound::~LLPreviewSound()
{
	--sPreviewSoundCount;
}

// static
void LLPreviewSound::playSound(void* userdata)
{
	LLPreviewSound* self = (LLPreviewSound*)userdata;
	if (self)
	{
		const LLInventoryItem* item = self->getItem();
		if (item && gAudiop)
		{
			send_sound_trigger(item->getAssetUUID(), SOUND_GAIN);
		}
	}
}

// static
void LLPreviewSound::auditionSound(void* userdata)
{
	LLPreviewSound* self = (LLPreviewSound*)userdata;
	if (self)
	{
		const LLInventoryItem* item = self->getItem();
		if (item && gAudiop)
		{
			gAudiop->triggerSound(item->getAssetUUID(), gAgentID, SOUND_GAIN,
								  LLAudioEngine::AUDIO_TYPE_SFX);
		}
	}
}
