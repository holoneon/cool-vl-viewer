/**
 * @file llpacketack.cpp
 * @author Phoenix
 * @date 2007-05-09
 * @brief Implementation of the LLReliablePacket.
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

#include "linden_common.h"

#include "llpacketack.h"

#if !LL_WINDOWS
#include <netinet/in.h>
#else
#include "winsock2.h"
#endif

#include "llmessage.h"

LLReliablePacket::LLReliablePacket(S32 socket, U8* buf_ptr, S32 buf_len,
								   LLReliablePacketParams* params)
:	mBuffer(NULL),
	mBufferLength(0)
{
	if (params)
	{
		mHost = params->mHost;
		mRetries = params->mRetries;
		mPingBasedRetry = params->mPingBasedRetry;
		mTimeout = params->mTimeout;
		mCallback = params->mCallback;
		mCallbackData = params->mCallbackData;
		mMessageName = params->mMessageName;
	}
	else
	{
		mRetries = 0;
		mPingBasedRetry = true;
		mTimeout = 0.f;
		mCallback = NULL;
		mCallbackData = NULL;
		mMessageName = NULL;
	}

	mExpirationTime = (F64)((S64)LLTimer::totalTime()) / 1000000.0 + mTimeout;
	mPacketID = ntohl(*((U32*)(&buf_ptr[PHL_PACKET_ID])));

	mSocket = socket;
	if (mRetries)
	{
		mBuffer = new U8[buf_len];
		if (mBuffer != NULL)
		{
			memcpy(mBuffer,buf_ptr,buf_len);
			mBufferLength = buf_len;
		}
	}
}
