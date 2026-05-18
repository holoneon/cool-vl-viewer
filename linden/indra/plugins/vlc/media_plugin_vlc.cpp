/**
 * @file media_plugin_vlc.cpp
 * @brief GStreamer-1.0 plugin for LLMedia API plugin system
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llgl.h"
#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "media_plugin_base.h"

#if LL_WINDOWS
# if LL_MSVC
#  include <basetsd.h>
typedef SSIZE_T ssize_t;
# endif
# include <mmsystem.h>				// For waveOutSetVolume()
#endif

#if LL_WINDOWS
# include <process.h>				// For _getpid()
#else
# include <unistd.h>				// For getpid()
#endif

#include "vlc/vlc.h"
#include "vlc/libvlc_version.h"

class MediaPluginVLC : public MediaPluginBase
{
public:
	MediaPluginVLC(LLPluginInstance::sendMessageFunction fn,
					  void* host_user_datap);

	void receiveMessage(const char* msg_str) override;

private:
	bool initVLC();
	void playMedia();
	void resetVLC();
	void setVolume(F64 volume);
	void setVolumeVLC();
	void metaData(const std::string& title, const std::string& artist);

	void postDebugMessage(const std::string& msg, bool warn = true);

	static const std::string& getVersion();

	static void* lock(void* datap, void** pixelsp);
	// Nothing to do here for the moment; we *could* modify pixels here to, for
	// example, Y flip, but this is done with a VLC video filter transform.
	static void unlock(void*, void*, void* const*) {}

	static void display(void* datap, void* id);

	void setDirty(int left, int top, int right, int bottom) override;
	void setDurationDirty();

	static void eventCallbacks(const libvlc_event_t* eventp, void* ptrp);

private:
	struct LLLibVLCContext
	{
		unsigned char*			mTexture;
		libvlc_media_player_t*	mPlayer;
		MediaPluginVLC*		mParent;
	};
	struct LLLibVLCContext	mLibVLCCallbackContext;

	libvlc_instance_t*		mLibVLC;
	libvlc_media_t*			mLibVLCMedia;
	libvlc_media_player_t*	mLibVLCMediaPlayer;

	std::string				mURL;
	F64						mCurVolume;
	F64						mCurTime;
	F64						mDuration;
	EStatus					mVlcStatus;
	bool					mIsLooping;
	bool					mEnableMediaPluginDebugging;
};

MediaPluginVLC::MediaPluginVLC(LLPluginInstance::sendMessageFunction fn,
									 void* host_user_datap)
:	MediaPluginBase(fn, host_user_datap),
	mLibVLC(NULL),
	mLibVLCMedia(NULL),
	mLibVLCMediaPlayer(NULL),
	mCurVolume(0.0),
	mCurTime(0.0),
	mDuration(0.0),
	mVlcStatus(STATUS_NONE),
	mIsLooping(false),
	mEnableMediaPluginDebugging(false)
{
	mTextureWidth = mTextureHeight = 0;
	mWidth = mHeight = 0;
	mDepth = 4;
	mPixels = 0;
	setStatus(STATUS_NONE);
}

//static
const std::string& MediaPluginVLC::getVersion()
{
	static std::string plugin_version =
		llformat("VLC media plugin, libvlc version %d.%d.%d",
				 LIBVLC_VERSION_MAJOR, LIBVLC_VERSION_MINOR,
				 LIBVLC_VERSION_REVISION);
	return plugin_version;
}

void* MediaPluginVLC::lock(void* datap, void** pixelsp)
{
	struct LLLibVLCContext* contextp = (LLLibVLCContext*)datap;
	if (contextp)
	{
		*pixelsp = contextp->mTexture;
	}
	return NULL;
}

void MediaPluginVLC::display(void* datap, void*)
{
	struct LLLibVLCContext* contextp = (LLLibVLCContext*)datap;
	if (contextp)
	{
		contextp->mParent->setDirty(0, 0, contextp->mParent->mWidth,
									contextp->mParent->mHeight);
	}
}

bool MediaPluginVLC::initVLC()
{
	char const* vlc_argv[] =
	{
		"--no-xlib",
		// MAINT-6578 Y flip textures in plugin vs client
		"--video-filter=transform{type=vflip}",
	};

	constexpr int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
	mLibVLC = libvlc_new(vlc_argc, vlc_argv);

	return mLibVLC != NULL;
}

void MediaPluginVLC::resetVLC()
{
	if (mLibVLC)
	{
		libvlc_media_player_stop(mLibVLCMediaPlayer);
		libvlc_media_player_release(mLibVLCMediaPlayer);
		libvlc_release(mLibVLC);
	}
}

//virtual
void MediaPluginVLC::setDirty(int left, int top, int right, int bottom)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "updated");

	message.setValueS32("left", left);
	message.setValueS32("top", top);
	message.setValueS32("right", right);
	message.setValueS32("bottom", bottom);

	message.setValueReal("current_time", mCurTime);
	message.setValueReal("duration", mDuration);
	message.setValueReal("current_rate", 1.f);

	sendMessage(message);
}

void MediaPluginVLC::setDurationDirty()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "updated");

	message.setValueReal("current_time", mCurTime);
	message.setValueReal("duration", mDuration);
	message.setValueReal("current_rate", 1.f);

	sendMessage(message);
}

void MediaPluginVLC::eventCallbacks(const libvlc_event_t* eventp, void* ptrp)
{
	MediaPluginVLC* parentp = (MediaPluginVLC*)ptrp;
	if (!parentp)
	{
		return;
	}

	switch (eventp->type)
	{
		case libvlc_MediaPlayerOpening:
			parentp->mVlcStatus = STATUS_LOADING;
			break;

		case libvlc_MediaPlayerPlaying:
			parentp->mDuration =
				0.001 * libvlc_media_get_duration(parentp->mLibVLCMedia);
			parentp->mVlcStatus = STATUS_PLAYING;
			parentp->setVolumeVLC();
			parentp->setDurationDirty();
			break;

		case libvlc_MediaPlayerPaused:
			parentp->mVlcStatus = STATUS_PAUSED;
			break;

		case libvlc_MediaPlayerStopped:
			parentp->mVlcStatus = STATUS_DONE;
			break;

		case libvlc_MediaPlayerEndReached:
			parentp->mVlcStatus = STATUS_DONE;
			parentp->mCurTime = parentp->mDuration;
			parentp->setDurationDirty();
			break;

		case libvlc_MediaPlayerEncounteredError:
			parentp->mVlcStatus = STATUS_ERROR;
			break;

		case libvlc_MediaPlayerTimeChanged:
			parentp->mCurTime =
				0.001 *
				libvlc_media_player_get_time(parentp->mLibVLCMediaPlayer);
			if (parentp->mVlcStatus == STATUS_DONE &&
				libvlc_media_player_is_playing(parentp->mLibVLCMediaPlayer))
			{
				parentp->mVlcStatus = STATUS_PLAYING;
			}
			parentp->setDurationDirty();
			break;

		case libvlc_MediaPlayerPositionChanged:
			break;

		case libvlc_MediaPlayerLengthChanged:
			parentp->mDuration = 0.001 *
								 libvlc_media_get_duration(parentp->mLibVLCMedia);
			parentp->setDurationDirty();
			break;

		case libvlc_MediaPlayerTitleChanged:
		{
			parentp->postDebugMessage("Received MediaPlayerTitleChanged event",
									  false);
			std::string title, artist;
			char* str = libvlc_media_get_meta(parentp->mLibVLCMedia,
											  libvlc_meta_Title);
			if (str)
			{
				title.assign(str);
				free(str);
			}
			str = libvlc_media_get_meta(parentp->mLibVLCMedia,
										libvlc_meta_Artist);
			if (str)
			{
				artist.assign(str);
				free(str);
			}
			if (!title.empty() || !artist.empty())
			{
				parentp->metaData(title, artist);
			}
		}
	}
}

void MediaPluginVLC::playMedia()
{
	if (!mLibVLC || mURL.empty())
	{
		return;
	}

	// A new call to play the media is received after the initial one.
	// Typically this is due to a size change request either as the media
	// naturally resizes to the size of the prim container, or else, as a 2D
	// window is resized by the user. Stopping the media helps avoid a race
	// condition where the media pixel buffer size is out of sync with the
	// declared size (width/height) for a frame or two and the plugin crashes
	// as VLC tries to decode a frame into unallocated memory.
	if (mLibVLCMediaPlayer)
	{
		libvlc_media_player_stop(mLibVLCMediaPlayer);
	}

	mLibVLCMedia = libvlc_media_new_location(mLibVLC, mURL.c_str());
	if (!mLibVLCMedia)
	{
		mLibVLCMediaPlayer = 0;
		setStatus(STATUS_ERROR);
		return;
	}

	mLibVLCMediaPlayer = libvlc_media_player_new_from_media(mLibVLCMedia);
	if (!mLibVLCMediaPlayer)
	{
		setStatus(STATUS_ERROR);
		return;
	}

	// Listen to events
	libvlc_event_manager_t* emp =
		libvlc_media_player_event_manager(mLibVLCMediaPlayer);
	if (emp)
	{
		libvlc_event_attach(emp, libvlc_MediaPlayerOpening,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerPlaying,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerPaused,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerStopped,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerEndReached,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerEncounteredError,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerTimeChanged,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerPositionChanged,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerLengthChanged,
							eventCallbacks, this);
		libvlc_event_attach(emp, libvlc_MediaPlayerTitleChanged,
							eventCallbacks, this);
	}

	libvlc_video_set_callbacks(mLibVLCMediaPlayer, lock, unlock, display,
							   &mLibVLCCallbackContext);
	libvlc_video_set_format(mLibVLCMediaPlayer, "RV32", mWidth, mHeight,
							mWidth * mDepth);

	mLibVLCCallbackContext.mParent = this;
	mLibVLCCallbackContext.mTexture = mPixels;
	mLibVLCCallbackContext.mPlayer = mLibVLCMediaPlayer;

	// Send a "navigate begin" event. This is really a browser message but the
	// QuickTime plugin did it and the media system relies on this message to
	// update internal state so we must send it too.
	// Note: see the "navigate_complete" message below too.
	LLPluginMessage message_begin(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
								  "navigate_begin");
	message_begin.setValue("uri", mURL);
	message_begin.setValueBoolean("history_back_available", false);
	message_begin.setValueBoolean("history_forward_available", false);
	sendMessage(message_begin);

	// Volume level gets set before VLC is initialized (thanks media system) so
	// we have to record it in mCurVolume and set it again here so that volume
	// levels are correctly initialized
	setVolume(mCurVolume);

	setStatus(STATUS_LOADED);

	// Note that this relies on the "set_loop" message arriving before the
	// "start" (play) one but that appears to always be the case.
	if (mIsLooping)
	{
		libvlc_media_add_option(mLibVLCMedia, "input-repeat=65535");
	}

	libvlc_media_player_play(mLibVLCMediaPlayer);

	// Send a "location_changed" message: this informs the media system that a
	// new URL is the 'current' one and is used extensively. Again, this is
	// really a browser message but we will use it here.
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
							"location_changed");
	message.setValue("uri", mURL);
	sendMessage(message);

	// Send a "navigate complete" event. This is really a browser message but
	// the QuickTime plugin did it and the media system relies on this message
	// to update internal state so we must send it too.
	// Note: see "navigate_begin" message above too.
	LLPluginMessage message_complete(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
									 "navigate_complete");
	message_complete.setValue("uri", mURL);
	message_complete.setValueS32("result_code", 200);
	message_complete.setValue("result_string", "OK");
	sendMessage(message_complete);
}

void MediaPluginVLC::metaData(const std::string& title,
							  const std::string& artist)
{
	postDebugMessage("Got new title/artist metadata", false);
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", title);
	message.setValue("artist", artist);
	sendMessage(message);
}

void MediaPluginVLC::setVolumeVLC()
{
	if (mLibVLCMediaPlayer)
	{
		// VLC volume is logarithmic. Let's exponentiate our linear volume. HB
		F64 vlc_volume = pow(mCurVolume, 1.0 / 3.0) * 100.0;
		libvlc_audio_set_volume(mLibVLCMediaPlayer, (int)vlc_volume);
#if LL_WINDOWS
		// Avoids having to recourse to the volume catcher.
		DWORD left_channel = (DWORD)(mCurVolume * 65535.0);
		DWORD right_channel = (DWORD)(mCurVolume * 65535.0);
		DWORD hw_volume = left_channel << 16 | right_channel;
		waveOutSetVolume(NULL, hw_volume);
#endif
	}
}

void MediaPluginVLC::setVolume(F64 volume)
{
	mCurVolume = volume;
	setVolumeVLC();
}

void MediaPluginVLC::postDebugMessage(const std::string& msg, bool warn)
{
	if (mEnableMediaPluginDebugging)
	{
		std::string log_msg = "VLC plugin (pid ";
# if LL_WINDOWS
		log_msg += std::to_string(_getpid());
# else
		log_msg += std::to_string(getpid());
# endif
		log_msg += "): " + msg;
		LLPluginMessage debug_message(LLPLUGIN_MESSAGE_CLASS_MEDIA,
									  "debug_message");
		debug_message.setValue("message_text", log_msg);
		debug_message.setValue("message_level", warn ? "warn" : "info");
		sendMessage(debug_message);
	}
}

void MediaPluginVLC::receiveMessage(const char* msg_str)
{
	LLPluginMessage message_in;

	if (message_in.parse(msg_str) >= 0)
	{
		std::string message_class = message_in.getClass();
		std::string message_name = message_in.getName();
		if (message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
		{
			if (message_name == "init")
			{
				if (!initVLC())
				{
					postDebugMessage("VLC initialization failed");
				}

				LLPluginMessage message("base", "init_response");
				LLSD versions = LLSD::emptyMap();
				versions[LLPLUGIN_MESSAGE_CLASS_BASE] =
					LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] =
					LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME] =
					LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME_VERSION;
				message.setValueLLSD("versions", versions);
				message.setValue("plugin_version", getVersion());
				sendMessage(message);
			}
			else if (message_name == "idle")
			{
				setStatus(mVlcStatus);
			}
			else if (message_name == "cleanup")
			{
				resetVLC();
			}
			else if (message_name == "force_exit")
			{
				mDeleteMe = true;
			}
			else if (message_name == "shm_added")
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer("address");
				info.mSize = (size_t)message_in.getValueS32("size");
				std::string name = message_in.getValue("name");
				mSharedSegments[name] = info;
			}
			else if (message_name == "shm_remove")
			{
				std::string name = message_in.getValue("name");

				shared_mem_map_t::iterator iter = mSharedSegments.find(name);
				if (iter != mSharedSegments.end())
				{
					if (mPixels == iter->second.mAddress)
					{
						libvlc_media_player_stop(mLibVLCMediaPlayer);
						libvlc_media_player_release(mLibVLCMediaPlayer);
						mLibVLCMediaPlayer = 0;

						mPixels = NULL;
						mTextureSegmentName.clear();
					}
					mSharedSegments.erase(iter);
				}
				else
				{
					postDebugMessage("Unknown shared memory region !");
				}

				// Send the response so it can be cleaned up.
				LLPluginMessage message("base", "shm_remove_response");
				message.setValue("name", name);
				sendMessage(message);
			}
			else
			{
				postDebugMessage("Unknown base message: " + message_name);
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
		{
			if (message_name == "init")
			{
				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA,
										"texture_params");
				message.setValueS32("default_width", 1024);
				message.setValueS32("default_height", 1024);
				message.setValueS32("depth", mDepth);
				message.setValueU32("internalformat", GL_RGB);
				message.setValueU32("format", GL_BGRA_EXT);
				message.setValueU32("type", GL_UNSIGNED_BYTE);
				message.setValueBoolean("coords_opengl", true);
				sendMessage(message);
			}
			else if (message_name == "size_change")
			{
				std::string name = message_in.getValue("name");
				S32 width = message_in.getValueS32("width");
				S32 height = message_in.getValueS32("height");
				S32 texture_width = message_in.getValueS32("texture_width");
				S32 texture_height = message_in.getValueS32("texture_height");

				if (!name.empty())
				{
					// Find the shared memory region with this name
					shared_mem_map_t::iterator iter = mSharedSegments.find(name);
					if (iter != mSharedSegments.end())
					{
						mPixels = (unsigned char*)iter->second.mAddress;
						mWidth = width;
						mHeight = height;
						mTextureWidth = texture_width;
						mTextureHeight = texture_height;

						libvlc_time_t time = (libvlc_time_t)(1000.0 * mCurTime);

						playMedia();

						if (mLibVLCMediaPlayer)
						{
							libvlc_media_player_set_time(mLibVLCMediaPlayer, time);
							time = libvlc_media_player_get_time(mLibVLCMediaPlayer);
							if (time < 0)
							{
								// -1 if there is no media
								mCurTime = 0;
							}
							else
							{
								mCurTime = (F64)time * 0.001;
							}
						}
					}
				}

				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA,
										"size_change_response");
				message.setValue("name", name);
				message.setValueS32("width", width);
				message.setValueS32("height", height);
				message.setValueS32("texture_width", texture_width);
				message.setValueS32("texture_height", texture_height);
				sendMessage(message);
			}
			else if (message_name == "load_uri")
			{
				mURL = message_in.getValue("uri");
				playMedia();
			}
			else if (message_name == "enable_media_plugin_debugging")
			{
				mEnableMediaPluginDebugging =
					message_in.getValueBoolean("enable");
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME)
		{
			if (message_name == "stop")
			{
				if (mLibVLCMediaPlayer)
				{
					libvlc_media_player_stop(mLibVLCMediaPlayer);
				}
			}
			else if (message_name == "start")
			{
				if (mLibVLCMediaPlayer)
				{
					if (mVlcStatus == STATUS_DONE &&
						!libvlc_media_player_is_playing(mLibVLCMediaPlayer))
					{
						// Stop or VLC will ignore 'play', it will just make an
						// MediaPlayerEndReached event even if seek was used.
						libvlc_media_player_stop(mLibVLCMediaPlayer);
					}
					libvlc_media_player_play(mLibVLCMediaPlayer);
				}
			}
			else if (message_name == "pause")
			{
				if (mLibVLCMediaPlayer)
				{
					libvlc_media_player_set_pause(mLibVLCMediaPlayer, 1);
				}
			}
			else if (message_name == "seek")
			{
				if (mLibVLCMediaPlayer)
				{
					libvlc_time_t time =
						(libvlc_time_t)(1000.0 *
										message_in.getValueReal("time"));
					libvlc_media_player_set_time(mLibVLCMediaPlayer, time);
					time = libvlc_media_player_get_time(mLibVLCMediaPlayer);
					if (time < 0)
					{
						// -1 if there is no media
						mCurTime = 0;
					}
					else
					{
						mCurTime = (F64)time * 0.001;
					}
					if (!libvlc_media_player_is_playing(mLibVLCMediaPlayer))
					{
						// If paused, it would not trigger update, update now
						setDurationDirty();
					}
				}
			}
			else if (message_name == "set_loop")
			{
				mIsLooping = message_in.getValueBoolean("loop");
			}
			else if (message_name == "set_volume")
			{
				// Volume comes in 0 -> 1.0
				F64 volume = message_in.getValueReal("volume");
				setVolume(volume);
			}
		}
		else
		{
			postDebugMessage("Unknown message class: " + message_class);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Plugin interface
///////////////////////////////////////////////////////////////////////////////

int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_fn,
					  void* hostdatap,
					  LLPluginInstance::sendMessageFunction* plugin_send_fn,
					  void** plugindatap)
{
	MediaPluginVLC* self = new MediaPluginVLC(host_send_fn, hostdatap);
	*plugin_send_fn = MediaPluginVLC::staticReceiveMessage;
	*plugindatap = (void*)self;

	return 0;	// Success
}
