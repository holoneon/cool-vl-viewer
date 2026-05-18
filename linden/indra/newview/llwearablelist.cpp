/**
 * @file llwearablelist.cpp
 * @brief LLWearableList class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llwearablelist.h"

#include "llapp.h"
#include "llassetstorage.h"
#include "llnotifications.h"
#include "lltrans.h"
#include "llmessage.h"

#include "llagent.h"
#include "llfloaterperms.h"
#include "llviewerinventory.h"
#include "llviewerstats.h"
#include "llvoavatar.h"

// Callback struct
struct LLWearableArrivedData
{
	LLWearableArrivedData(LLAssetType::EType asset_type,
						  const std::string& wearable_name,
						  LLAvatarAppearance* avatarp,
						  void (*asset_arrived_callback)(LLViewerWearable*,
														 void*),
						  void* userdata)
	:	mAssetType(asset_type),
		mCallback(asset_arrived_callback),
		mUserdata(userdata),
		mName(wearable_name),
		mRetries(0),
		mAvatarp(avatarp)
	{
	}

	LLAssetType::EType	mAssetType;
	void				(*mCallback)(LLViewerWearable*, void* userdata);
	void*				mUserdata;
	std::string			mName;
	S32					mRetries;
	LLAvatarAppearance*	mAvatarp;
};

////////////////////////////////////////////////////////////////////////////
// LLWearableList

LLWearableList::~LLWearableList()
{
	cleanup();
}

void LLWearableList::cleanup()
{
	for (wearable_map_t::iterator it = mList.begin(), end = mList.end();
		 it != end; ++it)
	{
		// Ensure that the corresponding LLWearable still exists !
		if (LLWearable::sWearableList.count((LLWearable*)it->second))
		{
			delete it->second;
		}
	}
	mList.clear();
}

void LLWearableList::getAsset(const LLAssetID& asset_id,
							  const std::string& wearable_name,
							  LLAvatarAppearance* avatarp,
							  LLAssetType::EType asset_type,
							  void(*asset_arrived_callback)(LLViewerWearable*,
															void*),
							  void* userdata)
{
	llassert(asset_type == LLAssetType::AT_CLOTHING ||
			 asset_type == LLAssetType::AT_BODYPART);

	LLViewerWearable* instance = get_ptr_in_map(mList, asset_id);
	if (instance &&
		// Ensure that the corresponding LLWearable still exists !
		LLWearable::sWearableList.count((LLWearable*)instance))
	{
		asset_arrived_callback(instance, userdata);
	}
	else
	{
		gAssetStoragep->getAssetData(asset_id, asset_type,
									 LLWearableList::processGetAssetReply,
									 (void*)new LLWearableArrivedData(asset_type,
																	  wearable_name,
																	  avatarp,
																	  asset_arrived_callback,
																	  userdata),
									 true);
	}
}

// static
void LLWearableList::processGetAssetReply(const char* filename,
										  const LLAssetID& uuid,
										  void* userdata,
										  S32 status,
										  LLExtStat ext_status)
{
	LLWearableArrivedData* data = (LLWearableArrivedData*)userdata;
	if (LLApp::isExiting())
	{
		// Abort in case we got disconnected before the reply came back (seen
		// happening due to network disconnections). HB
		delete data;
		return;
	}

	bool is_new_wearable = false;
	LLViewerWearable* wearable = NULL; // NULL indicates failure
	LLAvatarAppearance* avatarp = data->mAvatarp;

	if (!filename)
	{
		llwarns << "Bad Wearable Asset: missing file." << llendl;
	}
	else if (!avatarp)
	{
		llwarns << "Bad asset request: missing avatar pointer." << llendl;
	}
	else if (status >= 0)
	{
		// read the file
		llifstream ifs(filename, std::ifstream::binary);
		if (!ifs.is_open())
		{
			llwarns << "Bad Wearable Asset: unable to open file: '" << filename
					<< "'" << llendl;
		}
		else
		{
			wearable = new LLViewerWearable(uuid);
			if (wearable->importStream(ifs, avatarp) != LLWearable::SUCCESS)
			{
				if (wearable->getType() == LLWearableType::WT_COUNT)
				{
					is_new_wearable = true;
				}
				delete wearable;
				wearable = NULL;
			}

			if (ifs.is_open())
			{
				ifs.close();
			}
			LLFile::remove(filename);
		}
	}
	else
	{
		if (filename)
		{
			LLFile::remove(filename);
		}
		gViewerStats.incStat(LLViewerStats::ST_DOWNLOAD_FAILED);

		llwarns << "Wearable download failed: "
				<< LLAssetStorage::getErrorString(status) << " " << uuid
				<< llendl;

		switch (status)
		{
			case LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE:
				break;	// failed

			default:
			{
				constexpr S32 MAX_RETRIES = 3;
				if (data->mRetries < MAX_RETRIES)
				{
					// Try again
					++data->mRetries;
					gAssetStoragep->getAssetData(uuid, data->mAssetType,
												 processGetAssetReply,
												 // re-use instead of deleting:
												 userdata);
					return;
				}
				// failed
			}
		}
	}

	if (wearable) // success
	{
		LLWearableList::getInstance()->mList[uuid] = wearable;
		LL_DEBUGS("Wearables") << "Success getting wearable: " << uuid.asString()
							   << LL_ENDL;
	}
	else
	{
		LLSD args;
		args["TYPE"] = LLTrans::getString(LLAssetType::lookupHumanReadable(data->mAssetType));
		if (is_new_wearable)
		{
			gNotifications.add("InvalidWearable");
		}
		else if (data->mName.empty())
		{
			gNotifications.add("FailedToFindWearableUnnamed", args);
		}
		else
		{
			args["DESC"] = data->mName;
			gNotifications.add("FailedToFindWearable", args);
		}
	}

	// Always call callback; wearable will be NULL if we failed
	if (data->mCallback)
	{
		data->mCallback(wearable, data->mUserdata);
	}
	delete data;
}

LLViewerWearable* LLWearableList::createCopy(LLViewerWearable* old_wearable,
											 const std::string& new_name)
{
	LLViewerWearable* wearable = generateNewWearable();
	wearable->copyDataFrom(old_wearable);

	LLPermissions perm(old_wearable->getPermissions());
	perm.setOwnerAndGroup(LLUUID::null, gAgentID, LLUUID::null, true);
	wearable->setPermissions(perm);
	if (!new_name.empty())
	{
		wearable->setName(new_name);
	}

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}

LLViewerWearable* LLWearableList::createNewWearable(LLWearableType::EType type,
													LLAvatarAppearance* avatarp)
{
	LLViewerWearable* wearable = generateNewWearable();
	wearable->setType(type, avatarp);

	std::string name =
		LLTrans::getString(LLWearableType::getTypeDefaultNewName(wearable->getType()));
	wearable->setName(name);

	LLPermissions perm;
	perm.set(gAgentID, gAgentID, LLUUID::null, LLUUID::null, PERM_ALL,
			 PERM_ALL, LLFloaterPerms::getEveryonePerms(),
			 LLFloaterPerms::getGroupPerms(),
			 LLFloaterPerms::getNextOwnerPerms() | PERM_MOVE);
	wearable->setPermissions(perm);

	wearable->setDefinitionVersion(LLWearable::getCurrentDefinitionVersion());

	// Description and sale info have default values.
	wearable->setParamsToDefaults();
	wearable->setTexturesToDefaults();

	// Mark all values (params & images) as saved
	wearable->saveValues();

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}

LLViewerWearable* LLWearableList::generateNewWearable()
{
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

	LLViewerWearable* wearable = new LLViewerWearable(tid);
	mList[new_asset_id] = wearable;
	return wearable;
}
