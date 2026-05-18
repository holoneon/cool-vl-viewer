/**
 * @file llpreviewsound.h
 * @brief LLPreviewSound class definition
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

#include "llpreview.h"

class LLPreviewSound final : public LLPreview
{
public:
	LLPreviewSound(const std::string& name, const LLRect& rect,
				   const std::string& title, const LLUUID& item_uuid,
				   const LLUUID& object_uuid = LLUUID::null);

	~LLPreviewSound() override;

	static void playSound(void* userdata);
	static void auditionSound(void* userdata);

	static S32 getPreviewCount()				{ return sPreviewSoundCount; }

protected:
	const char* getTitleName() const override	{ return "Sound"; }

protected:
	static S32 sPreviewSoundCount;
};
