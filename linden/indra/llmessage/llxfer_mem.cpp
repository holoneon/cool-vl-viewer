/**
 * @file llxfer_mem.cpp
 * @brief implementation of LLXfer_Mem class for a single xfer
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

#include "linden_common.h"

#include "llxfer_mem.h"

#include "llmath.h"

LLXfer_Mem::LLXfer_Mem()
:	LLXfer(-1)
{
	init();
}

LLXfer_Mem::~LLXfer_Mem()
{
	cleanup();
}

void LLXfer_Mem::init()
{
	mRemoteFilename.clear();
	mRemotePath = LL_PATH_NONE;
	mDeleteRemoteOnCompletion = false;
}

void LLXfer_Mem::cleanup()
{
	LLXfer::cleanup();
}

void LLXfer_Mem::setXferSize(S32 xfer_size)
{
	mXferSize = xfer_size;

	delete[] mBuffer;
	mBuffer = new char[xfer_size];

	mBufferLength = 0;
	mBufferStartOffset = 0;
	mBufferContainsEOF = true;
}

S32 LLXfer_Mem::startSend(U64 xfer_id, const LLHost& remote_host)
{
	S32 retval = LL_ERR_NOERR;  // presume success

	if (mXferSize <= 0)
	{
		return LL_ERR_FILE_EMPTY;
	}

    mRemoteHost = remote_host;
	mID = xfer_id;
   	mPacketNum = -1;

	mStatus = e_LL_XFER_PENDING;

	return retval;
}

S32 LLXfer_Mem::processEOF()
{
	S32 retval = 0;

	mStatus = e_LL_XFER_COMPLETE;

	llinfos << "xfer complete: " << getFileName() << llendl;

	if (mCallback)
	{
		mCallback((void*)mBuffer, mBufferLength, mCallbackDataHandle,
				  mCallbackResult, LLExtStat::NONE);
	}

	return retval;
}

S32 LLXfer_Mem::initializeRequest(U64 xfer_id,
								  const std::string& remote_filename,
								  ELLPath remote_path,
								  const LLHost& remote_host,
								  bool delete_remote_on_completion,
								  void (*callback)(void*, S32, void**, S32, LLExtStat),
								  void** user_data)
{
 	S32 retval = 0;  // presume success

	mRemoteHost = remote_host;

	// create a temp filename string using a GUID
	mID = xfer_id;
	mCallback = callback;
	mCallbackDataHandle = user_data;
	mCallbackResult = LL_ERR_NOERR;

	mRemoteFilename = remote_filename;
	mRemotePath = remote_path;
	mDeleteRemoteOnCompletion = delete_remote_on_completion;

	llinfos << "Requesting file: " << remote_filename << llendl;

	if (mBuffer)
	{
		delete[] mBuffer;
		mBuffer = NULL;
	}

	mBufferLength = 0;
	mPacketNum = 0;
 	mStatus = e_LL_XFER_PENDING;
	return retval;
}

S32 LLXfer_Mem::startDownload()
{
 	S32 retval = 0;  // presume success
	LLMessageSystem* msg = gMessageSystemp;
	msg->newMessageFast(_PREHASH_RequestXfer);
	msg->nextBlockFast(_PREHASH_XferID);
	msg->addU64Fast(_PREHASH_ID, mID);
	msg->addStringFast(_PREHASH_Filename, mRemoteFilename);
	msg->addU8("FilePath", (U8) mRemotePath);
	msg->addBool("DeleteOnCompletion", mDeleteRemoteOnCompletion);
	msg->addBool("UseBigPackets", mChunkSize == LL_XFER_LARGE_PAYLOAD);
	msg->addUUIDFast(_PREHASH_VFileID, LLUUID::null);
	msg->addS16Fast(_PREHASH_VFileType, -1);

	msg->sendReliable(mRemoteHost);
	mStatus = e_LL_XFER_IN_PROGRESS;

	return retval;
}

U32 LLXfer_Mem::getXferTypeTag()
{
	return LLXfer::XFER_MEM;
}
