/**
 * @file lldrawpoolwater.h
 * @brief LLDrawPoolWater class definition
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

class LLFace;
class LLGLSLShader;
class LLHeavenBody;
class LLSettingsWater;
class LLWaterSurface;

class LLDrawPoolWater final : public LLFacePool
{
	friend class LLDrawPoolWaterExclusion;

public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0
	};

	LL_INLINE U32 getVertexDataMask() override
	{
		return VERTEX_DATA_MASK;
	}

	LLDrawPoolWater();

	void prerender() override;

	S32 getNumPasses() override;				// Returns 0 in PBR mode
	// The three following methods are only for use by the EE renderer.
	void render(S32 pass = 0) override;
	S32 getNumDeferredPasses() override;
	void renderDeferred(S32 pass = 0) override;

	// The two following methods are only for use by the PBR renderer.
	S32 getNumPostDeferredPasses() override;
	void renderPostDeferred(S32) override;

	void setOpaqueTexture(const LLUUID& tex_id);
	void setTransparentTextures(const LLUUID& tex1_id,
								const LLUUID& tex2_id = LLUUID::null);
	void setNormalMaps(const LLUUID& tex1_id,
					   const LLUUID& tex2_id = LLUUID::null);

	LL_INLINE static void restoreGL()
	{
		sNeedsReflectionUpdate = sNeedsTexturesReload = true;
	}

	// Only for use by the PBR renderer
	void renderSSR();

private:
	S32 getWaterPasses();
	// Methods for use by the EE renderer only
	void renderReflection(LLFace* facep);
	void renderWater();
	void renderOpaqueLegacyWater();
	void shadeWater(const LLSettingsWater* waterp, LLGLSLShader* shaderp,
					bool edge);
	// Only for use by the PBR renderer
	void shadeWaterPBR(const LLSettingsWater* waterp, LLGLSLShader* shaderp,
					   bool edge);
	void pushWaterPlanes(bool edge);

private:
	LLPointer<LLViewerTexture>	mWaterImagep[2];
	LLPointer<LLViewerTexture>	mWaterNormp[2];
	LLPointer<LLViewerTexture>	mOpaqueWaterImagep;
	LLVector3					mLightDir;
	LLColor4					mLightColor;
	LLColor3					mLightDiffuse;

public:
	static LLColor4				sWaterFogColor;
	static bool					sNeedsReflectionUpdate;
	static bool					sNeedsTexturesReload;
};

// Only for use by the PBR renderer
class LLDrawPoolWaterExclusion final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX
	};

	LL_INLINE U32 getVertexDataMask() override
	{
		return VERTEX_DATA_MASK;
	}

	LLDrawPoolWaterExclusion();

	LL_INLINE S32 getNumPasses() override		{ return 1; }

	void render(S32 pass = 0) override;
};
