/** 
 * @file llpreprocessor.h
 * @brief This file should be included in all Linden Lab files and
 * should only contain special preprocessor directives
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

#pragma once

// Figure out endianness of platform
#ifdef LL_LINUX
# define __ENABLE_WSTRING
# include <endian.h>
#endif	//	LL_LINUX

#if (defined(LL_WINDOWS) || (defined(LL_LINUX) && (__BYTE_ORDER == __LITTLE_ENDIAN)))
# define LL_BIG_ENDIAN 0
#else
# define LL_BIG_ENDIAN 1
#endif

// Mark-up expressions with branch prediction hints. Do NOT use this with
// reckless abandon: it is an obfuscating micro-optimization outside of inner
// loops or other places where you are OVERWHELMINGLY sure which way an
// expression almost-always evaluates.
#if __GNUC__
# define LL_LIKELY(EXPR) __builtin_expect ((bool)(EXPR), true)
# define LL_UNLIKELY(EXPR) __builtin_expect ((bool)(EXPR), false)
#else
# define LL_LIKELY(EXPR) (EXPR)
# define LL_UNLIKELY(EXPR) (EXPR)
#endif

// Figure out differences between compilers
#if defined(__clang__)
	#ifndef LL_CLANG
		#define LL_CLANG 1
	#endif
	#ifndef CLANG_VERSION
		#define CLANG_VERSION (__clang_major__ * 10000 + \
							   __clang_minor__ * 100 + \
							   __clang_patchlevel__)
	#endif
#elif defined(__GNUC__)
	#ifndef LL_GNUC
		#define LL_GNUC 1
	#endif
	#ifndef GCC_VERSION
		#define GCC_VERSION (__GNUC__ * 10000 + \
							 __GNUC_MINOR__ * 100 + \
							 __GNUC_PATCHLEVEL__)
	#endif
#elif defined(__MSVC_VER__) || defined(_MSC_VER)
	#ifndef LL_MSVC
		#define LL_MSVC 1
	#endif
#endif

// No force-inlining for debug builds (letting the compiler options decide)
#if LL_DEBUG || LL_NO_FORCE_INLINE
# define LL_INLINE inline
#elif LL_GNUC || LL_CLANG
# define LL_INLINE inline __attribute__((always_inline))
#elif LL_MSVC
# define LL_INLINE __forceinline
#else
# define LL_INLINE inline
#endif

#if LL_GNUC || LL_CLANG
# define LL_NO_INLINE __attribute__((noinline))
#elif LL_MSVC
# define LL_NO_INLINE __declspec(noinline)
#else
# define LL_NO_INLINE
#endif

#if !defined(__x86_64__) && !(LL_MSVC && _M_X64) && !SSE2NEON
# error "Only 64 builds are now supported, sorry !"
#endif

// Handy function name macro for std::cerr messages, for when we cannot use
// llinfos & Co (e.g. in plugins).
#ifdef LL_MSVC
# define LL_FUNC __FUNCSIG__ 
#else
# define LL_FUNC __PRETTY_FUNCTION__
#endif

// Deal with minor differences between OSes.
#if LL_LINUX
// Different name, same functionality.
# define stricmp strcasecmp
# define strnicmp strncasecmp

// Not sure why this is different, but...
# ifndef MAX_PATH
#  define MAX_PATH PATH_MAX
# endif
#endif

// Deal with Visual Studio problems
#if LL_MSVC

// Warning: deprecated
# pragma warning(disable : 4996)

// Conditional expression is constant (e.g. while(1) )
# pragma warning(disable : 4127)

// Possible loss of data on conversions
# pragma warning(disable : 4244)

// Conversion from 'type1' to 'type2' of greater size
# pragma warning(disable : 4312)

// The inline specifier cannot be used when a friend declaration refers to a
// specialization of a function template
# pragma warning(disable : 4396)

// Assignment operator could not be generated
# pragma warning(disable : 4512)

// Assignment within conditional (even if((x = y)) )
# pragma warning(disable : 4706)

// Member needs to have dll-interface to be used by clients of class
# pragma warning(disable : 4251)

// Non dll-interface class used as base for dll-interface class
# pragma warning(disable : 4275)

// 'var': conversion from 'size_t' to 'type', possible loss of data)
# pragma warning(disable : 4267)

#endif	//	LL_MSVC
