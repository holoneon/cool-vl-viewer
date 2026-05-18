/**
 * @file lldrawpoolsky.h
 * @brief LLDrawPoolSky class definition
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

class LLSkyTex;

class LLDrawPoolSky final : public LLFacePool
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0
	};

	LL_INLINE U32 getVertexDataMask() override
	{
		return VERTEX_DATA_MASK;
	}

	LLDrawPoolSky();

	void prerender() override;

	// All the render methods are no-ops with the PBR renderer.

	void render(S32 pass = 0) override;
	LL_INLINE void endRenderPass(S32) override			{}

	// This will return 1 in EE mode and 0 in PBR mode since it indirectly
	// calls LLDrawpool::getNumPasses().
	LL_INLINE S32 getNumPostDeferredPasses() override	{ return getNumPasses(); }
	LL_INLINE void endPostDeferredPass(S32 p) override	{ endRenderPass(p); }
	LL_INLINE void renderPostDeferred(S32 p) override	{ render(p); }

	LL_INLINE void setSkyTex(LLSkyTex* st)				{ mSkyTex = st; }

private:
	void renderSkyFace(U8 side);

private:
	LLSkyTex* mSkyTex;
};
