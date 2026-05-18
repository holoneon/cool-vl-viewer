/**
 * @file llxfer_vfile.cpp
 * @brief implementation of LLXfer_VFile class for a single xfer (vfile).
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

#include "linden_common.h"

#include "llxfer_vfile.h"

#include "lldir.h"
#include "llfilesystem.h"
#include "llmath.h"

LLXfer_VFile::LLXfer_VFile()
:	LLXfer(-1),
	mDeleteTempFile(false)
{
	init(LLUUID::null, LLAssetType::AT_NONE);
}

LLXfer_VFile::LLXfer_VFile(const LLUUID& local_id, LLAssetType::EType type)
:	LLXfer(-1),
	mDeleteTempFile(false)
{
	init(local_id, type);
}

LLXfer_VFile::~LLXfer_VFile()
{
	cleanup();
}

void LLXfer_VFile::init(const LLUUID& local_id, LLAssetType::EType type)
{
	mLocalID = local_id;
	mType = type;

	mVFile = NULL;

	std::string id_string;
	mLocalID.toString(id_string);

	mName = llformat("VFile %s:%s", id_string.c_str(),
					 LLAssetType::lookup(mType));
}

void LLXfer_VFile::cleanup()
{
	if (mTempID.notNull() && mDeleteTempFile)
	{
		LLFileSystem file(mTempID);
		if (file.exists())
		{
			file.remove();
		}
		else
		{
			llwarns << "No matching cache file " << file.getName()
					<< ". Nothing deleted." << llendl;
		}
	}

	if (mVFile)
	{
		delete mVFile;
		mVFile = NULL;
	}

	LLXfer::cleanup();
}

S32 LLXfer_VFile::initializeRequest(U64 xfer_id, const LLUUID& local_id,
									const LLUUID& remote_id,
									LLAssetType::EType type,
									const LLHost& remote_host,
									void (*callback)(void**, S32, LLExtStat),
									void** user_data)
{
	mRemoteHost = remote_host;

	mLocalID = local_id;
	mRemoteID = remote_id;
	mType = type;

	mID = xfer_id;
	mCallback = callback;
	mCallbackDataHandle = user_data;
	mCallbackResult = LL_ERR_NOERR;

	std::string id_string;
	mLocalID.toString(id_string);

	mName = llformat("VFile %s:%s", id_string.c_str(),
					 LLAssetType::lookup(mType));

	llinfos << "Requesting " << mName << llendl;

	if (mBuffer)
	{
		delete[] mBuffer;
		mBuffer = NULL;
	}

	mBuffer = new char[LL_MAX_XFER_FILE_BUFFER];

	mBufferLength = 0;
	mPacketNum = 0;
	mTempID.generate();
	mDeleteTempFile = true;
	mStatus = e_LL_XFER_PENDING;

	return LL_ERR_NOERR;
}

S32 LLXfer_VFile::startDownload()
{
	LLMessageSystem* msg = gMessageSystemp;
	msg->newMessageFast(_PREHASH_RequestXfer);
	msg->nextBlockFast(_PREHASH_XferID);
	msg->addU64Fast(_PREHASH_ID, mID);
	msg->addStringFast(_PREHASH_Filename, "");
	msg->addU8("FilePath", (U8) LL_PATH_NONE);
	msg->addBool("DeleteOnCompletion", false);
	msg->addBool("UseBigPackets", mChunkSize == LL_XFER_LARGE_PAYLOAD);
	msg->addUUIDFast(_PREHASH_VFileID, mRemoteID);
	msg->addS16Fast(_PREHASH_VFileType, (S16)mType);

	msg->sendReliable(mRemoteHost);
	mStatus = e_LL_XFER_IN_PROGRESS;

	return LL_ERR_NOERR;
}

S32 LLXfer_VFile::startSend(U64 xfer_id, const LLHost& remote_host)
{
	mRemoteHost = remote_host;
	mID = xfer_id;
	mPacketNum = -1;

	if (mBuffer)
	{
		delete[] mBuffer;
	}
	mBuffer = new char[LL_MAX_XFER_FILE_BUFFER];

	mBufferLength = 0;
	mBufferStartOffset = 0;

	if (mVFile)
	{
		delete mVFile;
	}
	mVFile = new LLFileSystem(mLocalID);

	if (!mVFile->exists())
	{
		llwarns << "Cannot read cache file " << mVFile->getName()
				<< ". Aborted." << llendl;
		delete mVFile;
		mVFile = NULL;
		return LL_ERR_FILE_NOT_FOUND;
	}

	S32 size = mVFile->getSize();
	if (size <= 0)
	{
		llwarns << "Empty cache file " << mVFile->getName() << ". Aborted."
				<< llendl;
		delete mVFile;
		mVFile = NULL;
		return LL_ERR_FILE_EMPTY;
	}

	setXferSize(size);
	mStatus = e_LL_XFER_PENDING;

	return LL_ERR_NOERR;
}

void LLXfer_VFile::closeFileHandle()
{
	if (mVFile)
	{
		delete mVFile;
		mVFile = NULL;
	}
}

S32 LLXfer_VFile::reopenFileHandle()
{
	if (!mVFile)
	{
		mVFile = new LLFileSystem(mLocalID);
		if (!mVFile->exists())
		{
			llwarns << "Cannot read cache file; " << mVFile->getName()
					<< llendl;
			delete mVFile;
			mVFile = NULL;
			return LL_ERR_FILE_NOT_FOUND;
		}
	}
	return LL_ERR_NOERR;
}

void LLXfer_VFile::setXferSize(S32 xfer_size)
{
	LLXfer::setXferSize(xfer_size);

	// Do not do this on the server side, where we have a persistent mVFile
	// It would be nice if LLXFers could tell which end of the pipe they were
	if (!mVFile)
	{
		LLFileSystem file(mTempID, LLFileSystem::APPEND);
	}
}

S32 LLXfer_VFile::suck(S32 start_position)
{
	S32 retval = 0;

	if (mVFile)
	{
		// Grab a buffer from the right place in the file
		if (!mVFile->seek(start_position, 0))
		{
			llwarns << "VFile Xfer Can't seek to position: " << start_position
					<< " - File length: " << mVFile->getSize()
					<< " - While sending file " << mLocalID << llendl;
			return -1;
		}

		if (mVFile->read((U8*)mBuffer, LL_MAX_XFER_FILE_BUFFER))
		{
			mBufferLength = mVFile->getLastBytesRead();
			mBufferStartOffset = start_position;

			mBufferContainsEOF = mVFile->eof();
		}
		else
		{
			retval = -1;
		}
	}
	else
	{
		retval = -1;
	}

	return retval;
}

S32 LLXfer_VFile::flush()
{
	if (mBufferLength)
	{
		LLFileSystem file(mTempID, LLFileSystem::APPEND);
		file.write((U8*)mBuffer, mBufferLength);
		mBufferLength = 0;
	}
	return LL_ERR_NOERR;
}

S32 LLXfer_VFile::processEOF()
{
	mStatus = e_LL_XFER_COMPLETE;

	flush();

	if (!mCallbackResult)
	{
		LLFileSystem file(mTempID);
		if (file.exists())
		{
			if (file.rename(mLocalID))
			{
				// Rename worked and the original file is gone. Clear flag so
				// that we do not attempt to delete the gone file in cleanup().
				mDeleteTempFile = false;
			}
			else
			{
				llwarns << "Unable to rename cache file: " << file.getName()
						<< llendl;
			}
		}
		else
		{
			llwarns << "Cannot open cache file: " << file.getName() << llendl;
		}
	}

	if (mVFile)
	{
		delete mVFile;
		mVFile = NULL;
	}

	return LLXfer::processEOF();
}
