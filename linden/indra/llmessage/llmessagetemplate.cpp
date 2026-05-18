/**
 * @file llmessagetemplate.cpp
 * @brief Implementation of message template classes.
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

#include "linden_common.h"

#include "llmessagetemplate.h"

#include "llmessage.h"

void LLMsgVarData::addData(const void* data, S32 size, EMsgVariableType type,
						   S32 data_size)
{
	mSize = size;
	mDataSize = data_size;
	if (type != MVT_VARIABLE && type != MVT_FIXED && mType != MVT_VARIABLE &&
		mType != MVT_FIXED)
	{
		if (mType != type)
		{
			llwarns << "Type mismatch for " << mName << " - Expected type: "
					<< variableTypeToString(mType) << " - Passed type: "
					<< variableTypeToString(type) << llendl;
		}
	}
	if (size)
	{
		delete[] mData; // Delete it if it already exists
		mData = new U8[size];
		htonmemcpy(mData, data, mType, size);
	}
}

void LLMsgData::addDataFast(char* blockname, char* varname, const void* data,
							S32 size, EMsgVariableType type, S32 data_size)
{
	// remember that if the blocknumber is > 0 then the number is appended to
	// the name
	char* namep = (char*)blockname;
	LLMsgBlkData* block_data = mMemberBlocks[namep];
	if (block_data->mBlockNumber)
	{
		namep += block_data->mBlockNumber;
		block_data->addData(varname, data, size, type, data_size);
	}
	else
	{
		block_data->addData(varname, data, size, type, data_size);
	}
}

//static
std::string LLMsgVarData::variableTypeToString(EMsgVariableType type)
{
#define RTNENUM(E) case E: return #E
	switch (type)
	{
		RTNENUM(MVT_NULL);
		RTNENUM(MVT_FIXED);
		RTNENUM(MVT_VARIABLE);
		RTNENUM(MVT_U8);
		RTNENUM(MVT_U16);
		RTNENUM(MVT_U32);
		RTNENUM(MVT_U64);
		RTNENUM(MVT_S8);
		RTNENUM(MVT_S16);
		RTNENUM(MVT_S32);
		RTNENUM(MVT_S64);
		RTNENUM(MVT_F32);
		RTNENUM(MVT_F64);
		RTNENUM(MVT_LLVector3);
		RTNENUM(MVT_LLVector3d);
		RTNENUM(MVT_LLVector4);
		RTNENUM(MVT_LLQuaternion);
		RTNENUM(MVT_LLUUID);
		RTNENUM(MVT_BOOL);
		RTNENUM(MVT_IP_ADDR);
		RTNENUM(MVT_IP_PORT);
		RTNENUM(MVT_U16Vec3);
		RTNENUM(MVT_U16Quat);
		RTNENUM(MVT_S16Array);

		default:
			return llformat("%d", (S32)type);
	}
#undef RTNENUM
}

// LLMessageVariable functions and friends

std::ostream& operator<<(std::ostream& s, LLMessageVariable& msg)
{
	s << "\t\t" << msg.mName << " (";
	switch (msg.mType)
	{
		case MVT_FIXED:
			s << "Fixed, " << msg.mSize << " bytes total)\n";
			break;

		case MVT_VARIABLE:
			s << "Variable, " << msg.mSize << " bytes of size info)\n";
			break;

		default:
			s << "Unknown\n";
			break;
	}
	return s;
}

// LLMessageBlock functions and friends

std::ostream& operator<<(std::ostream& s, LLMessageBlock& msg)
{
	s << "\t" << msg.mName << " (";
	switch (msg.mType)
	{
		case MBT_SINGLE:
			s << "Fixed";
			break;

		case MBT_MULTIPLE:
			s << "Multiple - " << msg.mNumber << " copies";
			break;

		case MBT_VARIABLE:
			s << "Variable";
			break;

		default:
			s << "Unknown";
			break;
	}
	if (msg.mTotalSize != -1)
	{
		s << ", " << msg.mTotalSize << " bytes each, "
		  << msg.mNumber * msg.mTotalSize << " bytes total)\n";
	}
	else
	{
		s << ")\n";
	}


	for (LLMessageBlock::message_variable_map_t::iterator
			iter = msg.mMemberVariables.begin(),
			end = msg.mMemberVariables.end();
		 iter != end; ++iter)
	{
		LLMessageVariable& ci = *(*iter);
		s << ci;
	}

	return s;
}

// LLMessageTemplate functions and friends

std::ostream& operator<<(std::ostream& s, LLMessageTemplate& msg)
{
	switch (msg.mFrequency)
	{
	case MFT_HIGH:
		s << "========================================\n" << "Message #"
		  << msg.mMessageNumber << "\n" << msg.mName << " (";
		s << "High";
		break;
	case MFT_MEDIUM:
		s << "========================================\n" << "Message #";
		s << (msg.mMessageNumber & 0xFF) << "\n" << msg.mName << " (";
		s << "Medium";
		break;
	case MFT_LOW:
		s << "========================================\n" << "Message #";
		s << (msg.mMessageNumber & 0xFFFF) << "\n" << msg.mName << " (";
		s << "Low";
		break;
	default:
		s << "Unknown";
		break;
	}

	if (msg.mTotalSize != -1)
	{
		s << ", " << msg.mTotalSize << " bytes total)\n";
	}
	else
	{
		s << ")\n";
	}

	for (LLMessageTemplate::message_block_map_t::iterator
			iter = msg.mMemberBlocks.begin(), end = msg.mMemberBlocks.end();
		 iter != end; ++iter)
	{
		LLMessageBlock* ci = *iter;
		s << *ci;
	}

	return s;
}

void LLMessageTemplate::banUdp()
{
	static const char* deprecation[] = {
		"NotDeprecated",
		"Deprecated",
		"UDPDeprecated",
		"UDPBlackListed"
	};

	if (mDeprecation != MD_DEPRECATED)
	{
		llinfos << "Setting " << mName << " to UDPBlackListed was "
				<< deprecation[mDeprecation] << llendl;
		mDeprecation = MD_UDPBLACKLISTED;
	}
	else
	{
		llinfos << mName << " is already more deprecated than UDPBlackListed"
				<< llendl;
	}
}
