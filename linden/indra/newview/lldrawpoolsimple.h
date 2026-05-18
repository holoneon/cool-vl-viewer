/**
 * @file lldrawpoolsimple.h
 * @brief LLDrawPoolSimple class definition
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

class LLDrawPoolSimple final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};

	LL_INLINE U32 getVertexDataMask() override			{ return VERTEX_DATA_MASK; }

	LLDrawPoolSimple();

	LL_INLINE S32 getNumDeferredPasses() override		{ return 1; }
	void renderDeferred(S32 pass) override;

	// Only for use with the EE renderer

	void prerender() override;
	void render(S32 pass = 0) override;
};

class LLDrawPoolGrass final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};

	LL_INLINE U32 getVertexDataMask() override			{ return VERTEX_DATA_MASK; }

	LLDrawPoolGrass();

	LL_INLINE S32 getNumDeferredPasses() override		{ return 1; }
	void renderDeferred(S32 pass) override;

	// Only for use with the EE renderer

	void beginRenderPass(S32 pass) override;
	void endRenderPass(S32 pass) override;

	void prerender() override;
	void render(S32 pass = 0) override;
};

class LLDrawPoolAlphaMask final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};

	LL_INLINE U32 getVertexDataMask() override			{ return VERTEX_DATA_MASK; }

	LLDrawPoolAlphaMask();

	LL_INLINE S32 getNumDeferredPasses() override		{ return 1; }
	void renderDeferred(S32 pass) override;

	// Only for use with the EE renderer

	void prerender() override;
	void render(S32 pass = 0) override;
};

class LLDrawPoolFullbrightAlphaMask final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};

	LL_INLINE U32 getVertexDataMask() override			{ return VERTEX_DATA_MASK; }

	LLDrawPoolFullbrightAlphaMask();

	LL_INLINE S32 getNumPostDeferredPasses() override	{ return 1; }
	void renderPostDeferred(S32 pass) override;

	// Only for use with the EE renderer

	void prerender() override;
	void render(S32 pass = 0) override;
};

class LLDrawPoolFullbright final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};

	LL_INLINE U32 getVertexDataMask() override			{ return VERTEX_DATA_MASK; }

	LLDrawPoolFullbright();

	LL_INLINE S32 getNumPostDeferredPasses() override	{ return 1; }
	void renderPostDeferred(S32 pass) override;

	// Only for use with the EE renderer

	void prerender() override;
	void render(S32 pass = 0) override;
};

class LLDrawPoolGlow final : public LLRenderPass
{
public:
	LLDrawPoolGlow()
	:	LLRenderPass(LLDrawPool::POOL_GLOW)
	{
	}

	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_EMISSIVE
	};

	LL_INLINE U32 getVertexDataMask() override			{ return VERTEX_DATA_MASK; }

	LL_INLINE S32 getNumPostDeferredPasses() override	{ return 1; }
	void renderPostDeferred(S32 pass) override;

	// Only for use with the EE renderer
	void render(S32 pass = 0) override;

private:
	void render(LLGLSLShader* shaderp);
};
