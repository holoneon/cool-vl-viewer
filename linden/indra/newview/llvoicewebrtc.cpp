 /**
 * @file llvoicewebrtc.cpp
 * @brief Implementation of LLVoiceWebRTC class.
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

#include "llviewerprecompiledheaders.h"

#include "json.hpp"

#include "llvoicewebrtc.h"

#include "llcachename.h"
#include "llcallbacklist.h"
#include "llcorehttputil.h"
#include "lldir.h"
#include "llcallbacklist.h"
#include "llpluginclassmedia.h"
#include "llparcel.h"
#include "llrand.h"				// For ll_frand()
#include "llsdutil.h"
#include "hbtracy.h"

#include "llagent.h"
#include "llappviewer.h"
#include "hbfloaterdebugtags.h"	// For HBFloaterDebugTags::setTag()
#include "llgridmanager.h"		// For gIsInSecondLife*
#include "llimmgr.h"
#include "llmutelist.h"
#include "llstartup.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llvoicechannel.h"
#include "llworld.h"

using namespace std::placeholders;
using lljson = nlohmann::json;

static const std::string WEBRTCSTR = "webrtc";

// Helper function
static bool region_has_webrtc(const LLUUID& region_id)
{
	LLViewerRegion* regionp = gWorld.getRegionFromID(region_id);
	return regionp && regionp->hasWebRTCVoice();
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceConnection class
///////////////////////////////////////////////////////////////////////////////

// LLVoiceConnection and its derived classes manage state transitions,
// negotiating WebRTC connections and other such things for a single connection
// to a WebRTC server. Multiple of these connections may be active at once, in
// the case of cross-region voice, or when a new connection is being created
// before the old one has a chance to shut down.

class LLVoiceConnection
{
	LOG_CLASS(LLVoiceConnection);

public:
	LLVoiceConnection(const LLUUID& region_id, const std::string& channel_id);
	virtual ~LLVoiceConnection();

	LL_INLINE const LLUUID& getRegionID() const			{ return mRegionID; }
	LL_INLINE const std::string& getRegionName() const	{ return mRegionName; }

	LL_INLINE void shutDown()							{ mShutDown = true; }
	LL_INLINE bool isShuttingDown() const				{ return mShutDown; }

	bool pluginCreateSession();
	bool processPluginData();

	void sendJoin();
	void sendData(const std::string& data);

	bool connectionStateMachine();

	void setUserVolume(const LLUUID& id, F32 volume);
	void setUserMute(const LLUUID& id, bool mute);

	// New virtual methods
	virtual void setMuteMic(bool muted);
	virtual void setSpeakerVolume(F32 volume);

	virtual bool isSpatial() = 0;

protected:
	void onVoiceConnectionRequestSuccess(const LLSD& body);

	void onDataReceivedImpl(const std::string& data);

	void processIceUpdatesCoro();

	typedef enum e_voice_connection_state : U32
	{
		VS_ERROR					= 0x0,
		VS_START_SESSION			= 0x1,
		VS_WAIT_FOR_SESSION_START	= 0x2,
		VS_REQUEST_CONNECTION		= 0x4,
		VS_CONNECTION_WAIT			= 0x8,
		VS_SESSION_ESTABLISHED		= 0x10,
		VS_WAIT_FOR_DATA_CHANNEL	= 0x20,
		VS_SESSION_UP				= 0x40,
		VS_SESSION_RETRY			= 0x80,
		VS_DISCONNECT				= 0x100,
		VS_WAIT_FOR_EXIT			= 0x200,
		VS_SESSION_EXIT				= 0x400,
		VS_WAIT_FOR_CLOSE			= 0x800,
		VS_CLOSED					= 0x1000,
		VS_SESSION_STOPPING			= 0x1F80,
		// Used when no WebRTC server is available for a particular sim, to
		// avoid infinite connection retries. HB
		VS_SESSION_JAIL				= 0x2000,
	} EVoiceConnectionState;

	std::string state2string(EVoiceConnectionState status);

	void setVoiceConnectionState(EVoiceConnectionState state,
								 bool force = false);

	LL_INLINE EVoiceConnectionState getVoiceConnectionState() const
	{
		return mVoiceConnectionState;
	}

	virtual void requestVoiceConnection() = 0;

	void breakVoiceConnectionCoro();

protected:
	LLUUID										mID;
	LLSD										mIceCandidates;

	LLCore::HttpOptions::ptr_t					mHttpOptions;

	LLUUID										mViewerSession;

	LLUUID										mRegionID;
	std::string									mRegionName;

	std::string									mChannelID;

	std::string									mChannelSDP;

	LLTimer										mStateTransitionTimer;

	EVoiceConnectionState						mVoiceConnectionState;

	S32											mOutstandingRequests;
	// Number of seconds to wait before next retry.
	F32											mRetryWaitSecs;
	// Number of UPDATE_THROTTLE_SECONDS we have waited since our last attempt
	// to connect.
	S32											mRetryWaitPeriod;

	LLVoiceClientStatusObserver::EStatusType	mCurrentStatus;

	F32											mSpeakerVolume;
	bool										mMuted;

	// *HACK: to get the audio devices to properly be taken into account by
	// WebRTC after a voice channel change. For a full explanation of how this
	// hack works, see the comment in connectionStateMachine() inside the
	// VS_SESSION_UP state. HB
	bool										mNeedsTuning;

	bool										mPluginSessionCreated;
	bool										mHasDataInterface;
	bool										mIceCompleted;

	bool										mShutDown;

	bool										mPrimary;
};

// Since LLVoiceConnection uses coroutines and each instance may get removed
// from memory by the main coroutine, we must check on each return from a yield
// in the coroutines that the connection still exists !  This vector holds a
// list of the live LLVoiceConnection internal UUIDs for this purpose. HB
static uuid_list_t sVoiceConnections;
// Do not auto-tune when the channel Id did not change since last time. HB
static std::string sLastTunedChannelId;

LLVoiceConnection::LLVoiceConnection(const LLUUID& region_id,
									 const std::string& channel_id)
:	mRegionID(region_id),
	mChannelID(channel_id),
	mHasDataInterface(false),
	mHttpOptions(new LLCore::HttpOptions),
	mVoiceConnectionState(VS_START_SESSION),
	mCurrentStatus(LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED),
	mMuted(true),
	mNeedsTuning(false),
	mShutDown(false),
	mPrimary(false),
	mIceCompleted(false),
	mSpeakerVolume(0.f),
	mOutstandingRequests(0),
	mRetryWaitPeriod(0),
	mRetryWaitSecs(ll_frand() + 0.5f)
{
	mHttpOptions->setWantHeaders(true);
	LLViewerRegion* regionp = gWorld.getRegionFromID(mRegionID);
	if (regionp)
	{
		mRegionName = regionp->getIdentity();
	}
	mID.generate();
	sVoiceConnections.emplace(mID);
	mPluginSessionCreated = pluginCreateSession();
	LL_DEBUGS("Voice") << "New connection (" << mID << ") for region: "
					   << mRegionName << LL_ENDL;
	mStateTransitionTimer.start();
}

//virtual
LLVoiceConnection::~LLVoiceConnection()
{
	sVoiceConnections.erase(mID);
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCommand(mID, "delete_connection");
	}
}

std::string LLVoiceConnection::state2string(EVoiceConnectionState status)
{
	std::string result = "UNKNOWN";
	// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break
	switch (status)
	{
		CASE(VS_ERROR);
		CASE(VS_START_SESSION);
		CASE(VS_WAIT_FOR_SESSION_START);
		CASE(VS_REQUEST_CONNECTION);
		CASE(VS_CONNECTION_WAIT);
		CASE(VS_SESSION_ESTABLISHED);
		CASE(VS_WAIT_FOR_DATA_CHANNEL);
		CASE(VS_SESSION_UP);
		CASE(VS_SESSION_RETRY);
		CASE(VS_DISCONNECT);
		CASE(VS_WAIT_FOR_EXIT);
		CASE(VS_SESSION_EXIT);
		CASE(VS_WAIT_FOR_CLOSE);
		CASE(VS_CLOSED);
		CASE(VS_SESSION_STOPPING);
		CASE(VS_SESSION_JAIL);

		default:
			break;
	}
#undef CASE
	return result;
}

void LLVoiceConnection::setVoiceConnectionState(EVoiceConnectionState state,
												bool force)
{
	// Never change state after jailing. HB
	if (mVoiceConnectionState == VS_SESSION_JAIL)
	{
		return;
	}
	if (force || (state & VS_SESSION_STOPPING) ||	// Shutdown or restart
		// Ignore state changes *while* shutting down or restarting.
		!(mVoiceConnectionState & VS_SESSION_STOPPING))
	{
		LL_DEBUGS("Voice") << (isSpatial() ? "Spatial" : "Ad-hoc")
						   << " connection for region '" << mRegionName
						   << "' entering state " << state2string(state)
						   << " - Transition time from previous state "
						   << state2string(mVoiceConnectionState) << ": "
						   << mStateTransitionTimer.getElapsedTimeF32() << "s"
						   << LL_ENDL;
		mVoiceConnectionState = state;
		mStateTransitionTimer.reset();
		if (state == VS_WAIT_FOR_SESSION_START ||
			state == VS_CONNECTION_WAIT || state == VS_WAIT_FOR_CLOSE ||
			state == VS_DISCONNECT)
		{
			static LLCachedControl<U32> timeout(gSavedSettings,
												"WebRTCVoiceTimeout");
			if (timeout >= 10)	// Clamp to a realistic value
			{
				// Let's guard against a stuck connection process... HB
				mStateTransitionTimer.setTimerExpirySec(timeout);
			}
		}
	}
}

bool LLVoiceConnection::pluginCreateSession()
{
	// Wait for the WebRTC plugin to reach the running state so that it can
	// accept commands. HB
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (!pluginp || !pluginp->isPluginRunning())
	{
		return false;
	}
	if (pluginp)
	{
		pluginp->sendVoiceCommand(mID, "add_connection");
		if (mVoiceConnectionState != VS_START_SESSION)
		{
			// This is a re-creation after a plugin failure: reset the
			// connection steps flags and update the state. HB
			mHasDataInterface = mIceCompleted = false;
			setVoiceConnectionState(VS_SESSION_EXIT);
		}
		mPluginSessionCreated = true;
		return true;
	}
	return false;
}

// ICE candidates may be streamed in before or after the SDP offer is available
// (see below). This method determines whether candidates are available to send
// to the WebRTC server via the simulator. If so, and there are no more
// candidates, this code will make the capability call to the server sending up
// the ICE candidates.
void LLVoiceConnection::processIceUpdatesCoro()
{
	if (gDisconnected || LLApp::isQuitting() ||gVoiceWebRTC.isTerminated() ||
		mShutDown)
	{
		return;
	}

	LLViewerRegion* regionp = gWorld.getRegionFromID(mRegionID);
	if (!regionp)
	{
		LL_DEBUGS("Voice") << "Region is gone. Ignoring." << LL_ENDL;
		return;
	}
	if (!regionp->capabilitiesReceived())
	{
		LL_DEBUGS("Voice") << "Capabilities not yet received for region "
						   << regionp->getIdentity() << LL_ENDL;
		return;
	}

	const std::string& url = regionp->getCapability("VoiceSignalingRequest");
	if (url.empty())
	{
		LL_DEBUGS("Voice") << "Missing VoiceSignalingRequest capability for region "
						   << regionp->getIdentity() << LL_ENDL;
		return;
	}

	LL_DEBUGS("Voice") << "Sending candidates data" << LL_ENDL;
	LLSD body;
	body["viewer_session"] = mViewerSession;
	body["voice_server_type"] = WEBRTCSTR;
	if (mIceCandidates.isDefined())
	{
		body["candidates"] = mIceCandidates;
		mIceCandidates.clear();
	}
	else if (mIceCompleted)
	{
		LLSD data;
		data["completed"] = true;
		body["candidate"] = data;
		mIceCompleted = false;
	}
	else	// This should never happen. HB
	{
		llwarns << "Incoherent state: mIceCandidates empty and mIceCompleted false"
				<< llendl;
		return;
	}

	LL_DEBUGS("Voice") << "Posting for " << regionp->getIdentity()
					   << " - URL: " << url << " - Body: " << body << LL_ENDL;
	++mOutstandingRequests;
	LLUUID connection_id = mID;	// Keep a copy on the stack. HB
	LLCoreHttpUtil::HttpCoroutineAdapter adapter("processIceUpdatesCoro");
	LLSD result = adapter.postAndSuspend(url, body, mHttpOptions);
	if (!sVoiceConnections.count(connection_id))
	{
		return;
	}
	--mOutstandingRequests;

	if (gDisconnected || LLApp::isQuitting() || gVoiceWebRTC.isTerminated())
	{
		return;
	}

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (!status)
	{
		// Could not trickle the candidates, so restart the session.
		llwarns << "Could not trickle the candidates. HTTP error "
				<< status.getType() << ": " << status.toString() << llendl;
		setVoiceConnectionState(VS_SESSION_RETRY);
	}
}

//virtual
void LLVoiceConnection::setMuteMic(bool muted)
{
	mMuted = muted;
	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || mRegionID != regionp->getRegionID())
	{
		// Always mute this agent with respect to neighboring regions.
		// Peers do not want to hear this agent from multiple regions as
		// that would echo.
		muted = true;
	}
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCmdWithBool(mID, "mute", "muted", muted);
	}
}

//virtual
void LLVoiceConnection::setSpeakerVolume(F32 volume)
{
	mSpeakerVolume = volume;
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCmdWithReal(mID, "receive_volume", "volume", volume);
	}
}

void LLVoiceConnection::setUserVolume(const LLUUID& id, F32 volume)
{
	lljson root = lljson::object();
	lljson user_gain = lljson::object();
	// Give it two decimal places with a range from 0-200, where 100 is normal
	user_gain[id.asString()] = (U32)(volume * 200.f);
	root["ug"] = user_gain;
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCmdWithString(mID, "send_data", "json_data",
										to_string(root));
	}
}

void LLVoiceConnection::setUserMute(const LLUUID& id, bool mute)
{
	lljson root = lljson::object();
	lljson muted = lljson::object();
	muted[id.asString()] = mute;
	root["m"] = muted;
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCmdWithString(mID, "send_data", "json_data",
										to_string(root));
	}
}

// Sends data to the WebRTC server via the webrtc data channel.
void LLVoiceConnection::sendData(const std::string& data)
{
	if (getVoiceConnectionState() == VS_SESSION_UP)
	{
		LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
		if (pluginp)
		{
			pluginp->sendVoiceCmdWithString(mID, "send_data", "json_data",
											data);
		}
	}
}

// Tells the simulator that we are shutting down a voice connection. The
// simulator will pass this on to the WebRTC server.
void LLVoiceConnection::breakVoiceConnectionCoro()
{
	LL_DEBUGS("Voice") << "Disconnecting voice." << LL_ENDL;

	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCommand(mID, "close_data_interface");
	}
	mHasDataInterface = false;

	LLViewerRegion* regionp = gWorld.getRegionFromID(mRegionID);
	if (!regionp || !regionp->capabilitiesReceived())
	{
		LL_DEBUGS("Voice") << "Still waiting for region capabilities..."
						   << LL_ENDL;
		setVoiceConnectionState(VS_SESSION_RETRY);
		return;
	}

	const std::string& url =
		regionp->getCapability("ProvisionVoiceAccountRequest");
	if (url.empty())
	{
		llwarns_once << "No ProvisionVoiceAccountRequest capability for agent region; will retry."
					 << llendl;
		setVoiceConnectionState(VS_SESSION_RETRY);
		return;
	}

	LL_DEBUGS("Voice") << "Returning to wait state and loging out voice server..."
					   << LL_ENDL;
	setVoiceConnectionState(VS_WAIT_FOR_EXIT);

	LLSD body;
	body["logout"] = true;
	body["viewer_session"] = mViewerSession;
	body["voice_server_type"] = WEBRTCSTR;

	++mOutstandingRequests;
	LLUUID connection_id = mID;	// Keep a copy on the stack. HB
	LLCoreHttpUtil::HttpCoroutineAdapter adapter("breakVoiceConnection");
	LLSD result = adapter.postAndSuspend(url, body, mHttpOptions);
	if (!sVoiceConnections.count(connection_id))
	{
		return;
	}
	--mOutstandingRequests;

	if (!gDisconnected && !LLApp::isQuitting() && !gVoiceWebRTC.isTerminated())
	{
		setVoiceConnectionState(VS_SESSION_EXIT);
	}
}

void LLVoiceConnection::onVoiceConnectionRequestSuccess(const LLSD& result)
{
	if (gDisconnected || LLApp::isQuitting() || gVoiceWebRTC.isTerminated())
	{
		return;
	}

	if (!result.has("viewer_session") || !result.has("jsep") ||
		!result["jsep"].has("sdp") || !result["jsep"].has("type") ||
		result["jsep"]["type"].asString() != "answer")
	{
		llwarns << "Invalid voice provision request result: " << result
				<< llendl;
		setVoiceConnectionState(VS_SESSION_EXIT);
		return;
	}

	mViewerSession = result["viewer_session"];
	std::string sdp = result["jsep"]["sdp"].asString();
	LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response: channel sdp "
					   << sdp << LL_ENDL;
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCmdWithString(mID, "answer", "remote_sdp", sdp);
	}
}

static LLSD get_connection_urls(const LLUUID& region_id)
{
	LLSD urls = LLSD::emptyArray();
	std::string url;

	if (gIsInSecondLife)
	{
		std::string grid;
		U32 num_servers;
		if (gIsInSecondLifeBetaGrid)
		{
			grid = "aditi";
			num_servers = 2;
		}
		else
		{
			grid = "agni";
			num_servers = 3;
		}
		static const char* url_format = "stun:stun%d.%s.secondlife.io:3478";
		for (U32 i = 1; i <= num_servers; ++i)
		{
			url = llformat(url_format, i, grid.c_str());
			urls.append(LLSD::String(url));
			LL_DEBUGS("Voice") << "Added WebRTC server URI: " << url
							   << LL_ENDL;
		}
	}
	else
	{
		LLViewerRegion* regionp = gWorld.getRegionFromID(region_id);
		if (regionp)
		{
			std::string stuns = regionp->getStunServers();
			while (!stuns.empty())
			{
				size_t i = stuns.find(',');
				if (i != std::string::npos)
				{
					url = stuns.substr(0, i);
					stuns.erase(0, i + 1);
				}
				else
				{
					url = stuns;
					stuns.clear();
				}
				LLStringUtil::trim(url);
				if (!url.empty())
				{
					urls.append(LLSD::String(url));
					LL_DEBUGS("Voice") << "Added WebRTC server URI: " << url
									   << LL_ENDL;
				}
			}
		}
	}
	return urls;
}

bool LLVoiceConnection::processPluginData()
{
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (!pluginp)
	{
		return false;
	}
	pluginp->lockWebRTCData();
	LLSD& webrtc_data = pluginp->getVoiceData(mID);
	if (webrtc_data.isDefined() && webrtc_data.isArray())
	{
		for (LLSD::array_const_iterator it = webrtc_data.beginArray(),
										end = webrtc_data.endArray();
			 it != end; ++it)
		{
			const LLSD& data = *it;
			if (data.has("ice_completed"))
			{
				mIceCompleted = true;
				LL_DEBUGS("Voice") << "ICE complete" << LL_ENDL;
			}
			if (data.has("ice_candidates") && data["ice_candidates"].isArray())
			{
				if (mIceCandidates.isUndefined())
				{
					mIceCandidates = LLSD::emptyArray();
				}
				U32 count = 0;
				const LLSD& candidates = data["ice_candidates"];
				for (LLSD::array_const_iterator iter = candidates.beginArray(),
												end = candidates.endArray();
					 iter != end; ++iter)
				{
					mIceCandidates.append(*iter);
					++count;
				}
				LL_DEBUGS("Voice") << "Got " << count
								   << " new ICE candidate(s)." << LL_ENDL;
			}
			if (data.has("channel_sdp"))
			{
				mChannelSDP = data["channel_sdp"].asString();
				LL_DEBUGS("Voice") << "Received offer: " << mChannelSDP
								   << LL_ENDL;
				if (mVoiceConnectionState == VS_WAIT_FOR_SESSION_START)
				{
					setVoiceConnectionState(VS_REQUEST_CONNECTION, true);
				}
			}
			if (data.has("got_data_interface"))
			{
				LL_DEBUGS("Voice") << "Data interface ready." << LL_ENDL;
				mHasDataInterface = true;
			}
			if (data.has("channel_data"))
			{
				const LLSD& chan_data = data["channel_data"];
				if (chan_data.isArray())
				{
					for (LLSD::array_const_iterator
							iter = chan_data.beginArray(),
							end = chan_data.endArray();
						 iter != end; ++iter)
					{
						onDataReceivedImpl(iter->asString());
					}
				}
				else
				{
					llwarns << "Channel data LLSD is not an array: "
							<< chan_data << llendl;
				}
			}
			if (data.has("established"))
			{
				setVoiceConnectionState(VS_SESSION_ESTABLISHED);
			}
			if (data.has("closed"))
			{
				setVoiceConnectionState(VS_CLOSED);
			}
			if (data.has("renegotiate"))
			{
				setVoiceConnectionState(VS_SESSION_RETRY);
				mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
			}
		}
	}
	webrtc_data.clear();
	pluginp->unlockWebRTCData();
	return true;
}

// Primary state machine for negotiating a single voice connection to the
// WebRTC server.
bool LLVoiceConnection::connectionStateMachine()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	if (!processPluginData())
	{
		llwarns << "Failed to process plugin data. Aborting." << llendl;
		setVoiceConnectionState(VS_SESSION_EXIT);
		return false;
	}

	// Process ICE updates if needed.
	if (mIceCompleted || mIceCandidates.isDefined())
	{
		gCoros.launch("processIceUpdates",
					  std::bind(&LLVoiceConnection::processIceUpdatesCoro,
								this));
	}

	if (mStateTransitionTimer.hasExpiration() &&
		mStateTransitionTimer.hasExpired())
	{
		LL_DEBUGS("Voice") << "Timeout waiting for state transition."
						   << LL_ENDL;
		setVoiceConnectionState(VS_SESSION_RETRY, true);
	}

	// *HACK: see below in VS_SESSION_UP case for explanation. HB
	static LLCachedControl<F32> tune_duration(gSavedSettings,
											  "VoiceWebRTCAutoTuneDuration");
	switch (getVoiceConnectionState())
	{
		case VS_START_SESSION:
		{
			mNeedsTuning = false;
			if (mShutDown)
			{
				LL_DEBUGS("Voice") << "Shutting voice down." << LL_ENDL;
				setVoiceConnectionState(VS_SESSION_EXIT);
				break;
			}
			// Wait for our session to be declared to the running WebRTC plugin
			if (!mPluginSessionCreated)
			{
				pluginCreateSession();
				break;
			}
			mIceCompleted = false;
			setVoiceConnectionState(VS_WAIT_FOR_SESSION_START);
			LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
			if (pluginp)
			{
				pluginp->sendVoiceCmdWithLLSD(mID, "connect", "urls",
											  get_connection_urls(mRegionID));
				// Note: in case of failure to connect, the plugin would queue
				// "renegociate" request for this connection, which will also
				// trigger a change to VS_SESSION_RETRY. HB
			}
			break;
		}

		case VS_WAIT_FOR_SESSION_START:
		case VS_CONNECTION_WAIT:
			if (mShutDown)
			{
				LL_DEBUGS("Voice") << "Shutting voice down." << LL_ENDL;
				setVoiceConnectionState(VS_SESSION_EXIT);
			}
			break;

		case VS_REQUEST_CONNECTION:
			if (mShutDown)
			{
				LL_DEBUGS("Voice") << "Shutting voice down." << LL_ENDL;
				setVoiceConnectionState(VS_SESSION_EXIT);
				break;
			}
			// Ask the sim to relay to the WebRTC server our request for a
			// connection to a given voice channel. On completion, we will move
			// on to VS_SESSION_ESTABLISHED via a callback on a webrtc thread.
			setVoiceConnectionState(VS_CONNECTION_WAIT);
			gCoros.launch("requestVoiceConnectionCoro",
						  std::bind(&LLVoiceConnection::requestVoiceConnection,
									this));
			break;

		case VS_SESSION_ESTABLISHED:
			if (mShutDown)
			{
				LL_DEBUGS("Voice") << "Shutting voice down." << LL_ENDL;
				setVoiceConnectionState(VS_DISCONNECT);
				break;
			}
			// Update the peer connection with the various characteristics of
			// this connection.
			if (isSpatial())
			{
				// We will determine primary state later and set mute
				// accordinly
				mPrimary = false;
			}
			setSpeakerVolume(mSpeakerVolume);
			gVoiceWebRTC.onConnectionEstablished(mChannelID, mRegionID);
			setVoiceConnectionState(VS_WAIT_FOR_DATA_CHANNEL);
			break;

		case VS_WAIT_FOR_DATA_CHANNEL:
			if (mShutDown)
			{
				LL_DEBUGS("Voice") << "Shutting voice down." << LL_ENDL;
				setVoiceConnectionState(VS_DISCONNECT);
				break;
			}
			if (mHasDataInterface)	// Wait for data interface
			{
				// *HACK: see below in VS_SESSION_UP case for explanation. HB
				mNeedsTuning = tune_duration > 0.f &&
							   // Do not do this on login or after (re)enabling
							   // voice...
							   !sLastTunedChannelId.empty() &&
#if 0						   // YES: do it too in this case... So glitchy !
							   // Do not do this on returning to the same
							   // channel Id
							   sLastTunedChannelId != mChannelID &&
#endif
							   !gVoiceWebRTC.inTuningMode();
				// Tuned or not, remember this channel.
				sLastTunedChannelId = mChannelID;
				// Tell the WebRTC server we are here via the data channel.
				sendJoin();
				setVoiceConnectionState(VS_SESSION_UP);
				if (isSpatial())
				{
					gVoiceWebRTC.updatePosition();
					gVoiceWebRTC.sendPositionUpdate(true);
				}
				else
				{
					setMuteMic(mMuted);
				}
			}
			break;

		case VS_SESSION_UP:
			mRetryWaitPeriod = 0;
			mRetryWaitSecs = ll_frand() + 0.5f;
			// We stay in this sate for as long as the session remains up.
			if (mShutDown)
			{
				LL_DEBUGS("Voice") << "Shutting voice down." << LL_ENDL;
				setVoiceConnectionState(VS_DISCONNECT);
				break;
			}
			if (isSpatial() && gAgent.getRegion())
			{
				bool primary = mRegionID == gAgent.getRegion()->getRegionID();
				if (primary != mPrimary)
				{
					mPrimary = primary;
					setMuteMic(mMuted || !mPrimary);
					sendJoin();
				}
			}
			// *HACK: auto re-tune after bringing up the channel so that audio
			// devices are properly accounted for by WebRTC; this basically
			// mimics what happens when opening and closing the voice audio
			// devices settings floater, which also fixes such issues... HB
			if (mNeedsTuning)
			{
				F32 delay = llclamp(F32(tune_duration), 0.5f, 3.f);
				if (!gVoiceWebRTC.inTuningMode())
				{
					LL_DEBUGS("Voice") << "Auto-tuning..." << LL_ENDL;
					gVoiceWebRTC.refreshDeviceLists(false);
					gVoiceWebRTC.setTuningMode(true);
					LLVoiceChannel::suspend();
					// Re-use this timer (not covering VS_SESSION_UP since the
					// latter is not subject to timeouts) to delay the
					// simulated closing of the device settings floater. HB
					mStateTransitionTimer.reset();
				}
				else if (mStateTransitionTimer.getElapsedTimeF32() > delay)
				{
					gVoiceWebRTC.setTuningMode(false);
					LLVoiceChannel::resume();
					mNeedsTuning = false;
					LL_DEBUGS("Voice") << "Auto-tuning done." << LL_ENDL;
				}
			}
			break;

		case VS_SESSION_RETRY:
		{
			constexpr F32 UPDATE_THROTTLE_SECONDS = 0.1f;
			constexpr F32 MAX_RETRY_WAIT_SECONDS = 10.f;

			if (mRetryWaitPeriod++ * UPDATE_THROTTLE_SECONDS <= mRetryWaitSecs)
			{
				break;
			}
			// Something went wrong, so notify that the connection has failed.
			gVoiceWebRTC.onConnectionFailure(mChannelID, mRegionID,
											 mCurrentStatus);
			setVoiceConnectionState(VS_DISCONNECT);
			mRetryWaitPeriod = 0;
			if (mRetryWaitSecs < MAX_RETRY_WAIT_SECONDS)
			{
				// Back off the retry period, and do it by a small random bit
				// so all clients do not reconnect at once.
				mRetryWaitSecs += ll_frand() + 0.5f;
			}
			break;
		}


		case VS_DISCONNECT:
			LL_DEBUGS("Voice") << "Disconnecting..." << LL_ENDL;
			gCoros.launch("breakVoiceConnectionCoro",
						  std::bind(&LLVoiceConnection::breakVoiceConnectionCoro,
									this));
			break;

		case VS_SESSION_EXIT:
		{
			LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
			if (pluginp)
			{
				setVoiceConnectionState(VS_WAIT_FOR_CLOSE);
				pluginp->sendVoiceCommand(mID, "disconnect");
				LL_DEBUGS("Voice") << "Log out message sent to voice server."
								   << LL_ENDL;
			}
			break;
		}

		case VS_CLOSED:
			if (mShutDown)
			{
				// If we still have outstanding HTTP or webrtc calls, wait for
				// them to complete so we do not delete objects while they
				// still may be used.
				if (mOutstandingRequests <= 0)
				{
					gVoiceWebRTC.onConnectionShutDown(mChannelID, mRegionID);
					return false;
				}
				LL_DEBUGS_SPARSE("Voice") << "Waiting for outstanding requests before exiting."
										  << LL_ENDL;
			}
			else
			{
				setVoiceConnectionState(VS_START_SESSION, true);
			}
			break;

		case VS_WAIT_FOR_EXIT:
		case VS_WAIT_FOR_CLOSE:
		case VS_SESSION_JAIL:
			break;

		default:	// This should never happen
			llwarns_sparse << "Unknown control state: "
						   << getVoiceConnectionState() << llendl;
			llassert(false);
			return false;
	}

	return true;
}

void LLVoiceConnection::onDataReceivedImpl(const std::string& data)
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	if (gDisconnected || LLApp::isQuitting() || mShutDown)
	{
		return;
	}

	// 'false' = no throw, 'true' = ignore comments.
	lljson voice_data = lljson::parse(data, NULL, false, true);
	if (voice_data.is_discarded())
	{
		llwarns << "Failed to parse JSON data: " << data << llendl;
		return;
	}
	if (!voice_data.is_object())
	{
		llwarns << "Malformed JSON data (missing object): " << data << llendl;
		return;
	}

	bool is_primary_region = mPrimary;
	LLViewerRegion* regionp = gAgent.getRegion();
	if (!is_primary_region && regionp && isSpatial() &&
		mRegionID == regionp->getRegionID())
	{
		llwarns << "mPrimary is false in agent region and spatial voice. Connection state: 0x"
				<< std::hex << getVoiceConnectionState() << std::dec
				<< llendl;
		is_primary_region = true;
	}
	bool has_mute = false;
	bool has_gain = false;
	lljson mute = lljson::object();
	lljson user_gain = lljson::object();
	for (const auto& entry : voice_data.items())
	{
		LLUUID av_id(entry.key(), false);
		LL_DEBUGS("Voice") << "Processing: " << av_id << LL_ENDL;
		if (av_id.isNull())
		{
			// Likely a test client: ignore.
			continue;
		}
		const lljson& val = entry.value();

		LLVoiceWebRTC::pstate_ptr_t participantp =
			gVoiceWebRTC.findParticipantByID(mChannelID, av_id);
		bool muted = false;
		// We ignore any 'joins' reported about participants that come from
		// voice servers which are not their primary voice server; this may
		// happen with cross-region voice where a participant on a neighboring
		// region may be connected to multiple servers. We do not want to add
		// new identical participants from all of those servers.
		bool primary = false;
		bool joined = val.contains("j") && val["j"].is_object();
		if (joined)
		{
			LL_DEBUGS("Voice") << "New participant: " << av_id << LL_ENDL;
			const lljson& jval = val["j"];
			// A new participant has announced that they are joining.
			if (jval.contains("p") && jval["p"].is_boolean())
			{
				jval["p"].get_to(primary);
			}
			muted = LLMuteList::isMuted(av_id, LLMute::flagVoiceChat);
			if (muted)
			{
				mute[entry.key()] = true;
				has_mute = true;
			}
			// Default to an average volume.
			// *TODO: _maybe_ backport LLSpeakerVolumeStorage ?  HB
			user_gain[entry.key()] = 100;	// Note: max is 200.
			has_gain = true;
		}
		if (!participantp && joined && (primary || !isSpatial()))
		{
			participantp = gVoiceWebRTC.addParticipantByID(mChannelID, av_id,
														   mRegionID);
		}
		if (!participantp)
		{
			// Mute info message can be received before join message, so try to
			// mute again later
			if ((is_primary_region || primary) && isSpatial() &&
				val.contains("m") && val["m"].is_boolean())
			{
				constexpr F32 DELAY = 2.f;
				bool is_muted = false;
				val["m"].get_to(is_muted);
				std::string channel_id = mChannelID;
				doAfterInterval([channel_id, av_id, is_muted]()
								{
									LLVoiceWebRTC::pstate_ptr_t participantp =
											gVoiceWebRTC.findParticipantByID(channel_id,
																			 av_id);
									if (!participantp) return;
									participantp->mIsModeratorMuted = is_muted;
									if (av_id == gAgentID)
									{
										gVoiceClient.setMutedInfo(channel_id,
																  is_muted);
									}
								}, DELAY);
			}
			continue;
		}

		// Keep a cached value of the muted voice chat flag. HB
		if (joined && muted)
		{
			LL_DEBUGS("Voice") << "Muted participant: " << av_id << LL_ENDL;
			participantp->mIsMuted = true;
		}

		bool leaving = false;
		if (val.contains("l") && val["l"].is_boolean())
		{
			val["l"].get_to(leaving);
		}
		if (leaving)
		{
			// An existing participant is leaving
			if (av_id != gAgentID)
			{
				LL_DEBUGS("Voice") << "Leaving participant: " << av_id
								   << LL_ENDL;
				gVoiceWebRTC.removeParticipantByID(mChannelID, av_id,
												   mRegionID);
			}
			continue;
		}
		// We got a 'power' update.
		if (val.contains("p") && val["p"].is_number_integer())
		{
			S32 level;
			val["p"].get_to(level);
			participantp->mLevel = F32(level) / 128.f;
		}

		if (val.contains("v") && val["v"].is_boolean())
		{
			val["v"].get_to(participantp->mIsSpeaking);
		}

		if (val.contains("m") && val["m"].is_boolean())
		{
			bool is_moderator_muted = false;
			val["m"].get_to(is_moderator_muted);
			if (isSpatial())
			{
				// Ignore muted flags from non-primary server
				if (!primary && !is_primary_region)
				{
					is_moderator_muted = false;
				}
				else if (av_id == gAgentID)
				{
					gVoiceClient.setMutedInfo(mChannelID, is_moderator_muted);
				}
			}
			participantp->mIsModeratorMuted = is_moderator_muted;
			if (is_moderator_muted)
			{
				LL_DEBUGS("Voice") << "Moderator-muted participant: " << av_id
								   << LL_ENDL;
			}
		}
	}

	// Tell the simulator to set the mute and volume data for these
	// participants, if there are any updates.
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (!pluginp || (!has_mute && !has_gain))
	{
		return;
	}
	lljson root = lljson::object();
	if (has_mute)
	{
		root["m"] = mute;
	}
	if (has_gain)
	{
		root["ug"] = user_gain;
	}
	pluginp->sendVoiceCmdWithString(mID, "send_data", "json_data",
									to_string(root));
}

// Tells the WebRTC server that we are joining and whether we are joining a
// server associated with the the region we currently occupy or not (primary).
// The WebRTC voice server will pass this info to peers.
void LLVoiceConnection::sendJoin()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp)	// Disconnected
	{
		return;
	}

	LL_DEBUGS("Voice") << "Sending join request." << LL_ENDL;
	lljson root = lljson::object();
	lljson join_obj = lljson::object();
	if (mPrimary)
	{
		join_obj["p"] = true;
	}
	root["j"] = join_obj;
	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCmdWithString(mID, "send_data", "json_data",
										to_string(root));
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceConnectionSpatial class
///////////////////////////////////////////////////////////////////////////////

class LLVoiceConnectionSpatial final : public LLVoiceConnection
{
	LOG_CLASS(LLVoiceConnectionSpatial);

public:
	LLVoiceConnectionSpatial(const LLUUID& region_id, S32 parcel_id,
							 const std::string& channel_id);

	LL_INLINE bool isSpatial() override				{ return true; }

	void setMuteMic(bool muted) override;

protected:
	void requestVoiceConnection() override;

private:
	S32 mParcelLocalID;
};

LLVoiceConnectionSpatial::LLVoiceConnectionSpatial(const LLUUID& region_id,
												   S32 parcel_id,
												   const std::string& chan_id)
:	LLVoiceConnection(region_id, chan_id),
	mParcelLocalID(parcel_id)
{
	// Will be set to primary after the connection is established
	mPrimary = false;
}

//virtual
void LLVoiceConnectionSpatial::setMuteMic(bool muted)
{
	if (mMuted == muted)
	{
		return;
	}
	mMuted = muted;

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || mRegionID != regionp->getRegionID())
	{
		// Always mute this agent with respect to neighboring regions: peers do
		// not want to hear this agent from multiple regions which would just
		// be echos.
		muted = true;
	}

	LLPluginClassMedia* pluginp = gVoiceWebRTC.getPlugin();
	if (pluginp)
	{
		pluginp->sendVoiceCmdWithBool(mID, "mute", "muted", muted);
	}
}

// Asks the simulator to forward our request to the WebRTC server for a voice
// connection. The SDP is sent up as part of this, and the simulator will
// respond with an 'answer' which is in the form of another SDP. The webrtc
// library will use the offer and answer to negotiate the session.
//virtual
void LLVoiceConnectionSpatial::requestVoiceConnection()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	LLViewerRegion* regionp = gWorld.getRegionFromID(mRegionID);
	if (!regionp || !regionp->capabilitiesReceived())
	{
		setVoiceConnectionState(VS_SESSION_RETRY);
		return;
	}

	const std::string& url =
		regionp->getCapability("ProvisionVoiceAccountRequest");
	if (url.empty())
	{
		llwarns_once << "No ProvisionVoiceAccountRequest capability for agent region; will retry."
					 << llendl;
		setVoiceConnectionState(VS_SESSION_RETRY);
		return;
	}

	LL_DEBUGS("Voice") << "Requesting spatial voice connection." << LL_ENDL;
	LLSD body;
	body["channel_type"] = "local";
	body["voice_server_type"] = WEBRTCSTR;
	if (mParcelLocalID != INVALID_PARCEL_ID)
	{
		body["parcel_local_id"] = mParcelLocalID;
	}
	LLSD data;
	data["type"] = "offer";
	data["sdp"] = mChannelSDP;
	body["jsep"] = data;

	++mOutstandingRequests;
	LLUUID connection_id = mID;	// Keep a copy on the stack. HB
	LLCoreHttpUtil::HttpCoroutineAdapter adapter("requestVoiceConnection");
	LLSD result = adapter.postAndSuspend(url, body, mHttpOptions);
	if (!sVoiceConnections.count(connection_id))
	{
		return;
	}
	--mOutstandingRequests;

	if (gDisconnected || LLApp::isQuitting() || gVoiceWebRTC.isTerminated())
	{
		return;
	}

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (status)
	{
		onVoiceConnectionRequestSuccess(result);
		return;
	}

	LL_DEBUGS("Voice") << "Region: " << mRegionID << ". Status: "
					   << status.toString() << LL_ENDL;
	switch (status.getType())
	{
		case HTTP_CONFLICT:
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL;
			break;

		case HTTP_UNAUTHORIZED:
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED;
			break;

		case HTTP_NOT_FOUND:	// The sim does not have WebRTC. HB
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
			// We must jail the state machine, else the connection is retried
			// indefinitely !  HB
			setVoiceConnectionState(VS_SESSION_JAIL, true);
			return;

		default:
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
	}
	setVoiceConnectionState(VS_SESSION_EXIT);
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceConnectionAdHoc class
///////////////////////////////////////////////////////////////////////////////

class LLVoiceConnectionAdHoc final : public LLVoiceConnection
{
	LOG_CLASS(LLVoiceConnectionAdHoc);

public:
	LLVoiceConnectionAdHoc(const LLUUID& region_id,
						   const std::string& channel_id,
						   const std::string& credentials)
	:	LLVoiceConnection(region_id, channel_id),
		mCredentials(credentials)
	{
	}

	LL_INLINE bool isSpatial() override				{ return false; }

protected:
	void requestVoiceConnection() override;

private:
	std::string mCredentials;
};

// Add-hoc connections require a different channel type as they go to a
// different set of WebRTC servers. They also require credentials for the given
// channels.
//virtual
void LLVoiceConnectionAdHoc::requestVoiceConnection()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	LLViewerRegion* regionp = gWorld.getRegionFromID(mRegionID);
	if (!regionp)
	{
		LL_DEBUGS("Voice") << "Region " << mRegionID << " not yet live."
						   << LL_ENDL;
		setVoiceConnectionState(VS_SESSION_RETRY);
		return;
	}
	if (!regionp->capabilitiesReceived())
	{
		LL_DEBUGS("Voice") << "Capabilities not yet received for region "
						   << regionp->getIdentity() << LL_ENDL;
		setVoiceConnectionState(VS_REQUEST_CONNECTION);
		return;
	}

	const std::string& url = regionp->getCapability("VoiceSignalingRequest");
	if (url.empty())
	{
		LL_DEBUGS("Voice") << "VoiceSignalingRequest capability not yet received for region "
						   << regionp->getIdentity() << LL_ENDL;
		setVoiceConnectionState(VS_SESSION_RETRY);
		return;
	}

	LLSD body;
	body["credentials"] = mCredentials;
	body["channel"] = mChannelID;
	body["channel_type"] = "multiagent";
	body["voice_server_type"] = WEBRTCSTR;
	LLSD data;
	data["type"] = "offer";
	data["sdp"] = mChannelSDP;
	body["jsep"] = data;

	++mOutstandingRequests;
	LLUUID connection_id = mID;	// Keep a copy on the stack. HB
	LLCoreHttpUtil::HttpCoroutineAdapter adapter("requestVoiceConnection");
	LLSD result = adapter.postAndSuspend(url, body, mHttpOptions);
	if (!sVoiceConnections.count(connection_id))
	{
		return;
	}
	--mOutstandingRequests;

	if (gDisconnected || LLApp::isQuitting() || gVoiceWebRTC.isTerminated())
	{
		return;
	}

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (status)
	{
		onVoiceConnectionRequestSuccess(result);
		return;
	}

	LL_DEBUGS("Voice") << "Region: " << mRegionID << ". Status: "
					   << status.toString() << LL_ENDL;
	switch (status.getType())
	{
		case HTTP_CONFLICT:
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL;
			break;

		case HTTP_UNAUTHORIZED:
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED;
			break;

		case HTTP_NOT_FOUND:	// The sim does not have WebRTC. HB
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
			// We must jail the state machine, else the connection is retried
			// indefinitely !  HB
			setVoiceConnectionState(VS_SESSION_JAIL, true);
			return;

		default:
			mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
	}
	setVoiceConnectionState(VS_SESSION_EXIT);
}

///////////////////////////////////////////////////////////////////////////////
// LLWebRTCMuteListObserver class
///////////////////////////////////////////////////////////////////////////////

class LLWebRTCMuteListObserver : public LLMuteListObserver
{
	LL_INLINE void onChange() override
	{
		gVoiceWebRTC.muteListChanged();
	}
};

static LLWebRTCMuteListObserver sMutelistListener;

///////////////////////////////////////////////////////////////////////////////
// LLVoiceWebRTC class proper
///////////////////////////////////////////////////////////////////////////////

LLVoiceWebRTC gVoiceWebRTC;

LLVoiceWebRTC::LLVoiceWebRTC()
:	mTerminated(false),
	mInitDone(false),
	mDeviceSettingsAvailable(false),
	mNoCaptureDevice(true),
	mNoRenderDevice(true),
	mCaptureDevice("Default"),
	mRenderDevice("Default"),
	mIsInTuningMode(false),
	mTuningMicGain(0.f),
	mTuningSpeakerVolume(50),
	mSpatialCoordsDirty(false),
	mMuteMic(false),
	mEarLocation(0),
	mMicGain(0.f),
	mSpeakerVolume(0.f),
	mVoiceEnabled(false),
	mProcessChannels(false),
	mIsProcessingChannels(false),
	mPluginNeedsConfig(true),
	mPluginHasDied(false),
	mWebRTCPlugin(NULL)
{
}

LLVoiceWebRTC::~LLVoiceWebRTC()
{
	// Note: since gVoiceWebRTC is a global instance and not a singleton,
	// this member variable can be tested until the program _exit() call
	// without needing it to be static. HB
	mTerminated = true;
}

//virtual
const std::string& LLVoiceWebRTC::getName() const
{
	return WEBRTCSTR;
}

void LLVoiceWebRTC::updateSettings()
{
	static LLCachedControl<bool> echo(gSavedSettings,
									  "VoiceWebRTCEchoCancellation");
	static LLCachedControl<bool> agc(gSavedSettings,
									 "VoiceWebRTCAutomaticGainControl");
	static LLCachedControl<U32> noise(gSavedSettings,
									  "VoiceWebRTCNoiseSuppression");
	if (!mWebRTCPlugin)
	{
		return;
	}
	llinfos << "Updating WebRTC voice settings." << llendl;
	LLSD config;
	config["cancel_echo"] = LLSD::Boolean(echo);
	config["agc"] = LLSD::Boolean(agc);
	config["noise_suppression"] = LLSD::Integer(llmin(4, noise));
	mWebRTCPlugin->setConfiguration(config);
}

void LLVoiceWebRTC::init()
{
	if (mTerminated || mInitDone)
	{
		return;
	}
	mInitDone = true;
	llinfos << "Initializing WebRTC voice." << llendl;
	if (gSavedSettings.getBool("DebugWebRTCVoice"))
	{
		HBFloaterDebugTags::setTag("Voice", true);
		HBFloaterDebugTags::setTag("Plugin", true);
	}
	gIdleCallbacks.addFunction(LLVoiceWebRTC::idle, &gVoiceWebRTC);
	LLMuteList::addObserver(&sMutelistListener);
}

void LLVoiceWebRTC::terminate()
{
	if (mTerminated)
	{
		return;
	}
	if (mInitDone)
	{
		LLMuteList::removeObserver(&sMutelistListener);
		if (mSession)
		{
			llinfos << "Leaving WebRTC voice session..." << llendl;
			mSession->shutdownAllConnections();
		}
		gIdleCallbacks.deleteFunction(LLVoiceWebRTC::idle, &gVoiceWebRTC);
		killPlugin();
		mVoiceEnabled = false;
		llinfos << "WebRTC voice terminated." << llendl;
	}
	mTerminated = true;
	cleanUp();
}

void LLVoiceWebRTC::cleanUp()
{
	LL_DEBUGS("Voice") << "Cleaning up..." << LL_ENDL;
	mNextSession.reset();
	mSession.reset();
	mNeighboringRegions.clear();
	sessionState::forEach(std::bind(predShutdownSession, _1));
	killPlugin();
   	// We simply removed the plugin: no need to ask sessions to reconnect
	// after we will relaunch another. HB
	mPluginHasDied = false;
}

bool LLVoiceWebRTC::launchPlugin()
{
	// Remove any (dead) plugin media instance from memory. HB
	killPlugin();

	mCaptureDevice = "Default";
	mRenderDevice = "Default";
	mPluginNeedsConfig = true;

	std::string launcher_name = gDirUtil.getLLPluginLauncher();
	std::string plugin_name =
		gDirUtil.getLLPluginFilename("voice_plugin_webrtc");
	if (!LLFile::isfile(launcher_name) || !LLFile::isfile(plugin_name))
	{
		llwarns << "Missing plugin or launcher executable: cannot procceed."
				<< llendl;
		terminate();
		return false;
	}
	// We specify a NULL owner because we do not care about media events
	// which are not used by our plugin. HB
	mWebRTCPlugin = new LLPluginClassMedia(NULL);
	const std::string& plugin_dir = gDirUtil.getLLPluginDir();
	if (!mWebRTCPlugin->init(launcher_name, plugin_dir, plugin_name, false))
	{
		llwarns << "Failed to initialize the voice plugin: cannot procceed."
				<< llendl;
		mWebRTCPlugin->setDeleteOK(true);
		delete mWebRTCPlugin;
		mWebRTCPlugin = NULL;
		terminate();
		return false;
	}
	mWebRTCPlugin->setLockupTimeout(3.f);	// Timeout after 3 seconds freezes
	return true;
}

void LLVoiceWebRTC::killPlugin()
{
	if (mWebRTCPlugin)
	{
		LLPluginClassMedia* old_pluginp = mWebRTCPlugin;
		mWebRTCPlugin = NULL;
		old_pluginp->setDeleteOK(true);
		delete old_pluginp;
	}
}

//static
void LLVoiceWebRTC::idle(void* userdatap)
{
	LLVoiceWebRTC* self = (LLVoiceWebRTC*)userdatap;
	if (self != &gVoiceWebRTC || !self->mVoiceEnabled)
	{
		return;
	}

	// At this point, check that a plugin has been launched and if not, launch
	// one (this will happen when voice has just been (re-)enabled).
	if (!self->mWebRTCPlugin && !self->launchPlugin())
	{
		return;	// Failure to launch. Voice terminated at this point.
	}

	// Check to see if the current plugin has exited (crash or timeout due to a
	// deadlock); if yes, relaunch another. HB
	if (self->mWebRTCPlugin->isPluginExited())
	{
		llwarns << "WebRTC plugin died: relaunching another..." << llendl;
		if (!self->launchPlugin())
		{
			return;	// Failure to launch. Voice terminated at this point.
		}
		self->mPluginHasDied = true;
	}

	// Here, we know we have a healthy plugin, but it might not yet be ready to
	// process commands, so check it does have reached the "running" state.
	if (self->mWebRTCPlugin->isPluginRunning())
	{
		static F32 last_ping = 0.f;
		if (gFrameTimeSeconds - last_ping >= 1.f)
		{
			last_ping = gFrameTimeSeconds;
			// Note: this command will be ignored by the plugin, but allows to
			// detect a frozen plugin when no voice command has been sent to it
			// for a while.
			self->mWebRTCPlugin->sendVoiceCommand(LLUUID::null, "ping");
		}
		if (self->mPluginNeedsConfig)
		{
			self->mPluginNeedsConfig = false;
			// Push the settings, devices and audio/mic levels to the new
			// plugin.
			gVoiceClient.updateSettings();
		}
		if (self->mPluginHasDied)
		{
			self->mPluginHasDied = false;
			// Signal to all sessions that they need to reconnect with our new
			// plugin.
			sessionState::forEach(std::bind(predReconnect, _1));
		}
		// Refresh the debug flag (this is a no operation when it has not
		// changed since plugin start).
		static LLCachedControl<bool> debug(gSavedSettings, "DebugVoicePlugin");
		self->mWebRTCPlugin->setVoiceDebug(debug);
	}

	// This call allows the plugin to process commands and send back its data.
	self->mWebRTCPlugin->idle();

	// Now, process channels, every 100ms or so.
	static F32 last_process_channel_time = 0.f;
	if (gFrameTimeSeconds - last_process_channel_time >= 0.1f)
	{
		last_process_channel_time = gFrameTimeSeconds;
		self->processChannels();
	}
}

//static
void LLVoiceWebRTC::predReconnect(const sessionState::ptr_t& sessionp)
{
	sessionp->reconnectAllSessions();
}

//static
void LLVoiceWebRTC::predShutdownSession(const sessionState::ptr_t& sessionp)
{
	sessionp->shutdownAllConnections();
}

bool LLVoiceWebRTC::isVoiceWorking() const
{
	return !mTerminated && mVoiceEnabled && mIsProcessingChannels;
}

#if 0	// Not used in the Cool VL Viewer
void LLVoiceWebRTC::addObserver(LLVoiceClientParticipantObserver* observerp)
{
	mParticipantObservers.insert(observerp);
}

void LLVoiceWebRTC::removeObserver(LLVoiceClientParticipantObserver* observerp)
{
	mParticipantObservers.erase(observerp);
}

void LLVoiceWebRTC::notifyParticipantObservers()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	for (observer_set_t::iterator it = mParticipantObservers.begin();
		 it != mParticipantObservers.end(); )
	{
		LLVoiceClientParticipantObserver* observerp = *it;
		observerp->onParticipantsChanged();
		// In case onParticipantsChanged() deleted an entry.
		it = mParticipantObservers.upper_bound(observerp);
	}
}
#endif

void LLVoiceWebRTC::addObserver(LLVoiceClientStatusObserver* observerp)
{
	mStatusObservers.insert(observerp);
}

void LLVoiceWebRTC::removeObserver(LLVoiceClientStatusObserver* observerp)
{
	mStatusObservers.erase(observerp);
}

void LLVoiceWebRTC::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	mIsProcessingChannels =
		status == LLVoiceClientStatusObserver::STATUS_JOINED;

	LL_DEBUGS("Voice") << LLVoiceClientStatusObserver::status2string(status)
					   << " - Channel info: " << getAudioSessionChannelInfo()
					   << " - Proximal is "
					   << (inSpatialChannel() ? "true" : "false") << LL_ENDL;
#if 0
	if (!mProcessChannels)
	{
		return;
	}
#endif
	LLSD channel_info = getAudioSessionChannelInfo();
	for (status_observer_set_t::iterator it = mStatusObservers.begin();
		 it != mStatusObservers.end(); )
	{
		LLVoiceClientStatusObserver* observerp = *it;
		observerp->onChange(status, channel_info, inSpatialChannel());
		// In case onParticipantsChanged() deleted an entry.
		it = mStatusObservers.upper_bound(observerp);
	}
}

// Determines if we do channel processing depending on whether we are running a
// WebRTC voice channel or one from another voice provider. Called every 100ms
// or so via the idle() callback, while WebRTC voice is enabled.
void LLVoiceWebRTC::processChannels()
{
	if (gDisconnected || LLApp::isQuitting() || mTerminated)
	{
		return;
	}

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || regionp->getRegionID().isNull() ||
		!isAgentAvatarValid())
	{
		return;
	}

	checkDevicesChanged();

	bool voice_enabled = mVoiceEnabled;

	if (!mProcessChannels)
	{
		// We have switched away from WebRTC voice, so shut all channels down.
		// Note that leaveChannel() can be called again and again without
		// adverse effects: it merely tells channels to shut down if they are
		// not already doing so.
		leaveChannel(false);
		return;
	}

	if (inSpatialChannel())
	{
		bool use_estate_voice = true;
		// Add session for region or parcel voice.
		if (!regionp->isVoiceEnabled())
		{
			voice_enabled = false;
		}
		if (voice_enabled)
		{
			LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
			if (parcelp && parcelp->getLocalID() != INVALID_PARCEL_ID)
			{
				if (!parcelp->getParcelFlagAllowVoice())
				{
					voice_enabled = false;
				}
				else if (!parcelp->getParcelFlagUseEstateVoiceChannel())
				{
					// Use the parcel-specific voice channel.
					use_estate_voice = false;
					S32 parcel_id = parcelp->getLocalID();
					std::string channel_id =
						llformat("%s-%d",
								 regionp->getRegionID().asString().c_str(),
								 parcel_id);
					if (!inOrJoiningChannel(channel_id))
					{
						startParcelSession(channel_id, parcel_id);
					}
				}
				if (voice_enabled && use_estate_voice && !inEstateChannel())
				{
					startEstateSession();
				}
			}
			if (voice_enabled)
			{
				// We are in spatial voice, and voice is enabled, so determine
				// positions in order to send position updates.
				updatePosition();
			}
			else
			{
				// Voice is disabled, so leave and disable PTT
				leaveChannel(true);
				// Also stop processing channels. HB
				mProcessChannels = false;
			}
		}
	}

	sessionState::processSessionStates();
#if 0	// mHidden is not used in the Cool VL Viewer
	if (voice_enabled && mProcessChannels && !mHidden)
#else
	if (voice_enabled && mProcessChannels)
#endif
	{
		sendPositionUpdate(false);
		updateOwnVolume();
	}
}

void LLVoiceWebRTC::updateNeighboringRegions()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	constexpr F64 SQRT2BY2 = 0.5 * F_SQRT2;
	static const std::vector<LLVector3d> neighbors
	{
		LLVector3d(0.0, 1.0, 0.0), LLVector3d(SQRT2BY2, SQRT2BY2, 0.0),
		LLVector3d(1.0, 0.0, 0.0),  LLVector3d(SQRT2BY2, -SQRT2BY2, 0.0),
		LLVector3d(0.0, -1.0, 0.0), LLVector3d(-SQRT2BY2, -SQRT2BY2, 0.0),
		LLVector3d(-1.0, 0.0, 0.0), LLVector3d(-SQRT2BY2, SQRT2BY2, 0.0)
	};

	// Estate voice requires connection to neighboring regions.
	mNeighboringRegions.clear();
	mNeighboringRegions.insert(gAgent.getRegion()->getRegionID());

	// Base off of speaker position as it will move more slowly than camera
	// position. Once we have hysteresis, we may be able to track off of
	// speaker and camera position at 50m.
	// *TODO: add hysteresis so we do not flip-flop connections to neighbors.
	LLVector3d pos;
	for (size_t i = 0, count = neighbors.size(); i < count; ++i)
	{
		const LLVector3d& neighbor_pos = neighbors[i];
		// Include every region within 100m to deal with the fact that the
		// camera can stray 50m away from the avatar.
		pos = mAvatarPosition + 100 * neighbor_pos;
		LLViewerRegion* regionp = gWorld.getRegionFromPosGlobal(pos);
		if (regionp && regionp->getRegionID().notNull())
		{
			mNeighboringRegions.insert(regionp->getRegionID());
		}
	}
}

//virtual
void LLVoiceWebRTC::leaveAudioSession()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	if (mSession)
	{
		LL_DEBUGS("Voice") << "Leaving session: " << mSession->mChannelID
						   << LL_ENDL;
		mSession->shutdownAllConnections();
	}
	else
	{
		llwarns << "Called with no active session" << llendl;
	}
}

void LLVoiceWebRTC::checkDevicesChanged()
{
	if (!mWebRTCPlugin)
	{
		return;
	}

	mWebRTCPlugin->lockWebRTCData();
	LLSD& capture = mWebRTCPlugin->getCaptureDevices();
	if (capture.isMap())
	{
		mCaptureDevices.clear();
		for (LLSD::map_const_iterator it = capture.beginMap(),
									  end = capture.endMap();
			 it != end; ++it)
		{
			addCaptureDevice(it->first, it->second.asString());
		}
		capture.clear();
	}
	LLSD& render = mWebRTCPlugin->getRenderDevices();
	if (render.isMap())
	{
		mRenderDevices.clear();
		for (LLSD::map_const_iterator it = render.beginMap(),
									  end = render.endMap();
			 it != end; ++it)
		{
			addRenderDevice(it->first, it->second.asString());
		}
		render.clear();
	}
	mNoCaptureDevice = mCaptureDevices.empty();
	mNoRenderDevice = mRenderDevices.empty();
	mDeviceSettingsAvailable = !mNoCaptureDevice && !mNoRenderDevice;
	mWebRTCPlugin->unlockWebRTCData();
}

void LLVoiceWebRTC:: addCaptureDevice(const std::string& display_name,
									  const std::string& device_id)
{
	// Yep, it may happen... In this case, and since we will not be able to
	// set (and therefore use) this device, just skip it.  HB
	if (device_id.empty())
	{
		llwarns << "Got an empty device Id for render device name: "
				<< display_name << llendl;
		return;
	}
	if (display_name.empty())	// Could happen, I suppose... HB
	{
		mCaptureDevices.emplace(device_id, device_id);
	}
	else	// Normal case
	{
		mCaptureDevices.emplace(display_name, device_id);
	}
}

void LLVoiceWebRTC::setCaptureDevice(const std::string& device_id)
{
	if (!mWebRTCPlugin || device_id == mCaptureDevice)
	{
		return;
	}
	mCaptureDevice = device_id;
	if (device_id.empty())
	{
		// Interpret as "Default". HB
		llinfos << "Setting audio capture device to: Default" << llendl;
		mWebRTCPlugin->setCaptureDevice("Default");
	}
	else
	{
		llinfos << "Setting audio capture device to: " << device_id << llendl;
		mWebRTCPlugin->setCaptureDevice(device_id);
	}
	setMicGain(-1.f);	// Force a mic level update. HB
}

void LLVoiceWebRTC::addRenderDevice(const std::string& display_name,
									const std::string& device_id)
{
	// Yep, it may happen... In this case, and since we will not be able to
	// set (and therefore use) this device, just skip it.  HB
	if (device_id.empty())
	{
		llwarns << "Got an empty device Id for render device name: "
				<< display_name << llendl;
		return;
	}
	if (display_name.empty())	// Could happen, I suppose... HB
	{
		mRenderDevices.emplace(device_id, device_id);
	}
	else	// Normal case
	{
		mRenderDevices.emplace(display_name, device_id);
	}
}

void LLVoiceWebRTC::setRenderDevice(const std::string& device_id)
{
	if (!mWebRTCPlugin || device_id == mRenderDevice)
	{
		return;
	}
	mRenderDevice = device_id;
	if (device_id.empty())
	{
		// Interpret as "Default". HB
		llinfos << "Setting audio render device to: Default" << llendl;
		mWebRTCPlugin->setRenderDevice("Default");
	}
	else
	{
		llinfos << "Setting audio render device to: " << device_id << llendl;
		mWebRTCPlugin->setRenderDevice(device_id);
	}
}

void LLVoiceWebRTC::refreshDeviceLists(bool clear_current_list)
{
	if (clear_current_list)
	{
		LL_DEBUGS("Voice") << "Clearing current list of audio devices"
						   << LL_ENDL;
		mDeviceSettingsAvailable = false;
		mNoCaptureDevice = mNoRenderDevice = true;
		mCaptureDevices.clear();
		mRenderDevices.clear();
	}
	if (mWebRTCPlugin)
	{
		LL_DEBUGS("Voice") << "Requesting audio devices list..." << LL_ENDL;
		mWebRTCPlugin->refreshDevices();
	}
}

void LLVoiceWebRTC::setTuningMode(bool tuning_on)
{
	if (mIsInTuningMode != tuning_on && mWebRTCPlugin)
	{
		mIsInTuningMode = tuning_on;
		mWebRTCPlugin->setTuningMode(tuning_on);
		llinfos << (tuning_on ? "Entering" : "Exiting") << " tuning mode."
				<< llendl;
	}
}

F32 LLVoiceWebRTC::getAudioLevel()
{
	if (!mWebRTCPlugin || mNoCaptureDevice)
	{
		return 0.f;
	}
	F32 rms = mWebRTCPlugin->getAudioLevel();
	if (mIsInTuningMode)
	{
		constexpr F32 TUNING_LEVEL_START_POINT = 0.8f;
		constexpr F32 TUNING_LEVEL_SCALE = 0.01f;
		return TUNING_LEVEL_START_POINT - TUNING_LEVEL_SCALE * rms;
	}
	constexpr F32 LEVEL_START_POINT = 0.18f;
	constexpr F32 LEVEL_SCALE = 0.005f;
	return LEVEL_START_POINT - LEVEL_SCALE * rms;
}

// The user's mute list has been updated. This method goes through the current
// participants list and syncs it with the mute list.
void LLVoiceWebRTC::muteListChanged()
{
	sessionState::forEach(std::bind(predMuteListChanged, _1));
}

//static
void LLVoiceWebRTC::predMuteListChanged(const sessionState::ptr_t& sessionp)
{
	sessionp->muteListChanged();
}

void LLVoiceWebRTC::onConnectionEstablished(const std::string& channel_id,
											const LLUUID& region_id)
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || regionp->getRegionID() != region_id)
	{
		return;
	}

	if (mNextSession && mNextSession->mChannelID == channel_id)
	{
		if (mSession)
		{
			mSession->shutdownAllConnections();
		}
		mSession = mNextSession;
		mNextSession.reset();
		// Add ourselves as a participant.
		if (mSession)
		{
			LLViewerRegion* regionp = gAgent.getRegion();
			if (regionp)
			{
				mSession->addParticipant(gAgentID, regionp->getRegionID());
			}
		}
	}

	if (mSession && mSession->mChannelID == channel_id)
	{
		// The current session was established.
		notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);
		// Only set status to joined if asked to. This will happen in the case
		// where we are not doing an ad-hoc based p2p session. Those sessions
		// expect a STATUS_JOINED when the peer has, in fact, joined, which we
		// detect elsewhere.
		if (!mSession->mNotifyOnFirstJoin)
		{
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
		}
	}
}

void LLVoiceWebRTC::onConnectionShutDown(const std::string& channel_id,
										 const LLUUID& region_id)
{
	if (mSession && mSession->mChannelID == channel_id)
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		if (regionp && regionp->getRegionID() == region_id)
		{
			mSession->removeAllParticipants(region_id);
		}
	}
}

void LLVoiceWebRTC::onConnectionFailure(const std::string& channel_id,
										const LLUUID& region_id,
										LLVoiceClientStatusObserver::EStatusType status)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || regionp->getRegionID() != region_id)
	{
		return;
	}

	if ((mNextSession && mNextSession->mChannelID == channel_id) ||
		(mSession && mSession->mChannelID == channel_id))
	{
		notifyStatusObservers(status);
	}
}

LLVoiceWebRTC::particip_map_t* LLVoiceWebRTC::getParticipantList()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	particip_map_t* result = NULL;
	if (mProcessChannels && mSession)
	{
		result = &(mSession->mParticipantsByUUID);
	}
	return result;
}

bool LLVoiceWebRTC::isParticipant(const LLUUID& id)
{
	return mProcessChannels && mSession &&
		   mSession->mParticipantsByUUID.count(id);
}

LLVoiceWebRTC::pstate_ptr_t LLVoiceWebRTC::findParticipantByID(const std::string& channel_id,
															   const LLUUID& id)
{
	pstate_ptr_t result;

	sessionState::ptr_t sessionp =
		sessionState::matchSessionByChannelID(channel_id);
	if (sessionp)
	{
		result = sessionp->findParticipantByID(id);
	}

	return result;
}

LLVoiceWebRTC::pstate_ptr_t LLVoiceWebRTC::addParticipantByID(const std::string& channel_id,
															  const LLUUID& id,
															  const LLUUID& region_id)
{
	pstate_ptr_t result;

	sessionState::ptr_t sessionp =
		sessionState::matchSessionByChannelID(channel_id);
	if (sessionp)
	{
		LL_DEBUGS("Voice") << "Adding participant " << id << " to channel "
						   << channel_id << LL_ENDL;
		result = sessionp->addParticipant(id, region_id);
		if (sessionp->mNotifyOnFirstJoin && id != gAgentID)
		{
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
		}
	}

	return result;
}

void LLVoiceWebRTC::removeParticipantByID(const std::string& channel_id,
										  const LLUUID& id,
										  const LLUUID& region_id)
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	pstate_ptr_t result;
	sessionState::ptr_t sessionp =
		sessionState::matchSessionByChannelID(channel_id);
	if (!sessionp)
	{
		return;
	}
	pstate_ptr_t participantp = sessionp->findParticipantByID(id);
	if (!participantp || participantp->mRegion != region_id)
	{
		return;
	}

	LL_DEBUGS("Voice") << "Removing participant " << id << " from channel "
					   << channel_id << LL_ENDL;
	sessionp->removeParticipant(participantp);
}

void LLVoiceWebRTC::startEstateSession()
{
	leaveChannel(false);
	mNextSession = addSession("Estate",
							  sessionState::ptr_t(new estateSessionState()));
}

void LLVoiceWebRTC::startParcelSession(const std::string& channel_id,
									   S32 parcel_id)
{
	leaveChannel(false);
	mNextSession =
		addSession(channel_id,
				   sessionState::ptr_t(new parcelSessionState(channel_id,
															  parcel_id)));
}

void LLVoiceWebRTC::setSpatialChannel(const LLSD& channel_info)
{
	if (!channel_info.isMap() || !channel_info.has("channel_uri"))
	{
		return;
	}
	bool allow_voice = !channel_info["channel_uri"].asString().empty();

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || !isAgentAvatarValid())
	{
		return;
	}
	LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
	if (parcelp)
	{
		parcelp->setParcelFlag(PF_ALLOW_VOICE_CHAT, allow_voice);
		bool use_estate =
			channel_info["channel_uri"].asUUID() == regionp->getRegionID();
		parcelp->setParcelFlag(PF_USE_ESTATE_VOICE_CHAN, use_estate);
	}
	else
	{
		regionp->setRegionFlag(REGION_FLAGS_ALLOW_VOICE, allow_voice);
	}
}

// This is synonymous to startAdHocSession() in LL's sources. HB
//virtual
void LLVoiceWebRTC::setNonSpatialChannel(const LLSD& channel_info,
										 bool notify_join,
										 bool hangup_on_last)
{
	leaveChannel(false);
	LL_DEBUGS("Voice") << "Starting AdHoc session: " << channel_info
					   << LL_ENDL;
	std::string channel_id = channel_info["channel_uri"];
	std::string credentials = channel_info["channel_credentials"];
	mNextSession =
		addSession(channel_id,
				   sessionState::ptr_t(new adhocSessionState(channel_id,
															 credentials,
															 notify_join,
															 hangup_on_last)));
}

//virtual
void LLVoiceWebRTC::leaveNonSpatialChannel()
{
	LL_DEBUGS("Voice") << "Request to leave non-spatial channel." << LL_ENDL;
	deleteSession(mNextSession);
	leaveChannel(true);
}

bool LLVoiceWebRTC::inOrJoiningChannel(const std::string& channel_id)
{
	return (mSession && mSession->mChannelID == channel_id) ||
		   (mNextSession && mNextSession->mChannelID == channel_id);
}

bool LLVoiceWebRTC::inEstateChannel()
{
	return (mSession && mSession->isEstate()) ||
		   (mNextSession && mNextSession->isEstate());
}

bool LLVoiceWebRTC::inSpatialChannel() const
{
	if (mNextSession)
	{
		return mNextSession->isSpatial();
	}
	if (mSession)
	{
		return mSession->isSpatial();
	}
	return true;
}

LLSD LLVoiceWebRTC::getAudioSessionChannelInfo()
{
	LLSD result;
	if (mSession)
	{
		result["voice_server_type"] = WEBRTCSTR;
		result["channel_uri"] = mSession->mChannelID;
	}
	return result;
}

void LLVoiceWebRTC::leaveChannel(bool stop_talking)
{
	if (mSession)
	{
		LL_DEBUGS("Voice") << "Leaving channel for teleport/logout" << LL_ENDL;
		deleteSession(mSession);
	}

	if (mNextSession)
	{
		LL_DEBUGS("Voice") << "Leaving next channel for teleport/logout"
						   << LL_ENDL;
		deleteSession(mNextSession);
	}

	if (stop_talking && gVoiceClient.getUserPTTState())
	{
		gVoiceClient.setUserPTTState(false);
	}
}

bool LLVoiceWebRTC::isCurrentChannel(const LLSD& channel_info)
{
	if (!mProcessChannels || !mSession ||
		channel_info["voice_server_type"].asString() != WEBRTCSTR)
	{
		return false;
	}
	if (mSession)
	{
		if (!channel_info["session_handle"].asString().empty())
		{
			return mSession->mHandle == channel_info["session_handle"].asString();
		}
		return channel_info["channel_uri"].asString() == mSession->mChannelID;
	}
	return false;
}

bool LLVoiceWebRTC::compareChannels(const LLSD& info1, const LLSD& info2)
{
	return info1["voice_server_type"].asString() == WEBRTCSTR &&
		   info2["voice_server_type"].asString() == WEBRTCSTR &&
		   info1["sip_uri"].asString() == info2["sip_uri"].asString();
}

LLVoiceWebRTC::sessionState::ptr_t
LLVoiceWebRTC::addSession(const std::string& channel_id,
						  sessionState::ptr_t sessionp)
{
#if 0	// Reviving does not properly work... Any old session will then be
		// deleted when the call to sessionState::addSession() will overwrite
		// the smart pointer in sSessions. HB
	sessionState::ptr_t old_sessionp =
		sessionState::matchSessionByChannelID(channel_id);
	if (old_sessionp)
	{
		LL_DEBUGS("Voice") << "Reviving existing session for channel: "
						   << channel_id << LL_ENDL;
		old_sessionp->revive();
		return old_sessionp;
	}
#endif

	LL_DEBUGS("Voice") << "Adding a new session for channel: " << channel_id
					   << LL_ENDL;
	sessionp->setMuteMic(mMuteMic);
	sessionp->setSpeakerVolume(mSpeakerVolume);
	sessionState::addSession(channel_id, sessionp);
	return sessionp;
}

LLVoiceWebRTC::sessionState::ptr_t LLVoiceWebRTC::findP2PSession(const LLUUID& id)
{
	sessionState::ptr_t sessionp =
		sessionState::matchSessionByChannelID(id.asString());
	if (sessionp && !sessionp->isSpatial())
	{
		return sessionp;
	}
	sessionp.reset();
	return sessionp;
}

void LLVoiceWebRTC::deleteSession(const sessionState::ptr_t& sessionp)
{
	if (!sessionp) return;

	// At this point, the session should be unhooked from all lists and all
	// states should be consistent.
	sessionp->shutdownAllConnections();

	if (sessionp == mSession)
	{
		mSession.reset();
	}
	else if (sessionp == mNextSession)
	{
		mNextSession.reset();
	}
}

#if 0	// Not used in the Cool VL Viewer
bool LLVoiceWebRTC::setHidden(bool hidden)
{
	if (mHidden == hidden)
	{
		return;
	}
	if (inSpatialChannel())
	{
		if (mHidden)
		{
			// Get out of the channel entirely; mute the microphone.
			if (mWebRTCPlugin)
			{
				mWebRTCPlugin->setGlobalMute(true, 0);
			}
		}
		else	// Put it back
		{
			if (mWebRTCPlugin)
			{
				// Delay for 200ms so as to not pile up mutes/unmutes.
				constexpr F32 HIDDEN_RESTORE_DELAY_MS = 200;
				mWebRTCPlugin->setGlobalMute(mMuteMic,
											 HIDDEN_RESTORE_DELAY_MS);
			}
			updatePosition();
			sendPositionUpdate(true);
		}
	}
}
#endif

void LLVoiceWebRTC::setMuteMic(bool muted)
{
	if (mMuteMic != muted)
	{
		llinfos << "Microphone " << (muted ? " muted" : " unmuted") << llendl;
	}
	mMuteMic = muted;
	if (mIsInTuningMode)
	{
		return;
	}
	if (mWebRTCPlugin)
	{
		// 20ms fade followed by 480ms silence gets rid of the click just after
		// unmuting.
		constexpr F32 MUTE_FADE_DELAY_MS = 500;
		mWebRTCPlugin->setGlobalMute(muted, muted ? MUTE_FADE_DELAY_MS : 0);
	}
#if 0	// Not used in the Cool VL Viewer
	if (!mHidden)
#endif
	{
		sessionState::forEach(std::bind(predSetMuteMic, _1, muted));
	}
}

//static
void LLVoiceWebRTC::predSetMuteMic(const sessionState::ptr_t& sessionp,
								   bool muted)
{
	pstate_ptr_t participantp = sessionp->findParticipantByID(gAgentID);
	if (participantp)
	{
		participantp->mLevel = 0.f;
	}
	sessionp->setMuteMic(muted);
}

void LLVoiceWebRTC::setVoiceVolume(F32 volume)
{
	if (mSpeakerVolume != volume)
	{
		mSpeakerVolume = volume;
		sessionState::forEach(std::bind(predSetSpeakerVolume, _1, volume));
	}
}

//static
void LLVoiceWebRTC::predSetSpeakerVolume(const sessionState::ptr_t& sessionp,
										 F32 volume)
{
	sessionp->setSpeakerVolume(volume);
}

void LLVoiceWebRTC::tuningSetMicVolume(F32 volume)
{
	if (mTuningMicGain != volume && mWebRTCPlugin)
	{
		mWebRTCPlugin->setTuningMicGain(volume);
	}
	mTuningMicGain = volume;
}

void LLVoiceWebRTC::setMicGain(F32 gain)
{
	if (mMicGain != gain)
	{
		// Note: a negative 'gain' reasserts last mMicGain value. HB
		if (gain >= 0.f)
		{
			mMicGain = gain;
		}
		if (mWebRTCPlugin)
		{
			mWebRTCPlugin->setMicGain(mMicGain);
		}
	}
}

void LLVoiceWebRTC::setEarLocation(S32 loc)
{
	if (mEarLocation != loc && loc >= 0 && loc <= (S32)earLocMixed)
	{
		LL_DEBUGS("Voice") << "Setting ear location to " << loc << LL_ENDL;
		mEarLocation = loc;
		mSpatialCoordsDirty = true;
	}
}

void LLVoiceWebRTC::updatePosition()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	if (mTerminated || !mInitDone)
	{
		return;
	}

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp || !isAgentAvatarValid())
	{
		return;
	}

	LLVector3d agent_pos = gAgentAvatarp->getPositionGlobal();
	agent_pos.mdV[VZ] += 1.0;	// Bump it up to head height
	LLQuaternion agent_rot = gAgentAvatarp->getRootJoint()->getWorldRotation();

	LLVector3d ear_pos;
	LLQuaternion ear_rot;
	switch (mEarLocation)
	{
		case earLocCamera:
			ear_pos =
				regionp->getPosGlobalFromRegion(gViewerCamera.getOrigin());
			ear_rot = gViewerCamera.getQuaternion();
			break;

		case earLocAvatar:
			ear_pos = mAvatarPosition;
			ear_rot = mAvatarRot;
			break;

		case earLocMixed:
			ear_pos = mAvatarPosition;
			ear_rot = gViewerCamera.getQuaternion();
			break;

		default:
			llerrs << "Invalid ear location: " << mEarLocation << llendl;
	}

	setListenerPosition(ear_pos, LLVector3::zero, ear_rot);
	setAvatarPosition(agent_pos, LLVector3::zero, agent_rot);
	enforceTether();
	updateNeighboringRegions();

	// Update own region Id to be the region Id avatar is currently in.
	pstate_ptr_t participantp = findParticipantByID("Estate", gAgentID);
	if (participantp)
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		if (regionp && participantp->mRegion != regionp->getRegionID())
		{
			participantp->mRegion = regionp->getRegionID();
			setMuteMic(mMuteMic);
		}
	}
}

void LLVoiceWebRTC::setListenerPosition(const LLVector3d& position,
										const LLVector3& velocity,
										const LLQuaternion& rot)
{
	mListenerRequestedPosition = position;

	if (mListenerVelocity != velocity)
	{
		mListenerVelocity = velocity;
		mSpatialCoordsDirty = true;
	}

	if (mListenerRot != rot)
	{
		mListenerRot = rot;
		mSpatialCoordsDirty = true;
	}
}

void LLVoiceWebRTC::setAvatarPosition(const LLVector3d& position,
									  const LLVector3& velocity,
									  const LLQuaternion& rot)
{
	if (dist_vec(mAvatarPosition, position) > 0.1)
	{
		mAvatarPosition = position;
		mSpatialCoordsDirty = true;
	}

	if (mAvatarVelocity != velocity)
	{
		mAvatarVelocity = velocity;
		mSpatialCoordsDirty = true;
	}

	// If the two rotations are not exactly equal test their dot product to get
	// the cosinus of the angle between them. If too small, do not update.
	if (mAvatarRot != rot)
	{
		static const F32 minuscule_angle_cos = cosf(2.f * DEG_TO_RAD);
		if (fabs(dot(mAvatarRot, rot)) < minuscule_angle_cos)
		{
			mAvatarRot = rot;
			mSpatialCoordsDirty = true;
		}
	}
}

void LLVoiceWebRTC::enforceTether()
{
	LLVector3d tethered	= mListenerRequestedPosition;

	// Constrain 'tethered' to within 50m of mAvatarPosition.
	constexpr F32 max_dist = 50.f;
	LLVector3d camera_offset = mListenerRequestedPosition - mAvatarPosition;
	F32 camera_distance = (F32)camera_offset.length();
	if (camera_distance > max_dist)
	{
		tethered = mAvatarPosition +
				   (max_dist / camera_distance) * camera_offset;
	}

	if (dist_vec_squared(mListenerPosition, tethered) > 0.01)
	{
		mListenerPosition = tethered;
		mSpatialCoordsDirty = true;
	}
}

void LLVoiceWebRTC::sendPositionUpdate(bool force)
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	if (force || mSpatialCoordsDirty)
	{
		lljson spatial = lljson::object();
		spatial["sp"] = lljson::object();
		spatial["sp"]["x"] = (S32)(mAvatarPosition[0] * 100.0);
		spatial["sp"]["y"] = (S32)(mAvatarPosition[1] * 100.0);
		spatial["sp"]["z"] = (S32)(mAvatarPosition[2] * 100.0);
		spatial["sh"] = lljson::object();
		spatial["sh"]["x"] = (S32)(mAvatarRot[0] * 100.0);
		spatial["sh"]["y"] = (S32)(mAvatarRot[1] * 100.0);
		spatial["sh"]["z"] = (S32)(mAvatarRot[2] * 100.0);
		spatial["sh"]["w"] = (S32)(mAvatarRot[3] * 100.0);
		spatial["lp"] = lljson::object();
		spatial["lp"]["x"] = (S32)(mListenerPosition[0] * 100.0);
		spatial["lp"]["y"] = (S32)(mListenerPosition[1] * 100.0);
		spatial["lp"]["z"] = (S32)(mListenerPosition[2] * 100.0);
		spatial["lh"] = lljson::object();
		spatial["lh"]["x"] = (S32)(mListenerRot[0] * 100.0);
		spatial["lh"]["y"] = (S32)(mListenerRot[1] * 100.0);
		spatial["lh"]["z"] = (S32)(mListenerRot[2] * 100.0);
		spatial["lh"]["w"] = (S32)(mListenerRot[3] * 100.0);
		std::string spatial_data = to_string(spatial);
		sessionState::forEach(std::bind(predSendData, _1, spatial_data));
		mSpatialCoordsDirty = false;
	}
}

void LLVoiceWebRTC::updateOwnVolume()
{
	F32 audio_level = 0.f;
	if (!mMuteMic && !mIsInTuningMode)
	{
		audio_level = getAudioLevel();
		sessionState::forEach(std::bind(predUpdateOwnVolume, _1, audio_level));
	}
}

void LLVoiceWebRTC::setVoiceEnabled(bool enabled)
{
	if (mVoiceEnabled == enabled)
	{
		return;
	}
	mVoiceEnabled = enabled;
	sLastTunedChannelId.clear();

	LLVoiceClientStatusObserver::EStatusType status;
	if (enabled)
	{
		LLVoiceChannel::getCurrentVoiceChannel()->activate();
		status = LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED;
		mSpatialCoordsDirty = true;
		updatePosition();
	}
	else
	{
		// Turning voice off looses your current channel: this makes sure the
		// UI is not out of sync when you re-enable it.
		LLVoiceChannel::getCurrentVoiceChannel()->deactivate();
		status = LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED;
		cleanUp();
	}
	notifyStatusObservers(status);
}

bool LLVoiceWebRTC::getIsSpeaking(const LLUUID& id)
{
	if (mProcessChannels && mSession)
	{
		// Since WebRTC accounts only for the mic input level for the agent
		// (and not the level of the transmitted voice), we must ignore the
		// speaking flag when the mic toggle is off (no voice transmitted). HB
		if (id == gAgentID && !gVoiceClient.isAgentMicOpen())
		{
			return false;
		}
		pstate_ptr_t participantp = mSession->findParticipantByID(id);
		if (participantp)
		{
			return participantp->mIsSpeaking;
		}
	}
	return false;
}

bool LLVoiceWebRTC::getIsModeratorMuted(const LLUUID& id)
{
	if (mProcessChannels && mSession)
	{
		pstate_ptr_t participantp = mSession->findParticipantByID(id);
		if (participantp)
		{
			return participantp->mIsModeratorMuted;
		}
	}
	return false;
}

F32 LLVoiceWebRTC::getCurrentPower(const LLUUID& id)
{
	if (!mProcessChannels || !mSession)
	{
		return -1.f;	// Id not in session
	}
	pstate_ptr_t participantp = mSession->findParticipantByID(id);
	if (!participantp)
	{
		return -1.f;	// Id not in session
	}
	// Since WebRTC reports the mic input level for the agent (and not the
	// level of the transmitted voice), we must set the power to zero when the
	// mic toggle is off (no voice transmitted). HB
	if (id == gAgentID && !gVoiceClient.isAgentMicOpen())
	{
		return 0.f;
	}
	return participantp->mIsSpeaking ? participantp->mLevel : 0.f;
}

// External accessiors. Maps 0.0 to 1.0 to internal values 0-400 with .5 == 100
// internal = 400 * external^2
F32 LLVoiceWebRTC::getUserVolume(const LLUUID& id)
{
	if (!mSession)
	{
		return -1.f;
	}
	pstate_ptr_t participantp = mSession->findParticipantByID(id);
	if (!participantp)
	{
		return -1.f;	// Id not in session
	}
	// Since WebRTC reports the mic input level for the agent (and not the
	// level of the transmitted voice), we must set the volume to zero when the
	// mic toggle is off (no voice transmitted). HB
	if (id == gAgentID && !gVoiceClient.isAgentMicOpen())
	{
		return 0.f;
	}
	return participantp->mVolume;
}

void LLVoiceWebRTC::setUserVolume(const LLUUID& id, F32 volume)
{
	if (mSession)
	{
		pstate_ptr_t participantp = mSession->findParticipantByID(id);
		if (participantp)
		{
			participantp->mVolume = volume;
		}
	}
	sessionState::forEach(std::bind(predSetUserVolume, _1, id, volume));
}

//static
void LLVoiceWebRTC::predSetUserVolume(const sessionState::ptr_t& sessionp,
									  const LLUUID& id, F32 volume)
{
	sessionp->setUserVolume(id, volume);
}

//static
void LLVoiceWebRTC::predUpdateOwnVolume(const sessionState::ptr_t& sessionp,
										F32 audio_level)
{
	pstate_ptr_t participantp = sessionp->findParticipantByID(gAgentID);
	if (participantp)
	{
		participantp->mLevel = audio_level;
		// *TODO: add VAD for our own voice.
		constexpr F32 SPEAKING_AUDIO_LEVEL = 0.3f;
		participantp->mIsSpeaking = audio_level > SPEAKING_AUDIO_LEVEL;
	}
}

//static
void LLVoiceWebRTC::predSendData(const sessionState::ptr_t& sessionp,
								 const std::string& spatial_data)
{
	if (sessionp->isSpatial() && !spatial_data.empty())
	{
		sessionp->sendData(spatial_data);
	}
}

void LLVoiceWebRTC::lookupName(const LLUUID& id)
{
	if (gCacheNamep)
	{
		gCacheNamep->get(id, false, onAvatarNameLookup);
	}
}

//static
void LLVoiceWebRTC::onAvatarNameLookup(const LLUUID& id,
									   const std::string& fullname, bool)
{
	if (!gVoiceWebRTC.mTerminated)
	{
		gVoiceWebRTC.avatarNameResolved(id, fullname);
	}
}

void LLVoiceWebRTC::avatarNameResolved(const LLUUID& id,
									   const std::string& name)
{
	sessionState::forEach(std::bind(predAvatarName, _1, id, name));
}

//static
void LLVoiceWebRTC::predAvatarName(const sessionState::ptr_t& sessionp,
								   const LLUUID& id, const std::string& name)
{
	auto participantp = sessionp->findParticipantByID(id);
	if (participantp)
	{
		participantp->mLegacyName = name;
#if 0	// Not used in the Cool VL Viewer
		gVoiceWebRTC.notifyParticipantObservers();
#endif
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceWebRTC::sessionState sub-class
///////////////////////////////////////////////////////////////////////////////

LLVoiceWebRTC::sessionState::map_t LLVoiceWebRTC::sessionState::sSessions;

LLVoiceWebRTC::sessionState::sessionState()
:	mShuttingDown(false),
	mHangupOnLastLeave(false),
	mNotifyOnFirstJoin(false),
	mSpeakerVolume(1.f),
	mMuted(false)
{
}

LLVoiceWebRTC::sessionState::~sessionState()
{
	removeAllParticipants(LLUUID::null);	// Remove in all regions
}

//static
void LLVoiceWebRTC::sessionState::forEachPredicate(const std::pair<std::string,
																    wptr_t>& a,
												   func_t fn)
{
	ptr_t a_lck(a.second.lock());
	if (a_lck)
	{
		fn(a_lck);
	}
	else
	{
		llwarns << "Stale handle in session map." << llendl;
	}
}

//static
void LLVoiceWebRTC::sessionState::forEach(func_t fn)
{
	std::for_each(sSessions.begin(), sSessions.end(),
				  std::bind(forEachPredicate, _1, fn));
}

bool LLVoiceWebRTC::sessionState::isEmpty()
{
	return mConnections.empty();
}

//static
void LLVoiceWebRTC::sessionState::addSession(const std::string& channel_id,
											 ptr_t& sessionp)
{
	sSessions[channel_id] = sessionp;
}

//static
LLVoiceWebRTC::sessionState::ptr_t
LLVoiceWebRTC::sessionState::matchSessionByChannelID(const std::string& chan_id)
{
	ptr_t result;
	map_t::iterator it = sSessions.find(chan_id);
	if (it != sSessions.end())
	{
		result = it->second;
	}
	return result;
}

LLVoiceWebRTC::pstate_ptr_t LLVoiceWebRTC::sessionState::findParticipantByID(const LLUUID& id)
{
	pstate_ptr_t result;
	auto iter = mParticipantsByUUID.find(id);
	if (iter != mParticipantsByUUID.end())
	{
		result = iter->second;
	}
	return result;
}

LLVoiceWebRTC::pstate_ptr_t LLVoiceWebRTC::sessionState::addParticipant(const LLUUID& id,
																		const LLUUID& region_id)
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	if (gVoiceWebRTC.isTerminated())
	{
		llwarns << "Trying to add a parcipant after voice shutdown." << llendl;
		return NULL;
	}

	pstate_ptr_t participantp;

	particip_map_t::iterator it = mParticipantsByUUID.find(id);
	if (it != mParticipantsByUUID.end())
	{
		participantp = it->second;
		participantp->mRegion = region_id;
	}
	else
	{
		participantp.reset(new participantState(id, region_id));
		mParticipantsByUUID.emplace(id, participantp);
		if (!gVoiceWebRTC.isTerminated())
		{
			gVoiceWebRTC.lookupName(id);
		}
	}
	LL_DEBUGS("Voice") << "Participant: " << participantp->mURI << LL_ENDL;

#if 0	// Not used in the Cool VL Viewer
	gVoiceWebRTC.notifyParticipantObservers();
#endif

	return participantp;
}

void LLVoiceWebRTC::sessionState::removeParticipant(pstate_ptr_t participantp)
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	if (gVoiceWebRTC.isTerminated() || !participantp)
	{
		return;
	}

	LLUUID part_id = participantp->mAvatarID;
	auto iter = mParticipantsByUUID.find(part_id);
	if (iter == mParticipantsByUUID.end())
	{
		llwarns << "Internal error: participant "
				<< part_id << " not in UUID map" << llendl;
	}
	else
	{
		LL_DEBUGS("Voice") << "Participant \"" << participantp->mURI <<  "\" ("
						   << part_id << ") removed." << LL_ENDL;
		mParticipantsByUUID.erase(iter);
#if 0	// Not used in the Cool VL Viewer
		gVoiceWebRTC.notifyParticipantObservers();
#endif
	}


	if (mHangupOnLastLeave && part_id != gAgentID &&
		mParticipantsByUUID.size() <= 1)
	{
		gVoiceWebRTC.notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
	}
}

void LLVoiceWebRTC::sessionState::removeAllParticipants(const LLUUID& region_id)
{
	if (gVoiceWebRTC.isTerminated())
	{
		return;
	}

	bool all_regions = region_id.isNull();
	std::vector<pstate_ptr_t> to_remove;
	for (particip_map_t::iterator it = mParticipantsByUUID.begin(),
								  end = mParticipantsByUUID.end();
		 it != end; ++it)
	{
		if (all_regions || it->second->mRegion == region_id)
		{
			to_remove.emplace_back(it->second);
		}
	}
	for (size_t i = 0, count = to_remove.size(); i < count; ++i)
	{
		removeParticipant(to_remove[i]);
	}
}

void LLVoiceWebRTC::sessionState::sendData(const std::string& data)
{
	if (!gVoiceWebRTC.isTerminated())
	{
		for (auto& connectionp : mConnections)
		{
			connectionp->sendData(data);
		}
	}
}

void LLVoiceWebRTC::sessionState::setMuteMic(bool muted)
{
	if (!gVoiceWebRTC.isTerminated())
	{
		mMuted = muted;
		for (auto& connectionp : mConnections)
		{
			connectionp->setMuteMic(muted);
		}
	}
}

void LLVoiceWebRTC::sessionState::setSpeakerVolume(F32 volume)
{
	if (!gVoiceWebRTC.isTerminated())
	{
#if 0	// Avoid race conditions while devices are being fetched. HB
		mSpeakerVolume = mNoRenderDevice ? 0.f : volume;
#else
		mSpeakerVolume = volume;
#endif
		for (auto& connectionp : mConnections)
		{
			connectionp->setSpeakerVolume(volume);
		}
	}
}

void LLVoiceWebRTC::sessionState::setUserVolume(const LLUUID& id, F32 volume)
{
	if (!gVoiceWebRTC.isTerminated() && mParticipantsByUUID.count(id))
	{
		for (auto& connectionp : mConnections)
		{
			connectionp->setUserVolume(id, volume);
		}
	}
}

void LLVoiceWebRTC::sessionState::setUserMute(const LLUUID& id, bool muted)
{
	if (!gVoiceWebRTC.isTerminated() && mParticipantsByUUID.count(id))
	{
		for (auto& connectionp : mConnections)
		{
			connectionp->setUserMute(id, muted);
		}
	}
}

void LLVoiceWebRTC::sessionState:: muteListChanged()
{
	for (particip_map_t::iterator it = mParticipantsByUUID.begin(),
								  end = mParticipantsByUUID.end();
		 it != end; ++it)
	{
		bool muted = LLMuteList::isMuted(it->first, LLMute::flagVoiceChat);
		if (it->second->mIsMuted != muted)
		{
			setUserMute(it->first, muted);
		}
	}
}

//static
void LLVoiceWebRTC::sessionState::processSessionStates()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	auto it = sSessions.begin();
	while (it != sSessions.end())
	{
		if (!it->second->processConnectionStates() &&
			it->second->mShuttingDown)
		{
			// If the connections associated with a session are gone, and this
			// session is shutting down, remove it.
			it = sSessions.erase(it);
		}
		else
		{
			++it;
		}
	}
}

//virtual
bool LLVoiceWebRTC::sessionState::processConnectionStates()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	auto it = mConnections.begin();
	while (it != mConnections.end())
	{
		if (!it->get()->connectionStateMachine())
		{
			// If the state machine returns false, the connection is shut down
			// so delete it.
			it = mConnections.erase(it);
		}
		else
		{
			++it;
		}
	}

	return !mConnections.empty();
}

void LLVoiceWebRTC::sessionState::reconnectAllSessions()
{
	for (auto&& connectionp : mConnections)
	{
		connectionp->pluginCreateSession();
	}
}

void LLVoiceWebRTC::sessionState::shutdownAllConnections()
{
	mShuttingDown = true;
	for (auto&& connectionp : mConnections)
	{
		connectionp->shutDown();
	}
}

//static
void LLVoiceWebRTC::sessionState::reapEmptySessions()
{
	LL_TRACY_TIMER(TRC_WEBRTC_VOICE);

	auto it = sSessions.begin();
	while (it != sSessions.end())
	{
		if (it->second->isEmpty())
		{
			it = sSessions.erase(it);
		}
		else
		{
			++it;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceWebRTC::estateSessionState sub-class
///////////////////////////////////////////////////////////////////////////////

LLVoiceWebRTC::estateSessionState::estateSessionState()
{
	mNotifyOnFirstJoin = mHangupOnLastLeave = false;
	mChannelID = "Estate";

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp)	// Disconnected
	{
		return;
	}
	const LLUUID& region_id = regionp->getRegionID();
	mConnections.emplace_back(
		new LLVoiceConnectionSpatial(region_id, INVALID_PARCEL_ID,
									 mChannelID));
}

//virtual
bool LLVoiceWebRTC::estateSessionState::processConnectionStates()
{
	if (!mShuttingDown)
	{
		// Estate voice requires connection to neighboring regions.
		uuid_list_t neighbors = gVoiceWebRTC.getNeighboringRegions();
		for (auto& connectionp : mConnections)
		{
			// Do check this is a spatial connection first. HB
			if (!connectionp->isSpatial())
			{
				continue;
			}
			const LLUUID& region_id = connectionp.get()->getRegionID();
			uuid_list_t::iterator it = neighbors.find(region_id);
			if (it == neighbors.end() || !region_has_webrtc(region_id))
			{
				// Shut down connections to neighbors that are too far away
				// or without WebRTC voice support.
				connectionp.get()->shutDown();
			}
			if (!connectionp.get()->isShuttingDown())
			{
				neighbors.erase(it);
			}
		}

		// Add new connections for active neighbors
		typedef std::shared_ptr<LLVoiceConnection> con_ptr_t;
		for (auto& region_id : neighbors)
		{
			if (!region_has_webrtc(region_id))
			{
				continue;
			}
			con_ptr_t connectp(new LLVoiceConnectionSpatial(region_id,
															INVALID_PARCEL_ID,
															mChannelID));
			mConnections.push_back(connectp);
			connectp->setMuteMic(mMuted);
			connectp->setSpeakerVolume(mSpeakerVolume);
		}
	}
	return sessionState::processConnectionStates();
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceWebRTC::parcelSessionState sub-class
///////////////////////////////////////////////////////////////////////////////

LLVoiceWebRTC::parcelSessionState::parcelSessionState(const std::string& cid,
													  S32 parcel_id)
{
	mNotifyOnFirstJoin = mHangupOnLastLeave = false;
	mChannelID = cid;

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp)	// Disconnected
	{
		return;
	}
	const LLUUID& region_id = regionp->getRegionID();
	mConnections.emplace_back(new LLVoiceConnectionSpatial(region_id,
														   parcel_id, cid));
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceWebRTC::adhocSessionState sub-class
///////////////////////////////////////////////////////////////////////////////

LLVoiceWebRTC::adhocSessionState::adhocSessionState(const std::string& cid,
													const std::string& cred,
													bool notify_on_first_join,
													bool hangup_on_last_leave)
{
	mNotifyOnFirstJoin = notify_on_first_join;
	mHangupOnLastLeave = hangup_on_last_leave;
	mChannelID = cid;

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp)	// Disconnected
	{
		return;
	}
	const LLUUID& region_id = regionp->getRegionID();
	mConnections.emplace_back(new LLVoiceConnectionAdHoc(region_id, cid,
														 cred));
}
