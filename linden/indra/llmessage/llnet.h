/**
 * @file llnet.h
 * @brief OS-specific implementation of cross-platform utility functions.
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

#include "llhost.h"

// Returns 0 on success, non-zero on error. Sets socket handler/descriptor,
// changes port_num if port requested is unavailable.
S32 start_net(S32& socket_out, int& port_num);

void end_net(S32& socket_out);

// Returns size of packet or 0 in case of error
S32 receive_packet(int sock_num, char* recv_buffer);

// Returns true on success.
bool send_packet(int sock_num, const char* send_buffer, int size,
				 U32 recipient, int port_num);

LLHost get_sender();
U32 get_sender_port();
U32 get_sender_ip();
LLHost get_receiving_interface();
U32 get_receiving_interface_ip();
