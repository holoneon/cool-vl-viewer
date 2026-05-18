/**
 * @file llxfer_file.h
 * @brief definition of LLXfer_File class for a single xfer_file.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "lldir.h"
#include "llpreprocessor.h"
#include "llxfer.h"

class LLXfer_File : public LLXfer
{
protected:
	LOG_CLASS(LLXfer_File);

public:
	LLXfer_File(S32 chunk_size);
	LLXfer_File(const std::string& local_filename,
				bool delete_local_on_completion, S32 chunk_size);
	virtual ~LLXfer_File();

	virtual void init(const std::string& local_filename,
					  bool delete_local_on_completion, S32 chunk_size);
	virtual void cleanup();

	virtual S32 initializeRequest(U64 xfer_id,
								  const std::string& local_filename,
								  const std::string& remote_filename,
								  ELLPath remote_path,
								  const LLHost& remote_host,
								  bool delete_remote_on_completion,
								  void (*callback)(void**, S32, LLExtStat),
								  void** user_data);
	virtual S32 startDownload();

	virtual S32 processEOF();

	virtual S32 startSend(U64 xfer_id, const LLHost& remote_host);
	virtual void closeFileHandle();
	virtual S32 reopenFileHandle();

	virtual S32 suck(S32 start_position);
	virtual S32 flush();

	LL_INLINE virtual bool matchesLocalFilename(const std::string& filename)
	{
		return filename == mLocalFilename;
	}

	LL_INLINE virtual bool matchesRemoteFilename(const std::string& filename,
												 ELLPath remote_path)
	{
		return filename == mRemoteFilename && remote_path == mRemotePath;
	}

	LL_INLINE virtual S32  getMaxBufferSize()		{ return LL_MAX_XFER_FILE_BUFFER; }

	// Hacky: doesn't matter what this is as long as it's different from the
	// other classes:
	LL_INLINE virtual U32 getXferTypeTag()			{ return LLXfer::XFER_FILE; }

	LL_INLINE virtual std::string getFileName()		{ return mLocalFilename; }

protected:
 	LLFILE*		mFp;
	ELLPath		mRemotePath;
	bool		mDeleteLocalOnCompletion;
	bool		mDeleteRemoteOnCompletion;
	std::string	mLocalFilename;
	std::string	mRemoteFilename;
	std::string	mTempFilename;
};
