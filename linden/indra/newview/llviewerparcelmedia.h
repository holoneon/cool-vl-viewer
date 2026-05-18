/**
 * @file llviewerparcelmedia.h
 * @brief Handlers for multimedia on a per-parcel basis
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llviewermedia.h"

class LLMessageSystem;
class LLParcel;
class LLViewerParcelMediaNavigationObserver;

// This class understands land parcels, network traffic, LSL media transport
// commands, and talks to the LLViewerMedia class to actually do playback.

class LLViewerParcelMedia : public LLViewerMediaObserver
{
protected:
	LOG_CLASS(LLViewerParcelMedia);

public:
	static void initClass();
	static void cleanupClass();

	// Called when the agent's parcel has a new URL, or the agent has walked on
	// to a new parcel with media.
	static void update(LLParcel* parcel);

	// play the parcel music stream
	static void playStreamingMusic(LLParcel* parcel, bool filter = true);

	// User clicked play button in music controls
	// Can be used as a callback in menus
	static void playMusic(void* userdata = NULL);

	// User clicked pause button in music controls
	// Can be used as a callback in menus
	static void pauseMusic(void* userdata = NULL);

	// stop the parcel music stream
	static void stopStreamingMusic();

	// User clicked stop button in music controls
	// Can be used as a callback in menus
	static void stopMusic(void* userdata = NULL);

	static bool parcelMusicPlaying()	{ return sParcelMusicState == PLAYING; }
	static bool parcelMusicPaused()		{ return sParcelMusicState == PAUSED; }
	static bool parcelMusicStopped()	{ return sParcelMusicState == STOPPED; }

	// play the parcel media stream
	static void playMedia(LLParcel* parcel, bool filter = true);

	// User clicked play button in media controls
	// Can be used as a callback in menus
	static void play(void* userdata = NULL);

	// User clicked stop button in media controls
	// Can be used as a callback in menus
	static void stop(void* userdata = NULL);

	// user clicked pause button in media controls
	// Can be used as a callback in menus
	static void pause(void* userdata = NULL);

	// restart after pause - no need for all the setup
	static void start();

	static void focus(bool focus);

    // Jump to timecode time
	static void seek(F32 time);

	static LLViewerMediaImpl::EMediaStatus getStatus();
	static std::string getMimeType();
	static std::string getURL();
	static std::string getName();
	static std::string getParcelAudioURL();

	// These are just helper functions for the convenience of others working
	// with media
	LL_INLINE static viewer_media_t getParcelMedia()	{ return sMediaImpl; }
	LL_INLINE static bool hasParcelMedia()				{ return !getURL().empty(); }
	LL_INLINE static bool hasParcelAudio()				{ return !getParcelAudioURL().empty(); }
	LL_INLINE static bool isParcelMediaPlaying()		{ return sMediaImpl.notNull() &&
																 !getURL().empty() &&
																 sMediaImpl->hasMedia(); }
	static bool isParcelAudioPlaying()					{ return gAudiop && !getParcelAudioURL().empty() &&
																 gAudiop->isInternetStreamPlaying(); }


	static void processParcelMediaCommandMessage(LLMessageSystem* msg, void**);
	static void processParcelMediaUpdate(LLMessageSystem* msg, void**);
	static void sendMediaNavigateMessage(const std::string& url);

	// inherited from LLViewerMediaObserver
	virtual void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	static void registerStreamingAudioPlugin();

public:
	static S32 sMediaParcelLocalID;
	static LLUUID sMediaRegionID;
	// HACK: this will change with Media on a Prim
	static viewer_media_t sMediaImpl;

private:
	enum { STOPPED = 0, PLAYING = 1, PAUSED = 2 };
	static S32 sParcelMusicState;
};

class LLViewerParcelMediaNavigationObserver
{
public:
	std::string mCurrentURL;
	bool mFromMessage;

	//void onNavigateComplete(const EventType& event_in);
};
