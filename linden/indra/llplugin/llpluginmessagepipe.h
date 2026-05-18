/** 
 * @file llpluginmessagepipe.h
 * @brief Classes that implement connections from the plugin system to pipes/pumps.
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

#include "lliosocket.h"
#include "llthread.h"

class LLPluginMessagePipe;

// Inherit from this to be able to receive messages from the LLPluginMessagePipe
class LLPluginMessagePipeOwner
{
protected:
	LOG_CLASS(LLPluginMessagePipeOwner);

public:
	LLPluginMessagePipeOwner();
	virtual ~LLPluginMessagePipeOwner();

	// Called with incoming messages
	virtual void receiveMessageRaw(const std::string& message) = 0;

	// Called when the socket has an error
	virtual apr_status_t socketError(apr_status_t error);

	// Called from LLPluginMessagePipe to manage the connection with
	// LLPluginMessagePipeOwner: do not use !
	virtual void setMessagePipe(LLPluginMessagePipe* message_pipe);

protected:
	// Returns false if writeMessageRaw() would drop the message
	LL_INLINE bool canSendMessage()			{ return mMessagePipe != NULL; }

	// SendS a message over the pipe
	bool writeMessageRaw(const std::string& message);

	// Closes the pipe
	void killMessagePipe();
	
protected:
	LLPluginMessagePipe*	mMessagePipe;
	apr_status_t			mSocketError;
};

class LLPluginMessagePipe
{
protected:
	LOG_CLASS(LLPluginMessagePipe);

public:
	LLPluginMessagePipe(LLPluginMessagePipeOwner* owner,
						LLSocket::ptr_t socket);
	virtual ~LLPluginMessagePipe();
	
	// Called when the owner is done with this pipe. The next call to
	// process_impl should send any remaining data and exit.
	LL_INLINE void clearOwner()				{ mOwner = NULL; }
	
	bool addMessage(const std::string& message);

	bool pump(F64 timeout = 0.0);
	bool pumpOutput();
	bool pumpInput(F64 timeout = 0.0);
		
protected:	
	void processInput();

	// Used internally by pump()
	void setSocketTimeout(apr_interval_time_t timeout_usec);
	
protected:	
	LLPluginMessagePipeOwner*	mOwner;
	LLSocket::ptr_t				mSocket;
	LLMutex						mInputMutex;
	std::string					mInput;
	LLMutex						mOutputMutex;
	std::string					mOutput;
	size_t						mOutputStartIndex;
};
