/**
 * @file streamingaudio.h
 * @author Tofu Linden
 * @brief Definition of LLStreamingAudioInterface base class abstracting the
 * streaming audio interface
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "stdtypes.h" // from llcommon

// Entirely abstract. Based exactly on the historic API.
class LLStreamingAudioInterface
{
 public:
	virtual ~LLStreamingAudioInterface() = default;

	virtual void start(const std::string& url) = 0;
	virtual void stop() = 0;
	virtual void pause(int pause) = 0;
	virtual void update() = 0;
	virtual S32 isPlaying() = 0;
	// Use a value from 0.0 to 1.0, inclusive
	virtual void setGain(F32 vol) = 0;
	virtual F32 getGain() = 0;
	virtual std::string getURL() = 0;
	virtual bool supportsAdjustableBufferSizes()		{ return false; }
	virtual void setBufferSizes(U32 streambuffertime,
								U32 decodebuffertime)	{}
	// Support for artist and title meta-data
	virtual bool newMetaData() const = 0;
	virtual void gotMetaData() = 0;
	virtual const std::string& getArtist() const = 0;
	virtual const std::string& getTitle() const = 0;
};
