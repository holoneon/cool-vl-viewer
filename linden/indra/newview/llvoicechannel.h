/**
 * @file llvoicechannel.h
 * @brief Voice channel related classes
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

#pragma once

#include "hbfastmap.h"
#include "llpanel.h"

#include "llvoiceclient.h"

class LLVoiceChannel : public LLVoiceClientStatusObserver
{
protected:
	LOG_CLASS(LLVoiceChannel);

public:
	typedef enum e_voice_channel_state
	{
		STATE_NO_CHANNEL_INFO,
		STATE_ERROR,
		STATE_HUNG_UP,
		STATE_READY,
		STATE_CALL_STARTED,
		STATE_RINGING,
		STATE_CONNECTED
	} EState;

	LLVoiceChannel(const LLUUID& session_id, const std::string& session_name);
	virtual ~LLVoiceChannel();

	// LLVoiceClientStatusObserver override
	void onChange(EStatusType status, const LLSD& channel_info,
				  bool proximal) override;

	virtual void handleStatusChange(EStatusType status);
	virtual void handleError(EStatusType status);
	virtual void deactivate();
	virtual void activate();
	virtual void setChannelInfo(const LLSD& channel_info);
	virtual void requestChannelInfo();
	virtual bool isActive();
	LL_INLINE virtual bool isP2P() const			{ return false; }

	LL_INLINE bool callStarted() const
	{
		return mState >= STATE_CALL_STARTED;
	}

	LL_INLINE const LLUUID& getSessionID() const	{ return mSessionID; }
	LL_INLINE EState getState() const 				{ return mState; }

	void updateSessionID(const LLUUID& new_session_id);
	LL_INLINE const LLSD& getNotifyArgs() const		{ return mNotifyArgs; }

	static LLVoiceChannel* getChannelByID(const LLUUID& session_id);

	LL_INLINE static LLVoiceChannel* getCurrentVoiceChannel()
	{
		return sCurrentVoiceChannel;
	}

	static void initClass();

	static void suspend();
	static void resume();

protected:
	virtual void setState(EState state);

protected:
	LLHandle<LLPanel>				mLoginNotificationHandle;
	LLUUID							mSessionID;
	std::string						mSessionName;
	std::string						mCredentials;
	LLSD							mChannelInfo;
	LLSD							mNotifyArgs;
	EState							mState;
	bool							mOutgoingCall;
	bool							mIgnoreNextSessionLeave;

	typedef fast_hmap<LLUUID, LLVoiceChannel*> voice_channel_map_t;
	static voice_channel_map_t		sVoiceChannelMap;

	static LLVoiceChannel*			sCurrentVoiceChannel;
	static LLVoiceChannel*			sSuspendedVoiceChannel;
	static bool						sSuspended;
};

class LLVoiceChannelGroup : public LLVoiceChannel
{
protected:
	LOG_CLASS(LLVoiceChannelGroup);

public:
	LLVoiceChannelGroup(const LLUUID& session_id,
						const std::string& session_name, bool is_p2p = false);

	void handleStatusChange(EStatusType status) override;
	void handleError(EStatusType status) override;
	void activate() override;
	void deactivate() override;
	void setChannelInfo(const LLSD& channel_info) override;
	void requestChannelInfo() override;
	LL_INLINE bool isP2P() const override			{ return mIsP2P; }

protected:
	void setState(EState state) override;

private:
	static void voiceCallCapCoro(const std::string& url, LLUUID session_id);

private:
	U32		mRetries;
	bool	mIsP2P;
	bool	mIsRetrying;
};

class LLVoiceChannelProximal final : public LLVoiceChannel,
									 public LLSingleton<LLVoiceChannelProximal>
{
	friend class LLSingleton<LLVoiceChannelProximal>;

protected:
	LOG_CLASS(LLVoiceChannelProximal);

public:
	LLVoiceChannelProximal();

	void onChange(EStatusType status, const LLSD& channel_info,
				  bool proximal) override;
	void handleStatusChange(EStatusType status) override;
	void handleError(EStatusType status) override;
	bool isActive() override;
	void activate() override;
	void deactivate() override;
};

class LLVoiceChannelP2P final : public LLVoiceChannelGroup
{
protected:
	LOG_CLASS(LLVoiceChannelP2P);

public:
	LLVoiceChannelP2P(const LLUUID& session_id,
					  const std::string& session_name,
					  const LLUUID& other_user_id, U32 server_type);

	void handleStatusChange(EStatusType status) override;
	void handleError(EStatusType status) override;
	void activate() override;
	void deactivate() override;
	void setChannelInfo(const LLSD& channel_info) override;
	void requestChannelInfo() override;

protected:
	void setState(EState state) override;

private:
	LLUUID		mOtherUserID;
	bool		mReceivedCall;
};
