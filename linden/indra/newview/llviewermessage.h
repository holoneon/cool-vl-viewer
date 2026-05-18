/**
 * @file llviewermessage.h
 * @brief Message system callbacks for viewer.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include <time.h>

#include "llextendedstatus.h"
#include "llinstantmessage.h"
#include "llinventory.h"
#include "llmessage.h"
#include "lltransactiontypes.h"

#include "llchat.h"
//
// Forward declarations
//
class LLColor4;
class LLDispatcher;
class LLViewerObject;
class LLViewerRegion;

//
// Prototypes
//

void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount,
				bool is_group = false, S32 trx_type = TRANS_GIFT,
				const std::string& desc = LLStringUtil::null);
void busy_message (LLMessageSystem* msg, LLUUID from_id);

void process_logout_reply(LLMessageSystem* msg, void**);
void process_layer_data(LLMessageSystem* msg, void**);
void process_derez_ack(LLMessageSystem*, void**);
void process_places_reply(LLMessageSystem* msg, void** data);
void send_sound_trigger(const LLUUID& sound_id, F32 gain);
void process_improved_im(LLMessageSystem* msg, void**);
void process_script_question(LLMessageSystem* msg, void**);
void process_chat_from_simulator(LLMessageSystem* msg, void**);

void add_floater_chat(const LLChat& chat, bool history);

void send_agent_update(bool force_send, bool send_reliable = false);
void process_object_update(LLMessageSystem* msg, void** data);
void process_compressed_object_update(LLMessageSystem* msg, void** data);
void process_cached_object_update(LLMessageSystem* msg, void** data);
void process_terse_object_update_improved(LLMessageSystem* msg, void** data);
void process_object_properties_family(LLMessageSystem* msg, void**);
void add_newly_created_object(const LLUUID& object_id);

void send_simulator_throttle_settings(const LLHost& host);
void process_kill_object(LLMessageSystem* msg, void**);
void process_time_synch(LLMessageSystem* msg, void**);
void process_sound_trigger(LLMessageSystem* msg, void**);
void process_preload_sound(LLMessageSystem* msg, void**);
void process_attached_sound(LLMessageSystem* msg, void**);
void process_attached_sound_gain_change(LLMessageSystem* msg, void**);
void process_health_message(LLMessageSystem* msg, void**);
void process_sim_stats(LLMessageSystem* msg, void**);
void process_avatar_animation(LLMessageSystem* msg, void**);
void process_object_animation(LLMessageSystem* msg, void**);
void process_avatar_appearance(LLMessageSystem* msg, void**);
void process_camera_constraint(LLMessageSystem* msg, void**);
void process_avatar_sit_response(LLMessageSystem* msg, void**);
void process_set_follow_cam_properties(LLMessageSystem* msg, void**);
void process_clear_follow_cam_properties(LLMessageSystem* msg, void**);
void process_name_value(LLMessageSystem* msg, void**);
void process_remove_name_value(LLMessageSystem* msg, void**);
void process_kick_user(LLMessageSystem* msg, void**);

void process_grant_godlike_powers(LLMessageSystem* msg, void**);

void process_economy_data(LLMessageSystem* msg, void**);
void process_money_balance_reply(LLMessageSystem* msg_system, void**);
void process_adjust_balance(LLMessageSystem* msg_system, void**);

bool attempt_standard_notification(LLMessageSystem* msg);
void process_alert_message(LLMessageSystem* msg, void**);
void process_agent_alert_message(LLMessageSystem* msgsystem, void**);
void process_alert_core(const std::string& message, bool modal);

void process_mean_collision_alert_message(LLMessageSystem* msg, void**);

void process_frozen_message(LLMessageSystem* msg, void**);

void process_derez_container(LLMessageSystem* msg, void**);

// Agent movement
void send_complete_agent_movement(const LLHost& sim_host);
void process_agent_movement_complete(LLMessageSystem* msg, void**);
void process_crossed_region(LLMessageSystem* msg, void**);
void process_teleport_start(LLMessageSystem* msg, void**);
void process_teleport_progress(LLMessageSystem* msg, void**);
void process_teleport_failed(LLMessageSystem* msg, void**);
void process_teleport_finish(LLMessageSystem* msg, void**);

void process_teleport_local(LLMessageSystem* msg, void**);
void process_user_sim_location_reply(LLMessageSystem* msg, void**);

void send_simple_im(const LLUUID& to_id, const std::string& message,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					const LLUUID& id = LLUUID::null);

void send_group_notice(const LLUUID& group_id, const std::string& subject,
					   const std::string& message,
					   const LLInventoryItem* item);

void send_lures(const LLSD& notification, const LLSD& response,
				bool censor_message = true);
void handle_lure(const uuid_vec_t& ids);

// Always from agent and routes through the agent's current simulator
void send_improved_im(const LLUUID& to_id, const std::string& name,
					  const std::string& message, U8 offline = IM_ONLINE,
					  EInstantMessage dialog = IM_NOTHING_SPECIAL,
					  const LLUUID& id = LLUUID::null,
					  U32 timestamp = NO_TIMESTAMP,
					  const U8* binary_bucket = (U8*)EMPTY_BINARY_BUCKET,
					  S32 binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE);

void process_user_info_reply(LLMessageSystem* msg, void**);

void busy_message(const LLUUID& from_id);

// Function to format the time using the "TimestampFormat" debug setting.
std::string formatted_time(time_t the_time);

void send_places_query(const LLUUID& query_id, const LLUUID& trans_id,
					   const std::string& query_text, U32 query_flags,
					   S32 category, const std::string& sim_name);
void process_script_dialog(LLMessageSystem* msg, void**);
void process_load_url(LLMessageSystem* msg, void**);
void process_script_teleport_request(LLMessageSystem* msg, void**);
void process_covenant_reply(LLMessageSystem* msg, void**);
void onCovenantLoadComplete(const LLUUID& asset_uuid, LLAssetType::EType type,
							void* data, S32 status, LLExtStat);

// Calling cards
void process_offer_callingcard(LLMessageSystem* msg, void**);
void process_accept_callingcard(LLMessageSystem*, void**);
void process_decline_callingcard(LLMessageSystem*, void**);

// Message system exception prototypes
void invalid_message_callback(LLMessageSystem*, void*, EMessageException);

void process_feature_disabled_message(LLMessageSystem* msg, void**);

void process_initiate_download(LLMessageSystem* msg, void**);

// Called from llappviewer.cpp
void start_new_inventory_observer();
void stop_new_inventory_observer();

// Inventory offers

void open_inventory_offer(const uuid_vec_t& items,
						  const std::string& from_name);

enum InventoryOfferResponse
{
	IOR_ACCEPT,
	IOR_DECLINE,
	IOR_MUTE,
	IOR_BUSY,
	IOR_MUTED
};

class LLOfferInfo
{
public:
	LLOfferInfo()
	:	mLogInChat(true)
	{
	}

	LLOfferInfo(const LLSD& sd);
	LLOfferInfo(const LLOfferInfo& other);

	void extractSLURL();
	void forceResponse(InventoryOfferResponse response);

	LLSD asLLSD();

	void inventoryOfferHandler();

private:
	bool inventoryOfferCallback(const LLSD& notification,
								const LLSD& response);
	void sendReceiveResponse(bool accept);

public:
	EInstantMessage		mIM;
	LLUUID 				mFromID;
	LLUUID				mTransactionID;
	LLUUID				mFolderID;
	LLUUID				mObjectID;
	LLHost				mHost;
	LLAssetType::EType	mType;
	std::string			mFromName;
	std::string			mDesc;
	std::string			mSLURL;
	bool				mLogInChat;
	bool				mFromGroup;
	bool				mFromObject;
};

// Generic message (formerly in llviewergenericmessage.h)

void send_generic_message(const char* method,
						  const std::vector<std::string>& strings,
						  const LLUUID& invoice = LLUUID::null);

void process_generic_message(LLMessageSystem* msg, void**);
void process_generic_streaming_message(LLMessageSystem* msg, void**);
void process_large_generic_message(LLMessageSystem* msg, void**);

extern LLDispatcher gGenericDispatcher;
