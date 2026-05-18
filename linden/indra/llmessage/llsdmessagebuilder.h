/**
 * @file llsdmessagebuilder.h
 * @brief Declaration of LLSDMessageBuilder class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include <map>

#include "llmessagebuilder.h"
#include "llsd.h"

class LLMessageTemplate;
class LLMsgData;

class LLSDMessageBuilder final : public LLMessageBuilder
{
protected:
	LOG_CLASS(LLSDMessageBuilder);

public:
	LLSDMessageBuilder();

	void newMessage(const char* name) override;

	void nextBlock(const char* blockname) override;

	// *TODO: Babbage: remove this horror...
	LL_INLINE bool removeLastBlock() override				{ return false; }

	/** All add* methods expect pointers to canonical varname strings. */
	void addBinaryData(const char* varname, const void* data,
					   S32 size) override;
	void addBool(const char* varname, bool b) override;
	void addS8(const char* varname, S8 s) override;
	void addU8(const char* varname, U8 u) override;
	void addS16(const char* varname, S16 i) override;
	void addU16(const char* varname, U16 i) override;
	void addF32(const char* varname, F32 f) override;
	void addS32(const char* varname, S32 s) override;
	void addU32(const char* varname, U32 u) override;
	void addU64(const char* varname, U64 lu) override;
	void addF64(const char* varname, F64 d) override;
	void addVector3(const char* varname, const LLVector3& vec) override;
	void addVector4(const char* varname, const LLVector4& vec) override;
	void addVector3d(const char* varname, const LLVector3d& vec) override;
	void addQuat(const char* varname, const LLQuaternion& quat) override;
	void addUUID(const char* varname, const LLUUID& uuid) override;
	void addIPAddr(const char* varname, U32 ip) override;
	void addIPPort(const char* varname, U16 port) override;
	void addString(const char* varname, const char* s) override;
	void addString(const char* varname, const std::string& s) override;

	LL_INLINE bool isMessageFull(const char*) const override
	{
		return false;
	}

	LL_INLINE void compressMessage(U8*&, U32&) override		{}

	LL_INLINE bool isBuilt() const override					{ return mSBuilt; }
	LL_INLINE bool isClear() const override					{ return mSClear; }

	// Null implementation which returns 0
	LL_INLINE U32 buildMessage(U8*, U32, U8) override		{ return 0; }

	void clearMessage() override;

	// *TODO: babbage: remove this horror.
	LL_INLINE void setBuilt(bool b) override				{ mSBuilt = b; }

	// Babbage: size is unknown as message stored as LLSD. Return non-zero if
	// pending data, as send can be skipped for 0 size. Return 1 to encourage
	// senders checking size against splitting message.
	LL_INLINE S32 getMessageSize() override					{ return mCurrentMessage.size() ? 1 : 0; }

	LL_INLINE const char* getMessageName() const override	{ return mCurrentMessageName.c_str(); }

	void copyFromMessageData(const LLMsgData& data) override;

	void copyFromLLSD(const LLSD& msg) override;

	LL_INLINE const LLSD& getMessage() const				{  return mCurrentMessage; }

private:

	/* mCurrentMessage is of the following format:
		mCurrentMessage = { 'block_name1' : [ { 'block1_field1' : 'b1f1_data',
												'block1_field2' : 'b1f2_data',
												...
												'block1_fieldn' : 'b1fn_data'},
											{ 'block2_field1' : 'b2f1_data',
												'block2_field2' : 'b2f2_data',
												...
												'block2_fieldn' : 'b2fn_data'},
											...
											{ 'blockm_field1' : 'bmf1_data',
												'blockm_field2' : 'bmf2_data',
												...
												'blockm_fieldn' : 'bmfn_data'} ],
							'block_name2' : ...,
							...
							'block_namem' }
	*/
	LLSD		mCurrentMessage;
	LLSD*		mCurrentBlock;
	std::string	mCurrentMessageName;
	std::string	mCurrentBlockName;
	bool		mSBuilt;
	bool		mSClear;
};
