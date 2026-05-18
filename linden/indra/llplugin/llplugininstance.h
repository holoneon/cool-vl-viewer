/**
 * @file llplugininstance.h
 * @brief LLPluginInstance handles loading the dynamic library of a plugin and setting up its entry points for message passing.
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

#include "llapr.h"
#include "llstring.h"

#include "apr_dso.h"

// LLPluginInstanceMessageListener receives messages sent from the plugin
// loader shell to the plugin.
class LLPluginInstanceMessageListener
{
public:
	virtual ~LLPluginInstanceMessageListener()	{}

	// Plugin receives message from plugin loader shell.
	virtual void receivePluginMessage(const std::string& message) = 0;
};

// LLPluginInstance handles loading the dynamic library of a plugin and setting
// up its entry points for message passing.
class LLPluginInstance
{
protected:
	LOG_CLASS(LLPluginInstance);

public:
	LLPluginInstance(LLPluginInstanceMessageListener* owner);
	virtual ~LLPluginInstance();

	// Loads a plugin dll/dylib/so
	// Returns 0 if successful, APR error code or error code returned from
	// the plugin's init function on failure.
	int load(const std::string& plugin_dir, const std::string& plugin_file);

	// Sends a message to the plugin.
	void sendMessage(const std::string &message);

	// The signature of the method for sending a message from plugin to plugin
	// loader shell. 'message' is a nul-terminated C string and 'data' is the
	// opaque reference that the callee supplied during setup.
 	typedef void (*sendMessageFunction) (const char* message, void** data);

	// The signature of the plugin init function. *TODO: check direction
	// (pluging loader shell to plugin ?)
	// 'host_user_data' is the data from plugin loader shell.
	// 'plugin_send_function' is the method for sending from the plugin loader
	// shell to plugin.
	typedef int (*pluginInitFunction)(sendMessageFunction host_send_func,
									  void* host_user_data,
									  sendMessageFunction* plugin_send_func,
									  void** plugin_user_data);

private:
	static void staticReceiveMessage(const char* message, void** data);
	void receiveMessage(const char* message);

public:
	// Name of plugin init function
	static const char*					PLUGIN_INIT_FUNCTION_NAME;

private:
	LLPluginInstanceMessageListener*	mOwner;

	apr_dso_handle_t*					mDSOHandle;

	void*								mPluginUserData;
	sendMessageFunction					mPluginSendMessageFunction;
};
