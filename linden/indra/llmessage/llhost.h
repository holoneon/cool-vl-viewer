/**
 * @file llhost.h
 * @brief a LLHost uniquely defines a host (Simulator, Proxy or other)
 * across the network
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include <iostream>
#include <string>

#include "llerror.h"

///////////////////////////////////////////////////////////////////////////////
// This used to be in llnet.h header, but it is better kept here, since it is
// OS-independent and used by modules using LLHost and not llnet.cpp cross-
// platform functions which implementation is OS-dependent. HB

// Useful MTU constants
constexpr S32 MTUBYTES = 1200;
constexpr S32 MTUBITS = MTUBYTES * 8;

// For automatic port discovery when running multiple viewers on one host
constexpr U32 PORT_DISCOVERY_RANGE_MIN = 13000;
constexpr U32 PORT_DISCOVERY_RANGE_MAX = PORT_DISCOVERY_RANGE_MIN + 50;

#define NET_BUFFER_SIZE (0x2000)

// Request a free local port from the operating system
#define NET_USE_OS_ASSIGNED_PORT 0

// 123.567.901.345 = 15 chars + \0 + 1 for good luck
constexpr U32 MAXADDRSTR = 17;

// Returns pointer to internal string buffer, "(bad IP addr)" on failure,
// cannot nest calls
const char*	u32_to_ip_string(U32 ip);

// NULL on failure, ip_string on success, you must allocate at least MAXADDRSTR
// chars
char* u32_to_ip_string(U32 ip, char* ip_string);

// Wrapper for inet_addr()
U32 ip_string_to_u32(const char* ip_string);

extern const char* LOOPBACK_ADDRESS_STRING;
extern const char* BROADCAST_ADDRESS_STRING;
///////////////////////////////////////////////////////////////////////////////

constexpr U32 INVALID_PORT = 0;
constexpr U32 INVALID_HOST_IP_ADDRESS = 0x0;

class LLHost
{
protected:
	LOG_CLASS(LLHost);

public:
	// CREATORS

	// STL's hash_map expect this T()
	LLHost()
	:	mPort(INVALID_PORT),
		mIP(INVALID_HOST_IP_ADDRESS)
	{
	}

	LLHost(U32 ipv4_addr, U32 port)
	:	mPort(port),
		mIP(ipv4_addr)
	{
	}

	LLHost(const std::string& ipv4_addr, U32 port)
	:	mPort(port)
	{
		mIP = ip_string_to_u32(ipv4_addr.c_str());
	}

	explicit LLHost(U64 ip_port)
	{
		U32 ip = (U32)(ip_port >> 32);
		U32 port = (U32)(ip_port & (U64)0xFFFFFFFF);
		mIP = ip;
		mPort = port;
	}

	explicit LLHost(const std::string& ip_and_port);

	~LLHost()										{}

	// MANIPULATORS
	void set(U32 ip, U32 port)						{ mIP = ip; mPort = port; }
	void set(const std::string& ipstr, U32 port)	{ mIP = ip_string_to_u32(ipstr.c_str()); mPort = port; }
	void setAddress(const std::string& ipstr)		{ mIP = ip_string_to_u32(ipstr.c_str()); }
	void setAddress(U32 ip)							{ mIP = ip; }
	void setPort(U32 port)							{ mPort = port; }
	bool setHostByName(const std::string& hname);

	LLHost&	operator=(const LLHost& rhs);
	void invalidate()                        		{ mIP = INVALID_HOST_IP_ADDRESS; mPort = INVALID_PORT;};

	// READERS
	LL_INLINE U32 getAddress() const				{ return mIP; }
	LL_INLINE U32 getPort() const					{ return mPort; }
	LL_INLINE bool isOk() const						{ return mIP != INVALID_HOST_IP_ADDRESS && mPort != INVALID_PORT; }
	LL_INLINE bool isInvalid() const				{ return mIP == INVALID_HOST_IP_ADDRESS || mPort == INVALID_PORT; }
	size_t hash() const								{ return (mIP << 16) | (mPort & 0xffff); }

	std::string getIPString() const;
	std::string getHostName() const;
	std::string getIPandPort() const;

	LL_INLINE std::string getUntrustedSimulatorCap() const
	{
		return mUntrustedSimCap;
	}

	LL_INLINE void setUntrustedSimulatorCap(const std::string& url)
	{
		mUntrustedSimCap = url;
	}

	friend std::ostream& operator<<(std::ostream& os, const LLHost& hh);

	// This operator is not well defined. does it expect a
	// "192.168.1.1:80" notation or "int int" format? Phoenix 2007-05-18
	//friend std::istream& operator>> (std::istream& is, LLHost& hh);

	friend LL_INLINE bool operator==(const LLHost& lhs, const LLHost& rhs);
	friend LL_INLINE bool operator!=(const LLHost& lhs, const LLHost& rhs);
	friend LL_INLINE bool operator<(const LLHost& lhs, const LLHost& rhs);

public:
	static const LLHost	invalid;

protected:
	std::string			mUntrustedSimCap;
	U32					mPort;
	U32					mIP;
};

// std::hash implementation for LLHost
namespace std
{
	template<> struct hash<LLHost>
	{
		LL_INLINE size_t operator()(const LLHost& host) const noexcept
		{
			return host.hash();
		}
	};
}

// For use with boost::unordered_map and boost::unordered_set
LL_INLINE size_t hash_value(const LLHost& host) noexcept
{
	return host.hash();
}

LL_INLINE bool operator==(const LLHost& lhs, const LLHost& rhs)
{
	return lhs.mIP == rhs.mIP && lhs.mPort == rhs.mPort;
}

LL_INLINE bool operator!=(const LLHost& lhs, const LLHost& rhs)
{
	return lhs.mIP != rhs.mIP || lhs.mPort != rhs.mPort;
}

LL_INLINE bool operator<(const LLHost& lhs, const LLHost& rhs)
{
	if (lhs.mIP < rhs.mIP)
	{
		return true;
	}
	if (lhs.mIP > rhs.mIP)
	{
		return false;
	}
	return lhs.mPort < rhs.mPort;
}
