/**
 * @file llgltfbufferutil.h
 * @brief LLGLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024, Linden Research, Inc.
 * Copyright (c) 2025, Henri Beauchamp.
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

// Inline template implementations for copying data out of glTF buffers.
// DO NOT include from header files. This header should only be needed and
// included by the llgltf *.cpp files.

#pragma once

#include "json.hpp"

#include "llcolor4u.h"
#include "llgltfasset.h"
#include "llgltfglm.h"
#include "llgltfprimitive.h"
#include "llmath.h"
#include "llstrider.h"
#include "llvector2.h"
#include "llvector3.h"

// I am using nlohmann::json (instead of boost::json in LL's viewer): beside
// the fact that nlohmann::json is already used everywhere else in the Cool VL
// Viewer (including by tinygltf which LL's viewer is using too), it is also a
// lightweight (33% smaller than boost::json), header-only, fully templatized
// implementation (unlike boost::json with which you still need to include a
// boost/json/src.hpp pseudo-header (actually a C++ module) in one of your own
// cpp modules); it also does not make a distinction between JSON values and
// JSON objects (objects are just values of the object type), which simplifies
// things in llgtlf, allowing to replace both 'using Value=boost::json::value'
// and 'using Object=boost::json::object' with just 'lljson' below, and saving
// from implementing any Object-signature-specific templates. HB
using lljson = nlohmann::json;

// Let's make things look as simple as get<std::string> while avoiding a
// costly string copy. HB
#define get_string template get_ref<const lljson::string_t&>

// Suppress unused function warning: clang complains here but these
// specializations are definitely used.
#if LL_CLANG
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunused-function"
#endif

namespace LLGLTF
{
	class Material;
	class Primitive;

	LL_NO_INLINE void not_implemented(const char* func_sig);

	// Copy one Scalar from src to dst
	template<class S, class T>
	LL_INLINE void copyScalar(S* src, T& dst)
	{
		not_implemented(LL_FUNC);
	}

	// Copy one vec2 from src to dst
	template<class S, class T>
	LL_INLINE void copyVec2(S* src, T& dst)
	{
		not_implemented(LL_FUNC);
	}

	// Copy one vec3 from src to dst
	template<class S, class T>
	LL_INLINE void copyVec3(S* src, T& dst)
	{
		not_implemented(LL_FUNC);
	}

	// Copy one vec4 from src to dst
	template<class S, class T>
	LL_INLINE void copyVec4(S* src, T& dst)
	{
		not_implemented(LL_FUNC);
	}

	// Copy one mat2 from src to dst
	template<class S, class T>
	LL_INLINE void copyMat2(S* src, T& dst)
	{
		not_implemented(LL_FUNC);
	}

	// Copy one mat3 from src to dst
	template<class S, class T>
	LL_INLINE void copyMat3(S* src, T& dst)
	{
		not_implemented(LL_FUNC);
	}

	// Copy one mat4 from src to dst
	template<class S, class T>
	LL_INLINE void copyMat4(S* src, T& dst)
	{
		not_implemented(LL_FUNC);
	}

	//=========================================================================
	// Concrete implementations for different types of source and destination
	//=========================================================================

	template<>
	LL_INLINE void copyScalar<F32, F32>(F32* src, F32& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U32, U32>(U32* src, U32& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S32, S32>(S32* src, S32& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U32, U16>(U32* src, U16& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S32, S16>(S32* src, S16& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U16, U16>(U16* src, U16& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S16, S16>(S16* src, S16& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U16, U32>(U16* src, U32& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S16, S32>(S16* src, S32& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U8, U16>(U8* src, U16& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S8, S16>(S8* src, S16& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U8, U32>(U8* src, U32& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S8, S32>(S8* src, S32& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U8, U64>(U8* src, U64& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S8, S64>(S8* src, S64& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U16, U64>(U16* src, U64& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S16, S64>(S16* src, S64& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U32, U64>(U32* src, U64& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<S32, S64>(S32* src, S64& dst)
	{
		dst = *src;
	}

	template<>
	LL_INLINE void copyScalar<U32, LLVector4a>(U32* src, LLVector4a& dst)
	{
		dst.set((F32)*src, 0.f, 0.f, 0.f);
	}

	template<>
	LL_INLINE void copyScalar<U16, LLVector4a>(U16* src, LLVector4a& dst)
	{
		dst.set((F32)*src, 0.f, 0.f, 0.f);
	}

	template<>
	LL_INLINE void copyScalar<U8, LLVector4a>(U8* src, LLVector4a& dst)
	{
		dst.set((F32)*src, 0.f, 0.f, 0.f);
	}

	template<>
	LL_INLINE void copyScalar<U32, LLVector2>(U32* src, LLVector2& dst)
	{
		dst.set((F32)*src, 0.f);
	}

	template<>
	LL_INLINE void copyScalar<U16, LLVector2>(U16* src, LLVector2& dst)
	{
		dst.set((F32)*src, 0.f);
	}

	template<>
	LL_INLINE void copyScalar<U8, LLVector2>(U8* src, LLVector2& dst)
	{
		dst.set((F32)*src, 0.f);
	}

	template<>
	LL_INLINE void copyVec2<F32, LLVector2>(F32* src, LLVector2& dst)
	{
		dst.set(src[0], src[1]);
	}

	template<>
	LL_INLINE void copyVec3<F32, LLVector2>(F32* src, LLVector2& dst)
	{
		dst.set(src[0], src[1]);
	}

	template<>
	LL_INLINE void copyVec3<F32, glm::vec3>(F32* src, glm::vec3& dst)
	{
		dst = glm::vec3(src[0], src[1], src[2]);
	}

	template<>
	LL_INLINE void copyVec3<F32, LLVector4a>(F32* src, LLVector4a& dst)
	{
		dst.load3(src);
	}

	template<>
	LL_INLINE void copyVec3<F32, LLColor4U>(F32* src, LLColor4U& dst)
	{
		dst.set(U8(src[0] * 255.f), U8(src[1] * 255.f), U8(src[2] * 255.f),
				255);
	}

	template<>
	LL_INLINE void copyVec3<U16, LLColor4U>(U16* src, LLColor4U& dst)
	{
		dst.set(src[0], src[1], src[2], 255);
	}

	template<>
	LL_INLINE void copyVec4<U8, LLColor4U>(U8* src, LLColor4U& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<U16, LLColor4U>(U16* src, LLColor4U& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<F32, LLColor4U>(F32* src, LLColor4U& dst)
	{
		dst.set(src[0]*255, src[1]*255, src[2]*255, src[3]*255);
	}

	template<>
	LL_INLINE void copyVec4<F32, LLVector4a>(F32* src, LLVector4a& dst)
	{
		dst.loadua(src);
	}

	template<>
	LL_INLINE void copyVec4<U64, LLVector4a>(U64* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<S64, LLVector4a>(S64* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<U32, LLVector4a>(U32* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<S32, LLVector4a>(S32* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<U16, LLVector4a>(U16* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<S16, LLVector4a>(S16* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<U8, LLVector4a>(U8* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<S8, LLVector4a>(S8* src, LLVector4a& dst)
	{
		dst.set(src[0], src[1], src[2], src[3]);
	}

	template<>
	LL_INLINE void copyVec4<F32, glm::quat>(F32* src, glm::quat& dst)
	{
		dst.x = src[0];
		dst.y = src[1];
		dst.z = src[2];
		dst.w = src[3];
	}

	// Used by glTF-Sample-Models-master/2.0/BrainStem/glTF/BrainStem.gltf but
	// does not work properly... I also tried with bit-shifting src[0...3] or
	// src[3...0] into dst, to no avail. HB
	template<>
	LL_INLINE void copyVec4<U16, U64>(U16* src, U64& dst)
	{
		U64* p = (U64*)src;
		dst = *p;
	}

	template<>
	LL_INLINE void copyMat4<F32, glm::mat4>(F32* src, glm::mat4& dst)
	{
		dst = glm::make_mat4(src);
	}

	//=========================================================================

	// Copy from src to dst, stride is the number of bytes between each element
	// in src, count is number of elements to copy.
	template<class S, class T>
	LL_INLINE void copyScalar(S* src, LLStrider<T> dst, S32 stride, S32 count)
	{
		for (S32 i = 0; i < count; ++i)
		{
			copyScalar(src, *dst++);
			src = (S*)((U8*)src + stride);
		}
	}

	// Copy from src to dst, stride is the number of bytes between each element
	// in src, count is number of elements to copy.
	template<class S, class T>
	LL_INLINE void copyVec2(S* src, LLStrider<T> dst, S32 stride, S32 count)
	{
		for (S32 i = 0; i < count; ++i)
		{
			copyVec2(src, *dst++);
			src = (S*)((U8*)src + stride);
		}
	}

	// Copy from src to dst, stride is the number of bytes between each element
	// in src, count is number of elements to copy.
	template<class S, class T>
	LL_INLINE void copyVec3(S* src, LLStrider<T> dst, S32 stride, S32 count)
	{
		for (S32 i = 0; i < count; ++i)
		{
			copyVec3(src, *dst++);
			src = (S*)((U8*)src + stride);
		}
	}

	// Copy from src to dst, stride is the number of bytes between each element
	// in src, count is number of elements to copy.
	template<class S, class T>
	LL_INLINE void copyVec4(S* src, LLStrider<T> dst, S32 stride, S32 count)
	{
		for (S32 i = 0; i < count; ++i)
		{
			copyVec4(src, *dst++);
			src = (S*)((U8*)src + stride);
		}
	}

	// Copy from src to dst, stride is the number of bytes between each element
	// in src, count is number of elements to copy.
	template<class S, class T>
	LL_INLINE void copyMat2(S* src, LLStrider<T> dst, S32 stride, S32 count)
	{
		for (S32 i = 0; i < count; ++i)
		{
			copyMat2(src, *dst++);
			src = (S*)((U8*)src + stride);
		}
	}

	// Copy from src to dst, stride is the number of bytes between each element
	// in src, count is number of elements to copy.
	template<class S, class T>
	LL_INLINE void copyMat3(S* src, LLStrider<T> dst, S32 stride, S32 count)
	{
		for (S32 i = 0; i < count; ++i)
		{
			copyMat3(src, *dst++);
			src = (S*)((U8*)src + stride);
		}
	}

	// Copy from src to dst, stride is the number of bytes between each element
	// in src, count is number of elements to copy.
	template<class S, class T>
	LL_INLINE void copyMat4(S* src, LLStrider<T> dst, S32 stride, S32 count)
	{
		for (S32 i = 0; i < count; ++i)
		{
			copyMat4(src, *dst++);
			src = (S*)((U8*)src + stride);
		}
	}

	LL_NO_INLINE void unsupported_accessor_type(const char* func_sig, U8 type);

	template<class S, class T>
	LL_INLINE void copy(Asset& asset, Accessor& accessor, const S* src,
						LLStrider<T>& dst, S32 bstride)
	{
		if (accessor.mType == Accessor::Type::SCALAR)
		{
			S32 stride = bstride == 0 ? sizeof(S) * 1 : bstride;
			copyScalar((S*)src, dst, stride, accessor.mCount);
		}
		else if (accessor.mType == Accessor::Type::VEC2)
		{
			S32 stride = bstride == 0 ? sizeof(S) * 2 : bstride;
			copyVec2((S*)src, dst, stride, accessor.mCount);
		}
		else if (accessor.mType == Accessor::Type::VEC3)
		{
			S32 stride = bstride == 0 ? sizeof(S) * 3 : bstride;
			copyVec3((S*)src, dst, stride, accessor.mCount);
		}
		else if (accessor.mType == Accessor::Type::VEC4)
		{
			S32 stride = bstride == 0 ? sizeof(S) * 4 : bstride;
			copyVec4((S*)src, dst, stride, accessor.mCount);
		}
		else if (accessor.mType == Accessor::Type::MAT2)
		{
			S32 stride = bstride == 0 ? sizeof(S) * 4 : bstride;
			copyMat2((S*)src, dst, stride, accessor.mCount);
		}
		else if (accessor.mType == Accessor::Type::MAT3)
		{
			S32 stride = bstride == 0 ? sizeof(S) * 9 : bstride;
			copyMat3((S*)src, dst, stride, accessor.mCount);
		}
		else if (accessor.mType == Accessor::Type::MAT4)
		{
			S32 stride = bstride == 0 ? sizeof(S) * 16 : bstride;
			copyMat4((S*)src, dst, stride, accessor.mCount);
		}
		else
		{
			unsupported_accessor_type(LL_FUNC, (U8)accessor.mType);
		}
	}

	LL_NO_INLINE void invalid_buffer(const char* func_sig);
	LL_NO_INLINE void invalid_buffer_view(const char* func_sig);
	LL_NO_INLINE void invalid_component_type(const char* func_sig, U8 type);

	// Copy data from accessor to strider
	template<class T>
	LL_INLINE void copy(Asset& asset, Accessor& accessor, LLStrider<T>& dst)
	{
		if (accessor.mBufferView == INVALID_INDEX ||
			(size_t)accessor.mBufferView >= asset.mBufferViews.size())
		{
			invalid_buffer(LL_FUNC);
			return;
		}
		const BufferView& buffview = asset.mBufferViews[accessor.mBufferView];
		if ((size_t)buffview.mBuffer >= asset.mBuffers.size())
		{
			invalid_buffer_view(LL_FUNC);
			return;
		}
		const Buffer& buffer = asset.mBuffers[buffview.mBuffer];
		const U8* srcp = buffer.mData.data() + buffview.mByteOffset +
						 accessor.mByteOffset;
		const S32 stride = buffview.mByteStride;
		switch ((Accessor::ComponentType)accessor.mComponentType)
		{
			case Accessor::ComponentType::BYTE:
				copy(asset, accessor, (const S8*)srcp, dst, stride);
				break;

			case Accessor::ComponentType::UNSIGNED_BYTE:
				copy(asset, accessor, (const U8*)srcp, dst, stride);
				break;

			case Accessor::ComponentType::SHORT:
				copy(asset, accessor, (const S16*)srcp, dst, stride);
				break;

			case Accessor::ComponentType::UNSIGNED_SHORT:
				copy(asset, accessor, (const U16*)srcp, dst, stride);
				break;

			case Accessor::ComponentType::UNSIGNED_INT:
				copy(asset, accessor, (const U32*)srcp, dst, stride);
				break;

			case Accessor::ComponentType::FLOAT:
				copy(asset, accessor, (const F32*)srcp, dst, stride);
				break;

			default:
				invalid_component_type(LL_FUNC, (U32)accessor.mComponentType);
		}
	}

	// Copy data from accessor to vector
	template<class T>
	LL_INLINE void copy(Asset& asset, Accessor& accessor, std::vector<T>& dst)
	{
		dst.resize(accessor.mCount);
		LLStrider<T> strider = dst.data();
		copy(asset, accessor, strider);
	}

	//=========================================================================
	// nlohmann::json copying utilities

	// To/from lljson
	template<typename T>
	LL_INLINE bool copy(const lljson& src, T& dst)
	{
		dst = src;
		return true;
	}

	template<typename T>
	LL_INLINE bool write(const T& src, lljson& dst)
	{
		dst = lljson::object();
		src.serialize(dst);
		return true;
	}

	template<typename T>
	LL_INLINE bool copy(const lljson& src, fast_hmap<std::string, T>& dst)
	{
		if (!src.is_object())
		{
			return false;
		}
		for (const auto& [key, value] : src.items())
		{
			copy<T>(value, dst[key]);
		}
		return true;
	}

	template<typename T>
	LL_INLINE bool write(const fast_hmap<std::string, T>& src, lljson& dst)
	{
		lljson obj;
		for (const auto& [key, value] : src)
		{
			lljson v;
			if (!write<T>(value, v))
			{
				return false;
			}
			obj[key] = v;
		}
		dst = obj;
		return true;
	}

	// To/from array
	template<typename T>
	LL_INLINE bool copy(const lljson& src, std::vector<T>& dst)
	{
		if (!src.is_array())
		{
			return false;
		}
		dst.resize(src.size());
		for (size_t i = 0; i < src.size(); ++i)
		{
			copy(src[i], dst[i]);
		}
		return true;
	}

	template<typename T>
	LL_INLINE bool write(const std::vector<T>& src, lljson& dst)
	{
		dst = lljson::array();
		for (const T& t : src)
		{
			lljson v;
			if (!write(t, v))
			{
				return false;
			}
			dst.push_back(v);
		}
		return true;
	}

	// Always write a member to an object without checking default
	template<typename T>
	LL_INLINE bool write_always(const T& src, const char* member, lljson& dst)
	{
		lljson& v = dst[member];
		if (!write(src, v))
		{
			dst.erase(member);
			return false;
		}
		return true;
	}

	// For internal use only: use copy_extensions instead.
	template<typename T>
	LL_INLINE bool _copy_extension(const lljson& exts, const char* member,
								   T* dst)
	{
		return exts.contains(member) && copy(exts.at(member), *dst);
	}

	// Copies all extensions from src.extensions to provided destinations
	// Usage:
	//	copy_extensions(src,"KHR_materials_unlit", &mUnlit,
	//					"KHR_materials_pbrSpecularGlossiness",
	//					&mPbrSpecularGlossiness);
	// Returns true if any of the extensions are copied.
	template<class... Types>
	LL_INLINE bool copy_extensions(const lljson& src, Types... args)
	{
		if (!src.is_object())
		{
			return false;
		}
		// Extract the extensions object (do not assume it exists and verify
		// that it is an object)
		if (!src.contains("extensions"))
		{
			return false;
		}
		const lljson& exts = src.at("extensions");
		if (!exts.is_object())
		{
			return false;
		}
		bool success = false;
		size_t count = sizeof...(args);
		for (size_t i = 0; i < count; i += 2)
		{
			if (_copy_extension(exts, args...))
			{
				success = true;
			}
		}
		return success;
	}

	// For internal use only: use write_extensions instead.
	template<typename T>
	LL_INLINE bool _write_extension(lljson& exts, T* src, const char* member)
	{
		if (src->mPresent)
		{
			lljson v;
			if (write(*src, v))
			{
				exts[member] = v;
				return true;
			}
		}
		return false;
	}

	// Writes all extensions to dst.extensions
	// Usage:
	//	write_extensions(dst, mUnlit, "KHR_materials_unlit",
	//					 mPbrSpecularGlossiness,
	//					 "KHR_materials_pbrSpecularGlossiness");
	// Returns true if any of the extensions are written.
	template<class... Types>
	LL_INLINE bool write_extensions(lljson& dst, Types... args)
	{
		bool success = false;
		lljson exts;
		size_t count = sizeof...(args) - 1;
		for (size_t i = 0; i < count; i += 2)
		{
			if (_write_extension(exts, args...))
			{
				success = true;
			}
		}
		if (success)
		{
			dst["extensions"] = exts;
		}
		return success;
	}

	// Conditionally write a member to an object if the member is not the
	// default value
	template<typename T>
	LL_INLINE bool write(const T& src, const char* member, lljson& dst,
						 const T& default_value = T())
	{
		return src != default_value && write_always(src, member, dst);
	}

	template<typename T>
	LL_INLINE bool write(const fast_hmap<std::string, T>& src,
						 const char* member, lljson& dst,
						 const fast_hmap<std::string, T>& default_value =
							fast_hmap<std::string, T>())
	{
		if (src.empty())
		{
			return false;
		}
		lljson v;
		if (!write<T>(src, v))
		{
			return false;
		}
		dst[member] = v;
		return true;
	}

	template<typename T>
	LL_INLINE bool write(const std::vector<T>& src, const char* member,
						 lljson& dst,
						 const std::vector<T>& default_value =
							std::vector<T>())
	{
		if (src.empty())
		{
			return false;
		}
		lljson v;
		if (!write(src, v))
		{
			return false;
		}
		dst[member] = v;
		return true;
	}

	template<typename T>
	LL_INLINE bool copy(const lljson& src, const char* member, T& dst)
	{
		if (!src.is_object())
		{
			return false;
		}
		auto it = src.find(member);
		return it != src.end() && copy(it.value(), dst);
	}

	// Accessor::ComponentType
	template<>
	LL_INLINE bool copy(const lljson& src, Accessor::ComponentType& dst)
	{
		if (!src.is_number_integer())
		{
			return false;
		}
		dst = (Accessor::ComponentType)src.get<int>();
		return true;
	}

	template<>
	LL_INLINE bool write(const Accessor::ComponentType& src, lljson& dst)
	{
		dst = (S32)src;
		return true;
	}

	// Primitive::Mode
	template<>
	LL_INLINE bool copy(const lljson& src, Primitive::Mode& dst)
	{
		if (!src.is_number_integer())
		{
			return false;
		}
		dst = (Primitive::Mode)src.get<int>();
		return true;
	}

	template<>
	LL_INLINE bool write(const Primitive::Mode& src, lljson& dst)
	{
		dst = (S32)src;
		return true;
	}

	LL_NO_INLINE void json_error(const char* func_sig,
								 const lljson::exception& e);

	// GLM types
	template<>
	LL_INLINE bool copy(const lljson& src, glm::vec4& dst)
	{
		if (!src.is_array() || src.size() != 4)
		{
			return false;
		}
		try
		{
			dst.x = src[0].get<F32>();
			dst.y = src[1].get<F32>();
			dst.z = src[2].get<F32>();
			dst.w = src[3].get<F32>();
			return true;
		}
		catch (const lljson::exception& e)
		{
			json_error(LL_FUNC, e);
			return false;
		}
	}

	template<>
	LL_INLINE bool write(const glm::vec4& src, lljson& dst)
	{
		dst = lljson::array();
		dst.push_back(src.x);
		dst.push_back(src.y);
		dst.push_back(src.z);
		dst.push_back(src.w);
		return true;
	}

	template<>
	LL_INLINE bool copy(const lljson& src, glm::quat& dst)
	{
		if (!src.is_array() || src.size() != 4)
		{
			return false;
		}
		try
		{
			dst.x = src[0].get<F32>();
			dst.y = src[1].get<F32>();
			dst.z = src[2].get<F32>();
			dst.w = src[3].get<F32>();
			return true;
		}
		catch (const lljson::exception& e)
		{
			json_error(LL_FUNC, e);
			return false;
		}
	}

	template<>
	LL_INLINE bool write(const glm::quat& src, lljson& dst)
	{
		dst = lljson::array();
		dst.push_back(src.x);
		dst.push_back(src.y);
		dst.push_back(src.z);
		dst.push_back(src.w);
		return true;
	}

	template<>
	LL_INLINE bool copy(const lljson& src, glm::vec2& dst)
	{
		if (!src.is_array() || src.size() != 2)
		{
			return false;
		}
		try
		{
			dst.x = src[0].get<F32>();
			dst.y = src[1].get<F32>();
			return true;
		}
		catch (const lljson::exception& e)
		{
			json_error(LL_FUNC, e);
			return false;
		}
	}

	template<>
	LL_INLINE bool write(const glm::vec2& src, lljson& dst)
	{
		dst = lljson::array();
		dst.push_back(src.x);
		dst.push_back(src.y);
		return true;
	}

	template<>
	LL_INLINE bool write(const glm::vec3& src, lljson& dst)
	{
		dst = lljson::array();
		dst.push_back(src.x);
		dst.push_back(src.y);
		dst.push_back(src.z);
		return true;
	}

	template<>
	LL_INLINE bool copy(const lljson& src, glm::vec3& dst)
	{
		if (!src.is_array() || src.size() != 3)
		{
			return false;
		}
		try
		{
			auto vec = src.template get<std::vector<F32> >();
			dst.x = vec[0];
			dst.y = vec[1];
			dst.z = vec[2];
			return true;
		}
		catch (const lljson::exception& e)
		{
			json_error(LL_FUNC, e);
			return false;
		}
	}

	template<>
	LL_INLINE bool copy(const lljson& src, glm::mat4& dst)
	{
		if (!src.is_array() || src.size() != 16)
		{
			return false;
		}
		try
		{
			auto vec = src.template get<std::vector<F32> >();
			F32* p = glm::value_ptr(dst);
			for (U32 i = 0; i < 16; ++i)
			{
				p[i] = vec[i];
			}
			return true;
		}
		catch (const lljson::exception& e)
		{
			json_error(LL_FUNC, e);
			return false;
		}
	}

	template<>
	LL_INLINE bool write(const glm::mat4& src, lljson& dst)
	{
		const F32* p = glm::value_ptr(src);
		dst = lljson::array();
		for (U32 i = 0; i < 16; ++i)
		{
			dst.emplace_back(p[i]);
		}
		return true;
	}

	// bool
	template<>
	LL_INLINE bool copy(const lljson& src, bool& dst)
	{
		if (src.is_boolean())
		{
			dst = src.get<bool>();
			return true;
		}
		return false;
	}

	template<>
	LL_INLINE bool write(const bool& src, lljson& dst)
	{
		dst = src;
		return true;
	}

	// S32
	template<>
	LL_INLINE bool copy(const lljson& src, S32& dst)
	{
		if (src.is_number_integer())
		{
			dst = (S32)src.get<int>();
			return true;
		}
		return false;
	}

	template<>
	LL_INLINE bool write(const S32& src, lljson& dst)
	{
		dst = src;
		return true;
	}

	// U32
	template<>
	LL_INLINE bool copy(const lljson& src, U32& dst)
	{
		if (src.is_number_integer())
		{
			dst = (U32)src.get<int>();
			return true;
		}
		return false;
	}

	template<>
	LL_INLINE bool write(const U32& src, lljson& dst)
	{
		dst = src;
		return true;
	}

	// F32
	template<>
	LL_INLINE bool copy(const lljson& src, F32& dst)
	{
		try
		{
			F32 t = (F32)src.get<F32>();
			dst = t;
			return true;
		}
		catch (const lljson::exception& e)
		{
			json_error(LL_FUNC, e);
			return false;
		}
	}

	template<>
	LL_INLINE bool write(const F32& src, lljson& dst)
	{
		dst = src;
		return true;
	}

	// F64
	template<>
	LL_INLINE bool copy(const lljson& src, F64& dst)
	{
		try
		{
			F64 t = src.get<F32>();
			dst = t;
			return true;
		}
		catch (const lljson::exception& e)
		{
			json_error(LL_FUNC, e);
			return false;
		}
	}

	template<>
	LL_INLINE bool write(const F64& src, lljson& dst)
	{
		dst = src;
		return true;
	}

	// std::string
	template<>
	LL_INLINE bool copy(const lljson& src, std::string& dst)
	{
		if (src.is_string())
		{
			dst = src.get_string().c_str();
			return true;
		}
		return false;
	}

	template<>
	LL_INLINE bool write(const std::string& src, lljson& dst)
	{
		dst = src;
		return true;
	}

	// Accessor::Type
	template<>
	LL_INLINE bool copy(const lljson& src, Accessor::Type& dst)
	{
		if (src.is_string())
		{
			dst = gltf_type_to_enum(src.get_string().c_str());
			return true;
		}
		return false;
	}

	template<>
	LL_INLINE bool write(const Accessor::Type& src, lljson& dst)
	{
		dst = enum_to_gltf_type(src);
		return true;
	}

	// Material::AlphaMode
	template<>
	LL_INLINE bool copy(const lljson& src, Material::AlphaMode& dst)
	{
		if (src.is_string())
		{
			dst = gltf_alpha_mode_to_enum(src.get_string());
			return true;
		}
		return false;
	}

	template<>
	LL_INLINE bool write(const Material::AlphaMode& src, lljson& dst)
	{
		dst = enum_to_gltf_alpha_mode(src);
		return true;
	}
}

#if LL_CLANG
# pragma clang diagnostic pop
#endif

// Do not pollute code with this define past this header. HB
#undef get_string
