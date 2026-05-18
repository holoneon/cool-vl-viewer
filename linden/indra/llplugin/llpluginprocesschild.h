/**
 * @file llpluginprocesschild.h
 * @brief LLPluginProcessChild handles the child side of the external-process plugin API.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 *
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

#include <queue>

#include "llhost.h"
#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessagepipe.h"
#include "llpluginsharedmemory.h"
#include "lltimer.h"

class LLPluginProcessChild : public LLPluginMessagePipeOwner,
							 public LLPluginInstanceMessageListener
{
protected:
	LOG_CLASS(LLPluginProcessChild);

public:
	enum EState
	{
		STATE_UNINITIALIZED,
		STATE_INITIALIZED,			// init() has been called
		STATE_CONNECTED,			// connected back to launcher
		STATE_PLUGIN_LOADING,		// plugin library needs to be loaded
		STATE_PLUGIN_LOADED,		// plugin library has been loaded
		STATE_PLUGIN_INITIALIZING,	// plugin is processing init message
		STATE_RUNNING,				// steady state (processing messages)
		STATE_SHUTDOWNREQ,			// Parent has requested a shutdown.
		STATE_UNLOADING,			// plugin has sent shutdown_response and needs to be unloaded
		STATE_UNLOADED,				// plugin has been unloaded
		STATE_ERROR,				// generic bailout state
		STATE_DONE					// state machine will sit in this state after either error or normal termination.
	};

	LLPluginProcessChild();
	~LLPluginProcessChild();

	void init(U32 launcher_port);
	void idle();
	void sleep(F64 seconds);
	void pump();

	// Returns true if the plugin is in the steady state (processing messages)
	LL_INLINE bool isRunning()					{ return mState == STATE_RUNNING; }

	// Returns true if the plugin is unloaded or we're in an unrecoverable
	// error state.
	LL_INLINE bool isDone()						{ return mState == STATE_DONE; }

	void killSockets();

	LL_INLINE F64 getSleepTime() const			{ return mSleepTime; }

	void sendMessageToPlugin(const LLPluginMessage& message);
	void sendMessageToParent(const LLPluginMessage& message);

	// Inherited from LLPluginMessagePipeOwner
	void receiveMessageRaw(const std::string& message) override;

	// Inherited from LLPluginInstanceMessageListener
	void receivePluginMessage(const std::string& message) override;

private:
	void setState(EState state);

	void deliverQueuedMessages();

private:
	LLPluginInstance*		mInstance;

	EState					mState;

	LLHost					mLauncherHost;
	LLSocket::ptr_t			mSocket;

	std::string				mPluginFile;
	std::string				mPluginDir;

	typedef std::map<std::string, LLPluginSharedMemory*,
					 std::less<> > shared_mem_regions_t;
	shared_mem_regions_t	mSharedMemoryRegions;

	std::queue<std::string>	mMessageQueue;

	LLTimer					mHeartbeat;
	LLTimer					mWaitGoodbye;

	F64						mSleepTime;
	F64						mCPUElapsed;

	bool					mBlockingRequest;
	bool					mBlockingResponseReceived;
};
