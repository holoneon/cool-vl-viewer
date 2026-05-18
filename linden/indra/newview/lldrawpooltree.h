/**
 * @file lldrawpooltree.h
 * @brief LLDrawPoolTree class definition
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

#include "lldrawpool.h"

class LLDrawPoolTree final : public LLFacePool
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_COLOR |
							LLVertexBuffer::MAP_TEXCOORD0
	};

	LL_INLINE U32 getVertexDataMask() override
	{
		return VERTEX_DATA_MASK;
	}

	LLDrawPoolTree(LLViewerTexture* texturep);

	void prerender() override;

	// These three methods are for EE rendering only
	void beginRenderPass(S32) override;
	void endRenderPass(S32) override;
	LL_INLINE void render(S32 pass = 0) override		{ renderDeferred(pass); }

	LL_INLINE S32 getNumDeferredPasses() override		{ return 1; }
	void beginDeferredPass(S32) override;
	void endDeferredPass(S32) override;
	void renderDeferred(S32 pass) override;

	LL_INLINE S32 getNumShadowPasses() override			{ return 1; }
	void beginShadowPass(S32 pass) override;
	void endShadowPass(S32 pass) override;
	LL_INLINE void renderShadow(S32 pass) override		{ render(pass); }

	LL_INLINE bool verify() const override				{ return true; }

	LL_INLINE LLViewerTexture* getTexture() override	{ return mTexturep; }

private:
	void renderTree();

private:
	LLPointer<LLViewerTexture>	mTexturep;
};
