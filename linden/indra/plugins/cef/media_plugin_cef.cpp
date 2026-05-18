/**
 * @file media_plugin_cef.cpp
 * @brief CEF (Chromium Embedded Framework) plugin.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 *
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2010-2026, Henri Beauchamp.
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

#if LL_WINDOWS
# include <process.h>				// For _getpid()
#else
# include <unistd.h>				// For getpid()
#endif

#include "dullahan.h"
#ifndef CEF_VERSION
# include "dullahan_version.h"		// LL's dullahan.h does not include this...
#endif

#include "hbcookiesmerger.h"

#include "indra_constants.h"		// For indra keyboard codes
#include "lldiriterator.h"			// LLDirIterator::deleteRecursivelyInDir()
#include "llglheaders.h"			// For the GL_* constants
#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "llsdutil.h"
#include "lltimer.h"				// For ms_sleep()
#include "media_plugin_base.h"
#include "volume_catcher.h"

using namespace std::placeholders;

// Since I removed all compatibility code, let's make sure no one tries to
// build this plugin against incompatible Dullahan/CEF versions.
#if DULLAHAN_VERSION_MAJOR < 2 && DULLAHAN_VERSION_MINOR < 21
# error This plugin is not compatible with Dullahan versions prior to 1.21.
#endif
#if CHROME_VERSION_MAJOR < 139
# error This plugin is not compatible with CEF versions prior to 139.
#endif

// Log filenames
#define CEF_LOG LL_DIR_DELIM_STR "cef_log.txt"
#define COOKIES_DEBUG_LOG LL_DIR_DELIM_STR "debug_log.txt"
// Cookies store filename and location in the CEF cache directory.
#define COOKIES LL_DIR_DELIM_STR "Cookies"
#define DEFAULT_SUB_DIR LL_DIR_DELIM_STR "Default"
// Under Windows, a different sub-directory is used by CEF for its cookies...
// Go figure as to why !  Beside, still under Windows, there is also a crypto
// key stored in "Local State", which is used to encrypt the cookies value and
// is therefore needed to be able reuse the cookies; so we must also save this
// file under Windows (under Linux, and while the cookies value is encrypted as
// well, the key is not stored anywhere in the CEF cache and likely derived
// from a machine Id or something: it is anyone's guess as to why the same
// principle is not used under Windows *sighs*). HB
#if LL_WINDOWS
# define NETWORK_SUB_DIR LL_DIR_DELIM_STR "Network"
# define LOCAL_STATE LL_DIR_DELIM_STR "Local State"
#endif
// This is the file that exists in the CEF cache while the instance is running
// and which gets removed at instance exit. When not defined, no check is done.
#if LL_LINUX
# define RUNNING_MARKER LL_DIR_DELIM_STR "SingletonLock"
#elif LL_WINDOWS
# define RUNNING_MARKER LL_DIR_DELIM_STR "lockfile"
#endif

class MediaPluginCEF : public MediaPluginBase
{
public:
	MediaPluginCEF(LLPluginInstance::sendMessageFunction host_send_func,
				   void* host_user_data);
	~MediaPluginCEF() override;

	void receiveMessage(const char* msgstr) override;

private:
	bool init();

	bool initCEF();

	void primeCache();
	void saveCookies();

	void onPageChangedCallback(const unsigned char* pixels, int x, int y,
							   int width, int height);
	void onLoadError(int status, const std::string error_text,
					 const std::string error_url);
	void onOpenPopupCallback(const std::string url, const std::string target);
	void onCustomSchemeURLCallback(const std::string url, bool user_gesture,
								   bool is_redirect);
	void onConsoleMessageCallback(std::string message, std::string source,
								  int line);
	void onStatusMessageCallback(const std::string value);
	void onTitleChangeCallback(std::string title);
	void onTooltipCallback(const std::string text);
	bool onJSDialogCallback(const std::string origin_url,
							const std::string message_text,
							const std::string default_prompt_text);
	bool onJSBeforeUnloadCallback();
	void onLoadStartCallback();
	void onLoadEndCallback(int http_status_code);
	void onAddressChangeCallback(const std::string url);
	bool onHTTPAuthCallback(const std::string host, const std::string realm,
							std::string& username, std::string& password);

	void onCursorChangedCallback(dullahan::ECursorType type);
	void onRequestExitCallback();

	void postDebugMessage(const std::string& msg, bool warn = true);

	void authResponse(LLPluginMessage& message);

	const std::vector<std::string> onFileDialog(dullahan::EFileDialogType type,
												const std::string dialog_title,
												const std::string default_file,
												const std::string filter,
												bool& use_default);

	void keyEvent(dullahan::EKeyEvent key_event,
				  LLSD native_key_data = LLSD::emptyMap());
	void unicodeInput(std::string event,
					  LLSD native_key_data = LLSD::emptyMap());

	void checkEditState();
	void setVolume();

private:
	dullahan*					mCEFLib;

	VolumeCatcher				mVolumeCatcher;
	F32							mCurVolume;

	U32							mMinimumFontSize;
	U32							mDefaultFontSize;

	std::string 				mPluginPidStr;

	std::string 				mHostLanguage;
	std::string					mAuthUsername;
	std::string					mAuthPassword;
	std::string					mUserAgent;
	std::string					mPreferredFont;

	std::string					mRootCacheDir;
	std::string					mInstanceCacheDir;
	std::string					mUserCookiesDir;
	std::string					mInstanceCookiesDir;
	std::string					mCookiesLogFile;

	std::string					mPickedFile;
	std::vector<std::string>	mPickedFiles;

	std::string					mProxyHost;
	U16							mProxyPort;
	bool						mProxyEnabled;

	bool						mCookiesEnabled;
	bool						mJavascriptEnabled;
	bool						mAuthOK;
	bool						mRemoteFonts;
	bool						mCanCopy;
	bool						mCanCut;
	bool						mCanPaste;
	bool						mCanUndo;
	bool						mCanRedo;
	bool						mCanDelete;
	bool						mCanSelectAll;
	bool						mEnableMediaPluginDebugging;
	bool						mInitialized;
	bool						mPixelsDirty;
	bool						mCleanupDone;
};

MediaPluginCEF::MediaPluginCEF(LLPluginInstance::sendMessageFunction send_fn,
							   void* host_user_data)
:	MediaPluginBase(send_fn, host_user_data),
	mMinimumFontSize(0),
	mDefaultFontSize(0),
	mHostLanguage("en"),
	mCurVolume(0.5f),					// Set default to a reasonnable level
	mProxyPort(0),
	mProxyEnabled(false),
	mCookiesEnabled(true),
	mJavascriptEnabled(true),
	mAuthOK(false),
	mRemoteFonts(true),
	mCanCopy(false),
	mCanCut(false),
	mCanPaste(false),
	mCanUndo(false),
	mCanRedo(false),
	mCanDelete(false),
	mCanSelectAll(false),
	mEnableMediaPluginDebugging(false),
	mInitialized(false),
	mPixelsDirty(false),
	mCleanupDone(false)
{
#if LL_WINDOWS
	mPluginPidStr = std::to_string(_getpid());
#else
	mPluginPidStr = std::to_string(getpid());
#endif

	mWidth = 0;
	mHeight = 0;
	mDepth = 4;
	mPixels = 0;

	mCEFLib = new dullahan();

	setVolume();
}

MediaPluginCEF::~MediaPluginCEF()
{
	if (!mCleanupDone)
	{
		mCleanupDone = true;
		mCEFLib->requestExit();
		// Let some time for CEF to actually cleanup
		ms_sleep(1000);
	}
	mCEFLib->shutdown();

#if defined(RUNNING_MARKER)
	// Wait for CEF to actually exit.
	std::string marker_file = mInstanceCacheDir + RUNNING_MARKER;
	U32 max_loops = 20;	// 20 * 250ms = 5s
	while (--max_loops && LLFile::exists(marker_file))
	{
		ms_sleep(250);
	}
#endif

	saveCookies();

	// With CEF 120+, delete the per-CEF instance cache sub-directory. Note:
	// sadly, under Windows, CEF will write more files at exit (which happens
	// only *after* this destructor returns), so there will be leftovers in
	// the per-session CEF caches directories... HB
	LLDirIterator::deleteRecursivelyInDir(mInstanceCacheDir);
	LLFile::rmdir(mInstanceCacheDir);
}

bool MediaPluginCEF::initCEF()
{
	if (mInitialized)
	{
		return true;
	}

	// Setup Dullahan callbacks
	mCEFLib->setOnPageChangedCallback(
		std::bind(&MediaPluginCEF::onPageChangedCallback, this, _1, _2, _3, _4,
				  _5));
	mCEFLib->setOnOpenPopupCallback(
		std::bind(&MediaPluginCEF::onOpenPopupCallback, this, _1, _2));
	mCEFLib->setOnFileDialogCallback(
		std::bind(&MediaPluginCEF::onFileDialog, this, _1, _2, _3, _4, _5));
	mCEFLib->setOnLoadErrorCallback(
		std::bind(&MediaPluginCEF::onLoadError, this, _1, _2, _3));
	mCEFLib->setOnCustomSchemeURLCallback(
		std::bind(&MediaPluginCEF::onCustomSchemeURLCallback, this, _1, _2,
				  _3));
	mCEFLib->setOnConsoleMessageCallback(
		std::bind(&MediaPluginCEF::onConsoleMessageCallback, this, _1, _2,
				  _3));
	mCEFLib->setOnStatusMessageCallback(
		std::bind(&MediaPluginCEF::onStatusMessageCallback, this, _1));
	mCEFLib->setOnTitleChangeCallback(
		std::bind(&MediaPluginCEF::onTitleChangeCallback, this, _1));
	mCEFLib->setOnTooltipCallback(
		std::bind(&MediaPluginCEF::onTooltipCallback, this, _1));
	mCEFLib->setOnLoadStartCallback(
		std::bind(&MediaPluginCEF::onLoadStartCallback, this));
	mCEFLib->setOnLoadEndCallback(
		std::bind(&MediaPluginCEF::onLoadEndCallback, this, _1));
	mCEFLib->setOnAddressChangeCallback(
		std::bind(&MediaPluginCEF::onAddressChangeCallback, this, _1));
	mCEFLib->setOnHTTPAuthCallback(
		std::bind(&MediaPluginCEF::onHTTPAuthCallback, this, _1, _2, _3, _4));
	mCEFLib->setOnCursorChangedCallback(
		std::bind(&MediaPluginCEF::onCursorChangedCallback, this, _1));
	mCEFLib->setOnRequestExitCallback(
		std::bind(&MediaPluginCEF::onRequestExitCallback, this));
	mCEFLib->setOnJSDialogCallback(
		std::bind(&MediaPluginCEF::onJSDialogCallback, this, _1, _2, _3));
	mCEFLib->setOnJSBeforeUnloadCallback(
		std::bind(&MediaPluginCEF::onJSBeforeUnloadCallback, this));

	// Prepare CEF settings
	dullahan::dullahan_settings settings;
	settings.initial_width = 1024;
	settings.initial_height = 1024;
	settings.user_agent_substring =
		mCEFLib->makeCompatibleUserAgentString(mUserAgent);
	settings.log_file = mInstanceCacheDir + CEF_LOG;
	settings.root_cache_path = mInstanceCacheDir;
	settings.cookies_enabled = mCookiesEnabled;
	settings.accept_language_list = mHostLanguage;
	settings.javascript_enabled = mJavascriptEnabled;
	if (mProxyEnabled && !mProxyHost.empty())
	{
		std::ostringstream proxy_url;
		proxy_url << mProxyHost << ":" << mProxyPort;
		settings.proxy_host_port = proxy_url.str();
	}
	// WebRTC media removed until we can add granularity or query UI
	settings.media_stream_enabled = false;
	settings.background_color = 0xffffffff;
	settings.disable_gpu = false;
	settings.flip_mouse_y = false;
	settings.flip_pixels_y = true;
	settings.frame_rate = 60;
	settings.force_wave_audio = false;
	settings.autoplay_without_gesture = true;
	settings.java_enabled = false;
	settings.webgl_enabled = true;
	settings.log_verbose = mEnableMediaPluginDebugging;
	// Disable remote debugging:
	settings.remote_debugging_port = -1;
	std::vector<std::string> custom_schemes;
	custom_schemes.emplace_back("secondlife");
	custom_schemes.emplace_back("hop");
	custom_schemes.emplace_back("x-grid-info");
	custom_schemes.emplace_back("x-grid-location-info");
	mCEFLib->setCustomSchemes(custom_schemes);
#if HB_DULLAHAN_EXTENDED
	// Not implemented in LL's pre-compiled Dullahan
	settings.minimum_font_size = mMinimumFontSize;
	settings.default_font_size = mDefaultFontSize;
	settings.remote_fonts = mRemoteFonts;
	settings.preferred_font = mPreferredFont;
#else	// HB_DULLAHAN_EXTENDED
# if LL_WINDOWS
	std::vector<wchar_t> buffer(MAX_PATH + 1);
	GetCurrentDirectoryW(MAX_PATH, &buffer[0]);
	settings.host_process_path = ll_convert_wide_to_string(&buffer[0]);
# endif
#endif	// HB_DULLAHAN_EXTENDED

	// Launch Dullahan
	mInitialized = mCEFLib->init(settings);
	return mInitialized;
}

void MediaPluginCEF::primeCache()
{
	// Used for saving our cookies central database.
	LLFile::mkdir(mUserCookiesDir);
	// Starting with CEF 120, we *must* use a different cache directory for
	// each new CEF instance. Let's make it so we can still share cookies,
	// by copying the files of a central cookie store into the instance cache
	// and updating the central store on instance exit. HB
	LLFile::mkdir(mRootCacheDir);
	mInstanceCacheDir = mRootCacheDir + LL_DIR_DELIM_STR + mPluginPidStr;
	// Create a private cache directory for this CEF instance. HB
	bool success = LLFile::mkdir(mInstanceCacheDir);
	// We also need to pre-create the sub-directory into which the cookies
	// files will be copied. HB
	mInstanceCookiesDir = mInstanceCacheDir + DEFAULT_SUB_DIR;
	success = LLFile::mkdir(mInstanceCookiesDir);
#if defined(NETWORK_SUB_DIR)
	mInstanceCookiesDir += NETWORK_SUB_DIR;
	if (success)
	{
		success = LLFile::mkdir(mInstanceCookiesDir);
	}
#endif
	if (!success)
	{
		postDebugMessage("Failed to create CEF cache directory tree in: " +
						 mInstanceCacheDir);
		return;
	}

	// Copy our cookies central store file(s) into this CEF instance cache. HB
	success = LLFile::copy(mUserCookiesDir + COOKIES,
						   mInstanceCookiesDir + COOKIES);
#if defined(LOCAL_STATE)
	if (success)
	{
		success = LLFile::copy(mUserCookiesDir + LOCAL_STATE,
							   mInstanceCacheDir + LOCAL_STATE);
	}
#endif
	if (!success)
	{
		postDebugMessage("Failed to restore the cookies store");
	}
	postDebugMessage("Using cache directory: " + mInstanceCacheDir, false);
}

void MediaPluginCEF::saveCookies()
{
	if (!mCookiesEnabled)
	{
		// Nothing to do.
		return;
	}

	std::string cookies_db = mInstanceCookiesDir + COOKIES;
#if defined(LOCAL_STATE)
	std::string local_state = mInstanceCacheDir + LOCAL_STATE;
	if (!LLFile::exists(cookies_db) || !LLFile::exists(local_state))
#else
	if (!LLFile::exists(cookies_db))
#endif
	{
		// Do not touch our central cookies store in these cases...
		if (!mCookiesLogFile.empty())
		{
			llofstream log(mCookiesLogFile, std::ios::out | std::ios::app);
			log << "No valid cookies store in closed CEF instance."
				<< std::endl;
		}
		return;
	}

	std::string saved_cookies_db = mUserCookiesDir + COOKIES;
#if defined(LOCAL_STATE)
	std::string saved_local_state = mUserCookiesDir + LOCAL_STATE;
	if (!LLFile::exists(saved_cookies_db) ||
		!LLFile::exists(saved_local_state))
#else
	if (!LLFile::exists(saved_cookies_db))
#endif
	{
		// If the cookies store has not yet been saved or one of the necessary
		// files is missing, just use the cookies from this closed CEF session.
		LLFile::copy(cookies_db, saved_cookies_db);
#if defined(LOCAL_STATE)
		LLFile::copy(local_state, saved_local_state);
#endif
		if (!mCookiesLogFile.empty())
		{
			llofstream log(mCookiesLogFile, std::ios::out | std::ios::app);
			log << "Cookies master files primed." << std::endl;
		}
		return;
	}

	HBCookiesMerger cm(cookies_db, saved_cookies_db, mCookiesLogFile);
	// Allow 3 attempts, in case the database is being merged by another CEF
	// plugin instance. HB
	U32 attempts = 3;
	while (--attempts)
	{
		if (cm.merge())
		{
			return;	// Success.
		}
		// Wait 500ms between attempts.
		ms_sleep(500);
	}

	// Failed to merge cookies: overwrite.
	LLFile::copy(cookies_db, saved_cookies_db);
#if defined(LOCAL_STATE)
	LLFile::copy(local_state, saved_local_state);
#endif
	if (!mCookiesLogFile.empty())
	{
		llofstream log(mCookiesLogFile, std::ios::out | std::ios::app);
		log << "Cookies master store overwritten." << std::endl;
	}
}

void MediaPluginCEF::postDebugMessage(const std::string& msg, bool warn)
{
	if (mEnableMediaPluginDebugging)
	{
		LLPluginMessage debug_message(LLPLUGIN_MESSAGE_CLASS_MEDIA,
									  "debug_message");
		debug_message.setValue("message_text",
							   "CEF plugin (pid " + mPluginPidStr + "): " +
							   msg);
		debug_message.setValue("message_level",  warn ? "warn" : "info");
		sendMessage(debug_message);
	}
}

void MediaPluginCEF::onPageChangedCallback(const unsigned char* pixels, int x,
										   int y, int width, int height)
{
	if (mPixels && pixels)
	{
		if (mWidth == width && mHeight == height)
		{
			memcpy(mPixels, pixels, mWidth * mHeight * mDepth);
			mPixelsDirty = true;	// Mark for update during idle()
		}
		else
		{
			mCEFLib->setSize(mWidth, mHeight);
			// When size changes we need this to be certain that the page will
			// get drawn. HB
			mCEFLib->reload(true);	// true = ignore cache
		}
	}
}

void MediaPluginCEF::onLoadError(int status, const std::string error_text,
								 const std::string error_url)
{
	std::stringstream msg;
	msg << "<b>Loading error !</b><p>Message: " << error_text;
	msg << "<br />Error URL: " << error_url;
	msg << "<br />Code: " << status << "</p>";
	mCEFLib->showBrowserMessage(msg.str());
}

void MediaPluginCEF::onConsoleMessageCallback(std::string message,
											  std::string source, int line)
{
	std::stringstream str;
	str << "Console message: " << message << " in file(" << source
		<< ") at line " << line;
	postDebugMessage(str.str());
}

void MediaPluginCEF::onStatusMessageCallback(const std::string value)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
							"status_text");
	message.setValue("status", value);
	sendMessage(message);
}

void MediaPluginCEF::onTitleChangeCallback(std::string title)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", title);
	message.setValue("artist", "");
	message.setValueBoolean("history_back_available", mCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available",
							mCEFLib->canGoForward());
	sendMessage(message);
}

void MediaPluginCEF::onTooltipCallback(const std::string text)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "tooltip_text");
	message.setValue("tooltip", text);
	sendMessage(message);
}

bool MediaPluginCEF::onJSDialogCallback(const std::string origin_url,
										const std::string message_text,
										const std::string default_prompt_text)
{
	// Indicates we suppress the JavaScript alert UI entirely
	return true;
}

bool MediaPluginCEF::onJSBeforeUnloadCallback()
{
	// Indicates we suppress the JavaScript alert UI entirely
	return true;
}

void MediaPluginCEF::onLoadStartCallback()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
							"navigate_begin");
#if 0	// Not easily available here in CEF - needed ?
	message.setValue("uri", event.getEventUri());
#endif
	message.setValueBoolean("history_back_available", mCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available",
							mCEFLib->canGoForward());
	sendMessage(message);
}

void MediaPluginCEF::onLoadEndCallback(int http_status_code)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
							"navigate_complete");
#if 0	// Not easily available here in CEF - needed ?
	message.setValue("uri", event.getEventUri());
#endif
	message.setValueS32("result_code", http_status_code);
	message.setValueBoolean("history_back_available", mCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available",
							mCEFLib->canGoForward());
	sendMessage(message);
}

void MediaPluginCEF::onAddressChangeCallback(const std::string url)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
							"location_changed");
	message.setValue("uri", url);
	sendMessage(message);
}

void MediaPluginCEF::onOpenPopupCallback(const std::string url,
										 const std::string target)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
							"click_href");
	message.setValue("uri", url);
	message.setValue("target", target);
	message.setValue("uuid", "");		// Not used right now
	sendMessage(message);
}

void MediaPluginCEF::onCustomSchemeURLCallback(const std::string url,
											   bool user_gesture,
											   bool is_redirect)
{
	postDebugMessage("onCustomSchemeURLCallback() called with url = " +
					 url, false);
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER,
							"click_nofollow");
	message.setValue("uri", url);
	message.setValue("nav_type", user_gesture ? "clicked" : "navigated");
	message.setValueBoolean("is_redirect", is_redirect);
	sendMessage(message);
}

bool MediaPluginCEF::onHTTPAuthCallback(const std::string host,
										const std::string realm,
										std::string& username,
										std::string& password)
{
	mAuthOK = false;

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "auth_request");
	message.setValue("url", host);
	message.setValue("realm", realm);
	message.setValueBoolean("blocking_request", true);

	// The "blocking_request" key in the message means this sendMessage call
	// will block until a response is received.
	sendMessage(message);

	if (mAuthOK)
	{
		username = mAuthUsername;
		password = mAuthPassword;
	}

	return mAuthOK;
}

const std::vector<std::string>
MediaPluginCEF::onFileDialog(dullahan::EFileDialogType dialog_type,
							 const std::string dialog_title,
							 const std::string default_file,
							 const std::string dialog_accept_filter,
							 bool& use_default)
{
	// Never use the default CEF file picker
	use_default = false;

	if (dialog_type == dullahan::FD_OPEN_FILE ||
		dialog_type == dullahan::FD_OPEN_MULTIPLE_FILES)
	{
		mPickedFiles.clear();

		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "pick_file");
		message.setValueBoolean("blocking_request", true);
		message.setValueBoolean("multiple_files",
								dialog_type == dullahan::FD_OPEN_MULTIPLE_FILES);

		// The "blocking_request" key in the message means this sendMessage
		// call will block until a response is received.
		sendMessage(message);

		return mPickedFiles;
	}
	else if (dialog_type == dullahan::FD_SAVE_FILE)
	{
		mAuthOK = false;

		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "file_download");
		message.setValue("filename", default_file);

		sendMessage(message);
	}

	return std::vector<std::string>();
}

void MediaPluginCEF::onCursorChangedCallback(dullahan::ECursorType type)
{
	std::string name;

	switch (type)
	{
		case dullahan::CT_IBEAM:
			name = "ibeam";
			break;

		case dullahan::CT_NORTHSOUTHRESIZE:
			name = "splitv";
			break;

		case dullahan::CT_EASTWESTRESIZE:
			name = "splith";
			break;

		case dullahan::CT_HAND:
			name = "hand";
			break;

		// For anything else, default to the arrow
		case dullahan::CT_POINTER:
		default:
			name = "arrow";
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "cursor_changed");
	message.setValue("name", name);
	sendMessage(message);
}

void MediaPluginCEF::onRequestExitCallback()
{
	postDebugMessage("onRequestExitCallback() called", false);
	mCleanupDone = true;

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_BASE, "goodbye");
	sendMessage(message);

	mDeleteMe = true;
}

void MediaPluginCEF::authResponse(LLPluginMessage& message)
{
	mAuthOK = message.getValueBoolean("ok");
	if (mAuthOK)
	{
		mAuthUsername = message.getValue("username");
		mAuthPassword = message.getValue("password");
	}
}

void MediaPluginCEF::receiveMessage(const char* msgstr)
{
	if (mCleanupDone)
	{
		postDebugMessage(llformat("Received message: \"%s\" after cleanup !",
								  msgstr));
		return;
	}

	LLPluginMessage message_in;
	if (message_in.parse(msgstr) < 0)
	{
		return;
	}

	std::string message_class = message_in.getClass();
	std::string message_name = message_in.getName();

	if (mEnableMediaPluginDebugging)
	{
		// Do not spam cerr with a gazillon of idle messages...
		if (message_name != "idle" &&
			// Neither with mouse move messages !
			(message_name != "mouse_event" ||
			 std::string(msgstr).find("<string>move</string>") ==
				std::string::npos))
		{
			postDebugMessage(llformat("Received message: \"%s\"", msgstr),
							 false);
		}
	}

	if (message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
	{
		if (message_name == "init")
		{
			LLPluginMessage message(message_class, "init_response");
			LLSD versions = LLSD::emptyMap();
			versions[LLPLUGIN_MESSAGE_CLASS_BASE] =
				LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
			versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] =
				LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
			versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER] =
				LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION;
			message.setValueLLSD("versions", versions);

			std::string cef_version = CEF_VERSION;
			// Shorten the version string by removing the last commit hash. HB
			size_t i = cef_version.find('+');
			size_t j = cef_version.rfind('-');
			if (j > i && i != std::string::npos)
			{
				cef_version = cef_version.substr(0, i) + "/Chromium " +
							  cef_version.substr(j + 1);
			}
			message.setValue("plugin_version",
							 llformat("Dullahan %d.%d.%d/CEF %s",
									  DULLAHAN_VERSION_MAJOR,
									  DULLAHAN_VERSION_MINOR,
									  DULLAHAN_VERSION_POINT,
									  cef_version.c_str()));
			sendMessage(message);
		}
		else if (message_name == "idle")
		{
			mCEFLib->update();
			mVolumeCatcher.pump();
			checkEditState();
			if (mPixelsDirty)
			{
				mPixelsDirty = false;
				setDirty(0, 0, mWidth, mHeight);
			}
		}
		else if (message_name == "cleanup")
		{
			mCEFLib->requestExit();
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
					mPixels = NULL;
					mTextureSegmentName.clear();
				}
				mSharedSegments.erase(iter);
			}
			else
			{
				postDebugMessage("Unknown shared memory region !");
			}

			LLPluginMessage message(message_class, "shm_remove_response");
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
			bool result = initCEF();
			if (!result)
			{
				postDebugMessage("Dullahan initialization failed");
			}

			// Plugin gets to decide the texture parameters to use.
			mDepth = 4;
			LLPluginMessage message(message_class, "texture_params");
			message.setValueS32("default_width", 1024);
			message.setValueS32("default_height", 1024);
			message.setValueS32("depth", mDepth);
			message.setValueU32("internalformat", GL_RGB);
			message.setValueU32("format", GL_BGRA);
			message.setValueU32("type", GL_UNSIGNED_BYTE);
			message.setValueBoolean("coords_opengl", true);
			sendMessage(message);
		}
		else if (message_name == "set_user_data_path")
		{
			// Note: the paths always got a trailing platform-specific
			// directory delimiter.
			mRootCacheDir = message_in.getValue("cache") + "cef_cache";
			mUserCookiesDir = message_in.getValue("path") + "cef_cookies";
			primeCache();
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
					mCEFLib->setSize(mWidth, mHeight);
				}
			}

			mCEFLib->setSize(mWidth, mHeight);

			LLPluginMessage message(message_class, "size_change_response");
			message.setValue("name", name);
			message.setValueS32("width", width);
			message.setValueS32("height", height);
			message.setValueS32("texture_width", texture_width);
			message.setValueS32("texture_height", texture_height);
			sendMessage(message);
		}
		else if (message_name == "set_language_code")
		{
			mHostLanguage = message_in.getValue("language");
		}
		else if (message_name == "load_uri")
		{
			std::string uri = message_in.getValue("uri");
			mCEFLib->navigate(uri);
		}
		else if (message_name == "set_cookie")
		{
			std::string uri = message_in.getValue("uri");
			std::string name = message_in.getValue("name");
			std::string value = message_in.getValue("value");
			std::string domain = message_in.getValue("domain");
			std::string path = message_in.getValue("path");
			bool httponly = message_in.getValueBoolean("httponly");
			bool secure = message_in.getValueBoolean("secure");
			mCEFLib->setCookie(uri, name, value, domain, path, httponly,
							   secure);
		}
		else if (message_name == "mouse_event")
		{
			std::string event = message_in.getValue("event");

			S32 x = message_in.getValueS32("x");
			S32 y = message_in.getValueS32("y");

			dullahan::EMouseButton btn = dullahan::MB_MOUSE_BUTTON_LEFT;
			S32 button = message_in.getValueS32("button");
#if 1		// Do not transmit middle or right clicks
			if (button == 1 || button == 2) return;
#else
			if (button == 1) btn = dullahan::MB_MOUSE_BUTTON_RIGHT;
			if (button == 2) btn = BROWSER_MB_MIDDLE;
#endif
#if 0		// Not used for now
			std::string modifiers = message_in.getValue("modifiers");
#endif
			if (event == "down")
			{
				mCEFLib->mouseButton(btn, dullahan::ME_MOUSE_DOWN, x, y);
				mCEFLib->setFocus();
				std::stringstream str;
				str << "Mouse down at = " << x << ", " << y;
				postDebugMessage(str.str(), false);
			}
			else if (event == "up")
			{
				mCEFLib->mouseButton(btn, dullahan::ME_MOUSE_UP, x, y);
				std::stringstream str;
				str << "Mouse up at = " << x << ", " << y;
				postDebugMessage(str.str(), false);
			}
			else if (event == "double_click")
			{
				mCEFLib->mouseButton(btn, dullahan::ME_MOUSE_DOUBLE_CLICK,
									 x, y);
			}
			else
			{
				mCEFLib->mouseMove(x, y);
			}
		}
		else if (message_name == "scroll_event")
		{
			S32 x = message_in.getValueS32("x");
			S32 y = message_in.getValueS32("y");
			S32 delta_x = 40 * message_in.getValueS32("clicks_x");
			S32 delta_y = -40 * message_in.getValueS32("clicks_y");
			mCEFLib->mouseWheel(x, y, delta_x, delta_y);
		}
		else if (message_name == "text_event")
		{
			LLSD native_key_data = message_in.getValueLLSD("native_key_data");
			std::string event = message_in.getValue("event");
			unicodeInput(event, native_key_data);
		}
		else if (message_name == "key_event")
		{
			LLSD native_key_data = message_in.getValueLLSD("native_key_data");
			std::string event = message_in.getValue("event");
			// Treat unknown events as key-up for safety.
			dullahan::EKeyEvent key_event = dullahan::KE_KEY_UP;
			if (event == "down")
			{
				key_event = dullahan::KE_KEY_DOWN;
			}
			else if (event == "repeat")
			{
				key_event = dullahan::KE_KEY_REPEAT;
			}
			keyEvent(key_event, native_key_data);
		}
		else if (message_name == "enable_media_plugin_debugging")
		{
			mEnableMediaPluginDebugging = message_in.getValueBoolean("enable");
			if (!mEnableMediaPluginDebugging)
			{
				mCookiesLogFile.clear();
			}
			else if (!mUserCookiesDir.empty())
			{
				mCookiesLogFile = mUserCookiesDir + COOKIES_DEBUG_LOG;
				postDebugMessage("Using cookies debug log: " +
								 mCookiesLogFile, false);
			}
		}
		else if (message_name == "pick_file_response")
		{
			mPickedFile = message_in.getValue("file");
			LLSD file_list = message_in.getValueLLSD("file_list");
			for (LLSD::array_const_iterator iter = file_list.beginArray(),
											end = file_list.endArray();
				 iter != end; ++iter)
			{
				mPickedFiles.emplace_back((*iter).asString());
			}
			if (mPickedFiles.empty() && !mPickedFile.empty())
			{
				mPickedFiles.emplace_back(mPickedFile);
			}
		}
		else if (message_name == "auth_response")
		{
			authResponse(message_in);
		}
		else if (message_name == "edit_copy")
		{
			mCEFLib->editCopy();
		}
		else if (message_name == "edit_cut")
		{
			mCEFLib->editCut();
		}
		else if (message_name == "edit_paste")
		{
			mCEFLib->editPaste();
		}
		else if (message_name == "edit_undo")
		{
			mCEFLib->editUndo();
		}
		else if (message_name == "edit_redo")
		{
			mCEFLib->editRedo();
		}
		else if (message_name == "edit_delete")
		{
			mCEFLib->editDelete();
		}
		else if (message_name == "edit_select_all")
		{
			mCEFLib->editSelectAll();
		}
		else if (message_name == "edit_show_source")
		{
			mCEFLib->viewSource();
		}
	}
	else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER)
	{
		if (message_name == "set_page_zoom_factor")
		{
			F32 factor = (F32)message_in.getValueReal("factor");
			mCEFLib->setPageZoom(factor);
		}
		else if (message_name == "proxy_setup")
		{
			mProxyEnabled = message_in.getValueBoolean("enable");
			mProxyHost = message_in.getValue("host");
			mProxyPort = (U16)message_in.getValueS32("port");
		}
		else if (message_name == "cookies_enabled")
		{
			mCookiesEnabled = message_in.getValueBoolean("enable");
			if (mEnableMediaPluginDebugging)
			{
				std::string msg = "Enable cookies: ";
				msg += mCookiesEnabled ? "yes" : "no";
				postDebugMessage(msg, false);
			}
		}
		else if (message_name == "show_web_inspector")
		{
			mCEFLib->showDevTools();
		}
		else if (message_name == "javascript_enabled")
		{
			mJavascriptEnabled = message_in.getValueBoolean("enable");
		}
		else if (message_name == "minimum_font_size")
		{
			mMinimumFontSize = message_in.getValueU32("size");
		}
		else if (message_name == "default_font_size")
		{
			mDefaultFontSize = message_in.getValueU32("size");
		}
		else if (message_name == "remote_fonts")
		{
			mRemoteFonts = message_in.getValueBoolean("enable");
		}
		else if (message_name == "preferred_font")
		{
			mPreferredFont = message_in.getValue("font_family");
		}
		else if (message_name == "browse_stop")
		{
			mCEFLib->stop();
		}
		else if (message_name == "browse_reload")
		{
			bool ignore_cache = true;
			mCEFLib->reload(ignore_cache);
		}
		else if (message_name == "browse_forward")
		{
			mCEFLib->goForward();
		}
		else if (message_name == "browse_back")
		{
			mCEFLib->goBack();
		}
		else if (message_name == "clear_cookies")
		{
			mCEFLib->deleteAllCookies();
		}
		else if (message_name == "set_user_agent")
		{
			mUserAgent = message_in.getValue("user_agent");
		}
	}
	else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME)
	{
		if (message_name == "set_volume")
		{
			mCurVolume = (F32)message_in.getValueReal("volume");
			setVolume();
		}
	}
	else
	{
		postDebugMessage("Unknown message class: " + message_class);
	}
}

void MediaPluginCEF::keyEvent(dullahan::EKeyEvent key_event,
							  LLSD native_key_data)
{
#if LL_WINDOWS
	U32 msg = ll_U32_from_sd(native_key_data["msg"]);
	U32 wparam = ll_U32_from_sd(native_key_data["w_param"]);
	U64 lparam = ll_U32_from_sd(native_key_data["l_param"]);
	mCEFLib->nativeKeyboardEventWin(msg, wparam, lparam);
#elif LL_LINUX
	U32 native_virtual_key = native_key_data["virtual_key"].asInteger();
	if (native_virtual_key == (U32)'\n')
	{
		native_virtual_key = (U32)'\r';
	}
	U32 native_modifiers = native_key_data["sdl_modifiers"].asInteger();
	if (mEnableMediaPluginDebugging)
	{
		postDebugMessage(llformat("key_event = %u - native_virtual_key = %u - native_modifiers = %u",
								  key_event, native_virtual_key,
								  native_modifiers), false);
	}

	mCEFLib->nativeKeyboardEventLin2(key_event, native_virtual_key,
									 native_modifiers, false);

	if (key_event == dullahan::KE_KEY_UP && native_virtual_key == (U32)'\r')
	{
		// *HACK: to have CEF honor enter (e.g. to accept form input), in
		// excess of sending KE_KEY_UP/DOWN we must send a KE_KEY_CHAR event.
		mCEFLib->nativeKeyboardEventLin2(dullahan::KE_KEY_CHAR,
										 native_virtual_key, native_modifiers,
										 false);
	}
#endif
}

void MediaPluginCEF::unicodeInput(std::string event, LLSD native_key_data)
{
#if LL_WINDOWS
	event = ""; // Not needed here but prevents unused var warning as error
	U32 msg = ll_U32_from_sd(native_key_data["msg"]);
	U32 wparam = ll_U32_from_sd(native_key_data["w_param"]);
	U64 lparam = ll_U32_from_sd(native_key_data["l_param"]);
	mCEFLib->nativeKeyboardEventWin(msg, wparam, lparam);
#elif LL_LINUX && HB_DULLAHAN_EXTENDED
	U32 native_virtual_key = native_key_data["virtual_key"].asInteger();
	if (native_virtual_key == (U32)'\n')
	{
		native_virtual_key = (U32)'\r';
	}
	U32 native_modifiers = native_key_data["sdl_modifiers"].asInteger();
	if (mEnableMediaPluginDebugging)
	{
		postDebugMessage(llformat("native_scan_code = %u - native_modifiers = %u",
								  native_virtual_key, native_modifiers),
						 false);
	}
	mCEFLib->nativeKeyboardEventLin2(dullahan::KE_KEY_CHAR, native_virtual_key,
									 native_modifiers, false);
#endif
}

void MediaPluginCEF::checkEditState()
{
	bool can_copy = mCEFLib->editCanCopy();
	bool can_cut = mCEFLib->editCanCut();
	bool can_paste = mCEFLib->editCanPaste();
	bool can_undo = mCEFLib->editCanUndo();
	bool can_redo = mCEFLib->editCanRedo();
	bool can_delete = mCEFLib->editCanDelete();
	bool can_select_all = mCEFLib->editCanSelectAll();
	if (can_copy != mCanCopy || can_cut != mCanCut || can_paste != mCanPaste ||
		can_undo != mCanUndo || can_redo != mCanRedo ||
		can_delete != mCanDelete || can_select_all != mCanSelectAll)
	{
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "edit_state");

		if (can_copy != mCanCopy)
		{
			mCanCopy = can_copy;
			message.setValueBoolean("copy", can_copy);
		}
		if (can_cut != mCanCut)
		{
			mCanCut = can_cut;
			message.setValueBoolean("cut", can_cut);
		}
		if (can_paste != mCanPaste)
		{
			mCanPaste = can_paste;
			message.setValueBoolean("paste", can_paste);
		}
		if (can_undo != mCanUndo)
		{
			mCanUndo = can_undo;
			message.setValueBoolean("undo", can_undo);
		}
		if (can_redo != mCanRedo)
		{
			mCanRedo = can_redo;
			message.setValueBoolean("redo", can_redo);
		}
		if (can_delete != mCanDelete)
		{
			mCanDelete = can_delete;
			message.setValueBoolean("delete", can_delete);
		}
		if (can_select_all != mCanSelectAll)
		{
			mCanSelectAll = can_select_all;
			message.setValueBoolean("select_all", can_select_all);
		}
		message.setValueBoolean("show_source", true);
		sendMessage(message);
	}
}

void MediaPluginCEF::setVolume()
{
	mVolumeCatcher.setVolume(mCurVolume);
}

bool MediaPluginCEF::init()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", "CEF Plugin");
	sendMessage(message);

	return true;
}

int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func,
					  void* host_user_data,
					  LLPluginInstance::sendMessageFunction* plugin_send_func,
					  void** plugin_user_data)
{
	MediaPluginCEF* self = new MediaPluginCEF(host_send_func, host_user_data);
	*plugin_send_func = MediaPluginCEF::staticReceiveMessage;
	*plugin_user_data = (void*)self;

	return 0;
}
