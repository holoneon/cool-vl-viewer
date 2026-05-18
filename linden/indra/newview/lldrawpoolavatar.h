/**
 * @file lldrawpoolavatar.h
 * @brief LLDrawPoolAvatar class definition
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

class LLGLSLShader;
class LLFace;
class LLVolume;
class LLVolumeFace;
class LLVOVolume;

class LLDrawPoolAvatar final : public LLFacePool
{
public:
	LLDrawPoolAvatar(U32 type);
	~LLDrawPoolAvatar() override;

	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_WEIGHT |
							LLVertexBuffer::MAP_CLOTHWEIGHT
	};

	typedef enum
	{
		SHADOW_PASS_AVATAR_OPAQUE,
		SHADOW_PASS_AVATAR_ALPHA_BLEND,
		SHADOW_PASS_AVATAR_ALPHA_MASK,
		NUM_SHADOW_PASSES
	} eShadowPass;

	U32 getVertexDataMask() override					{ return VERTEX_DATA_MASK; }

	static LLMatrix4& getModelView();

	LL_INLINE S32 getNumPasses() override				{ return 3; }
	void beginRenderPass(S32 pass) override;
	void endRenderPass(S32 pass) override;
	void prerender() override;
	void render(S32 pass = 0) override;

	LL_INLINE S32 getNumDeferredPasses() override		{ return 3; }
	void beginDeferredPass(S32 pass) override;
	void endDeferredPass(S32 pass) override;
	LL_INLINE void renderDeferred(S32 pass) override	{ render(pass); }

	LL_INLINE S32 getNumPostDeferredPasses() override	{ return 1; }
	void beginPostDeferredPass(S32 pass) override;
	void endPostDeferredPass(S32 pass) override;
	void renderPostDeferred(S32 pass) override;

	LL_INLINE S32 getNumShadowPasses() override			{ return NUM_SHADOW_PASSES; }
	void beginShadowPass(S32 pass) override;
	void endShadowPass(S32 pass) override;
	void renderShadow(S32 pass) override;

	static void beginRigid();
	static void beginImpostor();
	static void beginSkinned();

	static void endRigid();
	static void endImpostor();
	static void endSkinned();

	static void beginDeferredImpostor();
	static void beginDeferredRigid();
	static void beginDeferredSkinned();

	static void endDeferredImpostor();
	static void endDeferredRigid();
	static void endDeferredSkinned();

	// Renders only one avatar if single_avatar is not null.
	void renderAvatars(LLVOAvatar* single_avatar, S32 pass = -1);

public:
	static F32				sMinimumAlpha;
	static S32				sDiffuseChannel;
	static S32				sShadowPass;
	static bool				sSkipOpaque;
	static bool				sSkipTransparent;

	static LLGLSLShader*	sVertexProgram;
};
