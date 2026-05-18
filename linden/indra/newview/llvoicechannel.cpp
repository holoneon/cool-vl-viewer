/**
 * @file llvoicechannel.cpp
 * @brief Voice Channel related classes
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llvoicechannel.h"

#include "llcachename.h"
#include "llcorehttputil.h"
#include "llnotifications.h"

#include "llagent.h"
#include "llcommandhandler.h"
#include "llimmgr.h"
#include "llmediactrl.h"
#include "llviewercontrol.h"

constexpr U32 DEFAULT_RETRIES_COUNT = 3;

// Static variables
LLVoiceChannel::voice_channel_map_t LLVoiceChannel::sVoiceChannelMap;
LLVoiceChannel* LLVoiceChannel::sCurrentVoiceChannel = NULL;
LLVoiceChannel* LLVoiceChannel::sSuspendedVoiceChannel = NULL;
bool LLVoiceChannel::sSuspended = false;

///////////////////////////////////////////////////////////////////////////////
// Global command handler for voicecallavatar
///////////////////////////////////////////////////////////////////////////////

class LLVoiceCallAvatarHandler final : public LLCommandHandler
{
public:
	LLVoiceCallAvatarHandler()
	:	LLCommandHandler("voicecallavatar", UNTRUSTED_THROTTLE)
	{
	}

	bool handle(const LLSD& params, const LLSD&, LLMediaCtrl*) override
	{
		// Make sure we have some parameters
		if (params.size() == 0)
		{
			return false;
		}

		// Get the ID
		LLUUID id;
		if (!id.set(params[0], false))
		{
			return false;
		}

		std::string name;
		if (gIMMgrp && gCacheNamep && gCacheNamep->getFullName(id, name))
		{
			// Once the IM panel will be opened, and provided that both the
			// caller and the recipient are voice-enabled, the user will be
			// only one click away from an actual voice call...
			// When no voice is available, this action is still consistent
			// With the "Call" link it is associated with in web profiles.
			gIMMgrp->setFloaterOpen(true);
			gIMMgrp->addSession(name, IM_NOTHING_SPECIAL, id);
			make_ui_sound("UISndStartIM");
		}

		return true;
	}
};

LLVoiceCallAvatarHandler gVoiceCallAvatarHandler;

///////////////////////////////////////////////////////////////////////////////
// LLVoiceChannel class
///////////////////////////////////////////////////////////////////////////////

LLVoiceChannel::LLVoiceChannel(const LLUUID& session_id,
							   const std::string& session_name)
:	mSessionID(session_id),
	mState(STATE_NO_CHANNEL_INFO),
	mSessionName(session_name),
	mOutgoingCall(true),	// Set to false if needed in setChannelInfo(). HB
	mIgnoreNextSessionLeave(false)
{
	mNotifyArgs["VOICE_CHANNEL_NAME"] = mSessionName;

	if (!sVoiceChannelMap.emplace(session_id, this).second)
	{
		// A voice channel already exists for this session id, so this instance
		// will be orphaned the end result should simply be the failure to make
		// voice calls
		llwarns << "Duplicate voice channels registered for session_id "
				<< session_id << llendl;
	}
}

LLVoiceChannel::~LLVoiceChannel()
{
	if (gVoiceClient.ready())	// Be sure to keep this !
	{
		gVoiceClient.removeObserver(this);
	}
	if (sSuspendedVoiceChannel == this)
	{
		sSuspendedVoiceChannel = NULL;
	}
	if (sCurrentVoiceChannel == this)
	{
		sCurrentVoiceChannel = NULL;
	}
	sVoiceChannelMap.erase(mSessionID);
}

void LLVoiceChannel::setChannelInfo(const LLSD& channel_info)
{
	LL_DEBUGS("Voice") << "New channel info: " << channel_info << LL_ENDL;
	mChannelInfo = channel_info;

	// Possibly fix the call direction (defaulting to outgoing) based on
	// "incoming" presence which is set in LLIMMgr::inviteUserResponse(). HB
	if (mChannelInfo.has("incoming"))
	{
		mOutgoingCall = false;
	}

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		if (mChannelInfo.isUndefined() || !mChannelInfo.isMap() ||
			!mChannelInfo.size())
		{
			gNotifications.add("VoiceChannelJoinFailed", mNotifyArgs);
			deactivate();
		}
		else
		{
			setState(STATE_READY);

			// If we are supposed to be active, reconnect. This will happen on
			// initial connect, as we request credentials on first use
			if (sCurrentVoiceChannel == this)
			{
				// Just in case we got new channel info while active should
				// move over to new channel.
				activate();
			}
		}
	}
}

void LLVoiceChannel::onChange(EStatusType type, const LLSD& channel_info, bool)
{
	if (mChannelInfo.isUndefined() ||
		(mChannelInfo.isMap() && !mChannelInfo.size()))
	{
		LL_DEBUGS("Voice") << "New channel info: " << channel_info << LL_ENDL;
		mChannelInfo = channel_info;
	}
	else
	{
		LL_DEBUGS("Voice") << "Keeping current channel info: " << mChannelInfo
						   << LL_ENDL;
	}
	if (gVoiceClient.compareChannels(mChannelInfo, channel_info))
	{
		if (type < BEGIN_ERROR_STATUS)
		{
			handleStatusChange(type);
		}
		else
		{
			handleError(type);
		}
	}
}

void LLVoiceChannel::handleStatusChange(EStatusType type)
{
	switch (type)
	{
		case STATUS_LOGIN_RETRY:
			gNotifications.add("VoiceLoginRetry");
			break;

		case STATUS_LOGGED_IN:
			break;

		case STATUS_LEFT_CHANNEL:
			if (callStarted() && !sSuspended)
			{
				// If forcibly removed from channel update the UI and revert to
				// default channel.
				gNotifications.add("VoiceChannelDisconnected", mNotifyArgs);
				// This will set the State to STATE_HUNG_UP so when this method
				// is called again during shutdown callStarted() will return
				// false and this deactivate() would not be called again.
				deactivate();
			}
			break;

		case STATUS_JOINING:
			if (callStarted())
			{
				setState(STATE_RINGING);
			}
			break;

		case STATUS_JOINED:
			if (callStarted())
			{
				setState(STATE_CONNECTED);
			}

		default:
			break;
	}
}

// Default behavior is to just deactivate channel derived classes provide
// specific error messages
void LLVoiceChannel::handleError(EStatusType type)
{
	deactivate();
	setState(STATE_ERROR);
}

bool LLVoiceChannel::isActive()
{
	// Only considered active when currently bound channel matches what our
	// channel
	return callStarted() && gVoiceClient.isCurrentChannel(mChannelInfo);
}

void LLVoiceChannel::deactivate()
{
	if (mState >= STATE_RINGING)
	{
		// Ignore session leave event
		mIgnoreNextSessionLeave = true;
	}

	if (callStarted())
	{
		setState(STATE_HUNG_UP);
		// Mute the microphone if required when returning to the proximal
		// channel
		if (sCurrentVoiceChannel == this && gVoiceClient.getUserPTTState() &&
			gSavedSettings.getBool("AutoDisengageMic"))
		{
			gSavedSettings.setBool("PTTCurrentlyEnabled", true);
			gVoiceClient.setUserPTTState(false);
		}
	}
	gVoiceClient.removeObserver(this);

	if (sCurrentVoiceChannel == this)
	{
		// Default channel is proximal channel
		sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
		sCurrentVoiceChannel->activate();
	}
}

void LLVoiceChannel::activate()
{
	if (callStarted())
	{
		return;
	}

	// Deactivate old channel and mark ourselves as the active one
	if (sCurrentVoiceChannel != this)
	{
		// Mark as current before deactivating the old channel to prevent
		// activating the proximal channel between IM calls
		LLVoiceChannel* old_channelp = sCurrentVoiceChannel;
		sCurrentVoiceChannel = this;
		if (old_channelp)
		{
			old_channelp->deactivate();
		}
	}

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		// Responsible for setting status to active
		requestChannelInfo();
	}
	else
	{
		setState(STATE_CALL_STARTED);
	}

	gVoiceClient.addObserver(this);
}

void LLVoiceChannel::requestChannelInfo()
{
	// Pretend we have everything we need
	if (sCurrentVoiceChannel == this)
	{
		setState(STATE_CALL_STARTED);
	}
}

//static
LLVoiceChannel* LLVoiceChannel::getChannelByID(const LLUUID& session_id)
{
	voice_channel_map_t::iterator it = sVoiceChannelMap.find(session_id);
	return it != sVoiceChannelMap.end() ? it->second : NULL;
}

void LLVoiceChannel::updateSessionID(const LLUUID& new_session_id)
{
	sVoiceChannelMap.erase(sVoiceChannelMap.find(mSessionID));
	mSessionID = new_session_id;
	sVoiceChannelMap.emplace(mSessionID, this);
}

void LLVoiceChannel::setState(EState state)
{
	if (!gIMMgrp) return;

	switch (state)
	{
		case STATE_RINGING:
			gIMMgrp->addSystemMessage(mSessionID, "ringing", mNotifyArgs);
			break;

		case STATE_CONNECTED:
			gIMMgrp->addSystemMessage(mSessionID, "connected", mNotifyArgs);
			break;

		case STATE_HUNG_UP:
			gIMMgrp->addSystemMessage(mSessionID, "hang_up", mNotifyArgs);
			break;

		default:
			break;
	}

	mState = state;
}

//static
void LLVoiceChannel::initClass()
{
	sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
}

//static
void LLVoiceChannel::suspend()
{
	if (!sSuspended)
	{
		sSuspendedVoiceChannel = sCurrentVoiceChannel;
		sSuspended = true;
	}
}

//static
void LLVoiceChannel::resume()
{
	if (sSuspended)
	{
		if (LLVoiceClient::voiceEnabled())
		{
			if (sSuspendedVoiceChannel)
			{
				if (sSuspendedVoiceChannel->callStarted())
				{
					// Should have channel data already, restart
					sSuspendedVoiceChannel->setState(STATE_READY);
				}
				// Note: would not do anything if call is already started...
				sSuspendedVoiceChannel->activate();
			}
			else
			{
				LLVoiceChannelProximal::getInstance()->activate();
			}
		}
		sSuspended = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceChannelGroup class
///////////////////////////////////////////////////////////////////////////////

LLVoiceChannelGroup::LLVoiceChannelGroup(const LLUUID& session_id,
										 const std::string& session_name,
										 bool is_p2p)
:	LLVoiceChannel(session_id, session_name),
	mIsP2P(is_p2p)
{
	mRetries = DEFAULT_RETRIES_COUNT;
	mIsRetrying = false;
}

//virtual
void LLVoiceChannelGroup::deactivate()
{
	if (callStarted())
	{
		gVoiceClient.leaveNonSpatialChannel();
	}
	LLVoiceChannel::deactivate();
	if (mIsP2P)
	{
		// Void the channel info for P2P adhoc channels so that we request it
		// again, hence throwing up the connect dialogue on the other side.
		setState(STATE_NO_CHANNEL_INFO);
	}
}

//virtual
void LLVoiceChannelGroup::activate()
{
	if (callStarted()) return;

	LLVoiceChannel::activate();

	if (!callStarted())
	{
		return;
	}

	// We have the channel info, just need to use it now.
	gVoiceClient.setNonSpatialChannel(mChannelInfo, mIsP2P && mOutgoingCall,
									  mIsP2P);
}

//static
void LLVoiceChannelGroup::voiceCallCapCoro(const std::string& url,
										   LLUUID session_id)
{
	LLSD data;
	data["method"] = "call";
	data["session-id"] = session_id;
	LLSD params;
	params["preferred_voice_server_type"] = "webrtc";
	data["alt_params"] = params;

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("voiceCallCapCoro");
	LLSD result = adapter.postAndSuspend(url, data);

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);

	// Verify that the channel is still open on server reply, and bail if not.
	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(session_id);
	if (!channelp)
	{
		llinfos << "Got reply for closed session Id: " << session_id
				<< ". Ignored." << llendl;
		return;
	}

	if (!status)
	{
		if (status == gStatusForbidden)
		{
			// 403 == no ability
			gNotifications.add("VoiceNotAllowed", channelp->getNotifyArgs());
		}
		else
		{
			gNotifications.add("VoiceCallGenericError",
							   channelp->getNotifyArgs());
		}
		channelp->deactivate();
		return;
	}

	result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
	for (LLSD::map_const_iterator iter = result.beginMap(),
								  end = result.endMap();
		 iter != end; ++iter)
	{
		llinfos << "Got " << iter->first << llendl;
	}
	channelp->setChannelInfo(result["voice_credentials"]);
}

//virtual
void LLVoiceChannelGroup::requestChannelInfo()
{
	const std::string& url = gAgent.getRegionCapability("ChatSessionRequest");
	if (url.empty())
	{
		return;
	}
	gCoros.launch("LLVoiceChannelGroup::voiceCallCapCoro",
				  std::bind(&LLVoiceChannelGroup::voiceCallCapCoro, url,
							mSessionID));
}

//virtual
void LLVoiceChannelGroup::setChannelInfo(const LLSD& channel_info)
{
	mChannelInfo = channel_info;

	// Possibly fix the call direction (defaulting to outgoing) based on
	// "incoming" presence which is set in LLIMMgr::inviteUserResponse(). HB
	if (mChannelInfo.has("incoming"))
	{
		mOutgoingCall = false;
	}

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		if (mChannelInfo.isDefined() && mChannelInfo.isMap())
		{
			setState(STATE_READY);

			// If we are supposed to be active, reconnect. This will happen on
			// initial connect, as we request credentials on first use
			if (sCurrentVoiceChannel == this)
			{
				// Just in case we got new channel info while active should
				// move over to new channel
				activate();
			}
		}
		else
		{
			deactivate();
		}
	}
	else if (mIsRetrying)
	{
		// We have the channel info, just need to use it now
		gVoiceClient.setNonSpatialChannel(mChannelInfo, mOutgoingCall, mIsP2P);
	}
}

//virtual
void LLVoiceChannelGroup::handleStatusChange(EStatusType type)
{
	// Status updates
	if (type == STATUS_JOINED)
	{
		mRetries = 3;
		mIsRetrying = false;

		// Mic default state is OFF on initiating/joining Ad-Hoc/Group calls
		// while it is ON for P2P using the adhoc infra.
		gVoiceClient.setUserPTTState(mIsP2P);
	}
	LLVoiceChannel::handleStatusChange(type);
}

//virtual
void LLVoiceChannelGroup::handleError(EStatusType status)
{
	std::string notify;
	switch (status)
	{
		case ERROR_CHANNEL_LOCKED:
		case ERROR_CHANNEL_FULL:
			notify = "VoiceChannelFull";
			break;

		case ERROR_NOT_AVAILABLE:
			// Clear URI and credentials, set the state to be no info and
			// activate
			if (mRetries > 0)
			{
				--mRetries;
				mIsRetrying = true;
				mIgnoreNextSessionLeave = true;
				requestChannelInfo();
				return;
			}
			else
			{
				notify = "VoiceChannelJoinFailed";
				mRetries = DEFAULT_RETRIES_COUNT;
				mIsRetrying = false;
			}
			break;

		case ERROR_UNKNOWN:
		default:
			break;
	}

	// Notification
	if (!notify.empty())
	{
		LLNotificationPtr notifp = gNotifications.add(notify, mNotifyArgs);
		// Echo to IM window
		if (gIMMgrp)
		{
			gIMMgrp->addMessage(mSessionID, LLUUID::null, SYSTEM_FROM,
							    notifp->getMessage());
		}
	}

	LLVoiceChannel::handleError(status);
}

//virtual
void LLVoiceChannelGroup::setState(EState state)
{
	if (state == STATE_RINGING)
	{
		if (!mIsRetrying && gIMMgrp)
		{
			gIMMgrp->addSystemMessage(mSessionID, "ringing", mNotifyArgs);
		}
		mState = state;
	}
	else
	{
		LLVoiceChannel::setState(state);
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceChannelProximal class
///////////////////////////////////////////////////////////////////////////////

LLVoiceChannelProximal::LLVoiceChannelProximal()
:	LLVoiceChannel(LLUUID::null, LLStringUtil::null)
{
	activate();
}

//virtual
bool LLVoiceChannelProximal::isActive()
{
	return callStarted() && gVoiceClient.inProximalChannel();
}

#define LL_NEW_ACTIVATION_ORDER 1
//virtual
void LLVoiceChannelProximal::activate()
{
	if (callStarted()) return;

#if LL_NEW_ACTIVATION_ORDER
	if (sCurrentVoiceChannel != this && mState == STATE_CONNECTED)
	{
		// We are connected to a non-spatial channel, so disconnect.
		gVoiceClient.leaveNonSpatialChannel();
	}
#endif
	gVoiceClient.activateSpatialChannel(true);
	LLVoiceChannel::activate();
#if !LL_NEW_ACTIVATION_ORDER
	if (callStarted())
	{
		// This implicitly puts you back in the spatial channel
		gVoiceClient.leaveNonSpatialChannel();
	}
#endif
}

//virtual
void LLVoiceChannelProximal::onChange(EStatusType type, const LLSD&,
									  bool proximal)
{
	if (proximal)
	{
		if (type < BEGIN_ERROR_STATUS)
		{
			handleStatusChange(type);
		}
		else
		{
			handleError(type);
		}
	}
}

//virtual
void LLVoiceChannelProximal::handleStatusChange(EStatusType status)
{
	if (status == STATUS_LEFT_CHANNEL)
	{
		// Do not notify user when leaving proximal channel
		return;
	}

	if (status == STATUS_VOICE_DISABLED)
	{
		gVoiceClient.setUserPTTState(false);
		if (gIMMgrp)
		{
			gIMMgrp->addSystemMessage(LLUUID::null, "unavailable",
									  mNotifyArgs);
		}
		return;
	}

	LLVoiceChannel::handleStatusChange(status);
}

//virtual
void LLVoiceChannelProximal::handleError(EStatusType status)
{
	if (status == ERROR_CHANNEL_LOCKED || status == ERROR_CHANNEL_FULL)
	{
		gNotifications.add("ProximalVoiceChannelFull", mNotifyArgs);
	}
#if 0	// Proximal voice remains up and the provider will try to reconnect.
	LLVoiceChannel::handleError(status);
#endif
}

//virtual
void LLVoiceChannelProximal::deactivate()
{
	if (callStarted())
	{
		setState(STATE_HUNG_UP);
	}
	gVoiceClient.removeObserver(this);
	gVoiceClient.activateSpatialChannel(false);
}

///////////////////////////////////////////////////////////////////////////////
// LLVoiceChannelP2P class
///////////////////////////////////////////////////////////////////////////////

LLVoiceChannelP2P::LLVoiceChannelP2P(const LLUUID& session_id,
									 const std::string& session_name,
									 const LLUUID& other_user_id,
									 U32 server_type)
:	LLVoiceChannelGroup(session_id, session_name, true),
	mOtherUserID(other_user_id),
	mReceivedCall(false)
{
}

//virtual
void LLVoiceChannelP2P::handleStatusChange(EStatusType type)
{
	if (type == STATUS_LEFT_CHANNEL)
	{
		if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
		{
			if (mState == STATE_RINGING)
			{
				// Other user declined call
				gNotifications.add("P2PCallDeclined", mNotifyArgs);
			}
			else
			{
				// Other user hung up
				gNotifications.add("VoiceChannelDisconnectedP2P", mNotifyArgs);
			}
			deactivate();
		}
		mIgnoreNextSessionLeave = false;
		return;
	}
	if (type == STATUS_JOINING)
	{
		// Because we join session we expect to process session leave event in
		// the future.
		mIgnoreNextSessionLeave = false;
	}

	LLVoiceChannel::handleStatusChange(type);
}

//virtual
void LLVoiceChannelP2P::handleError(EStatusType type)
{
	if (type == ERROR_NOT_AVAILABLE)
	{
		gNotifications.add("P2PCallNoAnswer", mNotifyArgs);
	}

	LLVoiceChannel::handleError(type);
}

//virtual
void LLVoiceChannelP2P::activate()
{
	if (callStarted()) return;

	LLVoiceChannel::activate();

	if (!callStarted())
	{
		return;	// Nothing to do for now.
	}

	// No session yet, we are starting the call
	mReceivedCall = false;

	// Default mic is ON on initiating/joining P2P calls
	if (!gVoiceClient.getUserPTTState() && gVoiceClient.getPTTIsToggle())
	{
		gVoiceClient.inputUserControlState(true);
	}
}

//virtual
void LLVoiceChannelP2P::deactivate()
{
	LLVoiceChannel::deactivate();
}

//virtual
void LLVoiceChannelP2P::requestChannelInfo()
{
	// Pretend we have everything we need, since P2P does not use channel info.
	if (sCurrentVoiceChannel == this)
	{
		setState(STATE_CALL_STARTED);
	}
}

// Receiving session from other user who initiated call
//virtual
void LLVoiceChannelP2P::setChannelInfo(const LLSD& channel_info)
{
	mChannelInfo = channel_info;

	// Possibly fix the call direction (defaulting to outgoing) based on
	// "incoming" presence which is set in LLIMMgr::inviteUserResponse(). HB
	if (mChannelInfo.has("incoming"))
	{
		mOutgoingCall = false;
	}

	bool needs_activate = false;
	if (callStarted())
	{
		// Defer to lower agent id when already active
		if (mOtherUserID < gAgentID)
		{
			// Pretend we have not started the call yet, so we can connect to
			// this session instead
			deactivate();
			needs_activate = true;
		}
		else
		{
			// We are active and have priority, invite the other user again
			// under the assumption they will join this new session
			return;
		}
	}

	mReceivedCall = true;

	if (needs_activate)
	{
		activate();
	}
}

//virtual
void LLVoiceChannelP2P::setState(EState state)
{
	// You only "answer" voice invites in P2P mode so provide a special purpose
	// message here
	if (mReceivedCall && state == STATE_RINGING)
	{
		if (gIMMgrp)
		{
			gIMMgrp->addSystemMessage(mSessionID, "answering", mNotifyArgs);
		}
		mState = state;
		return;
	}
	LLVoiceChannel::setState(state);
}
