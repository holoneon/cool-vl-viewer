/** 
 * @file lldrawpoolwlsky.h
 * @brief LLDrawPoolWLSky class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llsettingssky.h"

#include "lldrawpool.h"

class LLGLSLShader;
class LLImageGL;

class LLDrawPoolWLSky final : public LLDrawPool
{
protected:
	LOG_CLASS(LLDrawPoolWLSky);

public:
	static constexpr U32 SKY_VERTEX_DATA_MASK =
		LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0;
	static constexpr U32 STAR_VERTEX_DATA_MASK =
		LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR |
		LLVertexBuffer::MAP_TEXCOORD0;
	static constexpr U32 ADV_ATMO_SKY_VERTEX_DATA_MASK =
		LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0;

	LLDrawPoolWLSky();

	LL_INLINE U32 getVertexDataMask() override		{ return SKY_VERTEX_DATA_MASK; }

	LL_INLINE bool isDead() override				{ return false; }

	void prerender() override;

	// For EE rendering only
	void beginRenderPass(S32) override;
	void endRenderPass(S32) override;
	void render(S32 pass = 0) override;

	LL_INLINE S32 getNumDeferredPasses() override	{ return 1; }
	void beginDeferredPass(S32) override;
	void endDeferredPass(S32) override;
	void renderDeferred(S32 pass) override;

	// Verify that all data in the draw pool is correct
	LL_INLINE bool verify() const override			{ return true; }

	LL_INLINE bool isFacePool() override			{ return false; }

	LL_INLINE static void cleanupGL()				{}
	static void restoreGL();

	// Returns an empty string on success, or an error message otherwise. HB
	static std::string loadHDRISky(const std::string& filename);
	// Returns a pointer on the HDRI sky image, or NULL when there is none. HB
	static LLImageGL* getHDRISky();
	static void resetHDRISky();
	// For speed, we use the cached result for this set or render passes. HB
	LL_INLINE static bool useHDRI()					{ return sUseHDRI; }

private:
	void renderDome(LLGLSLShader* shaderp) const;

	void renderSkyHaze() const;
	// NOTE: LL's EEP viewer also got a renderSkyCloudsDeferred() method , but
	// it is exactly identical to their renderSkyClouds() method.
	void renderSkyClouds() const;
	void renderStars() const;
	void renderHeavenlyBodies();

	// Extended environment specific methods
	void renderSkyHazeDeferred() const;
	void renderStarsDeferred() const;

private:
	LLSettingsSky::ptr_t				mCurrentSky;
	LLVector3							mCameraOrigin;
	F32									mCamHeightLocal;

	static LLGLSLShader*				sCloudShader;
	static LLGLSLShader*				sSkyShader;
	static LLGLSLShader*				sSunShader;
	static LLGLSLShader*				sMoonShader;
	static LLPointer<LLViewerTexture> 	sCloudNoiseTexture;
	static LLPointer<LLImageRaw>		sCloudNoiseRawImage;
	static LLPointer<LLImageGL> 		sEXRImage;
	static bool							sUseHDRI;
};
