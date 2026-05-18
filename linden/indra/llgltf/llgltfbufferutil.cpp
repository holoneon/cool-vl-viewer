/**
 * @file llgltfbufferutil.h
 * @brief LLGLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024-2025, Henri Beauchamp.
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

#include "llgltfbufferutil.h"

namespace LLGLTF
{
	LL_NO_INLINE void not_implemented(const char* func_sig)
	{
		// This may indeed happen: do not crash a release viewer, just warn. HB
		llwarns << "TODO: implement " << func_sig << llendl;
		llassert(false);
	}

	LL_NO_INLINE void unsupported_accessor_type(const char* func_sig, U8 type)
	{
		llerrs << func_sig << ": Unsupported accessor type: " << type
			   << llendl;
	}

	LL_NO_INLINE void invalid_component_type(const char* func_sig, U8 type)
	{
		llerrs << func_sig << ": Invalid component type: " << type << llendl;
	}

	LL_NO_INLINE void json_error(const char* func_sig,
								 const lljson::exception& e)
	{
		llwarns << func_sig << " encountered nlohmann::json error: "
				<< e.what() << llendl;
	}

	LL_NO_INLINE void invalid_buffer(const char* func_sig)
	{
		llwarns << func_sig << " was passed an invalid buffer." << llendl;
	}

	LL_NO_INLINE void invalid_buffer_view(const char* func_sig)
	{
		llwarns << func_sig << " was passed an invalid buffer view." << llendl;
	}
}
