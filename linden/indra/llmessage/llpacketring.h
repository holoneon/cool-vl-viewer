/**
 * @file llpacketring.h
 * @brief definition of LLPacketRing class for implementing a resend,
 * drop, or delay in packet transmissions
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

#include <queue>

#include "llhost.h"

class LLMessageSystem;
class LLPacketBuffer;

class LLPacketRing
{
protected:
	LOG_CLASS(LLPacketRing);

public:
	LLPacketRing();
    ~LLPacketRing();

	// Receives one packet: either buffered or from the socket.
	U32 receivePacket(S32 socket, char* datap);

	// Sends one packet.
	bool sendPacket(S32 h_socket, const char* datap, S32 data_size,
					LLHost host);

	// Drains packets from socket and returns final mNumBufferedPackets.
	U32 drainSocket(LLMessageSystem* msgsys, S32 socket);

	LL_INLINE LLHost getLastSender() const			{ return mLastSender; }

	LL_INLINE LLHost getLastReceivingInterface() const
	{
		return mLastReceivingIF;
	}

	LL_INLINE U32 getActualInBytes() const			{ return mActualBytesIn; }
	LL_INLINE U32 getActualOutBytes() const			{ return mActualBytesOut; }

	LL_INLINE U32 getAndResetActualInBits()
	{
		U32 bits = 8 * mActualBytesIn;
		mActualBytesIn = 0;
		return bits;
	}

	LL_INLINE U32 getAndResetActualOutBits()
	{
		U32 bits = 8 * mActualBytesOut;
		mActualBytesOut = 0;
		return bits;
	}

	LL_INLINE U32 getNumBufferedPackets() const
	{
		return mNumBufferedPackets;
	}

	LL_INLINE U32 getNumBufferedBytes() const
	{
		return mNumBufferedBytes;
	}

	// Returns 0 (empty) to 4 (1 - default size is full)
	F32 getBufferLoadRate() const;

	void setMaxBufferSize(U32 size);

private:
	// These methods return of received packet, zero or less if no packet found
	S32 receiveNetworkPacket(S32 socket, char* datap);
	S32 receiveBufferedPacket(char* datap);

	// Returns packet_size of packet buffered.
	S32 bufferInboundPacket(S32 socket);

	void expandRing();

protected:
	typedef std::vector<LLPacketBuffer*> packet_vec_t;
	packet_vec_t	mPacketRing;

	LLHost			mLastSender;
	LLHost			mLastReceivingIF;

	U32				mPacketRingMaxBufferSize;
	U32				mHeadIndex;
	U32				mNumBufferedPackets;
	U32				mNumBufferedBytes;
	U32				mActualBytesIn;
	U32				mActualBytesOut;
};
