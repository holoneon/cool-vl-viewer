/**
 * @file lltransfertargetvfile.h
 * @brief Transfer system for receiving a vfile.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llassetstorage.h"
#include "lltransfermanager.h"

// Lame, an S32 for now until I figure out the deal with how we want to do
// error codes.
typedef void (*LLTTVFCompleteCallback)(S32 status, const LLUUID& file_id,
									   LLAssetType::EType file_type,
									   LLBaseDownloadRequest* user_data,
									   LLExtStat ext_status);

class LLTransferTargetParamsVFile : public LLTransferTargetParams
{
	friend class LLTransferTargetVFile;

public:
	LLTransferTargetParamsVFile();

	void setAsset(const LLUUID& asset_id, LLAssetType::EType asset_type);

	void setCallback(LLTTVFCompleteCallback cb,
					 LLBaseDownloadRequest& user_data);

	LLUUID getAssetID() const						{ return mAssetID; }
	LLAssetType::EType getAssetType() const			{ return mAssetType; }

protected:
	bool unpackParams(LLDataPacker& dp);

protected:
	LLUUID					mAssetID;
	LLAssetType::EType		mAssetType;

	LLTTVFCompleteCallback	mCompleteCallback;
	LLBaseDownloadRequest*	mRequestDatap;
	S32						mErrCode;
};

class LLTransferTargetVFile : public LLTransferTarget
{
protected:
	LOG_CLASS(LLTransferTargetVFile);

public:
	LLTransferTargetVFile(const LLUUID& uuid, LLTransferSourceType src_type);
	~LLTransferTargetVFile() override;

	//static void requestTransfer(LLTransferTargetChannel* channelp,
	//							const char* local_filename,
	//							const LLTransferSourceParams& source_params,
	//							LLTTVFCompleteCallback callback);

	static void updateQueue(bool shutdown = false);

protected:
	bool unpackParams(LLDataPacker& dp) override;
	void applyParams(const LLTransferTargetParams& params) override;
	LLTSCode dataCallback(S32 packet_id, U8* in_datap, S32 in_size) override;
	void completionCallback(LLTSCode status) override;

protected:
	LLTransferTargetParamsVFile	mParams;
	LLUUID						mTempID;
	bool						mNeedsCreate;
};
