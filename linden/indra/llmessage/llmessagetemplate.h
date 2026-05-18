/**
 * @file llmessagetemplate.h
 * @brief Declaration of the message template classes.
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
#include <string>
#include <vector>

#include "llerror.h"
#include "llpreprocessor.h"
#include "llstl.h"
#include "llmessage.h"

template <typename Type, typename Key, int BlockSize = 32>
class LLIndexedVector
{
public:
	typedef typename std::vector<Type>::iterator iterator;
	typedef typename std::vector<Type>::const_iterator const_iterator;
	typedef typename std::vector<Type>::reverse_iterator reverse_iterator;
	typedef typename std::vector<Type>::const_reverse_iterator const_reverse_iterator;
	typedef typename std::vector<Type>::size_type size_type;

public:
	LL_INLINE LLIndexedVector()						{ mVector.reserve(BlockSize); }

	LL_INLINE iterator begin()						{ return mVector.begin(); }
	LL_INLINE const_iterator begin() const			{ return mVector.begin(); }
	LL_INLINE iterator end()						{ return mVector.end(); }
	LL_INLINE const_iterator end() const			{ return mVector.end(); }

	LL_INLINE reverse_iterator rbegin()				{ return mVector.rbegin(); }
	LL_INLINE const_reverse_iterator rbegin() const	{ return mVector.rbegin(); }
	LL_INLINE reverse_iterator rend()				{ return mVector.rend(); }
	LL_INLINE const_reverse_iterator rend() const	{ return mVector.rend(); }

	LL_INLINE void clear()							{ mVector.clear(); mIndexMap.clear(); }
	LL_INLINE bool empty() const					{ return mVector.empty(); }
	LL_INLINE size_type size() const				{ return mVector.size(); }

	Type& operator[](const Key& k)
	{
		typename std::map<Key, U32>::const_iterator iter = mIndexMap.find(k);
		if (iter == mIndexMap.end())
		{
			U32 n = mVector.size();
			mIndexMap[k] = n;
			mVector.push_back(Type());
			llassert(mVector.size() == mIndexMap.size());
			return mVector[n];
		}
		return mVector[iter->second];
	}

	const_iterator find(const Key& k) const
	{
		typename std::map<Key, U32>::const_iterator iter = mIndexMap.find(k);
		if (iter == mIndexMap.end())
		{
			return mVector.end();
		}
		return mVector.begin() + iter->second;
	}

protected:
	std::vector<Type>	mVector;
	std::map<Key, U32>	mIndexMap;
};

class LLMsgVarData
{
protected:
	LOG_CLASS(LLMsgVarData);

public:
	LL_INLINE LLMsgVarData()
	:	mName(NULL),
		mData(NULL),
		mSize(-1),
		mDataSize(-1),
		mType(MVT_U8)
	{
	}

	LL_INLINE LLMsgVarData(const char* name, EMsgVariableType type)
	:	mSize(-1),
		mDataSize(-1),
		mData(NULL),
		mType(type)
	{
		mName = (char*)name;
	}

	LL_INLINE void deleteData()
	{
		delete[] mData;
		mData = NULL;
	}

	void addData(const void* indata, S32 size, EMsgVariableType type,
				 S32 data_size = -1);

	LL_INLINE char* getName() const				{ return mName; }
	LL_INLINE S32 getSize() const				{ return mSize; }
	LL_INLINE void* getData()					{ return (void*)mData; }
	LL_INLINE const void* getData() const		{ return (const void*)mData; }
	LL_INLINE S32 getDataSize() const			{ return mDataSize; }
	LL_INLINE EMsgVariableType getType() const	{ return mType; }

	static std::string variableTypeToString(EMsgVariableType type);

protected:
	char*				mName;
	U8*					mData;
	S32					mSize;
	S32					mDataSize;
	EMsgVariableType	mType;
};

class LLMsgBlkData
{
public:
	LLMsgBlkData(const char* name, S32 blocknum)
	:	mBlockNumber(blocknum),
		mTotalSize(-1)
	{
		mName = (char*)name;
	}

	~LLMsgBlkData()
	{
		for (msg_var_data_map_t::iterator iter = mMemberVarData.begin(),
										  end = mMemberVarData.end();
			 iter != end; ++iter)
		{
			iter->deleteData();
		}
	}

	void addVariable(const char* name, EMsgVariableType type)
	{
		LLMsgVarData tmp(name,type);
		mMemberVarData[name] = tmp;
	}

	void addData(char* name, const void* datap, S32 size,
				 EMsgVariableType type, S32 data_size = -1)
	{
		// Creates a new entry if one does not exist:
		LLMsgVarData* temp = &mMemberVarData[name];
		temp->addData(datap, size, type, data_size);
	}

public:
	typedef LLIndexedVector<LLMsgVarData, const char*, 8> msg_var_data_map_t;
	msg_var_data_map_t	mMemberVarData;

	char*				mName;
	S32					mBlockNumber;
	S32					mTotalSize;
};

class LLMsgData
{
public:
	LLMsgData(const char* name)
	:	mTotalSize(-1)
	{
		mName = (char*)name;
	}

	~LLMsgData()
	{
		for_each(mMemberBlocks.begin(), mMemberBlocks.end(),
				 DeletePairedPointer());
		mMemberBlocks.clear();
	}

	LL_INLINE void addBlock(LLMsgBlkData* blockp)
	{
		mMemberBlocks[blockp->mName] = blockp;
	}

	void addDataFast(char* blockname, char* varname, const void* data,
					 S32 size, EMsgVariableType type, S32 data_size = -1);

public:
	typedef std::map<char*, LLMsgBlkData*> msg_blk_data_map_t;
	msg_blk_data_map_t	mMemberBlocks;
	char*				mName;
	S32					mTotalSize;
};

// LLMessage* classes store the template of messages
class LLMessageVariable
{
public:
	LL_INLINE LLMessageVariable()
	:	mName(NULL),
		mType(MVT_NULL),
		mSize(-1)
	{
	}

	LL_INLINE LLMessageVariable(char* name)
	:	mName(name),
		mType(MVT_NULL),
		mSize(-1)
	{
	}

	LL_INLINE LLMessageVariable(const char* name, EMsgVariableType type,
								S32 size)
	:	mName(gMessageStringTable.getString(name)),
		mType(type),
		mSize(size)
	{
	}

	friend std::ostream& operator<<(std::ostream& s, LLMessageVariable& msg);

	LL_INLINE EMsgVariableType	getType() const	{ return mType; }
	LL_INLINE S32				getSize() const	{ return mSize; }
	LL_INLINE char*				getName() const	{ return mName; }

protected:
	char*				mName;
	EMsgVariableType	mType;
	S32					mSize;
};

typedef enum e_message_block_type
{
	MBT_NULL,
	MBT_SINGLE,
	MBT_MULTIPLE,
	MBT_VARIABLE,
	MBT_EOF
} EMsgBlockType;

class LLMessageBlock
{
public:
	LLMessageBlock(const char* name, EMsgBlockType type, S32 number = 1)
	:	mType(type),
		mNumber(number),
		mTotalSize(0)
	{
		mName = gMessageStringTable.getString(name);
	}

	~LLMessageBlock()
	{
		for_each(mMemberVariables.begin(), mMemberVariables.end(),
				 DeletePointer());
		mMemberVariables.clear();
	}

	void addVariable(char* name, EMsgVariableType type, S32 size)
	{
		LLMessageVariable** varp = &mMemberVariables[name];
		if (*varp != NULL)
		{
			llerrs << name << " has already been used as a variable name !"
				   << llendl;
		}
		*varp = new LLMessageVariable(name, type, size);
		if ((*varp)->getType() != MVT_VARIABLE && mTotalSize != -1)
		{
			mTotalSize += (*varp)->getSize();
		}
		else
		{
			mTotalSize = -1;
		}
	}

	LL_INLINE EMsgVariableType getVariableType(char* name)
	{
		return (mMemberVariables[name])->getType();
	}

	LL_INLINE S32 getVariableSize(char* name)
	{
		return (mMemberVariables[name])->getSize();
	}

	LL_INLINE const LLMessageVariable* getVariable(char* name) const
	{
		message_variable_map_t::const_iterator iter = mMemberVariables.find(name);
		return iter != mMemberVariables.end() ? *iter : NULL;
	}

	friend std::ostream& operator<<(std::ostream& s, LLMessageBlock& msg);

public:
	typedef LLIndexedVector<LLMessageVariable*, const char*, 8> message_variable_map_t;
	message_variable_map_t 	mMemberVariables;
	char*					mName;
	EMsgBlockType			mType;
	S32						mNumber;
	S32						mTotalSize;
};

enum EMsgFrequency
{
	MFT_NULL	= 0,  // value is size of message number in bytes
	MFT_HIGH	= 1,
	MFT_MEDIUM	= 2,
	MFT_LOW		= 4
};

typedef enum e_message_trust
{
	MT_TRUST,
	MT_NOTRUST
} EMsgTrust;

enum EMsgEncoding
{
	ME_UNENCODED,
	ME_ZEROCODED
};

enum EMsgDeprecation
{
	MD_NOTDEPRECATED,
	MD_UDPDEPRECATED,
	MD_UDPBLACKLISTED,
	MD_DEPRECATED
};

class LLMessageTemplate
{
protected:
	LOG_CLASS(LLMessageTemplate);

public:
	LLMessageTemplate(const char* name, U32 message_number, EMsgFrequency freq)
	:	//mMemberBlocks(),
		mName(NULL),
		mFrequency(freq),
		mTrust(MT_NOTRUST),
		mEncoding(ME_ZEROCODED),
		mDeprecation(MD_NOTDEPRECATED),
		mMessageNumber(message_number),
		mTotalSize(0),
		mReceiveCount(0),
		mReceiveBytes(0),
		mReceiveInvalid(0),
		mDecodeTimeThisFrame(0.f),
		mTotalDecoded(0),
		mTotalDecodeTime(0.f),
		mMaxDecodeTimePerMsg(0.f),
		mBanFromTrusted(false),
		mBanFromUntrusted(false),
		mHandlerFunc(NULL),
		mUserData(NULL)
	{
		mName = gMessageStringTable.getString(name);
	}

	~LLMessageTemplate()
	{
		for_each(mMemberBlocks.begin(), mMemberBlocks.end(), DeletePointer());
		mMemberBlocks.clear();
	}

	void addBlock(LLMessageBlock* blockp)
	{
		LLMessageBlock** member_blockp = &mMemberBlocks[blockp->mName];
		if (*member_blockp != NULL)
		{
			llerrs << "Block " << blockp->mName
				<< "has already been used as a block name!" << llendl;
		}
		*member_blockp = blockp;
		if (mTotalSize != -1 && blockp->mTotalSize != -1 &&
			(blockp->mType == MBT_SINGLE || blockp->mType == MBT_MULTIPLE))
		{
			mTotalSize += blockp->mNumber * blockp->mTotalSize;
		}
		else
		{
			mTotalSize = -1;
		}
	}

	LL_INLINE LLMessageBlock* getBlock(char* name)
	{
		return mMemberBlocks[name];
	}

	// Trusted messages can only be received on trusted circuits.
	LL_INLINE void setTrust(EMsgTrust t)
	{
		mTrust = t;
	}

	LL_INLINE EMsgTrust getTrust() const
	{
		return mTrust;
	}

	// Controls for how the message should be encoded
	LL_INLINE void setEncoding(EMsgEncoding e)
	{
		mEncoding = e;
	}

	LL_INLINE EMsgEncoding getEncoding() const
	{
		return mEncoding;
	}

	LL_INLINE void setDeprecation(EMsgDeprecation d)
	{
		mDeprecation = d;
	}

	EMsgDeprecation getDeprecation() const
	{
		return mDeprecation;
	}

	LL_INLINE void setHandlerFunc(void (*handler_func)(LLMessageSystem*,
													   void**),
								  void** user_data)
	{
		mHandlerFunc = handler_func;
		mUserData = user_data;
	}

	LL_INLINE bool callHandlerFunc(LLMessageSystem* msgsystem) const
	{
		if (mHandlerFunc)
		{
			mHandlerFunc(msgsystem, mUserData);
			return true;
		}
		return false;
	}

	LL_INLINE bool isUdpBanned() const
	{
		return mDeprecation == MD_UDPBLACKLISTED;
	}

	void banUdp();

	LL_INLINE bool isBanned(bool trustedSource) const
	{
		return trustedSource ? mBanFromTrusted : mBanFromUntrusted;
	}

	friend std::ostream& operator<<(std::ostream& s, LLMessageTemplate& msg);

	LL_INLINE const LLMessageBlock* getBlock(char* name) const
	{
		message_block_map_t::const_iterator iter = mMemberBlocks.find(name);
		return iter != mMemberBlocks.end() ? *iter : NULL;
	}

private:
	// Message handler function (this is set by each application)
	void				(*mHandlerFunc)(LLMessageSystem* msgsys, void** datap);
	void**				mUserData;

public:
	typedef LLIndexedVector<LLMessageBlock*, char*, 8> message_block_map_t;
	message_block_map_t	mMemberBlocks;
	char*				mName;
	EMsgFrequency		mFrequency;
	EMsgTrust			mTrust;
	EMsgEncoding		mEncoding;
	EMsgDeprecation		mDeprecation;
	U32					mMessageNumber;
	S32					mTotalSize;
	// How many of this template have been received since last reset:
	U32					mReceiveCount;
	U32					mReceiveBytes;			// How many bytes received
	U32					mReceiveInvalid;		// How many "invalid" packets
	F32					mDecodeTimeThisFrame;	// Total seconds spent decoding this frame
	U32					mTotalDecoded;			// Total messages successfully decoded
	F32					mTotalDecodeTime;		// Total time successfully decoding messages
	F32					mMaxDecodeTimePerMsg;
	bool				mBanFromTrusted;
	bool				mBanFromUntrusted;
};
