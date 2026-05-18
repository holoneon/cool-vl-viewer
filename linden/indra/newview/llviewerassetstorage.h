/**
 * @file llviewerassetstorage.h
 * @brief Class for loading asset data to/from an external source.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include <list>

#include "llassetstorage.h"
#include "llcorehttprequest.h"

class LLViewerAssetStorage final : public LLAssetStorage
{
protected:
	LOG_CLASS(LLViewerAssetStorage);

public:
	LLViewerAssetStorage(LLMessageSystem* msg, LLXferManager* xfer);

	~LLViewerAssetStorage() override;

	void storeAssetData(const LLTransactionID& tid, LLAssetType::EType atype,
						LLStoreAssetCallback callback, void* user_data,
						bool temp_file = false, bool is_priority = false,
						bool store_local = false, bool user_waiting = false,
						F64 timeout = LL_ASSET_STORAGE_TIMEOUT) override;

	void storeAssetData(const std::string& fname, const LLTransactionID& tid,
						LLAssetType::EType type, LLStoreAssetCallback callback,
						void* user_data, bool temp_file = false,
						bool is_priority = false, bool user_waiting = false,
						F64 timeout = LL_ASSET_STORAGE_TIMEOUT) override;

	void checkForTimeouts() override;

protected:
	void queueDataRequest(const LLUUID& uuid, LLAssetType::EType type,
						  LLGetAssetCallback callback, void* user_data,
						  bool duplicate, bool is_priority) override;

	void queueUdpRequest(const LLUUID& uuid, LLAssetType::EType type,
						 LLGetAssetCallback callback, void* user_data,
						 bool duplicate, bool is_priority);

	void queueHttpRequest(const LLUUID& uuid, LLAssetType::EType type,
						  LLGetAssetCallback callback, void* user_data,
						  bool duplicate, bool is_priority);

	void assetRequestCoro(std::string query, LLUUID uuid,
						  LLAssetType::EType atype,
 						  LLGetAssetCallback callback, void* user_data);

protected:
	// Asset storage works through coprocedures which have a limited queue
	// capacity. This structure is meant to temporary store requests when the
	// coprocedure queue is full.
	struct CoroWaitList
	{
		CoroWaitList(const std::string& url, const LLUUID& asset_id,
					 LLAssetType::EType atype, LLGetAssetCallback callback,
					 void* user_data)
		:	mUrl(url),
			mId(asset_id),
			mType(atype),
			mCallback(callback),
			mUserData(user_data)
		{
		}

		std::string			mUrl;
		LLUUID				mId;
		LLAssetType::EType	mType;
		LLGetAssetCallback	mCallback;
		void*				mUserData;
	};
	typedef std::list<CoroWaitList> wait_list_t;
	wait_list_t	mCoroWaitList;

	LLCore::HttpRequest::policy_t	mHttpPolicyClass;
};
