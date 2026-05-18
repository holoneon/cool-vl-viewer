/**
 * @file lldrawpoolalpha.h
 * @brief LLDrawPoolAlpha class definition
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
#include "llframetimer.h"
#include "llrender.h"

class LLFace;
class LLColor4;
class LLGLSLShader;
class LLTexUnit;

class LLDrawPoolAlpha final : public LLRenderPass
{
protected:
	LOG_CLASS(LLDrawPoolAlpha);

public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_COLOR |
							LLVertexBuffer::MAP_TEXCOORD0
	};
	LL_INLINE U32 getVertexDataMask() override			{ return VERTEX_DATA_MASK; }

	LLDrawPoolAlpha(U32 type);

	LL_INLINE S32 getNumPostDeferredPasses() override	{ return 1; }
	void renderPostDeferred(S32 pass) override;

	// This method is only for EE rendering
	void render(S32 pass = 0) override;

	void prerender() override;

	void forwardRender(bool write_depth = false);

private:
	// PBR variants
	void renderPostDeferredPBR(S32 pass);

	void renderDebugAlpha();

	void renderAlpha(U32 mask, bool depth_only = false, bool rigged = false);
	// Note: 'mask' is not used/ignored for the PBR rendering mode
	void renderAlphaHighlight(U32 mask = 0);

	typedef std::vector<LLDrawInfo*> drawinfo_vec_t;
	void renderEmissives(U32 mask, const drawinfo_vec_t& emissives);
	void renderRiggedEmissives(U32 mask, const drawinfo_vec_t& emissives);
	void renderPbrEmissives(const drawinfo_vec_t& emissives);
	void renderRiggedPbrEmissives(const drawinfo_vec_t& emissives);

	bool texSetup(LLDrawInfo* infop, bool use_material, LLTexUnit* unitp);

public:
	static bool			sShowDebugAlpha;

private:
	LLGLSLShader*		mTargetShader;
	LLGLSLShader*		mSimpleShader;
	LLGLSLShader*		mFullbrightShader;
	LLGLSLShader*		mEmissiveShader;
	LLGLSLShader*		mPBRShader;
	LLGLSLShader*		mPBREmissiveShader;

	// Our 'normal' alpha blend function for this pass
	U32					mColorSFactor;
	U32					mColorDFactor;
	U32					mAlphaSFactor;
	U32					mAlphaDFactor;
};
