/**
 * @file llmediaremotectrl.h
 * @brief A remote control for media (video and music)
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 *
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llpanel.h"

class LLButton;
class LLIconCtrl;

class LLMediaRemoteCtrl final : public LLPanel
{
public:
	typedef enum
	{
		REMOTE_MASTER_VOLUME,
		REMOTE_PARCEL_MUSIC,
		REMOTE_PARCEL_MEDIA,
		REMOTE_SHARED_MEDIA
	} ERemoteType;

	LLMediaRemoteCtrl(const std::string& name, const LLRect& rect,
					  const std::string& xml_file, const ERemoteType type);

	bool postBuild() override;
	void draw() override;

private:
	ERemoteType mType;
	LLIconCtrl*	mIcon;
	LLButton*	mPlay;
	LLButton*	mPause;
	LLButton*	mStop;
	std::string	mIconToolTip;
	std::string mCachedURL;
	std::string mCachedMetaData;
};
