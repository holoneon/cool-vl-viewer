/**
*  @file llvoiceclient.h
 * @brief Declaration of LLVoiceClient class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
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

#include <map>

#include "boost/signals2.hpp"

#include "llstring.h"
#include "lluuid.h"

class LLPumpIO;

constexpr F32 OVERDRIVEN_POWER_LEVEL = 0.7f;

class LLVoiceClientStatusObserver
{
public:
	typedef enum e_voice_status_type
	{
		// NOTE: when updating this enum, please also update the switch in
		// LLVoiceClientStatusObserver::status2string().
		STATUS_LOGIN_RETRY,
		STATUS_LOGGED_IN,
		STATUS_JOINING,
		STATUS_JOINED,
		STATUS_LEFT_CHANNEL,
		STATUS_VOICE_DISABLED,
		STATUS_VOICE_ENABLED,
		BEGIN_ERROR_STATUS,
		ERROR_CHANNEL_FULL,
		ERROR_CHANNEL_LOCKED,
		ERROR_NOT_AVAILABLE,
		ERROR_UNKNOWN
	} EStatusType;

	virtual ~LLVoiceClientStatusObserver() = default;
	virtual void onChange(EStatusType status, const LLSD& channel_info,
						  bool proximal) = 0;

	static std::string status2string(EStatusType inStatus);
};

// Purely virtual class, used *only* for the few methods which are both common
// to all voice modules *and* get called by LLVoiceClient via module pointers:
// this is a *minimal* common API to avoid pointlessly virtual methods and
// their costlier calls. HB
class LLVoiceModule
{
	friend class LLVoiceClient;

public:
	LLVoiceModule() = default;
	virtual ~LLVoiceModule() = default;

	// Module identification
	virtual const std::string& getName() const = 0;

	// Channels related methods
	virtual bool inProximalChannel() const = 0;
	virtual void setSpatialChannel(const LLSD& channel_info) = 0;
	virtual void setNonSpatialChannel(const LLSD& channel_info,
									  bool notify_on_first_join,
									  bool hangup_on_last_leave) = 0;
	virtual void leaveNonSpatialChannel() = 0;
	virtual void leaveAudioSession() = 0;
	virtual void processChannels(bool enabled) = 0;

#if 0	// Not yet used by the Cool VL Viewer. *TODO: implement ?  HB
	virtual void setHidden(bool b) const = 0;

protected:
	bool mHidden = false;
#endif
};

class LLVoiceClient
{
	friend class LLViewerRequiredVoiceVersion;

protected:
	LOG_CLASS(LLVoiceClient);

public:
	LLVoiceClient();
	~LLVoiceClient();

	LL_INLINE bool ready() 							{ return mReady; }

	// Call after loading settings and whenever they change
	void updateSettings();

	// Methods used in llfloatervoicedevicesettings.cpp

	void setCaptureDevice(const std::string& device_id);
	void setRenderDevice(const std::string& device_id);

	const strings_map_t& getCaptureDevices() const;
	const strings_map_t& getRenderDevices() const;

	void tuningStart();
	void tuningStop();
	bool inTuningMode();
	bool tuningModeActive();

	void tuningSetMicVolume(F32 volume);
#if 0	// Not used
	void tuningSetSpeakerVolume(F32 volume);
#endif
	F32 tuningGetEnergy();

	// This returns true when it is safe to bring up the "device settings"
	// dialog in the prefs. I.e. when the daemon is running and connected, and
	// the device lists are populated.
	bool deviceSettingsAvailable();

	// Requery the voice engine for the current list of input/output devices.
	// If you pass true for clear_current_list, deviceSettingsAvailable() will
	// be false until the query has completed (use this if you want to know
	// when it is done). If you pass false, you will have no way to know when
	// the query finishes, but the device lists will not appear empty in the
	// interim.
	void refreshDeviceLists(bool clear_current_list);

	/////////////////////////////
	// Sending updates of current state

	// Use this to mute the local mic (for when the client is minimized, etc),
	// ignoring user PTT state. Used from llvieweraudio.cpp.
	void setMuteMic(bool muted);
	// Used from llvieweraudio.cpp
	void setVoiceVolume(F32 volume);
	void setMicGain(F32 volume);

	// Used to track moderation by channel.
	void setMutedInfo(const std::string& channel_id, bool muted);

	// Used from llfloateractivespeakers.cpp and llfloaterim.cpp

	F32 getUserVolume(const LLUUID& id);
	// Sets volume for specified agent, from 0-1 (where .5 is nominal)
	void setUserVolume(const LLUUID& id, F32 volume);

	// Used from llvoiceremotectrl.cpp
	void setUserPTTState(bool ptt);
	LL_INLINE bool getUserPTTState() const			{ return mUserPTTState; }
	LL_INLINE void toggleUserPTTState()				{ setUserPTTState(!mUserPTTState); }

	bool isAgentMicOpen() const;
	void setUsePTT(bool use_it);
	void setPTTIsToggle(bool set_as_toggle);
	LL_INLINE bool getPTTIsToggle() const			{ return mPTTIsToggle; }
	bool setPTTKey(const std::string& key);
	void setEarLocation(S32 loc);

	// PTT key triggering. Used from llviewerwindow.cpp
	void keyDown(KEY key, MASK mask);
	void keyUp(KEY key, MASK mask);
	void inputUserControlState(bool down);
	void middleMouseState(bool down);

	/////////////////////////////
	// Accessors for data related to nearby speakers

	// Used from llfloateractivespeakers.cpp and llvoavatar.cpp

	bool getIsSpeaking(const LLUUID& id);
	// true if we have received data for this avatar.
	bool getVoiceEnabled(const LLUUID& id);

	// "power" is related to "amplitude" in a defined way. I'm just not sure
	// what the formula is...
	F32 getCurrentPower(const LLUUID& id);

	// Used from llfloateractivespeakers.cpp
	bool getIsModeratorMuted(const LLUUID& id);
	bool getOnMuteList(const LLUUID& id);

	// This is used by the string-keyed maps below, to avoid storing the string
	// twice. The 'const std::string*' in the key points to a string actually
	// stored in the object referenced by the map. The add & delete operations
	// for each map allocate and delete in the right order to avoid dangling
	// references. The default compare operation would just compare pointers,
	// which is incorrect, so they must use this comparator instead.
	struct stringMapComparator
	{
		LL_INLINE bool operator()(const std::string* a,
								  const std::string* b) const
		{
			return a->compare(*b) < 0;
		}
	};

	struct uuidMapComparator
	{
		LL_INLINE bool operator()(const LLUUID* a, const LLUUID* b) const
		{
			return *a < *b;
		}
	};

	// Used in llfloateractivespeakers.cpp
	struct ParticipantData
	{
		LL_INLINE ParticipantData(const LLUUID& id, const std::string& name,
								  bool avatar)
		:	mId(id),
			mName(name),
			mIsAvatar(avatar)
		{
		}

		LLUUID		mId;
		std::string	mName;
		bool		mIsAvatar;
	};
	typedef std::vector<ParticipantData> participants_vec_t;
	bool getParticipants(participants_vec_t& participants);

	void setVoiceEnabled(bool enabled);

	// Used by llvoicechannel.cpp

	void addObserver(LLVoiceClientStatusObserver* observerp);
	void removeObserver(LLVoiceClientStatusObserver* observerp);

	void setNonSpatialChannel(const LLSD& channel_info,
							  bool notify_on_first_join,
							  bool hangup_on_last_leave);
	void leaveNonSpatialChannel();
	void activateSpatialChannel(bool activate);

	bool isCurrentChannel(const LLSD& channel_info);
	bool compareChannels(const LLSD& channel_info1, const LLSD& channel_info2);

	// Used by LLViewerParcelVoiceInfo
	void setSpatialChannel(const LLSD& channel_info);

	// Called from llfloateractivespeakers.cpp, llvoavatar.cpp and
	// llvoicechannel.cpp
	// Returns true iff the user is currently in a proximal (local spatial)
	// channel. Note that gestures should only fire if this returns true.
	bool inProximalChannel() const;

	// Called once from llappviewer.cpp at application startup (creates
	// the connector)
	void init(LLPumpIO* pumpp);
	// Called from llappviewer.cpp to clean up during shutdown
	void terminate();

	// Called from LLAgent::handleServerFeaturesTransition(). HB
	void handleSimFeaturesReceived(const LLSD& features);

	bool isVoiceWorking();

	// Called from various places in the viewer
	static bool voiceEnabled();

	// Used by llimmgr.cpp, so that we do not need to deal with module pointers
	// there... HB
	enum e_server_type : U32
	{
		UNKNOWN_SERVER,
		WEBRTC_SERVER,
	};
	// Returns the type of server as defined above. HB
	U32 getVoiceServerType(const LLSD& channel_info) const;

	// Called from llstartup.cpp
	void onParcelChange();

private:
	LLVoiceModule* getModuleFromType(const std::string& server_type);
	LLVoiceModule* getModuleFromChannelInfo(const LLSD& channel_info);

	static LLVoiceModule* getModuleFromSimFeatures(const LLSD& features);
	void setSpatialVoiceModule(LLVoiceModule* modulep);
	void setNonSpatialVoiceModule(LLVoiceModule* modulep);

	void updateMicMuteLogic();

private:
	// Used to store spatial credentials for Vivox so they are available when
	// the region voice server is retrieved.
	LLSD			mSpatialCredentials;

	LLVoiceModule*	mSpatialVoiceModulep;
	LLVoiceModule*	mNonSpatialVoiceModulep;

	typedef boost::signals2::connection connection_t;
	connection_t	mParcelChangedConnection;

	typedef std::map<std::string, bool> mute_map_t;
	mute_map_t		mChannelMuteMap;

	KEY				mPTTKey;
	bool			mUsePTT;
	bool			mPTTIsMiddleMouse;
	bool			mPTTIsToggle;
	bool			mUserPTTState;

	bool			mMuteMic;

	bool			mReady;
};

// Note: since this is a global class instance, all class members are always
// available, from viewer launch to _exit(): no need for static variables to
// track the shutdown (or ready) state !  HB
extern LLVoiceClient gVoiceClient;
