/**
 * @file llplugininstance.cpp
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

#include "linden_common.h"

#include "llplugininstance.h"

#include "llapr.h"

#if LL_WINDOWS
# include "direct.h"		// Needed for _chdir()
#endif

const char* LLPluginInstance::PLUGIN_INIT_FUNCTION_NAME = "LLPluginInitEntryPoint";

LLPluginInstance::LLPluginInstance(LLPluginInstanceMessageListener* ownerp)
:	mOwner(ownerp),
	mDSOHandle(NULL),
	mPluginUserData(NULL),
	mPluginSendMessageFunction(NULL)
{
}

LLPluginInstance::~LLPluginInstance()
{
	if (mDSOHandle)
	{
		apr_dso_unload(mDSOHandle);
		mDSOHandle = NULL;
	}
}

// Dynamically loads the plugin and runs the plugin's init function.
// plugin_file is the file name of plugin dll/dylib/so.
// Returns 0 if successful, APR error code or error code from the plugin init
// function on failure.
int LLPluginInstance::load(const std::string& plugin_dir,
						   const std::string& plugin_file)
{
	pluginInitFunction init_function = NULL;

	if (plugin_dir.length())
	{
#if LL_WINDOWS
		// VWR-21275:
		// *SOME* Windows systems fail to load the Qt plugins if the current
		// working directory is not the same as the directory with the Qt DLLs
		// in. This should not cause any run time issues since we are changing
		// the cwd for the plugin shell process and not the viewer.
		// Changing back to the previous directory is not necessary since the
		// plugin shell quits once the plugin exits.
		_chdir(plugin_dir.c_str());
#endif
	}

	int result = apr_dso_load(&mDSOHandle, plugin_file.c_str(), gAPRPoolp);
	if (result != APR_SUCCESS)
	{
		char buf[1024];
		apr_dso_error(mDSOHandle, buf, sizeof(buf));

		llwarns << "apr_dso_load() of " << plugin_file << " failed with error "
				<< result << " , additional info string: " << buf << llendl;

	}

	if (result == APR_SUCCESS)
	{
		result = apr_dso_sym((apr_dso_handle_sym_t*)&init_function,
							 mDSOHandle,
							 PLUGIN_INIT_FUNCTION_NAME);

		if (result != APR_SUCCESS)
		{
			llwarns << "apr_dso_sym() failed with error " << result << llendl;
		}
	}

	if (result == APR_SUCCESS)
	{
		result = init_function(staticReceiveMessage, (void*)this,
							   &mPluginSendMessageFunction, &mPluginUserData);

		if (result == APR_SUCCESS)
		{
			llinfos << "Loaded " << plugin_file << " successfully." << llendl;
		}
		else
		{
			llwarns << "Call to init function failed with error " << result
					<< llendl;
		}
	}

	return result;
}

// Sends a message to the plugin.
void LLPluginInstance::sendMessage(const std::string& message)
{
	if (mPluginSendMessageFunction)
	{
		LL_DEBUGS("PluginMessages") << "sending message to plugin: \""
									<< message << "\"" << LL_ENDL;
		mPluginSendMessageFunction(message.c_str(), &mPluginUserData);
	}
	else
	{
		llwarns << "Dropping message: \"" << message << "\"" << llendl;
	}
}

//static
void LLPluginInstance::staticReceiveMessage(const char* message_string,
											void** user_data)
{
	// *TODO: validate that the user_data argument is still a valid
	// LLPluginInstance pointer. We could also use a key that is looked up in a
	// map (instead of a direct pointer) for safety, but that is probably
	// overkill.
	LLPluginInstance* self = (LLPluginInstance*)*user_data;
	if (self)
	{
		self->receiveMessage(message_string);
	}
}

// Plugin receives message from plugin loader shell.
void LLPluginInstance::receiveMessage(const char* message_string)
{
	if (!mOwner)
	{
		llwarns_once << "No owner: cannot process messages !" << llendl;
		return;
	}

	LL_DEBUGS("PluginMessages") << "processing incoming message: \""
								<< message_string << "\"" << LL_ENDL;
	mOwner->receivePluginMessage(message_string);
}
