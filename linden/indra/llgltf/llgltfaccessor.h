/**
 * @file llgltfaccessor.h
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

#pragma once

#include <vector>
#include <string>

// Do not include the full header (json.hpp) here: we just need the forward
// declarations. HB
#include "json_fwd.hpp"

using lljson = nlohmann::json;

namespace LLGLTF
{
	constexpr S32 INVALID_INDEX = -1;

	class Asset;

	class Buffer
	{
	protected:
		LOG_CLASS(LLGLTF::Buffer);

	public:
		LL_INLINE Buffer()
		:	mByteLength(0)
		{
		}

		// Erase the given range from this buffer. Also updates all buffer
		// views in given asset that reference this buffer.
		void erase(Asset& asset, S32 offset, S32 length);

		bool prep(Asset& asset);

		void serialize(lljson& obj) const;
		const Buffer& operator=(const lljson& src);

		bool save(Asset& asset, const std::string& folder);

	public:
		std::string		mName;
		std::string		mUri;
		std::vector<U8>	mData;
		S32				mByteLength;
	};

	class BufferView
	{
	protected:
		LOG_CLASS(LLGLTF::BufferView);

	public:
		LL_INLINE BufferView()
		:	mBuffer(INVALID_INDEX),
			mByteLength(0),
			mByteOffset(0),
			mByteStride(0),
			mTarget(-1)
		{
		}

		void serialize(lljson& obj) const;
		const BufferView& operator=(const lljson& src);

	public:
		std::string	mName;
		S32			mBuffer;
		S32			mByteLength;
		S32			mByteOffset;
		S32			mByteStride;
		S32			mTarget;
	};
	
	class Accessor
	{
	protected:
		LOG_CLASS(LLGLTF::Accessor);

	public:
		enum class Type : U8
		{
			SCALAR,
			VEC2,
			VEC3,
			VEC4,
			MAT2,
			MAT3,
			MAT4
		};

		enum class ComponentType : U32
		{
			BYTE = 5120,
			UNSIGNED_BYTE = 5121,
			SHORT = 5122,
			UNSIGNED_SHORT = 5123,
			UNSIGNED_INT = 5125,
			FLOAT = 5126
		};

		LL_INLINE Accessor()
		:	mBufferView(INVALID_INDEX),
			mByteOffset(0),
			mComponentType(ComponentType::BYTE),
			mCount(0),
			mType(Type::SCALAR),
			mNormalized(false)
		{
		}

		void serialize(lljson& obj) const;
		const Accessor& operator=(const lljson& src);

	public:
		std::string			mName;
		std::vector<F64> 	mMax;
		std::vector<F64> 	mMin;
		S32					mBufferView;
		S32					mByteOffset;
		ComponentType		mComponentType;
		S32					mCount;
		Type				mType;
		bool				mNormalized;
	};

	// Converts from "SCALAR", "VEC2", etc to Accessor::Type
	Accessor::Type gltf_type_to_enum(const std::string& type);
	// Converts from Accessor::Type to "SCALAR", "VEC2", etc
	std::string enum_to_gltf_type(Accessor::Type type);
}
