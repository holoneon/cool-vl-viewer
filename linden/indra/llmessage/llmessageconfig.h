/**
 * @file llmessageconfig.h
 * @brief Live file handling for messaging
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

#include <string>

#include "llsd.h"

class LLMessageConfig
{
protected:
	LOG_CLASS(LLMessageConfig);

public:
	static void initClass(const std::string& config_dir);

	enum Flavor { NO_FLAVOR = 0, LLSD_FLAVOR = 1, TEMPLATE_FLAVOR = 2 };
	static Flavor getServerDefaultFlavor();

	// For individual messages
	static Flavor getMessageFlavor(const std::string& msg_name);
	static bool isValidMessage(const std::string& msg_name);
};
