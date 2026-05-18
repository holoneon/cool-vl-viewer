 /**
 * @file llvoiceclient.cpp
 * @brief Implementation of LLVoiceClient class.
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

#include "llviewerprecompiledheaders.h"

#include "llvoiceclient.h"

#include "llhttpnode.h"
#include "llkeyboard.h"
#include "llsdutil.h"

#include "llagent.h"
#include "llmutelist.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llvoicewebrtc.h"

// Global
LLVoiceClient gVoiceClient;

///////////////////////////////////////////////////////////////////////////////
// LLVoiceClientStatusObserver class
///////////////////////////////////////////////////////////////////////////////

std::string LLVoiceClientStatusObserver::status2string(LLVoiceClientStatusObserver::EStatusType status)
{
	std::string result = "UNKNOWN";

	// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break

	switch (status)
	{
		CASE(STATUS_LOGIN_RETRY);
		CASE(STATUS_LOGGED_IN);
		CASE(STATUS_JOINING);
		CASE(STATUS_JOINED);
		CASE(STATUS_LEFT_CHANNEL);
		CASE(STATUS_VOICE_DISABLED);
		CASE(STATUS_VOICE_ENABLED);
		CASE(BEGIN_ERROR_STATUS);
		CASE(ERROR_CHANNEL_FULL);
		CASE(ERROR_CHANNEL_LOCKED);
		CASE(ERROR_NOT_AVAILABLE);
		CASE(ERROR_UNKNOWN);

		default:
			break;
	}

#undef CASE

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceClient class
///////////////////////////////////////////////////////////////////////////////

LLVoiceClient::LLVoiceClient()
:	mSpatialVoiceModulep(NULL),
	mNonSpatialVoiceModulep(NULL),
	mReady(false),
	mUserPTTState(false),
	mUsePTT(true),
	mPTTIsToggle(false),
	mMuteMic(false)
{
}

LLVoiceClient::~LLVoiceClient()
{
	terminate();	// Just in case it was forgotten... HB
}

void LLVoiceClient::init(LLPumpIO* pumpp)
{
	if (mReady)
	{
		return;
	}
	mReady = true;

	gVoiceWebRTC.init();
#if 0	// Do not do this here: wait for full login before enabling voice, else
		// race conditions would happen in the WebRTC connection process and
		// would result in failures to bring up voice on login. Instead, an
		// explicit call to LLVoiceClient::updateSettings() is now performed
		// from llstartup.cpp on STATE_CLEANUP stage. HB
	updateSettings();
#endif
	// Register or parcel manager observer for agent pracle changes. HB
	mParcelChangedConnection =
		gViewerParcelMgr.addAgentParcelChangedCB(std::bind(&LLVoiceClient::onParcelChange,
														   this));
}

void LLVoiceClient::terminate()
{
	if (mReady)
	{
		if (mParcelChangedConnection.connected())
		{
			mParcelChangedConnection.disconnect();
		}
		gVoiceWebRTC.terminate();
		mReady = false;
	}
}

void LLVoiceClient::updateSettings()
{
	setVoiceEnabled(gSavedSettings.getBool("EnableVoiceChat"));
	setUsePTT(gSavedSettings.getBool("PTTCurrentlyEnabled"));
	if (!setPTTKey(gSavedSettings.getString("PushToTalkButton")))
	{
		llwarns << "Invalid push-to-talk key: trigger set to none" << llendl;
	}
	setPTTIsToggle(gSavedSettings.getBool("PushToTalkToggle"));
	setEarLocation(gSavedSettings.getS32("VoiceEarLocation"));
	setMicGain(gSavedSettings.getF32("AudioLevelMic"));
	setCaptureDevice(gSavedSettings.getString("VoiceWebRTCInputAudioDevice"));
	setRenderDevice(gSavedSettings.getString("VoiceWebRTCOutputAudioDevice"));
	gVoiceWebRTC.updateSettings();
	updateMicMuteLogic();
}

LLVoiceModule* LLVoiceClient::getModuleFromType(const std::string& server_type)
{
	if (server_type == "webrtc")
	{
		return &gVoiceWebRTC;
	}
	return NULL;
}

LLVoiceModule* LLVoiceClient::getModuleFromChannelInfo(const LLSD& info)
{
	if (info.has("voice_server_type"))
	{
		return getModuleFromType(info["voice_server_type"].asString());
	}
	return NULL;
}

U32 LLVoiceClient::getVoiceServerType(const LLSD& channel_info) const
{
	std::string type;
	if (channel_info.has("voice_server_type"))
	{
		type = channel_info["voice_server_type"].asString();
	}
	return type == "webrtc" ? WEBRTC_SERVER : UNKNOWN_SERVER;
}

//static
LLVoiceModule* LLVoiceClient::getModuleFromSimFeatures(const LLSD& features)
{
	LLVoiceModule* modulep = NULL;
	if (features.has("VoiceServerType"))
	{
		std::string type_str = features["VoiceServerType"].asString();
		if (type_str == "webrtc")
		{
			modulep = &gVoiceWebRTC;
		}
	}
	return modulep;
}

void LLVoiceClient::setSpatialVoiceModule(LLVoiceModule* modulep)
{
	if (!modulep || modulep == mSpatialVoiceModulep)
	{
		return;
	}

	bool proximal_active = mSpatialVoiceModulep &&
						   mSpatialVoiceModulep->inProximalChannel();
	if (proximal_active)
	{
		mSpatialVoiceModulep->processChannels(false);
	}
	mSpatialVoiceModulep = modulep;
	if (proximal_active)
	{
		mSpatialVoiceModulep->processChannels(true);
	}
	LL_DEBUGS("Voice") << "Spatial voice module changed for: "
					   << mSpatialVoiceModulep->getName() << LL_ENDL;
}

void LLVoiceClient::setNonSpatialVoiceModule(LLVoiceModule* modulep)
{
	mNonSpatialVoiceModulep = modulep;
	// If do not have a non-spatial module, revert to spatial when possible.
	if (!mNonSpatialVoiceModulep && mSpatialVoiceModulep)
	{
		mSpatialVoiceModulep->processChannels(true);
	}
}

void LLVoiceClient::handleSimFeaturesReceived(const LLSD& features)
{
	LL_DEBUGS("Voice") << "Processing simulator features for agent region"
					   << LL_ENDL;
	LLVoiceModule* modulep = getModuleFromSimFeatures(features);
	if (!modulep)
	{
		return;	// Unknown server type: nothing to do !
	}
	if (mSpatialVoiceModulep && !mNonSpatialVoiceModulep)
	{
		// Stop processing if we are going to change voice clients and we are
		// not currently in non-spatial.
		if (mSpatialVoiceModulep != modulep)
		{
			llinfos << "Disabling " << mSpatialVoiceModulep->getName()
					<< " voice processing." << llendl;
			mSpatialVoiceModulep->processChannels(false);
		}
	}
	// Switch to spatial voice to the new module if needed.
	setSpatialVoiceModule(modulep);
		
	// If we should be in spatial voice, switch to it and set the credentials
	if (mSpatialVoiceModulep && !mNonSpatialVoiceModulep)
	{
		LL_DEBUGS("Voice") << "Using spatial voice module." << LL_ENDL;
		if (!mSpatialCredentials.isUndefined())
		{
			mSpatialVoiceModulep->setSpatialChannel(mSpatialCredentials);
		}
		mSpatialVoiceModulep->processChannels(true);
	}
}

void LLVoiceClient::onParcelChange()
{
	LL_DEBUGS("Voice") << "Parcel change detected." << LL_ENDL;
	if (!mSpatialVoiceModulep || mNonSpatialVoiceModulep)
	{
		// Not in parcel/estate voice channel currently. HB
		return;
	}
	// Check for parcel voice permissions and act accordingly. HB
	if (gViewerParcelMgr.allowAgentVoice())
	{
		LL_DEBUGS("Voice") << "Parcel voice allowed." << LL_ENDL;
		mSpatialVoiceModulep->processChannels(true);
		setSpatialChannel(mSpatialCredentials);
		return;
	}
	// Parcel flag does not permit voice: make sure the spatial channel will
	// be shut down and will stay as such. HB
	if (mSpatialVoiceModulep && mSpatialVoiceModulep->inProximalChannel())
	{
		LL_DEBUGS("Voice") << "Parcel voice disabled. Switching off spatial voice."
						   << LL_ENDL;
		mSpatialVoiceModulep->processChannels(false);
	}
}

void LLVoiceClient::setVoiceEnabled(bool enabled)
{
	gVoiceWebRTC.setVoiceEnabled(enabled);
}

void LLVoiceClient::setCaptureDevice(const std::string& device_id)
{
	gVoiceWebRTC.setCaptureDevice(device_id);
}

void LLVoiceClient::setRenderDevice(const std::string& device_id)
{
	gVoiceWebRTC.setRenderDevice(device_id);
}

const strings_map_t& LLVoiceClient::getCaptureDevices() const
{
	return gVoiceWebRTC.getCaptureDevices();
}

const strings_map_t& LLVoiceClient::getRenderDevices() const
{
	return gVoiceWebRTC.getRenderDevices();
}

void LLVoiceClient::tuningStart()
{
	gVoiceWebRTC.setTuningMode(true);
}

void LLVoiceClient::tuningStop()
{
	gVoiceWebRTC.setTuningMode(false);
}

bool LLVoiceClient::inTuningMode()
{
	return gVoiceWebRTC.inTuningMode();
}

bool LLVoiceClient::tuningModeActive()
{
	return gVoiceWebRTC.inTuningMode();
}

void LLVoiceClient::tuningSetMicVolume(F32 volume)
{
	gVoiceWebRTC.tuningSetMicVolume(volume);
}

#if 0	// Not used
void LLVoiceClient::tuningSetSpeakerVolume(F32 volume)
{
	gVoiceWebRTC.tuningSetSpeakerVolume(volume);
}
#endif

F32 LLVoiceClient::tuningGetEnergy()
{
	return gVoiceWebRTC.tuningGetEnergy();
}

bool LLVoiceClient::deviceSettingsAvailable()
{
	return gVoiceWebRTC.deviceSettingsAvailable();
}

void LLVoiceClient::refreshDeviceLists(bool clear_current_list)
{
	gVoiceWebRTC.refreshDeviceLists(clear_current_list);
}

bool LLVoiceClient::getParticipants(participants_vec_t& participants)
{
	participants.clear();
	bool result = false;

	LLVoiceWebRTC::particip_map_t* list2p = gVoiceWebRTC.getParticipantList();
	if (list2p)
	{
		result = true;
		for (LLVoiceWebRTC::particip_map_t::const_iterator it = list2p->begin(),
														   end = list2p->end();
			 it != end; ++it)
		{
			LLVoiceWebRTC::pstate_ptr_t participantp = it->second;
			participants.emplace_back(participantp->mAvatarID,
									  participantp->mLegacyName, true);
		}
	}

	return result;
}

void LLVoiceClient::addObserver(LLVoiceClientStatusObserver* observerp)
{
	gVoiceWebRTC.addObserver(observerp);
}

void LLVoiceClient::removeObserver(LLVoiceClientStatusObserver* observerp)
{
	gVoiceWebRTC.removeObserver(observerp);
}

void LLVoiceClient::setSpatialChannel(const LLSD& channel_info)
{
	mSpatialCredentials = channel_info;

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || !regionp->getFeaturesReceived())
	{
		return;
	}

	const LLSD& features = regionp->getSimulatorFeatures();
	setSpatialVoiceModule(getModuleFromSimFeatures(features));

	if (mSpatialVoiceModulep)
	{
		mSpatialVoiceModulep->setSpatialChannel(channel_info);
	}
}

void LLVoiceClient::setNonSpatialChannel(const LLSD& channel_info,
										 bool notify_on_first_join,
										 bool hangup_on_last_leave)
{
	LLVoiceModule* modulep = getModuleFromChannelInfo(channel_info);
	setNonSpatialVoiceModule(modulep);

	if (mSpatialVoiceModulep &&
		mSpatialVoiceModulep != mNonSpatialVoiceModulep)
	{
		mSpatialVoiceModulep->processChannels(false);
	}
	if (mNonSpatialVoiceModulep)
	{
		mNonSpatialVoiceModulep->processChannels(true);
		mNonSpatialVoiceModulep->setNonSpatialChannel(channel_info,
													  notify_on_first_join,
													  hangup_on_last_leave);
	}
}

void LLVoiceClient::leaveNonSpatialChannel()
{
	if (mNonSpatialVoiceModulep)
	{
		mNonSpatialVoiceModulep->leaveNonSpatialChannel();
		mNonSpatialVoiceModulep->processChannels(false);
		mNonSpatialVoiceModulep = NULL;
	}
}

void LLVoiceClient::activateSpatialChannel(bool activate)
{
	if (mSpatialVoiceModulep)
	{
		if (activate && !gViewerParcelMgr.allowAgentVoice())
		{
			LL_DEBUGS("Voice") << "Not activating due to parcel no-voice flag"
							   << LL_ENDL;
			activate = false;
		}
		mSpatialVoiceModulep->processChannels(activate);
	}
}

bool LLVoiceClient::isCurrentChannel(const LLSD& channel_info)
{
	return gVoiceWebRTC.isCurrentChannel(channel_info);
}

bool LLVoiceClient::compareChannels(const LLSD& info1, const LLSD& info2)
{
	return gVoiceWebRTC.compareChannels(info1, info2);
}

bool LLVoiceClient::inProximalChannel() const
{
	return mSpatialVoiceModulep && mSpatialVoiceModulep->inProximalChannel();
}

bool LLVoiceClient::isVoiceWorking()
{
	return voiceEnabled() && gVoiceWebRTC.isVoiceWorking();
}

bool LLVoiceClient::voiceEnabled()
{
	static LLCachedControl<bool> enable_voice(gSavedSettings,
											  "EnableVoiceChat");
	static LLCachedControl<bool> disable_voice(gSavedSettings,
											   "CmdLineDisableVoice");
	return enable_voice && !disable_voice;
}

bool LLVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	return gVoiceWebRTC.isParticipant(id);
}

bool LLVoiceClient::getIsSpeaking(const LLUUID& id)
{
	return gVoiceWebRTC.getIsSpeaking(id);
}

bool LLVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	return gVoiceWebRTC.getIsModeratorMuted(id);
}

F32 LLVoiceClient::getCurrentPower(const LLUUID& id)
{
	return llmax(gVoiceWebRTC.getCurrentPower(id), 0.f);
}

void LLVoiceClient::setEarLocation(S32 loc)
{
	gVoiceWebRTC.setEarLocation(loc);
}

void LLVoiceClient::setVoiceVolume(F32 volume)
{
	gVoiceWebRTC.setVoiceVolume(volume);
}

void LLVoiceClient::setMicGain(F32 volume)
{
	gVoiceWebRTC.setMicGain(volume);
}

F32 LLVoiceClient::getUserVolume(const LLUUID& id)
{
	return llmax(gVoiceWebRTC.getUserVolume(id), 0.f);
}

void LLVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
	volume = llclamp(volume, 0.f, 1.f);
	gVoiceWebRTC.setUserVolume(id, volume);
}

bool LLVoiceClient::getOnMuteList(const LLUUID& id)
{
	return LLMuteList::isMuted(id, LLMute::flagVoiceChat);
}

// PTT related methods

bool LLVoiceClient::isAgentMicOpen() const
{
	// Not in push to talk mode, or push to talk is active means currently
	// talking.
	static LLCachedControl<bool> ptt_enabled(gSavedSettings,
											 "PTTCurrentlyEnabled");
	return mUserPTTState || !ptt_enabled;
}

void LLVoiceClient::updateMicMuteLogic()
{
	// If not configured to use PTT, the mic should be open (otherwise the user
	// would be unable to speak).
	bool new_mic_mute = false;
	if (mUsePTT)
	{
		new_mic_mute = !mUserPTTState;
	}
	if (mMuteMic)
	{
		// This always overrides any other PTT setting.
		new_mic_mute = true;
	}
	gVoiceWebRTC.setMuteMic(new_mic_mute);
}

void LLVoiceClient::setMuteMic(bool muted)
{
	if (mMuteMic != muted)
	{
		mMuteMic = muted;
		updateMicMuteLogic();
	}
}

void LLVoiceClient::setMutedInfo(const std::string& channel_id, bool muted)
{
	mute_map_t::iterator it = mChannelMuteMap.find(channel_id);
	if (it == mChannelMuteMap.end())
	{
		if (muted && inProximalChannel())
		{
			// Channel is new and being muted: notify
			gNotifications.add("NearbyVoiceMutedByModerator");
		}
		mChannelMuteMap[channel_id] = muted;
	}
	else if (it->second != muted)
	{
		// Channel mute status already known: notify about the change
		gNotifications.add(muted ? "NearbyVoiceMutedByModerator"
								 : "NearbyVoiceUnmutedByModerator");
		it->second = muted;
	}
	if (muted && mUserPTTState)
	{
		setUserPTTState(false);
	}
}

void LLVoiceClient::setUserPTTState(bool ptt)
{
	if (ptt)
	{
		// If nearby chat is muted by moderator, do not toggle PTT and notify
		// the user.
		if (!mUserPTTState && getIsModeratorMuted(gAgentID))
		{
			gNotifications.add("NearbyVoiceMutedByModerator");
			return;
		}
	}
	mUserPTTState = ptt;
	updateMicMuteLogic();
}

void LLVoiceClient::setUsePTT(bool use_it)
{
	if (use_it && !mUsePTT)
	{
		// When the user turns on PTT, reset the current state.
		mUserPTTState = false;
	}
	mUsePTT = use_it;
	updateMicMuteLogic();
}

void LLVoiceClient::setPTTIsToggle(bool set_as_toggle)
{
	if (!set_as_toggle && mPTTIsToggle)
	{
		// When the user turns off toggle, reset the current state.
		mUserPTTState = false;
	}
	mPTTIsToggle = set_as_toggle;
	updateMicMuteLogic();
}

bool LLVoiceClient::setPTTKey(const std::string& key)
{
	if (key == "MiddleMouse")
	{
		mPTTIsMiddleMouse = true;
		return true;
	}
	mPTTIsMiddleMouse = false;
	return LLKeyboard::keyFromString(key.c_str(), &mPTTKey);
}

void LLVoiceClient::keyDown(KEY key, MASK mask)
{
	if (!gKeyboardp || gKeyboardp->getKeyRepeated(key))
	{
		return;	// Ignore auto-repeat keys
	}

	if (!mPTTIsMiddleMouse)
	{
		if (mPTTIsToggle)
		{
			if (key == mPTTKey)
			{
				toggleUserPTTState();
			}
		}
		else if (mPTTKey != KEY_NONE)
		{
			setUserPTTState(gKeyboardp->getKeyDown(mPTTKey));
		}
	}
}

void LLVoiceClient::keyUp(KEY key, MASK mask)
{
	if (!mPTTIsMiddleMouse)
	{
		if (!mPTTIsToggle && (mPTTKey != KEY_NONE) && gKeyboardp)
		{
			setUserPTTState(gKeyboardp->getKeyDown(mPTTKey));
		}
	}
}

void LLVoiceClient::inputUserControlState(bool down)
{
	if (!mPTTIsToggle)
	{
		// Set open-mic state as an absolute
		setUserPTTState(down);
	}
	else if (down)	// Toggle open-mic state on 'down'
	{
		toggleUserPTTState();
	}
}

void LLVoiceClient::middleMouseState(bool down)
{
	if (mPTTIsMiddleMouse)
	{
		inputUserControlState(down);
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLViewerParcelVoiceInfo class
///////////////////////////////////////////////////////////////////////////////

class LLViewerParcelVoiceInfo final : public LLHTTPNode
{
	void post(LLHTTPNode::ResponsePtr response, const LLSD& context,
			  const LLSD& input) const override
	{
		// The parcel you are in has changed something about its voice
		// information. This is a misnomer, as it can also be when you are not
		// in a parcel at all. Should really be something like
		// LLViewerVoiceInfoChanged...
		if (input.has("body"))
		{
			const LLSD& body = input["body"];

			// body has "region_name" (str), "parcel_local_id"(int),
			// "voice_credentials" (map).

			// body["voice_credentials"] has "channel_uri" (str),
			// body["voice_credentials"] has "channel_credentials" (str)

			// If we really wanted to be extra careful, we would check the
			// supplied local parcel id to make sure it is for the same
			// parcel we believe we are in.
			if (body.has("voice_credentials"))
			{
				LL_DEBUGS("Voice") << "Got spatial voice channel info"
								   << LL_ENDL;
				gVoiceClient.setSpatialChannel(body["voice_credentials"]);
			}
		}
	}
};

LLHTTPRegistration<LLViewerParcelVoiceInfo>
    gHTTPRegistrationMessageParcelVoiceInfo("/message/ParcelVoiceInfo");

///////////////////////////////////////////////////////////////////////////////
// LLViewerRequiredVoiceVersion class
///////////////////////////////////////////////////////////////////////////////

class LLViewerRequiredVoiceVersion final : public LLHTTPNode
{
	void post(LLHTTPNode::ResponsePtr response, const LLSD& context,
			  const LLSD& input) const override
	{
		// You received this messsage (most likely on region cross or teleport)
		if (!gVoiceClient.ready() || !input.has("body"))
		{
			return;
		}
		LLViewerRegion* regionp = gAgent.getRegion();
		if (!regionp || !regionp->getFeaturesReceived())
		{
			return;
		}
		const LLSD& body = input["body"];
		if (!body.has("major_version"))
		{
			return;
		}
		LL_DEBUGS("Voice") << "Got Voice version info: "
						   << ll_pretty_print_sd(body) << LL_ENDL;
		// Check for the voice server version, based on its type.
		LLVoiceModule* modulep = gVoiceClient.getModuleFromChannelInfo(body);
		// Default to -1 to cause failure for unknown server type. HB
		S32 max_version = -1;
		if (modulep)
		{
			constexpr S32 WEBRTC_MAJOR_VERSION = 2;
			max_version = WEBRTC_MAJOR_VERSION;
		}
		if (body["major_version"].asInteger() > max_version)
		{
			gNotifications.add("VoiceVersionMismatch");
			// Toggles the listener
			gSavedSettings.setBool("EnableVoiceChat", false);
		}
	}
};

LLHTTPRegistration<LLViewerRequiredVoiceVersion>
    gHTTPRegistrationMessageRequiredVoiceVersion("/message/RequiredVoiceVersion");
