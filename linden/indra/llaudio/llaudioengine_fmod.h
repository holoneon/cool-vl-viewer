/**
 * @file llaudioengine_fmod.h
 * @brief Definition of LLAudioEngine class abstracting the audio support
 * as a FMOD Studio implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2014, Linden Research, Inc.
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

#include "llaudioengine.h"
#include "llwindgen.h"

// Stubs
class LLAudioStreamManagerFMOD;
namespace FMOD
{
	class System;
	class Channel;
	class ChannelGroup;
	class Sound;
	class DSP;
}

typedef struct FMOD_DSP_DESCRIPTION FMOD_DSP_DESCRIPTION;

// Interfaces
class LLAudioEngine_FMOD final : public LLAudioEngine
{
protected:
	LOG_CLASS(LLAudioEngine_FMOD);

public:
	LLAudioEngine_FMOD(bool enable_profiler);
	~LLAudioEngine_FMOD() override;

	// Initialization/startup/shutdown
	bool init(void* user_data) override;
	std::string getDriverName(bool verbose) override;
	void allocateListener() override;

	void shutdown() override;

	bool initWind() override;
	void cleanupWind() override;

	void updateWind(LLVector3 direction, F32 cam_height_above_water) override;

	FMOD::System* getSystem() const				{ return mSystem; }

	typedef F32 MIXBUFFERFORMAT;

protected:
	// Get a free buffer, or flush an existing one if you have to.
	LLAudioBuffer* createBuffer() override;

	// Create a new audio channel.
	LLAudioChannel* createChannel() override;

	void setInternalGain(F32 gain) override;

protected:
	FMOD::System*				mSystem;
	LLWindGen<MIXBUFFERFORMAT>*	mWindGen;
	FMOD_DSP_DESCRIPTION*		mWindDSPDesc;
	FMOD::DSP*					mWindDSP;
	bool						mInited;
	bool						mEnableProfiler;

public:
	static FMOD::ChannelGroup*	sChannelGroups[LLAudioEngine::AUDIO_TYPE_COUNT];
#if LL_LINUX
	static bool					sNoALSA;
	static bool					sNoPulseAudio;
#endif
};

class LLAudioChannelFMOD final : public LLAudioChannel
{
protected:
	LOG_CLASS(LLAudioChannelFMOD);

public:
	LLAudioChannelFMOD(FMOD::System* system);
	~LLAudioChannelFMOD() override;

protected:
	void play() override;
	void playSynced(LLAudioChannel* channelp) override;
	void cleanup() override;
	bool isPlaying() override;

	bool updateBuffer() override;
	void update3DPosition() override;
	void updateLoop() override;

	void set3DMode(bool use3d);

protected:
	FMOD::System* getSystem() const					{ return mSystemp; }

protected:
	FMOD::System*	mSystemp;
	FMOD::Channel*	mChannelp;
	U32				mLastSamplePos;
};

class LLAudioBufferFMOD final : public LLAudioBuffer
{
	friend class LLAudioChannelFMOD;

protected:
	LOG_CLASS(LLAudioBufferFMOD);

public:
	LLAudioBufferFMOD(FMOD::System* system);
	~LLAudioBufferFMOD() override;

	bool loadWAV(const std::string& filename) override;
	U32 getLength() override;

protected:
	FMOD::System* getSystem() const					{ return mSystemp; }
	FMOD::Sound* getSound() const					{ return mSoundp; }

protected:
	FMOD::System*	mSystemp;
	FMOD::Sound*	mSoundp;
};

bool checkFMerr(S32 result, const char* str);
