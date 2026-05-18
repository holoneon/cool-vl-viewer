/**
 * @file llpacketack.h
 * @brief Reliable UDP helpers for the message system.
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

#include "llhost.h"

class LLReliablePacketParams
{
public:
	LLReliablePacketParams()
	{
		clear();
	}

	~LLReliablePacketParams()	{}

	void clear()
	{
		mHost.invalidate();
		mRetries = 0;
		mPingBasedRetry = true;
		mTimeout = 0.f;
		mCallback = NULL;
		mCallbackData = NULL;
		mMessageName = NULL;
	}

	void set(const LLHost& host, S32 retries, bool ping_based_retry,
			 F32 timeout,  void (*callback)(void**, S32),
			 void** callback_data, char* name)
	{
		mHost = host;
		mRetries = retries;
		mPingBasedRetry = ping_based_retry;
		mTimeout = timeout;
		mCallback = callback;
		mCallbackData = callback_data;
		mMessageName = name;
	}

public:
	LLHost	mHost;
	S32		mRetries;
	F32		mTimeout;
	void	(*mCallback)(void **, S32);
	void**	mCallbackData;
	char*	mMessageName;
	bool	mPingBasedRetry;
};

class LLReliablePacket
{
	friend class LLCircuitData;

public:
	LLReliablePacket(S32 socket, U8* buf_ptr, S32 buf_len,
					 LLReliablePacketParams* params);

	~LLReliablePacket()
	{
		mCallback = NULL;
		delete[] mBuffer;
		mBuffer = NULL;
	}

protected:
	S32			mSocket;
	LLHost		mHost;
	S32			mRetries;
	F32			mTimeout;
	void		(*mCallback)(void**, S32);
	void**		mCallbackData;
	char*		mMessageName;

	U8*			mBuffer;
	S32			mBufferLength;

	TPACKETID	mPacketID;

	F64			mExpirationTime;

	bool		mPingBasedRetry;
};
