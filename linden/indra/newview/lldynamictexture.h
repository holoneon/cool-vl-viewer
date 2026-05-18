/**
 * @file lldynamictexture.h
 * @brief Implementation of LLDynamicTexture class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llcoord.h"

#include "llviewertexture.h"

class LLViewerDynamicTexture : public LLViewerTexture
{
public:
	enum
	{
		LL_VIEWER_DYNAMIC_TEXTURE = LLViewerTexture::DYNAMIC_TEXTURE,
		LL_TEX_LAYER_SET_BUFFER = LLViewerTexture::INVALID_TEXTURE_TYPE + 1,
		LL_VISUAL_PARAM_HINT,
		LL_VISUAL_PARAM_RESET,
		LL_PREVIEW_ANIMATION,
		LL_IMAGE_PREVIEW_SCULPTED,
		LL_IMAGE_PREVIEW_AVATAR,
		INVALID_DYNAMIC_TEXTURE
	};

protected:
	~LLViewerDynamicTexture() override;

public:
	enum EOrder
	{
		ORDER_FIRST = 0,	// Used only by GLTF material previews in PBR mode
		ORDER_MIDDLE,		// Used by various UI textures and texlayer hints
		ORDER_LAST,			// Used only by avatar tex layers
		ORDER_RESET,		// Resets appearance parameters and does not render
		ORDER_COUNT
	};

	LLViewerDynamicTexture(S32 width, S32 height,
					 	   S32 components,		// = 4,
					 	   EOrder order,		// = ORDER_MIDDLE,
					 	   bool clamp);

	S8 getType() const override;

	LL_INLINE S32 getOriginX() const			{ return mOrigin.mX; }
	LL_INLINE S32 getOriginY() const			{ return mOrigin.mY; }

	LL_INLINE S32 getSize()						{ return mFullWidth * mFullHeight * mComponents; }

	LL_INLINE virtual bool needsRender()		{ return true; }
	virtual void preRender(bool clear_depth = true);
	virtual bool render()						{ return false; }
	virtual void postRender(bool success);

	static bool	updateAllInstances();

protected:
	void generateGLTexture();
	void generateGLTexture(S32 internal_format, U32 primary_format,
						   U32 type_format, bool swap_bytes = false);

protected:
	alignas(16) LLCamera	mCamera;

	LLCoordGL				mOrigin;

	bool					mClamp;

	typedef std::set<LLViewerDynamicTexture*> instance_list_t;
	static instance_list_t	sInstances[ORDER_COUNT];

	static S32				sNumRenders;
};
