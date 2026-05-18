/**
 * @file llpacketring.cpp
 * @brief implementation of LLPacketRing class for a packet.
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

#include "linden_common.h"

#include "llpacketring.h"

#if LL_WINDOWS
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
#endif

#include "llmessage.h"
#include "llnet.h"
#include "llproxy.h"
#include "llrand.h"
#include "lltimer.h"

///////////////////////////////////////////////////////////////////////////////
// LLPacketBuffer class. Used to be in its own llpacketbuffer.h/cpp module, but
// is only used by LLPacketRing, so I moved it here. HB
///////////////////////////////////////////////////////////////////////////////

class LLPacketBuffer
{
public:
	LLPacketBuffer(const LLHost& host, const char* datap, S32 size);
	LL_INLINE LLPacketBuffer(S32 socket)			{ init(socket); }

	void init(S32 socket);
	void init(const char* bufferp, S32 data_size, const LLHost& host);

	LL_INLINE S32 getSize() const					{ return mSize; }
	LL_INLINE const char* getData() const			{ return mData; }
	LL_INLINE LLHost getHost() const				{ return mHost; }
	LL_INLINE LLHost getReceivingInterface() const	{ return mReceivingIF; }

protected:
	LLHost	mHost;					// Source/dest IP and port
	LLHost	mReceivingIF;			// Source/dest IP and port
	char	mData[NET_BUFFER_SIZE];	// Packet data
	S32		mSize;					// Size of buffer in bytes
};

LLPacketBuffer::LLPacketBuffer(const LLHost& host, const char* datap, S32 size)
:	mHost(host),
	mSize(0)
{
	mData[0] = '!';

	if (size > NET_BUFFER_SIZE)
	{
		llerrs << "Sending packet > " << NET_BUFFER_SIZE << " of size " << size
			   << llendl;
	}
	if (datap && size > 0)
	{
		memcpy(mData, datap, size);
		mSize = size;
	}
}

void LLPacketBuffer::init(S32 socket)
{
	mSize = receive_packet(socket, mData);
	mHost = get_sender();
	mReceivingIF = get_receiving_interface();
}

void LLPacketBuffer::init(const char* bufferp, S32 size, const LLHost& host)
{
	if (size > NET_BUFFER_SIZE)
	{
		llerrs << "Cannot initialize packet with more than " << NET_BUFFER_SIZE
			   << " bytes: requested size was: " << size << llendl;
	}
	if (bufferp && size > 0)
	{
		memcpy(mData, bufferp, size);
		mSize = size;
	}
	else
	{
		mSize = 0;
	}
	mHost = host;
	mReceivingIF = get_receiving_interface();
}

///////////////////////////////////////////////////////////////////////////////
// LLPacketRing class
///////////////////////////////////////////////////////////////////////////////

constexpr U32 MAX_BUFFER_RING_SIZE = 4096;
constexpr U32 DEFAULT_BUFFER_RING_SIZE = 256;
constexpr U32 BUFFER_RING_EXPANSION = 256;

LLPacketRing::LLPacketRing()
:	mPacketRingMaxBufferSize(1024),
	mHeadIndex(0),
	mNumBufferedPackets(0),
	mNumBufferedBytes(0),
	mActualBytesIn(0),
	mActualBytesOut(0)
{
	LLHost invalid_host;
	mPacketRing.reserve(DEFAULT_BUFFER_RING_SIZE);
	for (U32 i = 0; i < DEFAULT_BUFFER_RING_SIZE; ++i)
	{
		mPacketRing.emplace_back(new LLPacketBuffer(invalid_host, NULL, 0));
	}
}

LLPacketRing::~LLPacketRing()
{
	for (size_t i = 0, count = mPacketRing.size(); i < count; ++i)
	{
		delete mPacketRing[i];
	}
}

void LLPacketRing::setMaxBufferSize(U32 size)
{
	U32 current_size = mPacketRing.size();
	if (size > current_size)
	{
		mPacketRingMaxBufferSize = llclamp(size, 1024, MAX_BUFFER_RING_SIZE);
		return;
	}
	mPacketRingMaxBufferSize = current_size;
	if (size < current_size)
	{
		llwarns << "Cannot resize down the existing packet ring. Maximum size set to: "
				<< current_size << llendl;
	}
}

bool LLPacketRing::sendPacket(S32 socket, const char* datap, S32 data_size,
							  LLHost host)
{
	if (!datap || data_size <= 0)
	{
		llwarns << "Nothing to send to host: " << host.getIPandPort()
				<< llendl;
		llassert(false);
		return false;
	}

	mActualBytesOut += data_size;

	if (!LLProxy::isSOCKSProxyEnabled())
	{
		return send_packet(socket, datap, data_size, host.getAddress(),
						   host.getPort());
	}

	char headered_send_buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];

	proxywrap_t* headerp = (proxywrap_t*)((void*)&headered_send_buffer);
	headerp->rsv = 0;
	headerp->addr = host.getAddress();
	headerp->port = htons(host.getPort());
	headerp->atype = ADDRESS_IPV4;
	headerp->frag = 0;

	memcpy(headered_send_buffer + SOCKS_HEADER_SIZE, datap, data_size);

	LLHost proxyhost = LLProxy::getInstance()->getUDPProxy();
	return send_packet(socket, headered_send_buffer,
					   data_size + SOCKS_HEADER_SIZE,
					   proxyhost.getAddress(), proxyhost.getPort());
}

U32 LLPacketRing::receivePacket(S32 socket, char* datap)
{
	if (!datap)
	{
		llwarns << "NULL data pointer." << llendl;
		llassert(false);
		return 0;
	}
	return mNumBufferedPackets > 0 ? receiveBufferedPacket(datap)
								   : receiveNetworkPacket(socket, datap);
}

S32 LLPacketRing::receiveNetworkPacket(S32 socket, char* datap)
{
	S32 packet_size;

	if (LLProxy::isSOCKSProxyEnabled())
	{
		char buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];
		packet_size = receive_packet(socket, buffer);
		if (packet_size > 0)
		{
			mActualBytesIn += packet_size;
		}
		if (packet_size > SOCKS_HEADER_SIZE)
		{
			// *FIX: we are assuming ATYP is 0x01 (IPv4), not 0x03 (hostname)
			// or 0x04 (IPv6)
			packet_size -= SOCKS_HEADER_SIZE; // The unwrapped packet size
			memcpy(datap, buffer + SOCKS_HEADER_SIZE, packet_size);
			proxywrap_t* headerp = (proxywrap_t*)((void*)buffer);
			mLastSender.setAddress(headerp->addr);
			mLastSender.setPort(ntohs(headerp->port));
			mLastReceivingIF = get_receiving_interface();
		}
		else
		{
			packet_size = 0;
		}
	}
	else
	{
		packet_size = receive_packet(socket, datap);
		if (packet_size > 0)
		{
			mActualBytesIn += packet_size;
			mLastSender = get_sender();
			mLastReceivingIF = get_receiving_interface();
		}
	}

	return packet_size;
}

S32 LLPacketRing::receiveBufferedPacket(char* datap)
{
	U32 ring_size = mPacketRing.size();
	U32 packet_index = (mHeadIndex + ring_size - mNumBufferedPackets) %
					   ring_size;
	LLPacketBuffer* packetp = mPacketRing[packet_index];
	S32 packet_size = packetp->getSize();
	mLastSender = packetp->getHost();
	mLastReceivingIF = packetp->getReceivingInterface();

	--mNumBufferedPackets;
	mNumBufferedBytes -= packet_size;
	if (!mNumBufferedPackets && mNumBufferedBytes)
	{
		llwarns << "No buffered packet left but number of buffered bytes in non zero: "
				<< mNumBufferedBytes << llendl;
	}

	if (packet_size > 0 && packetp->getData())
	{
		memcpy(datap, packetp->getData(), packet_size);
	}
	else if (packet_size > 0)
	{
		llwarns << "Received a packet with non-zero size and no data. Zeroing data."
				<< llendl;
		memset(datap, 0, packet_size);
	}

	return packet_size;
}

S32 LLPacketRing::bufferInboundPacket(S32 socket)
{
	if (mNumBufferedPackets >= (U32)mPacketRing.size())
	{
		expandRing();
	}

	LLPacketBuffer* packetp = mPacketRing[mHeadIndex];
	S32 old_packet_size = packetp->getSize();

	S32 packet_size;

	if (LLProxy::isSOCKSProxyEnabled())
	{
		char buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];
		packet_size = receive_packet(socket, buffer);
		if (packet_size > 0)
		{
			mActualBytesIn += packet_size;
			if (packet_size > SOCKS_HEADER_SIZE)
			{
				packet_size -= SOCKS_HEADER_SIZE; // The unwrapped packet size
				// *FIX: we are assuming ATYP is 0x01 (IPv4), not 0x03
				// (hostname) or 0x04 (IPv6)
				proxywrap_t* headerp = (proxywrap_t*)((void*)buffer);
				LLHost sender;			
				sender.setAddress(headerp->addr);
				sender.setPort(ntohs(headerp->port));
				packetp->init(buffer + SOCKS_HEADER_SIZE, packet_size, sender);

				mHeadIndex = (mHeadIndex + 1) % U32(mPacketRing.size());
				if (mNumBufferedPackets < mPacketRingMaxBufferSize)
				{
					++mNumBufferedPackets;
					mNumBufferedBytes += packet_size;
				}
				else
				{
					// We overwrote an older packet
					mNumBufferedBytes += packet_size - old_packet_size;
				}
			}
			else
			{
				packet_size = 0;
			}
		}
	}
	else
	{
		packetp->init(socket);
		packet_size = packetp->getSize();
		if (packet_size > 0)
		{
			mHeadIndex = (mHeadIndex + 1) % U32(mPacketRing.size());
			mActualBytesIn += packet_size;
			if (mNumBufferedPackets < mPacketRingMaxBufferSize)
			{
				++mNumBufferedPackets;
				mNumBufferedBytes += packet_size;
			}
			else
			{
				// We overwrote an older packet
				mNumBufferedBytes += packet_size - old_packet_size;
			}
		}
	}

	return packet_size;
}

// Drains to buffer
U32 LLPacketRing::drainSocket(LLMessageSystem* msgsys, S32 socket)
{
	S32 packet_size = 1;
	S32 num_loops = 0;
	S32 old_num_packets = mNumBufferedPackets;
	while (packet_size > 0)
	{
		packet_size = bufferInboundPacket(socket);
		++num_loops;
	}
	S32 dropped = (num_loops - 1 + old_num_packets) - mNumBufferedPackets;
	if (dropped > 0)
	{
		msgsys->mDroppedPackets += dropped;
		llwarns << "Dropped " << dropped << " UDP packets." << llendl;
	}
	return mNumBufferedPackets;
}

void LLPacketRing::expandRing()
{
	U32 old_size = mPacketRing.size();
	U32 new_size = llmin(old_size + BUFFER_RING_EXPANSION,
						 mPacketRingMaxBufferSize);
	if (new_size == old_size)
	{
		llwarns_once << "Ring buffer size already maxed out." << llendl;
		return;
	}

	// Make a larger ring and copy packet pointers
	packet_vec_t new_ring;
	new_ring.reserve(new_size);
	for (U32 i = 0; i < old_size; ++i)
	{
		new_ring.emplace_back(mPacketRing[(mHeadIndex + i) % old_size]);
	}
	// Allocate new packets for the remainder of new_ring
	LLHost invalid_host;
	for (U32 i = old_size; i < new_size; ++i)
	{
		new_ring.emplace_back(new LLPacketBuffer(invalid_host, NULL, 0));
	}
	// Swap the rings and reset mHeadIndex
	mPacketRing.swap(new_ring);
	mHeadIndex = mNumBufferedPackets;

	llinfos << "Ring buffer expanded to hold up to " << new_size << " packets."
			<< llendl
}

F32 LLPacketRing::getBufferLoadRate() const
{
	// Goes up to mPacketRingMaxBufferSize
	constexpr F32 SCALER = 1.f / (F32)DEFAULT_BUFFER_RING_SIZE;
	return (F32)mNumBufferedPackets * SCALER;
}
