/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
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

#include "llaudioengine.h"
#include "lllistener_openal.h"
#include "llwindgen.h"

class LLAudioEngine_OpenAL final : public LLAudioEngine
{
protected:
	LOG_CLASS(LLAudioEngine_OpenAL);

public:
	LLAudioEngine_OpenAL();

	bool init(void* user_data) override;
	std::string getDriverName(bool verbose) override;
	void allocateListener() override;

	void shutdown() override;

	void setInternalGain(F32 gain) override;

	LLAudioBuffer* createBuffer() override;
	LLAudioChannel* createChannel() override;

	bool initWind() override;
	void cleanupWind() override;
	void updateWind(LLVector3 direction, F32 camera_altitude) override;

private:
	void* windDSP(void *newbuffer, int length);

private:
	typedef F32 wind_sample_t;
	LLWindGen<wind_sample_t>*	mWindGen;
	wind_sample_t*				mWindBuf;
	U32							mWindBufFreq;
	U32							mWindBufSamples;
	U32							mWindBufBytes;
	ALuint						mWindSource;
	int							mNumEmptyWindALBuffers;
};

class LLAudioChannelOpenAL final : public LLAudioChannel
{
protected:
	LOG_CLASS(LLAudioChannelOpenAL);

public:
	LLAudioChannelOpenAL();
	~LLAudioChannelOpenAL() override;

protected:
	void play() override;
	void playSynced(LLAudioChannel* channelp) override;
	void cleanup() override;
	bool isPlaying() override;

	bool updateBuffer() override;
	void update3DPosition() override;
	void updateLoop() override;

protected:
	ALuint mALSource;
	ALint mLastSamplePos;
};

class LLAudioBufferOpenAL final : public LLAudioBuffer
{
	friend class LLAudioChannelOpenAL;

protected:
	LOG_CLASS(LLAudioBufferOpenAL);

public:
	LLAudioBufferOpenAL();
	~LLAudioBufferOpenAL() override;

	bool loadWAV(const std::string& filename) override;
	U32 getLength() override;

protected:
	void cleanup();
	ALuint getBuffer()					{ return mALBuffer; }

protected:
	ALuint mALBuffer;
};
