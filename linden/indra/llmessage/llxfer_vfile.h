/**
 * @file llxfer_vfile.h
 * @brief definition of LLXfer_VFile class for a single xfer_vfile.
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

#include "llassetstorage.h"
#include "llpreprocessor.h"
#include "llxfer.h"

class LLFileSystem;

class LLXfer_VFile : public LLXfer
{
protected:
	LOG_CLASS(LLXfer_VFile);

public:
	LLXfer_VFile();
	LLXfer_VFile(const LLUUID& local_id, LLAssetType::EType type);
	virtual ~LLXfer_VFile();

	virtual void init(const LLUUID& local_id, LLAssetType::EType type);
	virtual void cleanup();

	virtual S32 initializeRequest(U64 xfer_id, const LLUUID& local_id,
								  const LLUUID& remote_id,
								  const LLAssetType::EType type,
								  const LLHost& remote_host,
								  void (*callback)(void**, S32, LLExtStat),
								  void** user_data);
	virtual S32 startDownload();

	virtual S32 processEOF();

	virtual S32 startSend(U64 xfer_id, const LLHost& remote_host);
	virtual void closeFileHandle();
	virtual S32 reopenFileHandle();

	virtual S32 suck(S32 start_position);
	virtual S32 flush();

	LL_INLINE virtual bool matchesLocalFile(const LLUUID& id,
											LLAssetType::EType type)
	{
		return id == mLocalID && type == mType;
	}

	LL_INLINE virtual bool matchesRemoteFile(const LLUUID& id,
											 LLAssetType::EType type)
	{
		return id == mRemoteID && type == mType;
	}

	virtual void setXferSize(S32 xfer_size);

	LL_INLINE virtual S32 getMaxBufferSize()		{ return LL_MAX_XFER_FILE_BUFFER; }


	// Hacky: doesn't matter what this is as long as it's different from the
	// other classes:
	LL_INLINE virtual U32 getXferTypeTag()			{ return LLXfer::XFER_VFILE; }

	LL_INLINE virtual std::string getFileName()		{ return mName; }

protected:
	LLUUID				mLocalID;
	LLUUID				mRemoteID;
	LLUUID				mTempID;
	LLAssetType::EType	mType;

	LLFileSystem*		mVFile;

	std::string			mName;

	bool				mDeleteTempFile;
};
