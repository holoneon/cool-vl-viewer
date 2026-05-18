/**
*  @file llvoicewebrtc.h
 * @brief Declaration of LLVoiceWebRTC class.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 *
 * Copyright (C) 2023-2024, Linden Research, Inc.
 * Copyright (C) 2024, Henri Beauchamp.
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

#include <functional>
#include <list>
#include <memory>
#include <utility>

#include "hbfastmap.h"

#include "llvoiceclient.h"

class LLVoiceConnection;

class LLPluginClassMedia;

class LLVoiceWebRTC  final : public LLVoiceModule
{
	friend class LLVoiceClient;

protected:
	LOG_CLASS(LLVoiceWebRTC);

public:
	LLVoiceWebRTC();
	~LLVoiceWebRTC();

	void init();
	void terminate();
	void updateSettings();

	LL_INLINE bool isTerminated() const					{ return mTerminated; }

	bool isVoiceWorking() const;

	// LLVoiceModule overrides
	const std::string& getName() const override;
	// Returns true if the user is currently in a proximal (local spatial)
	// channel. Note that gestures should only fire if this returns true.
	LL_INLINE bool inProximalChannel() const override
	{
		return inSpatialChannel();
	}

	void setSpatialChannel(const LLSD&channel_info) override;
	// Note: this is synonymous to startAdHocSession() in LL's sources. HB
	void setNonSpatialChannel(const LLSD& channel_info,
							  bool notify_on_first_join,
							  bool hangup_on_last_leave) override;
	void leaveNonSpatialChannel() override;
	void leaveAudioSession() override;
	LL_INLINE void processChannels(bool b) override		{ mProcessChannels = b; }

#if 0	// Not used in the Cool VL Viewer
	void setHidden(bool hidden) override;
#endif

	LL_INLINE LLPluginClassMedia* getPlugin()			{ return mWebRTCPlugin; }

	static void idle(void* userdatap);

	void setCaptureDevice(const std::string& device_id);
	void setRenderDevice(const std::string& device_id);

	LL_INLINE const strings_map_t& getCaptureDevices() const
	{
		return mCaptureDevices;
	}

	LL_INLINE const strings_map_t& getRenderDevices() const
	{
		return mRenderDevices;
	}

	void setTuningMode(bool tuning_on);
	LL_INLINE bool inTuningMode()					{ return mIsInTuningMode; }

	void tuningSetMicVolume(F32 volume);
#if 0	// Not used
	LL_INLINE void tuningSetSpeakerVolume(F32 v)	{ mTuningSpeakerVolume = v; }
#endif
	LL_INLINE F32 tuningGetEnergy()					{ return getAudioLevel(); }

	LL_INLINE bool isCaptureNoDevice() const		{ return mNoCaptureDevice; }
	LL_INLINE bool isRenderNoDevice() const			{ return mNoRenderDevice; }

	LL_INLINE bool deviceSettingsAvailable() const
	{
		return mDeviceSettingsAvailable;
	}

	void refreshDeviceLists(bool clear_current_list);

	void leaveChannel(bool stop_talking = false);

	bool isCurrentChannel(const LLSD& channel_info);
	bool compareChannels(const LLSD& channel_info1, const LLSD& channel_info2);

	void setVoiceVolume(F32 volume);
	// Note: passing a negative 'gain' reasserts last mMicGain value. HB
	void setMicGain(F32 gain);

	// Returns -1.f when 'id' does not correspond to a participant
	F32 getUserVolume(const LLUUID& id);
	// Sets volume for specified agent, from 0-1 (where .5 is nominal)
	void setUserVolume(const LLUUID& id, F32 volume);

	void setEarLocation(S32 loc);

	/////////////////////////////
	// Accessors for data related to nearby speakers

	bool getIsSpeaking(const LLUUID& id);
	// true if we have received data for this avatar.
	bool getVoiceEnabled(const LLUUID& id);

	// "power" is related to "amplitude" in a defined way. I am just not sure
	// what the formula is...
	// Note: changed to return -1.f when 'id' is not in session. HB
	F32 getCurrentPower(const LLUUID& id);

	bool getIsModeratorMuted(const LLUUID& id);

	class participantState
	{
	public:
		LL_INLINE participantState(const LLUUID& av_id,
								   const LLUUID& region_id)
		:	mAvatarID(av_id),
			mRegion(region_id),
			mURI(av_id.asString()),
			mLevel(0.f),
			mVolume(0.5f),
			mIsSpeaking(false),
			mIsModeratorMuted(false),
			mIsMuted(false)
		{
		}

	public:
		LLUUID			mAvatarID;
		LLUUID			mRegion;
		std::string		mURI;
		std::string		mLegacyName;
		// The current audio level of the participant
		F32				mLevel;
		// The gain applied to the participant
		F32				mVolume;
		bool			mIsSpeaking;
		bool			mIsModeratorMuted;
		bool			mIsMuted;
	};
	typedef std::shared_ptr<participantState> pstate_ptr_t;
	typedef fast_hmap<LLUUID, pstate_ptr_t> particip_map_t;
	particip_map_t* getParticipantList();

	pstate_ptr_t findParticipantByID(const std::string& channel_id,
									 const LLUUID& id);
	pstate_ptr_t addParticipantByID(const std::string& channel_id,
									const LLUUID& id, const LLUUID& region_id);
	void removeParticipantByID(const std::string& channel_id,
							   const LLUUID& id, const LLUUID& region_id);

	bool isParticipant(const LLUUID& id);

	bool isCallBackPossible(const LLUUID& session_id);

#if 0	// Not used in the Cool VL Viewer
	void addObserver(LLVoiceClientParticipantObserver* observerp);
	void removeObserver(LLVoiceClientParticipantObserver* observerp);
#endif

	void onConnectionEstablished(const std::string& channel_id,
								 const LLUUID& region_id);
	void onConnectionShutDown(const std::string& channel_id,
							  const LLUUID& region_id);
	void onConnectionFailure(const std::string& channel_id,
							 const LLUUID& region_id,
							 LLVoiceClientStatusObserver::EStatusType status =
								LLVoiceClientStatusObserver::ERROR_UNKNOWN);
	void updatePosition();
	void sendPositionUpdate(bool force);
	void updateOwnVolume();

	void addObserver(LLVoiceClientStatusObserver* observerp);
	void removeObserver(LLVoiceClientStatusObserver* observerp);

	// Used in mute list observer
	void muteListChanged();

private:
	class sessionState
	{
	protected:
		LOG_CLASS(LLVoiceWebRTC::sessionState);

		sessionState();

	public:
		virtual ~sessionState();

		virtual bool isSpatial() = 0;
		virtual bool isEstate() = 0;
		virtual bool isCallbackPossible() = 0;
		virtual bool processConnectionStates();
		virtual void sendData(const std::string& data);
		
		typedef std::shared_ptr<sessionState> ptr_t;
		typedef std::weak_ptr<sessionState> wptr_t;
		typedef std::function<void(const ptr_t &)> func_t;

		pstate_ptr_t addParticipant(const LLUUID& av_id,
									const LLUUID& region_id);
		void removeParticipant(pstate_ptr_t participantp);
		void removeAllParticipants(const LLUUID& region_id);

		pstate_ptr_t findParticipantByID(const LLUUID& id);

		void shutdownAllConnections();
		void reconnectAllSessions();

#if 0	// Reviving a session does not properly work: do not allow it ! HB
		LL_INLINE void revive()							{ mShuttingDown = false; }
#endif

		bool isEmpty();

		void setMuteMic(bool muted);
		void setMicGain(F32 volume);
		void setSpeakerVolume(F32 volume);
		void setUserVolume(const LLUUID& id, F32 volume);
		void setUserMute(const LLUUID& id, bool mute);

		void muteListChanged();

		static void addSession(const std::string& channel_id, ptr_t& sessionp);
		static bool hasSession(const std::string& session_id);
		static ptr_t matchSessionByChannelID(const std::string& channel_id);
		static void processSessionStates();
		static void reapEmptySessions();
		static void forEach(func_t fn);

	private:
		static void forEachPredicate(const std::pair<std::string, wptr_t>& a,
									 func_t fn);

	protected:
		std::list<std::shared_ptr<LLVoiceConnection> > mConnections;

	private:
		// Canonical list of outstanding sessions.
		typedef fast_hmap<std::string, ptr_t> map_t;
		static map_t		sSessions;

	public:
		std::string			mHandle;
		std::string			mChannelID;
		std::string			mName;

		particip_map_t		mParticipantsByUUID;

		F32					mSpeakerVolume;

		bool				mMuted;
		bool				mHangupOnLastLeave;
		bool				mNotifyOnFirstJoin;
		bool				mShuttingDown;
	};

	class estateSessionState final : public sessionState
	{
	protected:
		LOG_CLASS(LLVoiceWebRTC::estateSessionState);

	public:
		estateSessionState();

		bool processConnectionStates() override;

		LL_INLINE bool isSpatial() override 			{ return true; }
		LL_INLINE bool isEstate() override				{ return true; }
		LL_INLINE bool isCallbackPossible() override	{ return false; }
	};

	class parcelSessionState final : public sessionState
	{
	protected:
		LOG_CLASS(LLVoiceWebRTC::estateSessionState);

	public:
		parcelSessionState(const std::string& channel_id, S32 parcel_id);

		LL_INLINE bool isSpatial() override 			{ return true; }
		LL_INLINE bool isEstate() override				{ return false; }
		LL_INLINE bool isCallbackPossible() override	{ return false; }
	};

	class adhocSessionState final : public sessionState
	{
	protected:
		LOG_CLASS(LLVoiceWebRTC::adhocSessionState);

	public:
		adhocSessionState(const std::string& channel_id,
						  const std::string& credentials,
						  bool notify_on_first_join,
						  bool hangup_on_last_leave);

		LL_INLINE bool isSpatial() override 			{ return true; }
		LL_INLINE bool isEstate() override				{ return false; }

		LL_INLINE bool isCallbackPossible() override
		{
			return mNotifyOnFirstJoin && mHangupOnLastLeave;
		}

	protected:
		std::string mCredentials;
	};

	sessionState::ptr_t findP2PSession(const LLUUID& id);

	sessionState::ptr_t addSession(const std::string& channel_id,
								   sessionState::ptr_t session);

	void deleteSession(const sessionState::ptr_t& sessionp);

	void setVoiceEnabled(bool enabled);

	void setMuteMic(bool muted);

#if 0	// Not used in the Cool VL Viewer
	void setHidden(bool hidden) override;
#endif

	void addCaptureDevice(const std::string& display_name,
						  const std::string& device_id);
	void addRenderDevice(const std::string& display_name,
						 const std::string& device_id);

	void processChannels();

	void cleanUp();

	void lookupName(const LLUUID& id);
	static void onAvatarNameLookup(const LLUUID& id,
								   const std::string& fullname, bool);
	void avatarNameResolved(const LLUUID& id, const std::string& name);
	static void predAvatarName(const sessionState::ptr_t& sessionp,
							   const LLUUID& id, const std::string& name);
	/////////////////////////////
	// Sending updates of current state

	void setListenerPosition(const LLVector3d& position,
							 const LLVector3& velocity,
							 const LLQuaternion& rot);
	void setAvatarPosition(const LLVector3d& position,
						   const LLVector3& velocity,
						   const LLQuaternion& rot);
	void startEstateSession();
	void startParcelSession(const std::string& channel_id, S32 parcel_id);

	bool inSpatialChannel() const;
	bool inOrJoiningChannel(const std::string& channel_id);
	bool inEstateChannel();

	LLSD getAudioSessionChannelInfo();

	void enforceTether();

	void updateNeighboringRegions();

	LL_INLINE const uuid_list_t& getNeighboringRegions() const
	{
		return mNeighboringRegions;
	}

#if 0	// Not used in the Cool VL Viewer
	void notifyParticipantObservers();
#endif
	void notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status);

	F32 getAudioLevel();

	bool launchPlugin();
	void killPlugin();
	void checkDevicesChanged();

	static void predSendData(const sessionState::ptr_t& sessionp,
							 const std::string& spatial_data);
	static void predUpdateOwnVolume(const sessionState::ptr_t& sessionp,
									F32 audio_level);
	static void predSetMuteMic(const sessionState::ptr_t& sessionp, bool mute);
	static void predSetSpeakerVolume(const sessionState::ptr_t& sessionp,
									 F32 volume);
	static void predShutdownSession(const sessionState::ptr_t& sessionp);
	static void predReconnect(const sessionState::ptr_t& sessionp);
	static void predSetUserVolume(const sessionState::ptr_t& sessionp,
								  const LLUUID& id, F32 volume);
	static void predMuteListChanged(const sessionState::ptr_t& sessionp);

private:
	LLPluginClassMedia*		mWebRTCPlugin;

	// Session state for the current audio session
	sessionState::ptr_t					mSession;
	// Session state for the audio session we are trying to join
	sessionState::ptr_t					mNextSession;

	enum
	{
		earLocCamera = 0,	// Ear at camera
		earLocAvatar,		// Ear at avatar
		earLocMixed			// Ear at avatar location/camera direction
	};
	S32						mEarLocation;

	F32						mSpeakerVolume;
	F32						mMicGain;
	F32						mTuningMicGain;
	S32						mTuningSpeakerVolume;

	std::string				mSpatialSessionCredentials;

	std::string				mCaptureDevice;
	std::string				mRenderDevice;
	strings_map_t			mCaptureDevices;
	strings_map_t			mRenderDevices;

	LLVector3d				mListenerPosition;
	LLVector3d				mListenerRequestedPosition;
	LLVector3				mListenerVelocity;
	LLQuaternion			mListenerRot;

	LLVector3d				mAvatarPosition;
	LLVector3				mAvatarVelocity;
	LLQuaternion			mAvatarRot;

	uuid_list_t				mNeighboringRegions;

	typedef std::set<LLVoiceClientStatusObserver*> status_observer_set_t;
	status_observer_set_t	mStatusObservers;

	bool					mTerminated;
	bool					mInitDone;

	bool					mVoiceEnabled;
	bool					mProcessChannels;
	bool					mIsProcessingChannels;
	bool					mPluginNeedsConfig;
	bool					mPluginHasDied;

	bool					mDeviceSettingsAvailable;
	bool					mNoCaptureDevice;
	bool					mNoRenderDevice;
	bool					mIsInTuningMode;
	bool					mMuteMic;
#if 0	// mHidden is not used in the Cool VL Viewer
	bool					mHidden;
#endif

	bool					mSpatialCoordsDirty;
};

// Note: since this is a global class instance, all class members are always
// available, from viewer launch to _exit(): no need for static variables to
// track the shutdown (or ready) state !  HB
extern LLVoiceWebRTC gVoiceWebRTC;
