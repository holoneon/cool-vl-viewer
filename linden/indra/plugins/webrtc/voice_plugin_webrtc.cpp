/**
 * @file voice_plugin_webrtc.cpp
 * @brief Plugin for WebRTC voice support.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024-2025, Henri Beauchamp.
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

// Implementing communications with the WebRTC library as a plugin got three
// purposes:
//   1.- Isolate entirely the WebRTC custom threading model from the viewer's:
//		 the first llwebrtc/WebRTC versions dead-locked a lot on disconnection
//		 in shutdownConnection() under Linux and my hypothesis was that it was
//		 caused by the fact that WebRTC methods (which use their own threads)
//		 were called from coroutines (boost::fiber ones); the backtraces hinted
//		 for such a potential issue, but sadly this deadlock still happened in
//		 the plugin under Linux... At least, the plugin model avoids to see the
//		 viewer freezing due to any deadlock, allowing to destroy the frozen
//		 plugin and to relaunch a fresh one to re-kick voice connections.
//   2.- Make sure that in case of any crash or abort() in the WebRTC library
//       (which loves to use abort() when it gets "stuck", crashing its user
//       process instead of politely bowing out: seen, for example, when WebTRC
//		 is compiled with PulseAudio support under Linux and the latter is
//		 absent from the end user system, or badly configured), the viewer will
//		 keep running (and attempt to launch a new plugin instance to recover
//		 voice if possible).
//   3.- Avoid frame rate "hiccups" otherwise encountered when calling the
//       WebRTC library from the viewer main thread (or its coroutines).
// HB

#include "linden_common.h"

#include <map>
#include <mutex>
#include <sstream>

#include "llwebrtc.h"

#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "media_plugin_base.h"

using namespace llwebrtc;

// Note: MediaPluginBase is an overkill as a parent class, but I am too lazy to
// implement a simpler base plugin class just for the voice plugin... HB
class HBPluginVoiceWebRTC final : public MediaPluginBase,
								  public LLWebRTCDevicesObserver,
								  public LLWebRTCLogCallback
{
public:
	HBPluginVoiceWebRTC(LLPluginInstance::sendMessageFunction host_send_fn,
						void* hostdatap);
	~HBPluginVoiceWebRTC() override;

	static void shutdown();

	// MediaPluginBase override
	void receiveMessage(const char* message_string) override;

	// LLWebRTCDevicesObserver overrides
	typedef LLWebRTCVoiceDeviceList rtcdev_list_t;
	void OnDevicesChanged(const rtcdev_list_t& render_devices,
						  const rtcdev_list_t& capture_devices) override;

	// LLWebRTCLogCallback override
	void LogMessage(LogLevel level, const std::string& message) override;

	LL_INLINE F32 getAudioLevel()
	{
		if (!mDeviceInterface)
		{
			return 0.f;
		}
		if (mIsInTuningMode)
		{
			return mDeviceInterface->getTuningAudioLevel();
		}
		return mDeviceInterface->getPeerConnectionAudioLevel();
	}

	LL_INLINE void setTuningMode(bool tuning_on)
	{
		if (mIsInTuningMode != tuning_on && mDeviceInterface)
		{
			mIsInTuningMode = tuning_on;
			mDeviceInterface->setTuningMode(tuning_on);
		}
	}

	LL_INLINE void refreshDevices()
	{
		if (mDeviceInterface)
		{
			mDeviceInterface->refreshDevices();
		}
	}

	LL_INLINE void setCaptureDevice(const std::string& device_id)
	{
		if (mDeviceInterface)
		{
			mDeviceInterface->setCaptureDevice(device_id);
		}
	}

	LL_INLINE void setRenderDevice(const std::string& device_id)
	{
		if (mDeviceInterface)
		{
			mDeviceInterface->setRenderDevice(device_id);
		}
	}

	LL_INLINE void setAudioConfig(const LLSD& config)
	{
		if (!mDeviceInterface)
		{
			return;
		}
		LLWebRTCDeviceInterface::AudioConfig conf;
		conf.mEchoCancellation = config.has("cancel_echo") &&
								 config["cancel_echo"].asBoolean();
		conf.mAGC = config.has("agc") && config["agc"].asBoolean();
		S32 l = config.has("noise_suppression") ?
					config["noise_suppression"].asInteger() : 2;
		conf.mNoiseSuppressionLevel =
			(LLWebRTCDeviceInterface::AudioConfig::ENoiseSuppressionLevel)l;
		mDeviceInterface->setAudioConfig(conf);
	}

	LL_INLINE void setMicGain(F32 gain)
	{
		if (mDeviceInterface)
		{
		    mDeviceInterface->setMicGain(gain);
		}
	}

	LL_INLINE void setTuningGain(F32 gain)
	{
		if (mDeviceInterface)
		{
			mDeviceInterface->setTuningMicGain(gain);
		}
	}

	LL_INLINE void setGlobalMute(bool mute, F32 delay)
	{
		if (mDeviceInterface)
		{
			mDeviceInterface->setMute(mute, delay);
		}
	}

	// This sub-class is used for each connection instance. It basically
	// implements all the connection-related WebRTC-only code that used to be
	// implemented in llvoicewebrtc.cpp.
	class Connection final : public LLWebRTCSignalingObserver,
							 public LLWebRTCDataObserver
	{
	public:
		Connection();
		~Connection();

		// LLWebRTCSignalingObserver overrides
		void OnIceGatheringState(EIceGatheringState state) override;
		void OnIceCandidate(const LLWebRTCIceCandidate& cddt) override;
		void OnOfferAvailable(const std::string& sdp) override;
		void OnRenegotiationNeeded() override;
		void OnPeerConnectionClosed() override;
		void OnAudioEstablished(LLWebRTCAudioInterface* ifp) override;

		// LLWebRTCDataObserver overrides
		void OnDataChannelReady(LLWebRTCDataInterface* ifp) override;
		void OnDataReceived(const std::string& data, bool binary) override;

		// Must be called (with mDataMutex locked !) each time after we could
		// update the plugin owner about the last changes that came from
		// WebRTC.
		LL_INLINE void resetData()
		{
			mChannelSDP.clear();
			mChannelData.clear();
			mIceCandidates.clear();
			mIceCompleted = mIceCompleteChanged = mIceDataChanged = false;
			mGotDataInterface = mSessionEstablished = false;
			mConnectionClosed = mRenegotiationNeeded = false;
			mHasUpdates = false;
		}

		LL_INLINE void lock()
		{
			mDataMutex.lock();
		}

		LL_INLINE void unlock()
		{
			mDataMutex.unlock();
		}

		LL_INLINE bool connect(const LLWebRTCPeerConnectionInterface::InitOptions& opt)
		{
			return mPeerInterface && mPeerInterface->initializeConnection(opt);
		}

		LL_INLINE void disconnect()
		{
			if (mPeerInterface)
			{
				// Note: this llwebrtc method is prone to deadlocks (stuck
				// at pthread_cond_wait() which never sees its condition met)
				// under Linux.
				mPeerInterface->shutdownConnection();
			}
		}

		LL_INLINE void answer(const std::string& sdp)
		{
			if (mPeerInterface)
			{
				mPeerInterface->AnswerAvailable(sdp);
			}
		}

		LL_INLINE bool sendData(const std::string& json_data)
		{
			if (mDataInterface)
			{
				mDataInterface->sendData(json_data, false);
				return true;
			}
			return false;
		}

		LL_INLINE void closeDataInterface()
		{
			if (mDataInterface)
			{
				mDataInterface->unsetDataObserver(this);
				mDataInterface = NULL;
			}
			mAudioInterface = NULL;
		}

		LL_INLINE void mute(bool muted)
		{
			if (mAudioInterface)
			{
				mAudioInterface->setMute(muted);
			}
		}

		LL_INLINE void setReceiveVolume(F32 volume)
		{
			if (mAudioInterface)
			{
				mAudioInterface->setReceiveVolume(volume);
			}
		}

	private:
		std::mutex							mDataMutex;
		LLWebRTCPeerConnectionInterface*	mPeerInterface;
		LLWebRTCAudioInterface*				mAudioInterface;
		LLWebRTCDataInterface*				mDataInterface;

	public:
		std::string							mChannelSDP;
		LLSD								mChannelData;
		LLSD								mIceCandidates;
		bool								mIceCompleted;
		bool								mIceCompleteChanged;
		bool								mIceDataChanged;
		bool								mGotDataInterface;
		bool								mSessionEstablished;
		bool								mConnectionClosed;
		bool								mRenegotiationNeeded;
		bool								mHasUpdates;
	};
	typedef std::map<LLUUID, Connection*> connection_map_t;

	LL_INLINE void addConnection(const LLUUID& connection_id)
	{
		if (mConnections.count(connection_id))
		{
			return;	// Cannot add the same connection twice !
		}
		mConnections.emplace(connection_id, new Connection());
	}

	LL_INLINE void deleteConnection(const LLUUID& connection_id)
	{
		connection_map_t::iterator it = mConnections.find(connection_id);
		if (it != mConnections.end())
		{
			Connection* connectp = it->second;
			mConnections.erase(it);
			delete connectp;
		}
	}

	void connect(const LLUUID& connection_id, const LLSD& urls);

	LL_INLINE void disconnect(const LLUUID& connection_id)
	{
		connection_map_t::iterator it = mConnections.find(connection_id);
		if (it != mConnections.end())
		{
			it->second->disconnect();
		}
	}

	LL_INLINE void answer(const LLUUID& connection_id, const std::string& sdp)
	{
		connection_map_t::iterator it = mConnections.find(connection_id);
		if (it != mConnections.end())
		{
			it->second->answer(sdp);
		}
	}

	LL_INLINE bool sendData(const LLUUID& connection_id,
							const std::string& json_data)
	{
		connection_map_t::iterator it = mConnections.find(connection_id);
		return it != mConnections.end() && it->second->sendData(json_data);
	}

	LL_INLINE void closeDataInterface(const LLUUID& connection_id)
	{
		connection_map_t::iterator it = mConnections.find(connection_id);
		if (it != mConnections.end())
		{
			it->second->closeDataInterface();
		}
	}

	LL_INLINE void mute(const LLUUID& connection_id, bool muted)
	{
		connection_map_t::iterator it = mConnections.find(connection_id);
		if (it != mConnections.end())
		{
			it->second->mute(muted);
		}
	}

	LL_INLINE void setReceiveVolume(const LLUUID& connection_id, F32 volume)
	{
		connection_map_t::iterator it = mConnections.find(connection_id);
		if (it != mConnections.end())
		{
			it->second->setReceiveVolume(volume);
		}
	}

	static void postLogMessage(const char* func, const std::string& msg);

private:
	bool initialize();
	bool cleanup();
	void update();

private:
	LLWebRTCDeviceInterface*	mDeviceInterface;
	std::mutex					mDevicesMutex;
	LLSD						mCaptureDevices;
	LLSD						mRenderDevices;

	connection_map_t			mConnections;

	bool						mIsInTuningMode;

public:
	static HBPluginVoiceWebRTC*	sInstance;
	static bool					sTerminating;
	static bool					sDebug;
};

HBPluginVoiceWebRTC* HBPluginVoiceWebRTC::sInstance = NULL;
bool HBPluginVoiceWebRTC::sTerminating = false;
bool HBPluginVoiceWebRTC::sDebug = false;

///////////////////////////////////////////////////////////////////////////////
// HBPluginVoiceWebRTC::Connection sub-class
///////////////////////////////////////////////////////////////////////////////

HBPluginVoiceWebRTC::Connection::Connection()
:	mPeerInterface(newPeerConnection()),
	mAudioInterface(NULL),
	mDataInterface(NULL),
	mIceCompleted(false),
	mIceCompleteChanged(false),
	mIceDataChanged(false),
	mGotDataInterface(false),
	mSessionEstablished(false),
	mConnectionClosed(false),
	mRenegotiationNeeded(false),
	mHasUpdates(false)
{
	if (mPeerInterface)
	{
		mPeerInterface->setSignalingObserver(this);
	}
}

HBPluginVoiceWebRTC::Connection::~Connection()
{
	mAudioInterface = NULL;
	if (mDataInterface)
	{
		mDataInterface->unsetDataObserver(this);
		mDataInterface = NULL;
	}
	if (mPeerInterface)
	{
		mPeerInterface->unsetSignalingObserver(this);
		freePeerConnection(mPeerInterface);
		mPeerInterface = NULL;
	}
}

// ICE (Interactive Connectivity Establishment)
// When WebRTC tries to negotiate a connection to the WebRTC server, the
// negotiation will result in a few updates about the best path to which to
// connect. The SL servers are configured for ICE trickling, where, after a
// session is partially negotiated, updates about the best connectivity paths
// may trickle in.
//virtual
void HBPluginVoiceWebRTC::Connection::OnIceGatheringState(EIceGatheringState state)
{
	if (HBPluginVoiceWebRTC::sTerminating) return;

	bool complete = state == EIceGatheringState::ICE_GATHERING_COMPLETE;
	if (complete || state == EIceGatheringState::ICE_GATHERING_NEW)
	{
		if (HBPluginVoiceWebRTC::sDebug)
		{
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC,
												std::string("complete: ") +
												(complete ? "true" : "false"));
		}
		mDataMutex.lock();
		mIceCompleted = complete;
		mIceCompleteChanged = true;
		mHasUpdates = true;
		mDataMutex.unlock();
	}
}

//virtual
void HBPluginVoiceWebRTC::Connection::OnIceCandidate(const LLWebRTCIceCandidate& cddt)
{
	if (!HBPluginVoiceWebRTC::sTerminating)
	{
		LLSD data;
		data["sdpMid"] = cddt.mSdpMid;
		data["sdpMLineIndex"] = cddt.mMLineIndex;
		data["candidate"] = cddt.mCandidate;
		mDataMutex.lock();
		if (mIceCandidates.isUndefined())
		{
			mIceCandidates = LLSD::emptyArray();
		}
		mIceCandidates.append(data);
		mIceDataChanged = true;
		mHasUpdates = true;
		mDataMutex.unlock();
		if (HBPluginVoiceWebRTC::sDebug)
		{
			std::stringstream msg;
			msg << "got ICE candidate: " << data;
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC, msg.str());
		}
	}
}

// An 'offer' comes in the form of a SDP (Session Description Protocol) which
// contains all sorts of info about the session, from network paths to the type
// of session (audio, video) to characteristics (the encoder type). This SDP
// also serves as the 'ticket' to the server, security-wise. The offer is
// retrieved from the WebRTC library on the client, and is passed to the
// simulator via a capability, which then passes it on to the WebRTC server..
//virtual
void HBPluginVoiceWebRTC::Connection::OnOfferAvailable(const std::string& sdp)
{
	if (!HBPluginVoiceWebRTC::sTerminating)
	{
		if (HBPluginVoiceWebRTC::sDebug)
		{
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC, "got offer: " + sdp);
		}
		mDataMutex.lock();
		mChannelSDP = sdp;
		mHasUpdates = true;
		mDataMutex.unlock();
	}
}

//virtual
void HBPluginVoiceWebRTC::Connection::OnAudioEstablished(LLWebRTCAudioInterface* ifp)
{
	if (!HBPluginVoiceWebRTC::sTerminating)
	{
		if (HBPluginVoiceWebRTC::sDebug)
		{
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC,
												"got the audio interface.");
		}
		mAudioInterface = ifp;
		mDataMutex.lock();
		mSessionEstablished = true;
		mHasUpdates = true;
		// Mute will be set appropriately later when the voice client finishes
		// setting up.
		mute(true);
		mDataMutex.unlock();
	}
}

//virtual
void HBPluginVoiceWebRTC::Connection::OnRenegotiationNeeded()
{
	if (!HBPluginVoiceWebRTC::sTerminating)
	{
		if (HBPluginVoiceWebRTC::sDebug)
		{
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC,
												"got renegociate flag.");
		}
		mDataMutex.lock();
		mRenegotiationNeeded = true;
		mHasUpdates = true;
		mDataMutex.unlock();
	}
}

//virtual
void HBPluginVoiceWebRTC::Connection::OnPeerConnectionClosed()
{
	if (!HBPluginVoiceWebRTC::sTerminating)
	{
		if (HBPluginVoiceWebRTC::sDebug)
		{
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC,
												"got closed connection status.");
		}
		mDataMutex.lock();
		mConnectionClosed = true;
		mHasUpdates = true;
		mDataMutex.unlock();
	}
}

//virtual
void HBPluginVoiceWebRTC::Connection::OnDataChannelReady(LLWebRTCDataInterface* ifp)
{
	if (!HBPluginVoiceWebRTC::sTerminating && ifp)
	{
		if (HBPluginVoiceWebRTC::sDebug)
		{
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC,
												"got the data interface.");
		}
		mDataInterface = ifp;
		mDataInterface->setDataObserver(this);
		mDataMutex.lock();
		mGotDataInterface = true;
		mHasUpdates = true;
		mDataMutex.unlock();
	}
}

//virtual
void HBPluginVoiceWebRTC::Connection::OnDataReceived(const std::string& data,
													 bool binary)
{
	if (!HBPluginVoiceWebRTC::sTerminating && !binary)
	{
		if (HBPluginVoiceWebRTC::sDebug)
		{
			HBPluginVoiceWebRTC::postLogMessage(LL_FUNC, "got data: " + data);
		}
		mDataMutex.lock();
		if (mChannelData.isUndefined())
		{
			mChannelData = LLSD::emptyArray();
		}
		mChannelData.append(LLSD::String(data));
		mHasUpdates = true;
		mDataMutex.unlock();
	}
}

///////////////////////////////////////////////////////////////////////////////
// HBPluginVoiceWebRTC class proper
///////////////////////////////////////////////////////////////////////////////

HBPluginVoiceWebRTC::HBPluginVoiceWebRTC(LLPluginInstance::sendMessageFunction fn,
										 void* hostdatap)
:	MediaPluginBase(fn, hostdatap),
	mDeviceInterface(NULL),
	mIsInTuningMode(false)
{
	llwebrtc::init(this);	// 'this' since we are our own log sink.
}

HBPluginVoiceWebRTC::~HBPluginVoiceWebRTC()
{
	if (!sTerminating)
	{
		// Obviously we were not asked to cleanup properly, so do it now.
		cleanup();
	}
	llwebrtc::terminate();
}

//static
void HBPluginVoiceWebRTC::shutdown()
{
	if (sInstance)
	{
		delete sInstance;
		sInstance = NULL;
	}
}

bool HBPluginVoiceWebRTC::initialize()
{
	if (mDeviceInterface)
	{
		return true;	// Already initialized... Count as a success.
	}
	mDeviceInterface = getDeviceInterface();
	if (!mDeviceInterface)
	{
		return false;
	}
	mDeviceInterface->setDevicesObserver(this);
	return true;
}

bool HBPluginVoiceWebRTC::cleanup()
{
	sTerminating = true;	// Make sure any stale callback will be ignored
	if (mDeviceInterface)
	{
		mDeviceInterface->unsetDevicesObserver(this);
		mDeviceInterface = NULL;
	}
	for (connection_map_t::iterator it = mConnections.begin(),
									end = mConnections.end();
		 it != end; ++it)
	{
		delete it->second;
	}
	mConnections.clear();
	return true;
}

//static
void HBPluginVoiceWebRTC::postLogMessage(const char* func,
										 const std::string& msg)
{
	if (sInstance && func && *func)
	{
		// Let's keep only the 'class::method' part in the signature that got
		// passed to us. Note: since there is always at most one WebRTC plugin
		// running and our class names are explicit enough, we do not need to
		// bother to identify which plugin the message comes from.
		std::string fn(func);
		size_t i = fn.find('(');
		if (i != std::string::npos)
		{
			fn.erase(i);
		}
		LLStringUtil::trimTail(fn);	// Remove any trailing space
		i = fn.rfind(' ');
		if (i != std::string::npos)
		{
			fn.erase(0, i + 1);
		}
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_VOICE, "log_message");
		message.setValue("message_text", fn + ": " + msg);
		sInstance->sendMessage(message);
	}
}

//virtual
void HBPluginVoiceWebRTC::LogMessage(LogLevel level, const std::string& msg)
{
	switch (level)
	{
		case LOG_LEVEL_VERBOSE:
#if 0		// *Way* too verbose... *TODO: implement a "verbose debug" mode ?
			if (sDebug)
			{
				postLogMessage("LLWebRTCLogCallback", "DEBUG: " + msg);
			}
#endif
			break;

		case LOG_LEVEL_INFO:
			postLogMessage("LLWebRTCLogCallback", "INFO: " + msg);
			break;

		case LOG_LEVEL_WARNING:
			postLogMessage("LLWebRTCLogCallback", "WARNING: " + msg);
			break;

		case LOG_LEVEL_ERROR:
			postLogMessage("LLWebRTCLogCallback", "ERROR: " + msg);
	}
}

void HBPluginVoiceWebRTC::connect(const LLUUID& connection_id,
								  const LLSD& urls)
{
	connection_map_t::iterator iter = mConnections.find(connection_id);
	if (iter != mConnections.end())
	{
		Connection* connectp = iter->second;
		std::string url;
		LLWebRTCPeerConnectionInterface::InitOptions options;
		LLWebRTCPeerConnectionInterface::InitOptions::IceServers srv;
		if (urls.isArray())
		{
			for (LLSD::array_const_iterator it = urls.beginArray(),
											end = urls.endArray();
				 it != end; ++it)
			{
				url = it->asString();
				if (sDebug)
				{
					postLogMessage(LL_FUNC, "adding URL: " + url);
				}
				srv.mUrls.emplace_back(url);
			}
		}
		options.mServers.emplace_back(srv);
		
		if (connectp->connect(options))
		{
			if (sDebug)
			{
				postLogMessage(LL_FUNC, "connection request sent.");
			}
			return;	// Success: nothing to report
		}
	}
	if (sDebug)
	{
		postLogMessage(LL_FUNC, "connection request failed.");
	}

	// In case of a connection failure, ask for a renegotiation (which will
	// trigger the same state machine status change to VS_SESSION_RETRY).
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_VOICE, "webrtc_data");
	message.setValueLLUUID("connection_id", connection_id);
	message.setValueBoolean("renegotiate", true);
	sendMessage(message);
}

//virtual
void HBPluginVoiceWebRTC::OnDevicesChanged(const rtcdev_list_t& render_devices,
										   const rtcdev_list_t& capture_devices)
{
	if (sTerminating || (render_devices.empty() && capture_devices.empty()))
	{
		return;
	}

	LLSD input = LLSD::emptyMap();
	for (auto& device : capture_devices)
	{
		if (device.mID.empty())
		{
			continue;
		}
		if (device.mDisplayName.empty())
		{
			input[device.mID] = device.mID;
		}
		else
		{
			input[device.mDisplayName] = device.mID;
		}
	}

	LLSD output = LLSD::emptyMap();
	for (auto& device : render_devices)
	{
		if (device.mID.empty())
		{
			continue;
		}
		if (device.mDisplayName.empty())
		{
			output[device.mID] = device.mID;
		}
		else
		{
			output[device.mDisplayName] = device.mID;
		}
	}

	mDevicesMutex.lock();
	mCaptureDevices = input;
	mRenderDevices = output;
	mDevicesMutex.unlock();
}

// This scans all connections and sends any of their changed data down to
// LLPluginClassMedia for retrieval by the voice plugin owner.
void HBPluginVoiceWebRTC::update()
{
	// Always update the audio level
	F32 level = getAudioLevel();
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_VOICE, "audio_level");
	message.setValueReal("level", level);
	sendMessage(message);

	mDevicesMutex.lock();
	if (mCaptureDevices.isDefined() || mRenderDevices.isDefined())
	{
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_VOICE, "audio_devices");
		message.setValueLLSD("capture", mCaptureDevices);
		message.setValueLLSD("render", mRenderDevices);
		sendMessage(message);
		mCaptureDevices.clear();
		mRenderDevices.clear();
	}
	mDevicesMutex.unlock();

	for (connection_map_t::iterator it = mConnections.begin(),
									end = mConnections.end();
		 it != end; ++it)
	{
		const LLUUID& connection_id = it->first;
		Connection* connp = it->second;
		connp->lock();
		if (!connp->mHasUpdates)
		{
			connp->unlock();
			continue;
		}

		// Something changed in this connection: send the corresponding data.
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_VOICE, "webrtc_data");
		message.setValueLLUUID("connection_id", connection_id);
		if (connp->mIceCompleteChanged)
		{
			message.setValueBoolean("ice_completed", connp->mIceCompleted);
		}
		if (connp->mIceDataChanged)
		{
			message.setValueLLSD("ice_candidates", connp->mIceCandidates);
		}
		if (!connp->mChannelSDP.empty())
		{
			message.setValue("channel_sdp", connp->mChannelSDP);
		}
		if (connp->mGotDataInterface)
		{
			message.setValueBoolean("got_data_interface", true);
		}
		if (connp->mChannelData.isDefined())
		{
			message.setValueLLSD("channel_data", connp->mChannelData);
		}
		if (connp->mSessionEstablished)
		{
			message.setValueBoolean("established", true);
		}
		if (connp->mConnectionClosed)
		{
			message.setValueBoolean("closed", true);
		}
		if (connp->mRenegotiationNeeded)
		{
			message.setValueBoolean("renegotiate", true);
		}
		connp->resetData();
		connp->unlock();
		if (sDebug)
		{
			std::stringstream msg;
			msg << "Connection: " << connection_id
				<< " - Sending updates: " << message.getParams();
			postLogMessage(LL_FUNC, msg.str());
		}
		sendMessage(message);
	}
}

//virtual
void HBPluginVoiceWebRTC::receiveMessage(const char* message_string)
{
	LLPluginMessage message_in;
	if (message_in.parse(message_string) < 0)
	{
		return;
	}

	std::string message_class = message_in.getClass();
	std::string message_name = message_in.getName();

	if (message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
	{
		if (message_name == "init")
		{
			if (initialize())
			{
				// Note: the "init_response" message will push this plugin to
				// the "running" state.
				LLPluginMessage message(message_class, "init_response");
				LLSD versions = LLSD::emptyMap();
				versions[LLPLUGIN_MESSAGE_CLASS_BASE] =
					LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_VOICE] =
					LLPLUGIN_MESSAGE_CLASS_VOICE_VERSION;
				message.setValueLLSD("versions", versions);
				message.setValue("plugin_version", "WebRTC m137");
				sendMessage(message);
			}
		}
		else if (message_name == "idle")
		{
			// We use idle to update all the data at once.
			update();
		}
		else if (message_name == "cleanup")
		{
			if (sDebug)
			{
				postLogMessage(LL_FUNC, "cleaning up.");
			}
			cleanup();
			shutdown();
			// Note: the "goodbye" message will push this plugin to the
			// "unloaded" state.
			LLPluginMessage message(message_class, "goodbye");
			sendMessage(message);
		}
		else if (message_name == "force_exit")
		{
			if (sDebug)
			{
				postLogMessage(LL_FUNC, "force exit.");
			}
			// Allow to be deleted on next staticReceiveMessage() invocation.
			mDeleteMe = true;
		}
	}
	else if (message_class == LLPLUGIN_MESSAGE_CLASS_VOICE)
	{
		// Commands dealing with voice client common settings
		if (message_name == "get_devices")
		{
			if (sDebug)
			{
				postLogMessage(LL_FUNC, "refreshing audio devices.");
			}
			refreshDevices();
			return;
		}
		if (message_name == "set_capture")
		{
			if (sDebug)
			{
				postLogMessage(LL_FUNC, "setting capture device.");
			}
			setCaptureDevice(message_in.getValue("device_id"));
			return;
		}
		if (message_name == "set_render")
		{
			if (sDebug)
			{
				postLogMessage(LL_FUNC, "setting render device.");
			}
			setRenderDevice(message_in.getValue("device_id"));
			return;
		}
		if (message_name == "set_mic_gain")
		{
			setMicGain(message_in.getValueReal("gain"));
			return;
		}
		if (message_name == "configure")
		{
			if (sDebug)
			{
				postLogMessage(LL_FUNC, "configuring audio.");
			}
			setAudioConfig(message_in.getValueLLSD("config"));
			return;
		}
		if (message_name == "tuning_mode")
		{
			bool tuning_on = message_in.getValueBoolean("enabled");
			if (sDebug)
			{
				postLogMessage(LL_FUNC,
							   std::string("setting tuning mode to ") +
							   (tuning_on ? "on" : "off"));
			}
			setTuningMode(tuning_on);
			return;
		}
		if (message_name == "tuning_gain")
		{
			setTuningGain(message_in.getValueReal("gain"));
			return;
		}
		if (message_name == "global_mute")
		{
			bool mute = message_in.getValueBoolean("enabled");
			if (sDebug)
			{
				postLogMessage(LL_FUNC,
							   std::string("setting global mute to ") +
							   (mute ? "on" : "off"));
			}
			setGlobalMute(mute, message_in.getValueReal("delay"));
			return;
		}
		if (message_name == "debug")
		{
			sDebug = message_in.getValueBoolean("enabled");
			postLogMessage(LL_FUNC,
						   std::string("debug messages ") +
						   (sDebug ? "enabled" : "disabled"));
			return;
		}

		LLUUID connection_id = message_in.getValueLLUUID("connection_id");
		if (connection_id.isNull())
		{
			return;
		}
		if (sDebug)
		{
			std::stringstream msg;
			msg << "got command '" << message_name << "' for connection: "
				<< connection_id;
			postLogMessage(LL_FUNC, msg.str());
		}
		// Commands dealing with individual voice connections
		if (message_name == "add_connection")
		{
			addConnection(connection_id);
		}
		else if (message_name == "delete_connection")
		{
			deleteConnection(connection_id);
		}
		else if (message_name == "connect")
		{
			connect(connection_id, message_in.getValueLLSD("urls"));
		}
		else if (message_name == "close_data_interface")
		{
			closeDataInterface(connection_id);
		}
		else if (message_name == "disconnect")
		{
			disconnect(connection_id);
		}
		else if (message_name == "answer")
		{
			answer(connection_id, message_in.getValue("remote_sdp"));
		}
		else if (message_name == "send_data")
		{
			sendData(connection_id, message_in.getValue("json_data"));
		}
		else if (message_name == "mute")
		{
			mute(connection_id, message_in.getValueBoolean("muted"));
		}
		else if (message_name == "receive_volume")
		{
			setReceiveVolume(connection_id, message_in.getValueReal("volume"));
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
	HBPluginVoiceWebRTC::sInstance = new HBPluginVoiceWebRTC(host_send_fn,
															 hostdatap);
	*plugin_send_fn = HBPluginVoiceWebRTC::staticReceiveMessage;
	*plugindatap = (void*)HBPluginVoiceWebRTC::sInstance;

	return 0;	// Success
}
