/**
 * @file lltransfersourcefile.h
 * @brief Transfer system for sending a file.
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

#include "lltransfermanager.h"

class LLTransferSourceParamsFile : public LLTransferSourceParams
{
protected:
	LOG_CLASS(LLTransferSourceParamsFile);

public:
	LLTransferSourceParamsFile();
	virtual ~LLTransferSourceParamsFile()				{}
	void packParams(LLDataPacker& dp) const override;
	bool unpackParams(LLDataPacker& dp) override;

	void setFilename(const std::string& filename)		{ mFilename = filename; }
	std::string getFilename() const						{ return mFilename; }

	void setDeleteOnCompletion(bool enabled)			{ mDeleteOnCompletion = enabled; }
	bool getDeleteOnCompletion()						{ return mDeleteOnCompletion; }

protected:
	std::string	mFilename;

	// ONLY DELETE THINGS OFF THE SIM IF THE FILENAME BEGINS IN 'TEMP'
	bool		mDeleteOnCompletion;
};

class LLTransferSourceFile : public LLTransferSource
{
protected:
	LOG_CLASS(LLTransferSourceFile);

public:
	LLTransferSourceFile(const LLUUID& transfer_id, F32 priority);
	~LLTransferSourceFile() override;

protected:
	void initTransfer() override;
	F32 updatePriority() override					{ return 0; }
	LLTSCode dataCallback(S32 packet_id, S32 max_bytes, U8** datap,
						  S32& returned_bytes, bool& delete_returned) override;
	void completionCallback(LLTSCode status) override;

	void packParams(LLDataPacker& dp) const override;
	bool unpackParams(LLDataPacker& dp) override;

protected:
	LLTransferSourceParamsFile	mParams;
	LLFILE*						mFP;
};
