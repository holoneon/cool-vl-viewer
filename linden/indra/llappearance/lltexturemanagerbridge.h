/** 
 * @file lltexturemanagerbridge.h
 * @brief Bridge to an application-specific texture manager.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Linden Research, Inc.
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

#include "llavatarappearancedefines.h"
#include "llpointer.h"
#include "llgltexture.h"

// Abstract bridge interface
class LLTextureManagerBridge
{
public:
	virtual ~LLTextureManagerBridge() = default;

	virtual LLPointer<LLGLTexture> getLocalTexture(bool usemipmaps = true,
												   bool generate_gl_tex = true) = 0;
	virtual LLPointer<LLGLTexture> getLocalTexture(U32 width, U32 height,
												   U8 components,
												   bool usemipmaps,
												   bool generate_gl_tex = true) = 0;
	virtual LLGLTexture* getFetchedTexture(const LLUUID& image_id) = 0;
};

extern LLTextureManagerBridge* gTextureManagerBridgep;
