/**
 * @file lldrawpoolmaterials.h
 * @brief LLDrawPoolMaterials and LLDrawPoolMatPBR class definitions
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012-2022, Linden Research, Inc.
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

#include "llvector2.h"
#include "llvector3.h"
#include "llcolor4u.h"

#include "lldrawpool.h"

class LLViewerTexture;
class LLDrawInfo;
class LLGLSLShader;

///////////////////////////////////////////////////////////////////////////////
// LLDrawPoolMaterials class
///////////////////////////////////////////////////////////////////////////////

class LLDrawPoolMaterials final : public LLRenderPass
{
public:
	LLDrawPoolMaterials();

	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_TEXCOORD1 |
							LLVertexBuffer::MAP_TEXCOORD2 |
							LLVertexBuffer::MAP_COLOR |
							LLVertexBuffer::MAP_TANGENT
	};

	LL_INLINE U32 getVertexDataMask() override		{ return VERTEX_DATA_MASK; }

	void prerender() override;

	// Not used by the EE forward renderer.
	LL_INLINE S32 getNumPasses() override			{ return 0; }

	// 12 render passes times 2 (one for each rigged and non rigged)
	LL_INLINE S32 getNumDeferredPasses() override	{ return 24; }
	void beginDeferredPass(S32 pass) override;
	void endDeferredPass(S32 pass) override;
	void renderDeferred(S32 pass) override;

	// The following methods are for EE rendering only

	void bindSpecularMap(LLViewerTexture* texp);
	void bindNormalMap(LLViewerTexture* texp);

private:
	// For EE rendering only
	void pushMaterialsBatch(LLDrawInfo& params, U32 mask);

	// For PBR rendering only
	void renderDeferredPBR(S32 pass);

private:
	LLGLSLShader* mShader;
};

///////////////////////////////////////////////////////////////////////////////
// LLDrawPoolMatPBR class
//
// In LL's original code, this class is named LLDrawPoolGLTFPBR and held in a
// separate lldrawpoolpbropaque.h/cpp module. I renamed it for consistency and
// moved it here, where it logically belongs to, since it is used to render PBR
// *materials*. HB
///////////////////////////////////////////////////////////////////////////////

class LLDrawPoolMatPBR final : public LLRenderPass
{
public:
	LLDrawPoolMatPBR(U32 type);

	// This value returned by this method is ignored by the PBR renderer.
	LL_INLINE U32 getVertexDataMask() override		{ return 0; }

	// Not used by the EE forward renderer. HB
	LL_INLINE S32 getNumPasses() override			{ return 0; }

	// Returns 0 in EE rendering mode, or 1 in PBR mode. HB
	S32 getNumDeferredPasses() override;
	void renderDeferred(S32 pass) override;

	LL_INLINE S32 getNumPostDeferredPasses() override
	{
		return getNumDeferredPasses();
	}
	void renderPostDeferred(S32 pass) override;

public:
	U32 mRenderType;
};
