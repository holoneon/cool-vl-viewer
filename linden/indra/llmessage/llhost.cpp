/**
 * @file llhost.cpp
 * @brief Encapsulates an IP address and a port.
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

#include "linden_common.h"

#include "llhost.h"

#if LL_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
#else
	#include <netdb.h>
	#include <netinet/in.h>	// ntonl()
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif

const char* LOOPBACK_ADDRESS_STRING = "127.0.0.1";
const char* BROADCAST_ADDRESS_STRING = "255.255.255.255";

///////////////////////////////////////////////////////////////////////////////
// Utility functions (used to be in llnet.cpp, but not related to the OS
// specific network implementation, thus moved here). HB
///////////////////////////////////////////////////////////////////////////////

const char* u32_to_ip_string(U32 ip)
{
	static char buffer[MAXADDRSTR];

	// Convert the IP address into a string
	in_addr in;
	in.s_addr = ip;
	char* result = inet_ntoa(in);
	// NULL indicates error in conversion
	if (result)
	{
		strncpy(buffer, result, MAXADDRSTR);
		buffer[MAXADDRSTR - 1] = '\0';
		return buffer;
	}
	return "(bad IP addr)";
}

// Returns ip_string if successful, NULL if not. Copies into ip_string
char* u32_to_ip_string(U32 ip, char* ip_string)
{
	char* result;
	in_addr in;

	// Convert the IP address into a string
	in.s_addr = ip;
	result = inet_ntoa(in);

	// NULL indicates error in conversion
	if (result != NULL)
	{
		// the function signature needs to change to pass in the lengfth of
		// first and last.
		strcpy(ip_string, result);
		return ip_string;
	}
	else
	{
		return NULL;
	}
}

// Wrapper for inet_addr()
U32 ip_string_to_u32(const char* ip_string)
{
	// *NOTE: Windows does not support inet_aton() so we are using inet_addr().
	// Unfortunately, INADDR_NONE == INADDR_BROADCAST, so we have to check
	// whether the input is a broadcast address before deciding that @ip_string
	// is invalid. Also, our definition of INVALID_HOST_IP_ADDRESS does not
	// allow us to use wildcard addresses. -Ambroff
	U32 ip = inet_addr(ip_string);
	if (ip == INADDR_NONE &&
		strncmp(ip_string, BROADCAST_ADDRESS_STRING, MAXADDRSTR) != 0)
	{
		llwarns << "ip_string_to_u32() failed, Error: Invalid IP string '"
				<< ip_string << "'" << llendl;
		return INVALID_HOST_IP_ADDRESS;
	}
	return ip;
}

///////////////////////////////////////////////////////////////////////////////
// LLHost class proper
///////////////////////////////////////////////////////////////////////////////
 
//static
const LLHost LLHost::invalid(INVALID_PORT, INVALID_HOST_IP_ADDRESS);

LLHost::LLHost(const std::string& ip_and_port)
{
	std::string::size_type colon_index = ip_and_port.find(":");
	if (colon_index == std::string::npos)
	{
		mIP = ip_string_to_u32(ip_and_port.c_str());
		mPort = 0;
	}
	else
	{
		std::string ip_str(ip_and_port, 0, colon_index);
		std::string port_str(ip_and_port, colon_index+1);

		mIP = ip_string_to_u32(ip_str.c_str());
		mPort = atol(port_str.c_str());
	}
}

std::string LLHost::getIPandPort() const
{
	return llformat("%s:%u", u32_to_ip_string(mIP), mPort);
}

std::string LLHost::getIPString() const
{
	return std::string(u32_to_ip_string(mIP));
}

std::string LLHost::getHostName() const
{
	hostent* he;
	if (INVALID_HOST_IP_ADDRESS == mIP)
	{
		llwarns << "LLHost::getHostName() : Invalid IP address" << llendl;
		return std::string();
	}

	he = gethostbyaddr((char*)&mIP, sizeof(mIP), AF_INET);
	if (!he)
	{
#if LL_WINDOWS
		llwarns << "Could not find host name for address " << mIP
				<< ". Error: " << WSAGetLastError() << llendl;
#else
		llwarns << "Could not find host name for address " << mIP
				<< ". Error: " << h_errno << llendl;
#endif
		return std::string();
	}
	return ll_safe_string(he->h_name);
}

bool LLHost::setHostByName(const std::string& hostname)
{
	hostent* he;
	std::string local_name(hostname);

#if LL_WINDOWS
	// We may need an equivalent for Linux, but not sure - djs
	LLStringUtil::toUpper(local_name);
#endif

	he = gethostbyname(local_name.c_str());
	if (!he)
	{
		U32 ip_address = ip_string_to_u32(hostname.c_str());
		he = gethostbyaddr((char *)&ip_address, sizeof(ip_address), AF_INET);
	}

	if (he)
	{
		mIP = *(U32*)he->h_addr_list[0];
		return true;
	}
	else
	{
		setAddress(local_name);

		// In windows, h_errno is a macro for WSAGetLastError(), so store value
		// here
		S32 error_number = h_errno;
		switch (error_number)
		{
			case TRY_AGAIN:	// XXX how to handle this case?
				llwarns << "Try again" << llendl;
				break;

			case HOST_NOT_FOUND:
			case NO_ADDRESS:	// NO_DATA
				llwarns << "Host not found" << llendl;
				break;

			case NO_RECOVERY:
				llwarns << "Unrecoverable error" << llendl;
				break;

			default:
				llwarns << "Unknown error #" << error_number << llendl;
		}
		return false;
	}
}

LLHost&	LLHost::operator=(const LLHost& rhs)
{
	if (this != &rhs)
	{
		set(rhs.getAddress(), rhs.getPort());
	}
	return *this;
}

std::ostream& operator<<(std::ostream& os, const LLHost& hh)
{
	os << u32_to_ip_string(hh.mIP) << ":" << hh.mPort ;
	return os;
}

#if 0
std::istream& operator>>(std::istream& is, LLHost& rh)
{
	is >> rh.mIP;
	is >> rh.mPort;
	return is;
}
#endif
