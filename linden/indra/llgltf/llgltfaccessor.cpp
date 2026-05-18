/**
 * @file llgltfaccessor.cpp
 * @brief LLGLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024, Linden Research, Inc.
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

#include <fstream>

#include "lldir.h"
#include "llfilesystem.h"
#include "llgltfbufferutil.h"
#include "lluri.h"
#include "llvector2.h"
#include "llvector3.h"

using namespace LLGLTF;

namespace LLGLTF
{

Accessor::Type gltf_type_to_enum(const std::string& type)
{
	if (type == "VEC2")
	{
		return Accessor::Type::VEC2;
	}
	if (type == "VEC3")
	{
		return Accessor::Type::VEC3;
	}
	if (type == "VEC4")
	{
		return Accessor::Type::VEC4;
	}
	if (type == "MAT2")
	{
		return Accessor::Type::MAT2;
	}
	if (type == "MAT3")
	{
		return Accessor::Type::MAT3;
	}
	if (type == "MAT4")
	{
		return Accessor::Type::MAT4;
	}
	if (type != "SCALAR")
	{
		llwarns << "Unknown accessor type: " << type << llendl;
	}
	return Accessor::Type::SCALAR;
}

std::string enum_to_gltf_type(Accessor::Type type)
{
	switch (type)
	{
		case Accessor::Type::SCALAR:
			break;

		case Accessor::Type::VEC2:
			return "VEC2";

		case Accessor::Type::VEC3:
			return "VEC3";

		case Accessor::Type::VEC4:
			return "VEC4";

		case Accessor::Type::MAT2:
			return "MAT2";

		case Accessor::Type::MAT3:
			return "MAT3";

		case Accessor::Type::MAT4:
			return "MAT4";

		default:
			llwarns << "Unknown accessor type: " << (U8)type << llendl;
	}
	return "SCALAR";
}

}	// namespace LLGLTF

void Buffer::erase(Asset& asset, S32 offset, S32 length)
{
	S32 idx = (S32)(this - &asset.mBuffers[0]);
	mData.erase(mData.begin() + offset, mData.begin() + offset + length);
	mByteLength = mData.size();
	for (BufferView& view : asset.mBufferViews)
	{
		if (view.mBuffer == idx && view.mByteOffset >= offset)
		{
			view.mByteOffset -= length;
		}
	}
}

bool Buffer::prep(Asset& asset)
{
	if (mByteLength <= 0)
	{
		llwarns << "Invalid byte length: " << mByteLength << ". Aborted."
				<< llendl;
		return false;
	}

	if (mUri.find("data:") == 0)
	{
		llwarns << "Data URIs not yet supported. Aborted." << llendl;
		return false;
	}

	LLUUID id;
	if (mUri.size() == UUID_STR_SIZE &&
		LLUUID::parseUUID(mUri, &id) && id.notNull())
	{
		LLFileSystem file(id);
		S32 size = file.getSize();
		if (size < mByteLength)
		{
			llwarns << "Unexpected glbin size: " << id << " is "
					<< size << " bytes. Expected size was " << mByteLength
					<< " bytes. Aborted." << llendl;
			return false;
		}
		mData.resize(mByteLength);
		if (!file.read((U8*)mData.data(), mByteLength))
		{
			llwarns << "Failed to load buffer data from asset: " << id
					<< ". Aborted." << llendl;
			return false;
		}
		return true;
	}

	// Note: mUri could be empty if we are loading from .glb
	if (!asset.mFilename.empty() && !mUri.empty())
	{
		std::string dir = LLDir::getDirName(asset.mFilename);
		std::string bin_file = dir + LL_DIR_DELIM_STR + mUri;
		if (!LLFile::exists(bin_file))
		{
			// Characters might be escaped in the URI
			bin_file = dir + LL_DIR_DELIM_STR + LLURI::unescape(mUri);
		}
		llifstream file(bin_file, std::ios::binary);
		if (!file.is_open())
		{
			llwarns << "Failed to open file: " << bin_file << ". Aborted."
					<< llendl;
			return false;
		}

		file.seekg(0, std::ios::end);
		S32 size = file.tellg();
		if (size < mByteLength)
		{
			llwarns << "Unexpected file size: " << bin_file << " is "
					<< size << " bytes. Expected size was " << mByteLength
					<< " bytes. Aborted." << llendl;
			return false;
		}
		mData.resize(mByteLength);
		file.seekg(0, std::ios::beg);
		file.read((char*)mData.data(), mByteLength);
	}

	return true;
}

bool Buffer::save(Asset& asset, const std::string& folder)
{
	if (mUri.substr(0, 5) == "data:")
	{
		llwarns << "Data URIs not yet supported. Aborted." << llendl;
		return false;
	}

	if (folder.empty())
	{
		llwarns << "Empty save folder name. Aborted." << llendl;
		return false;
	}

	std::string bin_file = folder;
	if (bin_file.back() != LL_DIR_DELIM_CHR)
	{
		bin_file += LL_DIR_DELIM_CHR;
	}
	if (mUri.empty())
	{
		if (mName.empty())
		{
			S32 idx = (S32)(this - &asset.mBuffers[0]);
			mUri = llformat("buffer_%d.bin", idx);
		}
		else
		{
			mUri = mName + ".bin";
		}
	}
	bin_file += mUri;

	llofstream file(bin_file, std::ios::binary);
	if (!file.is_open())
	{
		llwarns << "Failed to open file: " << bin_file << ". Aborted."
				<< llendl;
		return false;
	}
	file.write((char*)mData.data(), mData.size());
	return true;
}

void Buffer::serialize(lljson& dst) const
{
	write(mName, "name", dst);
	write(mUri, "uri", dst);
	write_always(mByteLength, "byteLength", dst);
}

const Buffer& Buffer::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "name", mName);
		copy(src, "uri", mUri);
		copy(src, "byteLength", mByteLength);
		// Note: do not attempt to handle the URI here since it is a reference
		// to a file which is not loaded until after the JSON document is fully
		// parsed.
	}
	return *this;
}

void BufferView::serialize(lljson& dst) const
{
	write_always(mBuffer, "buffer", dst);
	write_always(mByteLength, "byteLength", dst);
	write(mByteOffset, "byteOffset", dst, 0);
	write(mByteStride, "byteStride", dst, 0);
	write(mTarget, "target", dst, -1);
	write(mName, "name", dst);
}

const BufferView& BufferView::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "buffer", mBuffer);
		copy(src, "byteLength", mByteLength);
		copy(src, "byteOffset", mByteOffset);
		copy(src, "byteStride", mByteStride);
		copy(src, "target", mTarget);
		copy(src, "name", mName);
	}
	return *this;
}

void Accessor::serialize(lljson& dst) const
{
	write(mName, "name", dst);
	write(mBufferView, "bufferView", dst, INVALID_INDEX);
	write(mByteOffset, "byteOffset", dst, 0);
	write_always(mComponentType, "componentType", dst);
	write_always(mCount, "count", dst);
	write_always(enum_to_gltf_type(mType), "type", dst);
	write(mNormalized, "normalized", dst, false);
	write(mMax, "max", dst);
	write(mMin, "min", dst);
}

const Accessor& Accessor::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "name", mName);
		copy(src, "bufferView", mBufferView);
		copy(src, "byteOffset", mByteOffset);
		copy(src, "componentType", mComponentType);
		copy(src, "count", mCount);
		copy(src, "type", mType);
		copy(src, "normalized", mNormalized);
		copy(src, "max", mMax);
		copy(src, "min", mMin);
	}
	return *this;
}
