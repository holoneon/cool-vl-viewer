/**
 * @file lltemplatemessagebuilder.h
 * @brief Declaration of LLTemplateMessageBuilder class.
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
#include "llpreprocessor.h"

class LLMsgData;
class LLMessageTemplate;
class LLMsgBlkData;
class LLMessageTemplate;

class LLTemplateMessageBuilder : public LLMessageBuilder
{
protected:
	LOG_CLASS(LLTemplateMessageBuilder);

public:
	typedef std::map<const char*, LLMessageTemplate*> template_name_map_t;

	LLTemplateMessageBuilder(const template_name_map_t&);
	virtual ~LLTemplateMessageBuilder();

	virtual void newMessage(const char* name);

	virtual void nextBlock(const char* blockname);
	// *TODO: Babbage: remove this horror...
	virtual bool removeLastBlock();

	/** All add* methods expect pointers to canonical varname strings. */
	virtual void addBinaryData(const char *varname, const void* data,
							   S32 size);
	virtual void addBool(const char* varname, bool b);
	virtual void addS8(const char* varname, S8 s);
	virtual void addU8(const char* varname, U8 u);
	virtual void addS16(const char* varname, S16 i);
	virtual void addU16(const char* varname, U16 i);
	virtual void addF32(const char* varname, F32 f);
	virtual void addS32(const char* varname, S32 s);
	virtual void addU32(const char* varname, U32 u);
	virtual void addU64(const char* varname, U64 lu);
	virtual void addF64(const char* varname, F64 d);
	virtual void addVector3(const char* varname, const LLVector3& vec);
	virtual void addVector4(const char* varname, const LLVector4& vec);
	virtual void addVector3d(const char* varname, const LLVector3d& vec);
	virtual void addQuat(const char* varname, const LLQuaternion& quat);
	virtual void addUUID(const char* varname, const LLUUID& uuid);
	virtual void addIPAddr(const char* varname, U32 ip);
	virtual void addIPPort(const char* varname, U16 port);
	virtual void addString(const char* varname, const char* s);
	virtual void addString(const char* varname, const std::string& s);

	virtual bool isMessageFull(const char* blockname) const;
	virtual void compressMessage(U8*& buf_ptr, U32& buffer_length);

	LL_INLINE virtual bool isBuilt() const					{ return mSBuilt; }
	LL_INLINE virtual bool isClear() const					{ return mSClear; }

	// Returns the built message size
	virtual U32 buildMessage(U8* buffer, U32 buffer_size, U8 offset_to_data);

	virtual void clearMessage();

	// *TODO: babbage: remove this horror.
	LL_INLINE virtual void setBuilt(bool b)					{ mSBuilt = b; }

	LL_INLINE virtual S32 getMessageSize()					{ return mCurrentSendTotal; }

	LL_INLINE virtual const char* getMessageName() const	{ return mCurrentSMessageName; }

	virtual void copyFromMessageData(const LLMsgData& data);
	virtual void copyFromLLSD(const LLSD&)					{}

	LL_INLINE LLMsgData* getCurrentMessage() const			{ return mCurrentSMessageData; }

private:
	void addData(const char* varname, const void* data, EMsgVariableType type,
				 S32 size);

	void addData(const char* varname, const void* data, EMsgVariableType type);

private:
	LLMsgData*					mCurrentSMessageData;
	const LLMessageTemplate*	mCurrentSMessageTemplate;
	LLMsgBlkData*				mCurrentSDataBlock;
	char*						mCurrentSMessageName;
	char*						mCurrentSBlockName;
	const template_name_map_t&	mMessageTemplates;
	S32							mCurrentSendTotal;
	bool						mSBuilt;
	bool						mSClear;
};
