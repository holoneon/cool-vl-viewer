/**
 * @file lldrawpoolterrain.h
 * @brief LLDrawPoolTerrain class definition
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

class LLViewerRegion;

class LLDrawPoolTerrain final : public LLFacePool
{
public:
	LLDrawPoolTerrain(LLViewerTexture* texp);


	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_TEXCOORD1 |
							LLVertexBuffer::MAP_TEXCOORD2 |
							LLVertexBuffer::MAP_TEXCOORD3,

		VERTEX_DATA_MASK_PBR =	LLVertexBuffer::MAP_VERTEX |
								LLVertexBuffer::MAP_NORMAL |
								LLVertexBuffer::MAP_TANGENT |	// PBR terrain
								LLVertexBuffer::MAP_TEXCOORD0 |
								LLVertexBuffer::MAP_TEXCOORD1 |
								LLVertexBuffer::MAP_TEXCOORD2 |
								LLVertexBuffer::MAP_TEXCOORD3
	};

	U32 getVertexDataMask() override;

	LL_INLINE S32 getNumDeferredPasses() override	{ return 1; }
	void beginDeferredPass(S32) override;
	void endDeferredPass(S32 pass) override;
	void renderDeferred(S32 pass) override;

	LL_INLINE S32 getNumShadowPasses() override		{ return 1; }
	void beginShadowPass(S32) override;
	void endShadowPass(S32 pass) override;
	void renderShadow(S32 pass) override;

	void prerender() override;

	// These three methods are used for EE rendering only
	void render(S32 pass = 0) override;
	void beginRenderPass(S32) override;
	void endRenderPass(S32 pass) override;

	// Only terrain pool got a need for a dirtyTextures() method. HB
	LL_INLINE bool isTerrainPool() override			{ return true; }
	void dirtyTextures(const LLViewerTextureList::dirty_list_t& tex);

	LL_INLINE LLViewerTexture* getTexture() override
	{
		return mTexturep;
	}

#if 0
	// Failed attempt at properly restoring terrain after GL restart with core
	// GL profile enabled. HB
	void rebuildPatches();
#endif

protected:
	void renderSimple();
	void renderOwnership();
	void hilightParcelOwners();
	void renderFull2TU();
	void renderFull4TU();
	void renderFullShader();
	void renderFullShaderTextures(LLViewerRegion* regionp);
	void renderFullShaderPBR(LLViewerRegion* regionp);
	void drawLoop();
	void boostTerrainDetailTextures();

public:
	LLPointer<LLViewerTexture>	mTexturep;
	LLPointer<LLViewerTexture>	mAlphaRampImagep;
	LLPointer<LLViewerTexture>	m2DAlphaRampImagep;
	LLPointer<LLViewerTexture>	mAlphaNoiseImagep;

private:
	static F32					sDetailScale;
};
