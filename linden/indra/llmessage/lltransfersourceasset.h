/**
 * @file lltransfersourceasset.h
 * @brief Transfer system for sending an asset.
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

class LLTransferSourceParamsAsset : public LLTransferSourceParams
{
public:
	LLTransferSourceParamsAsset();

	void packParams(LLDataPacker& dp) const override;
	bool unpackParams(LLDataPacker& dp) override;

	void setAsset(const LLUUID& asset_id, LLAssetType::EType asset_type);

	LLUUID getAssetID() const						{ return mAssetID; }
	LLAssetType::EType getAssetType() const			{ return mAssetType; }

protected:
	LLUUID				mAssetID;
	LLAssetType::EType	mAssetType;
};

class LLTransferSourceAsset : public LLTransferSource
{
protected:
	LOG_CLASS(LLTransferSourceAsset);

public:
	LLTransferSourceAsset(const LLUUID& request_id, F32 priority);

	static void responderCallback(const LLUUID& uuid, LLAssetType::EType type,
								  void* user_data, S32 result,
								  LLExtStat ext_status);
protected:
	void initTransfer() override;
	F32 updatePriority() override;
	LLTSCode dataCallback(S32 packet_id, S32 max_bytes, U8** datap,
						  S32& returned_bytes, bool& delete_returned) override;
	void completionCallback(LLTSCode status) override;

	void packParams(LLDataPacker& dp) const override;
	bool unpackParams(LLDataPacker& dp) override;

protected:
	LLTransferSourceParamsAsset mParams;
	bool mGotResponse;

	S32 mCurPos;
};
