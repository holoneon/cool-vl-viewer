/**
 * @file llstreamingaudio_fmod.h
 * @author Tofu Linden
 * @brief Definition of LLStreamingAudio_FMOD implementation
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

#include "stdtypes.h"

#include "llstreamingaudio.h"

// Stubs
class LLAudioStreamManagerFMOD;
namespace FMOD
{
	class System;
	class Channel;
	class ChannelGroup;
}

// Interfaces
class LLStreamingAudio_FMOD : public LLStreamingAudioInterface
{
protected:
	LOG_CLASS(LLStreamingAudio_FMOD);

public:
	LLStreamingAudio_FMOD(FMOD::System* system);
	~LLStreamingAudio_FMOD() override;

	bool supportsAdjustableBufferSizes() override		{ return true; }
	void setBufferSizes(U32 streambuffertime, U32 decodebuffertime) override;

	void start(const std::string& url) override;
	void stop() override;
	void pause(S32 pause) override;
	void update() override;
	S32 isPlaying() override;
	void setGain(F32 vol) override;
	F32 getGain() override								{ return mGain; }
	std::string getURL() override						{ return mURL; }

	bool newMetaData() const override					{ return mNewMetaData; }
	void gotMetaData() override							{ mNewMetaData = false; }
	const std::string& getArtist() const override		{ return mArtist; }
	const std::string& getTitle() const override		{ return mTitle; }

private:
	bool releaseDeadStreams(bool force);

private:
	FMOD::System*							mSystem;
	FMOD::Channel*							mFMODInternetStreamChannelp;
	FMOD::ChannelGroup*						mStreamGroup;

	LLAudioStreamManagerFMOD*				mCurrentInternetStreamp;

	U32										mBufferMilliSeconds;

	F32										mGain;
	F32										mLastStarved;

	std::string								mURL;
	std::string								mArtist;
	std::string								mTitle;

	std::list<LLAudioStreamManagerFMOD*>	mDeadStreams;

	bool									mPendingStart;
	bool									mNewMetaData;
};
