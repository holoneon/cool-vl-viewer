/**
 * @file media_plugin_base.h
 * @brief Media plugin base class for LLMedia API plugin system
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

#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"

class MediaPluginBase
{
public:
	// @param[in] host_send_func Function for sending messages from plugin to
	//			  plugin loader shell
	// @param[in] host_user_data Message data for messages from plugin to
	//			  plugin loader shell
	MediaPluginBase(LLPluginInstance::sendMessageFunction host_send_func,
					void* host_user_data);

	virtual ~MediaPluginBase() = default;

	// Handle received message from plugin loader shell.
	virtual void receiveMessage(const char* message_string) = 0;

	/**
	 * Receives message from plugin loader shell.
	 *
	 * @param[in] message_string Message string
	 * @param[in] user_data Message data
	 */
	static void staticReceiveMessage(const char* message_string,
									 void** user_data);

protected:
   // Plugin status.
	typedef enum
	{
		STATUS_NONE,
		STATUS_LOADING,
		STATUS_LOADED,
		STATUS_ERROR,
		STATUS_PLAYING,
		STATUS_PAUSED,
		STATUS_DONE
	} EStatus;

	// Plugin shared memory.
	class SharedSegmentInfo
	{
	public:
		void*	mAddress;
		size_t	mSize;
	};

	/**
	 * Sends message to plugin loader shell.
	 *
	 * @param[in] message Message data being sent to plugin loader shell
	 */
	void sendMessage(const LLPluginMessage& message);

	/**
	 * Sends "media_status" message to plugin loader shell ("loading",
	 * "playing", "paused", etc.)
	 */
	void sendStatus();

	/**
	 * Converts current media status enum value into string (STATUS_LOADING
	 * into "loading", etc.)
	 *
	 * @return Media status string ("loading", "playing", "paused", etc)
	 */
	std::string statusString();

	/**
	 * Sets media status.
	 *
	 * @param[in] status Media status (STATUS_LOADING, STATUS_PLAYING,
	 *			  STATUS_PAUSED, etc)
	 */
	void setStatus(EStatus status);

	/**
	 * Notifies plugin loader shell that part of display area needs to be redrawn.
	 *
	 * @param[in] left Left X coordinate of area to redraw
	 * @param[in] top Top Y coordinate of area to redraw
	 * @param[in] right Right X-coordinate of area to redraw
	 * @param[in] bottom Bottom Y-coordinate of area to redraw
	 * Note: the (0,0) coordinates correspond to the top left corner.
	 */
	virtual void setDirty(int left, int top, int right, int bottom);

protected:
	// Function to send message from plugin to plugin loader shell.
	LLPluginInstance::sendMessageFunction				mHostSendFunction;

	// Message data being sent to plugin loader shell by mHostSendFunction.
	void*												mHostUserData;

	// Pixel array to display. *TODO documentation: are pixels always 24 bits
	// RGB format, aligned on 32 bits boundary ?  Also, calling this a pixel
	// array may be misleading since 1 pixel > 1 char.
	unsigned char*										mPixels;

	// *TODO documentation: what is this for ?  Does a texture have its own
	// piece of shared memory ?  Updated on size_change_request, cleared on
	// shm_remove.
	std::string											mTextureSegmentName;

	// Map of shared memory names to shared memory.
	typedef std::map<std::string, SharedSegmentInfo,
					 std::less<> >	shared_mem_map_t;
	shared_mem_map_t									mSharedSegments;

	// Width of plugin display in pixels.
	int													mWidth;
	// Height of plugin display in pixels.
	int													mHeight;
	// Width of plugin texture.
	int													mTextureWidth;
	// Height of plugin texture.
	int													mTextureHeight;
	// Pixel depth (pixel size in bytes).
	int													mDepth;

	// Current status of plugin.
	EStatus												mStatus;

	// Flag to delete plugin instance (self).
	bool												mDeleteMe;
};

/** The plugin <b>must</b> define this function to create its instance.
 * It should look something like this:
 * @code
 * {
 *	MediaPluginFoo *self = new MediaPluginFoo(host_send_func, host_user_data);
 *	*plugin_send_func = MediaPluginFoo::staticReceiveMessage;
 *	*plugin_user_data = (void*)self;
 *
 *	return 0;
 * }
 * @endcode
 */
int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func,
					  void* host_user_data,
					  LLPluginInstance::sendMessageFunction* plugin_send_func,
					  void** plugin_user_data);
