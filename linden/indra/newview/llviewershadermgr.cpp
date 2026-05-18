/**
 * @file llviewershadermgr.cpp
 * @brief Viewer shader manager implementation.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 *
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#if LL_LINUX
# include <stdlib.h>			// For getenv()
#endif

#include "llviewershadermgr.h"

#include "lldir.h"
#include "llfeaturemanager.h"
#include "lljoint.h"
#include "llrender.h"
#include "hbtracy.h"

#include "llenvironment.h"
#include "llpipeline.h"
#include "llsky.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llworld.h"

static LLStaticHashedString sTexture0("texture0");
static LLStaticHashedString sTexture1("texture1");
static LLStaticHashedString sTex0("tex0");
static LLStaticHashedString sTex1("tex1");
static LLStaticHashedString sTex2("tex2");
static LLStaticHashedString sGlowMap("glowMap");
static LLStaticHashedString sScreenMap("screenMap");

LLViewerShaderMgr* gViewerShaderMgrp;

bool LLViewerShaderMgr::sInitialized = false;
bool LLViewerShaderMgr::sSkipReload = false;
bool LLViewerShaderMgr::sHasFXAA = false;
bool LLViewerShaderMgr::sHasSMAA = false;
bool LLViewerShaderMgr::sHasCAS = false;
bool LLViewerShaderMgr::sHasLut = false;
bool LLViewerShaderMgr::sHasIrrandiance = false;
bool LLViewerShaderMgr::sCanRenderGLTFScene = false;
bool LLViewerShaderMgr::sCanDoRP = false;
bool LLViewerShaderMgr::sHasRP = false;

LLVector4 gShinyOrigin;

// Utility shaders

LLGLSLShader gOcclusionProgram;
LLGLSLShader gSkinnedOcclusionProgram;
LLGLSLShader gOcclusionCubeProgram;
LLGLSLShader gGlowCombineProgram;
LLGLSLShader gReflectionMipProgram;								// PBR only
LLGLSLShader gGaussianProgram;									// PBR only
LLGLSLShader gRadianceGenProgram;								// PBR only
LLGLSLShader gHeroRadianceGenProgram;							// PBR only
LLGLSLShader gIrradianceGenProgram;								// PBR only
LLGLSLShader gSplatTextureRectProgram;							// EE/WL only
LLGLSLShader gGlowCombineFXAAProgram;
LLGLSLShader gOneTextureNoColorProgram;							// EE/WL only
LLGLSLShader gDebugProgram;
LLGLSLShader gSkinnedDebugProgram;
LLGLSLShader gNormalDebugProgram[NORMAL_DEBUG_SHADER_COUNT];	// PBR only
LLGLSLShader gSkinnedNormalDebugProgram[NORMAL_DEBUG_SHADER_COUNT]; // PBR only
LLGLSLShader gClipProgram;
LLGLSLShader gDownsampleDepthProgram;							// EE/WL only
LLGLSLShader gDownsampleDepthRectProgram;						// EE/WL only
LLGLSLShader gAlphaMaskProgram;
LLGLSLShader gBenchmarkProgram;
LLGLSLShader gReflectionProbeDisplayProgram;					// PBR only
LLGLSLShader gCopyProgram;										// PBR only
LLGLSLShader gCopyDepthProgram;									// PBR only
LLGLSLShader gHiZReduceProgram;									// PBR only

// Object shaders

LLGLSLShader gObjectSimpleProgram;								// EE/WL only
LLGLSLShader gSkinnedObjectSimpleProgram;						// EE/WL only
LLGLSLShader gObjectSimpleImpostorProgram;						// EE/WL only
LLGLSLShader gSkinnedObjectSimpleImpostorProgram;				// EE/WL only
LLGLSLShader gObjectPreviewProgram;
LLGLSLShader gSkinnedObjectPreviewProgram;						// PBR only
LLGLSLShader gPhysicsPreviewProgram;							// PBR only
LLGLSLShader gObjectSimpleAlphaMaskProgram;						// EE/WL only
LLGLSLShader gSkinnedObjectSimpleAlphaMaskProgram;				// EE/WL only
LLGLSLShader gObjectSimpleWaterProgram;							// EE/WL only
LLGLSLShader gSkinnedObjectSimpleWaterProgram;					// EE/WL only
LLGLSLShader gObjectSimpleWaterAlphaMaskProgram;				// EE/WL only
LLGLSLShader gSkinnedObjectSimpleWaterAlphaMaskProgram;			// EE/WL only
LLGLSLShader gObjectFullbrightProgram;							// EE/WL only
LLGLSLShader gSkinnedObjectFullbrightProgram;					// EE/WL only
LLGLSLShader gObjectFullbrightWaterProgram;						// EE/WL only
LLGLSLShader gSkinnedObjectFullbrightWaterProgram;				// EE/WL only
LLGLSLShader gObjectEmissiveProgram;							// EE/WL only
LLGLSLShader gSkinnedObjectEmissiveProgram;						// EE/WL only
LLGLSLShader gObjectEmissiveWaterProgram;						// EE/WL only
LLGLSLShader gSkinnedObjectEmissiveWaterProgram;				// EE/WL only
LLGLSLShader gObjectFullbrightAlphaMaskProgram;					// EE/WL only
LLGLSLShader gSkinnedObjectFullbrightAlphaMaskProgram;			// EE/WL only
LLGLSLShader gObjectFullbrightWaterAlphaMaskProgram;			// EE/WL only
LLGLSLShader gSkinnedObjectFullbrightWaterAlphaMaskProgram;		// EE/WL only
LLGLSLShader gObjectFullbrightShinyProgram;						// EE/WL only
LLGLSLShader gSkinnedObjectFullbrightShinyProgram;				// EE/WL only
LLGLSLShader gObjectFullbrightShinyWaterProgram;				// EE/WL only
LLGLSLShader gSkinnedObjectFullbrightShinyWaterProgram;			// EE/WL only
LLGLSLShader gObjectShinyProgram;								// EE/WL only
LLGLSLShader gSkinnedObjectShinyProgram;						// EE/WL only
LLGLSLShader gObjectShinyWaterProgram;							// EE/WL only
LLGLSLShader gSkinnedObjectShinyWaterProgram;					// EE/WL only
LLGLSLShader gObjectBumpProgram;
LLGLSLShader gSkinnedObjectBumpProgram;
LLGLSLShader gTreeProgram;										// EE/WL only
LLGLSLShader gTreeWaterProgram;									// EE/WL only
LLGLSLShader gObjectFullbrightNoColorWaterProgram;				// EE/WL only
LLGLSLShader gDrawColorProgram;									// PBR only

LLGLSLShader gObjectSimpleNonIndexedTexGenProgram;				// EE/WL only
LLGLSLShader gObjectSimpleNonIndexedTexGenWaterProgram;			// EE/WL only
LLGLSLShader gObjectAlphaMaskNonIndexedProgram;					// EE/WL only
LLGLSLShader gObjectAlphaMaskNonIndexedWaterProgram;			// EE/WL only
LLGLSLShader gObjectAlphaMaskNoColorProgram;
LLGLSLShader gObjectAlphaMaskNoColorWaterProgram;				// EE/WL only

// Environment shaders

LLGLSLShader gMoonProgram;										// EE/WL only
LLGLSLShader gStarsProgram;										// EE/WL only
LLGLSLShader gTerrainProgram;									// EE/WL only
LLGLSLShader gTerrainWaterProgram;								// EE/WL only
LLGLSLShader gWaterProgram;
LLGLSLShader gUnderWaterProgram;
LLGLSLShader gWaterEdgeProgram;

// Interface shaders

LLGLSLShader gHighlightProgram;
LLGLSLShader gSkinnedHighlightProgram;
LLGLSLShader gHighlightNormalProgram;
LLGLSLShader gHighlightSpecularProgram;

// Avatar shader handles

LLGLSLShader gAvatarProgram;
LLGLSLShader gAvatarWaterProgram;								// EE/WL only
LLGLSLShader gAvatarEyeballProgram;
LLGLSLShader gImpostorProgram;

// WindLight shader handles

LLGLSLShader gWLSkyProgram;										// EE/WL only
LLGLSLShader gWLCloudProgram;									// EE/WL only
LLGLSLShader gWLSunProgram;										// EE/WL only
LLGLSLShader gWLMoonProgram;									// EE/WL only

// Effects Shaders

LLGLSLShader gGlowProgram;
LLGLSLShader gGlowExtractProgram;


// Deferred rendering shaders

LLGLSLShader gDeferredImpostorProgram;
LLGLSLShader gDeferredWaterProgram;								// EE/WL only
LLGLSLShader gDeferredUnderWaterProgram;						// EE/WL only
LLGLSLShader gDeferredHighlightProgram;							// PBR only
LLGLSLShader gDeferredDiffuseProgram;
LLGLSLShader gDeferredDiffuseAlphaMaskProgram;
LLGLSLShader gDeferredSkinnedDiffuseAlphaMaskProgram;
LLGLSLShader gDeferredNonIndexedDiffuseAlphaMaskProgram;
LLGLSLShader gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
LLGLSLShader gDeferredSkinnedDiffuseProgram;
LLGLSLShader gDeferredSkinnedBumpProgram;
LLGLSLShader gDeferredBumpProgram;
LLGLSLShader gDeferredTerrainProgram;
LLGLSLShader gDeferredTerrainWaterProgram;						// EE/WL only
LLGLSLShader gDeferredTreeProgram;
LLGLSLShader gDeferredTreeShadowProgram;
LLGLSLShader gDeferredSkinnedTreeShadowProgram;
LLGLSLShader gDeferredAvatarProgram;
LLGLSLShader gDeferredAvatarAlphaProgram;
LLGLSLShader gDeferredLightProgram;
LLGLSLShader gDeferredMultiLightProgram[LL_DEFERRED_MULTI_LIGHT_COUNT];
LLGLSLShader gDeferredSpotLightProgram;
LLGLSLShader gDeferredMultiSpotLightProgram;
LLGLSLShader gDeferredSunProgram;
LLGLSLShader gDeferredSunProbeProgram;							// PBR only
LLGLSLShader gHazeProgram;										// PBR only
LLGLSLShader gHazeWaterProgram;									// PBR only
LLGLSLShader gDeferredBlurLightProgram;							// EE/WL only
LLGLSLShader gSSRFilterProgram;									// PBR only
LLGLSLShader gDeferredSoftenProgram;
LLGLSLShader gDeferredSoftenCubeProgram;						// PBR only
LLGLSLShader gDeferredSoftenWaterProgram;						// EE/WL only
LLGLSLShader gDeferredShadowProgram;
LLGLSLShader gDeferredSkinnedShadowProgram;
LLGLSLShader gDeferredShadowCubeProgram;
LLGLSLShader gDeferredShadowAlphaMaskProgram;
LLGLSLShader gDeferredSkinnedShadowAlphaMaskProgram;
LLGLSLShader gDeferredShadowGLTFAlphaMaskProgram;				// PBR only
LLGLSLShader gDeferredSkinnedShadowGLTFAlphaMaskProgram;		// PBR only
LLGLSLShader gDeferredShadowGLTFAlphaBlendProgram;				// PBR only
LLGLSLShader gDeferredSkinnedShadowGLTFAlphaBlendProgram;		// PBR only
LLGLSLShader gDeferredShadowFullbrightAlphaMaskProgram;
LLGLSLShader gDeferredSkinnedShadowFullbrightAlphaMaskProgram;
LLGLSLShader gDeferredAvatarShadowProgram;
LLGLSLShader gDeferredAvatarAlphaShadowProgram;
LLGLSLShader gDeferredAvatarAlphaMaskShadowProgram;
LLGLSLShader gDeferredAlphaProgram;
LLGLSLShader gHUDAlphaProgram;									// PBR only
LLGLSLShader gDeferredSkinnedAlphaProgram;
LLGLSLShader gDeferredAlphaImpostorProgram;
LLGLSLShader gDeferredSkinnedAlphaImpostorProgram;
LLGLSLShader gDeferredAlphaWaterProgram;						// EE/WL only
LLGLSLShader gDeferredSkinnedAlphaWaterProgram;					// EE/WL only
LLGLSLShader gDeferredAvatarEyesProgram;
LLGLSLShader gDeferredFullbrightProgram;
LLGLSLShader gDeferredFullbrightAlphaMaskProgram;
LLGLSLShader gDeferredFullbrightAlphaMaskAlphaProgram;			// PBR only
LLGLSLShader gHUDFullbrightProgram;								// PBR only
LLGLSLShader gHUDFullbrightAlphaMaskProgram;					// PBR only
LLGLSLShader gHUDFullbrightAlphaMaskAlphaProgram;				// PBR only
LLGLSLShader gDeferredFullbrightWaterProgram;					// EE/WL only
LLGLSLShader gDeferredSkinnedFullbrightWaterProgram;			// EE/WL only
LLGLSLShader gDeferredFullbrightAlphaMaskWaterProgram;			// EE/WL only
LLGLSLShader gDeferredSkinnedFullbrightAlphaMaskWaterProgram;	// EE/WL only
LLGLSLShader gDeferredEmissiveProgram;
LLGLSLShader gDeferredSkinnedEmissiveProgram;
LLGLSLShader gDeferredPostProgram;
LLGLSLShader gDeferredCoFProgram;
LLGLSLShader gDeferredDoFCombineProgram;
LLGLSLShader gDeferredPostGammaCorrectProgram;
LLGLSLShader gDeferredPostTonemapProgram;						// PBR only
LLGLSLShader gNoPostTonemapProgram;								// PBR only
LLGLSLShader gLegacyPostGammaCorrectProgram;					// PBR only
LLGLSLShader gExposureProgram;									// PBR only
LLGLSLShader gExposureProgramNoFade;							// PBR only
LLGLSLShader gLuminanceProgram;									// PBR only
LLGLSLShader gFXAAProgram[4];
LLGLSLShader gPostSMAAEdgeDetect[4];
LLGLSLShader gPostSMAABlendWeights[4];
LLGLSLShader gPostSMAANeighborhoodBlend[4];
LLGLSLShader gPostCASProgram;
LLGLSLShader gDeferredPostNoDoFProgram;
LLGLSLShader gDeferredPostNoDoFNoiseProgram;					// PBR only
LLGLSLShader gEnvironmentMapProgram;							// PBR only
LLGLSLShader gDeferredWLSkyProgram;
LLGLSLShader gDeferredWLCloudProgram;
LLGLSLShader gDeferredWLSunProgram;
LLGLSLShader gDeferredWLMoonProgram;
LLGLSLShader gDeferredStarProgram;
LLGLSLShader gDeferredFullbrightShinyProgram;
LLGLSLShader gHUDFullbrightShinyProgram;
LLGLSLShader gDeferredSkinnedFullbrightShinyProgram;
LLGLSLShader gDeferredSkinnedFullbrightProgram;
LLGLSLShader gDeferredSkinnedFullbrightAlphaMaskProgram;
LLGLSLShader gDeferredSkinnedFullbrightAlphaMaskAlphaProgram;	// PBR only
LLGLSLShader gNormalMapGenProgram;
LLGLSLShader gDeferredGenBrdfLutProgram;						// PBR only
LLGLSLShader gDeferredBufferVisualProgram;						// PBR only
LLGLSLShader gScreenSpaceReflTraceProgram;						// PBR only
LLGLSLShader gSSRAlphaProgram;									// PBR only
LLGLSLShader gSkinnedSSRAlphaProgram;							// PBR only
LLGLSLShader gSSRWaterProgram;									// PBR only

// Deferred materials shaders

LLGLSLShader gDeferredMaterialProgram[LLMaterial::SHADER_COUNT * 2];
LLGLSLShader gDeferredMaterialWaterProgram[LLMaterial::SHADER_COUNT * 2]; // EE/WL only
LLGLSLShader gHUDPBROpaqueProgram;								// PBR only
LLGLSLShader gPBRGlowProgram;									// PBR only
LLGLSLShader gPBRGlowSkinnedProgram;							// PBR only
LLGLSLShader gDeferredPBROpaqueProgram;							// PBR only
LLGLSLShader gDeferredSkinnedPBROpaqueProgram;					// PBR only
LLGLSLShader gHUDPBRAlphaProgram;								// PBR only
LLGLSLShader gDeferredPBRAlphaProgram;							// PBR only
LLGLSLShader gDeferredSkinnedPBRAlphaProgram;					// PBR only
LLGLSLShader gDeferredPBRTerrainProgram;						// PBR only

// glTF shaders
LLGLSLShader gGLTFPBRMetallicRoughnessProgram;					// PBR only

// Helper for creating a rigged variant *together* with a given shader. HB
static bool create_with_rigged(LLGLSLShader& shader,
							   LLGLSLShader& rigged_shader)
{
	shader.mRiggedVariant = &rigged_shader;
	std::string name = shader.mName;
	LLStringUtil::toLower(name);
	rigged_shader.mName = "Skinned " + name;
	rigged_shader.mShaderFiles = shader.mShaderFiles;
	rigged_shader.mShaderLevel = shader.mShaderLevel;
	rigged_shader.mShaderGroup = shader.mShaderGroup;
	rigged_shader.mFeatures = shader.mFeatures;
	rigged_shader.mFeatures.hasObjectSkinning = true;
	// Note: must come before addPermutation()
	rigged_shader.mDefines = shader.mDefines;
	rigged_shader.addPermutation("HAS_SKIN", "1");
	return rigged_shader.createShader() && shader.createShader();
}

// Helper for making a glTF variant from a given shader. HB
static bool make_gltf_variant(LLGLSLShader& shader, U8 i, bool use_sun_shadow)
{
	LLGLSLShader& variant = shader.mGLTFVariants[i];
	variant.mName = shader.mName + " variant #" + std::to_string(i);
	variant.mShaderFiles = shader.mShaderFiles;
	variant.mShaderLevel = shader.mShaderLevel;
	variant.mShaderGroup = shader.mShaderGroup;
	variant.mFeatures = shader.mFeatures;
	// Must  come before addPermutation
	variant.mDefines = shader.mDefines;
	constexpr U32 node_size = 16 * 3;
	U32 max_nodes = LLGLManager::sMaxUniformBlockSize / node_size;
	variant.addPermutation("MAX_NODES_PER_GLTF_OBJECT",
						   std::to_string(max_nodes));
	constexpr U32 material_size = 16 * 12;
	U32 max_materials = LLGLManager::sMaxUniformBlockSize / material_size;
	variant.addPermutation("MAX_MATERIALS_PER_GLTF_OBJECT",
						   std::to_string(max_materials));
	U32 max_vec4s = LLGLManager::sMaxUniformBlockSize / 16;
	variant.addPermutation("MAX_UBO_VEC4S", std::to_string(max_vec4s));
	if (i & LLGLSLShader::RIGGED)
	{
		variant.addPermutation("HAS_SKIN", "1");
	}
	if (i & LLGLSLShader::UNLIT)
	{
		variant.addPermutation("UNLIT", "1");
	}
	if (i & LLGLSLShader::MULTI_UV)
	{
		variant.addPermutation("MULTI_UV", "1");
	}

	if (!(i & LLGLSLShader::ALPHA_BLEND))
	{
		return variant.createShader();
	}

	variant.addPermutation("ALPHA_BLEND", "1");
	variant.mFeatures.calculatesLighting = false;
	variant.mFeatures.hasLighting = false;
	variant.mFeatures.isAlphaLighting = true;
	variant.mFeatures.hasSrgb = true;
	variant.mFeatures.calculatesAtmospherics = true;
	variant.mFeatures.hasAtmospherics = true;
	variant.mFeatures.hasGamma = true;
	variant.mFeatures.isDeferred = true;
	variant.mFeatures.hasReflectionProbes = true;
	variant.mFeatures.hasShadows = use_sun_shadow;
	if (use_sun_shadow)
	{
		variant.addPermutation("HAS_SUN_SHADOW", "1");
	}
	bool success = variant.createShader();
	if (success)
	{
		// *HACK: see LLRender::syncMatrices()
		variant.mFeatures.calculatesLighting = true;
		variant.mFeatures.hasLighting = true;
	}
	return success;
}

LLViewerShaderMgr::LLViewerShaderMgr()
:	mShaderLevel(SHADER_COUNT, 0),
	mMaxAvatarShaderLevel(0)
{
	sInitialized = true;
	init();
}

LLViewerShaderMgr::~LLViewerShaderMgr()
{
	mShaderLevel.clear();
	mShaderList.clear();
}

void LLViewerShaderMgr::init()
{
	sVertexShaderObjects.clear();
	sFragmentShaderObjects.clear();
	mShaderList.clear();

	std::string subdir = gUsePBRShaders ? "pbr" : "ee";
	mShaderDirPrefix = gDirUtil.getFullPath(LL_PATH_APP_SETTINGS, "shaders",
											subdir, "class");

	if (!LLGLManager::sHasRequirements)
	{
		llwarns << "Failed to pass minimum requirements for shaders."
				<< llendl;
		sInitialized = false;
		return;
	}

	// Make sure WL Sky is the first program. ONLY shaders that need WL param
	// management should be added here.
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gWLSkyProgram);
		mShaderList.push_back(&gWLCloudProgram);
		mShaderList.push_back(&gWLSunProgram);
		mShaderList.push_back(&gWLMoonProgram);
	}
	mShaderList.push_back(&gAvatarProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gObjectShinyProgram);
		mShaderList.push_back(&gSkinnedObjectShinyProgram);
	}
	mShaderList.push_back(&gWaterProgram);
	mShaderList.push_back(&gWaterEdgeProgram);
	mShaderList.push_back(&gAvatarEyeballProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gObjectSimpleProgram);
		mShaderList.push_back(&gSkinnedObjectSimpleProgram);
		mShaderList.push_back(&gObjectSimpleImpostorProgram);
		mShaderList.push_back(&gSkinnedObjectSimpleImpostorProgram);
	}
	mShaderList.push_back(&gImpostorProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gObjectFullbrightNoColorWaterProgram);
		mShaderList.push_back(&gObjectSimpleAlphaMaskProgram);
		mShaderList.push_back(&gSkinnedObjectSimpleAlphaMaskProgram);
	}
	mShaderList.push_back(&gObjectBumpProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gSkinnedObjectBumpProgram);
		mShaderList.push_back(&gObjectEmissiveProgram);
		mShaderList.push_back(&gSkinnedObjectEmissiveProgram);
		mShaderList.push_back(&gObjectEmissiveWaterProgram);
		mShaderList.push_back(&gSkinnedObjectEmissiveWaterProgram);
		mShaderList.push_back(&gObjectFullbrightProgram);
		mShaderList.push_back(&gSkinnedObjectFullbrightProgram);
	}
	mShaderList.push_back(&gObjectFullbrightAlphaMaskProgram);
	mShaderList.push_back(&gSkinnedObjectFullbrightAlphaMaskProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gObjectFullbrightShinyProgram);
		mShaderList.push_back(&gSkinnedObjectFullbrightShinyProgram);
		mShaderList.push_back(&gObjectFullbrightShinyWaterProgram);
		mShaderList.push_back(&gSkinnedObjectFullbrightShinyWaterProgram);
		mShaderList.push_back(&gObjectSimpleNonIndexedTexGenProgram);
		mShaderList.push_back(&gObjectSimpleNonIndexedTexGenWaterProgram);
		mShaderList.push_back(&gObjectAlphaMaskNonIndexedProgram);
		mShaderList.push_back(&gObjectAlphaMaskNonIndexedWaterProgram);
	}
	mShaderList.push_back(&gObjectAlphaMaskNoColorProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gObjectAlphaMaskNoColorWaterProgram);
		mShaderList.push_back(&gTreeProgram);
		mShaderList.push_back(&gTreeWaterProgram);
		mShaderList.push_back(&gMoonProgram);
		mShaderList.push_back(&gStarsProgram);
		mShaderList.push_back(&gTerrainProgram);
		mShaderList.push_back(&gTerrainWaterProgram);
		mShaderList.push_back(&gObjectSimpleWaterProgram);
		mShaderList.push_back(&gSkinnedObjectSimpleWaterProgram);
		mShaderList.push_back(&gObjectFullbrightWaterProgram);
		mShaderList.push_back(&gSkinnedObjectFullbrightWaterProgram);
		mShaderList.push_back(&gObjectSimpleWaterAlphaMaskProgram);
		mShaderList.push_back(&gSkinnedObjectSimpleWaterAlphaMaskProgram);
		mShaderList.push_back(&gObjectFullbrightWaterAlphaMaskProgram);
		mShaderList.push_back(&gSkinnedObjectFullbrightWaterAlphaMaskProgram);
		mShaderList.push_back(&gAvatarWaterProgram);
		mShaderList.push_back(&gObjectShinyWaterProgram);
		mShaderList.push_back(&gSkinnedObjectShinyWaterProgram);
	}
	mShaderList.push_back(&gUnderWaterProgram);
	mShaderList.push_back(&gDeferredSunProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gDeferredSunProbeProgram);
		mShaderList.push_back(&gHazeProgram);
		mShaderList.push_back(&gHazeWaterProgram);
	}
	mShaderList.push_back(&gDeferredSoftenProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gDeferredSoftenCubeProgram);
	}
	else
	{
		mShaderList.push_back(&gDeferredSoftenWaterProgram);
	}
	mShaderList.push_back(&gDeferredAlphaProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gHUDAlphaProgram);
	}
	else
	{
		mShaderList.push_back(&gDeferredSkinnedAlphaProgram);
	}
	mShaderList.push_back(&gDeferredAlphaImpostorProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gDeferredSkinnedAlphaImpostorProgram);
		mShaderList.push_back(&gDeferredAlphaWaterProgram);
		mShaderList.push_back(&gDeferredSkinnedAlphaWaterProgram);
	}
	mShaderList.push_back(&gDeferredFullbrightProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gHUDFullbrightProgram);
	}
	mShaderList.push_back(&gDeferredFullbrightAlphaMaskProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gHUDFullbrightAlphaMaskProgram);
		mShaderList.push_back(&gDeferredFullbrightAlphaMaskAlphaProgram);
		mShaderList.push_back(&gHUDFullbrightAlphaMaskAlphaProgram);
	}
	else
	{
		mShaderList.push_back(&gDeferredFullbrightWaterProgram);
		mShaderList.push_back(&gDeferredSkinnedFullbrightWaterProgram);
		mShaderList.push_back(&gDeferredFullbrightAlphaMaskWaterProgram);
		mShaderList.push_back(&gDeferredSkinnedFullbrightAlphaMaskWaterProgram);
	}
	mShaderList.push_back(&gDeferredFullbrightShinyProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gHUDFullbrightShinyProgram);
	}
	else
	{
		mShaderList.push_back(&gDeferredSkinnedFullbrightShinyProgram);
		mShaderList.push_back(&gDeferredSkinnedFullbrightProgram);
		mShaderList.push_back(&gDeferredSkinnedFullbrightAlphaMaskProgram);
	}
	mShaderList.push_back(&gDeferredEmissiveProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gDeferredSkinnedEmissiveProgram);
	}
	mShaderList.push_back(&gDeferredAvatarEyesProgram);
	if (!gUsePBRShaders)
	{
		mShaderList.push_back(&gDeferredWaterProgram);
		mShaderList.push_back(&gDeferredUnderWaterProgram);
		mShaderList.push_back(&gDeferredTerrainWaterProgram);
	}
	mShaderList.push_back(&gDeferredAvatarAlphaProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gEnvironmentMapProgram);
	}
	mShaderList.push_back(&gDeferredWLSkyProgram);
	mShaderList.push_back(&gDeferredWLCloudProgram);
	mShaderList.push_back(&gDeferredWLMoonProgram);
	mShaderList.push_back(&gDeferredWLSunProgram);
	if (gUsePBRShaders)
	{
		mShaderList.push_back(&gDeferredPBRAlphaProgram);
		mShaderList.push_back(&gHUDPBRAlphaProgram);
		mShaderList.push_back(&gDeferredPostTonemapProgram);
		mShaderList.push_back(&gNoPostTonemapProgram);
		// The two following shaders need the sky "gamma" value.
		mShaderList.push_back(&gDeferredPostGammaCorrectProgram);
		mShaderList.push_back(&gLegacyPostGammaCorrectProgram);

		mShaderList.push_back(&gDeferredDiffuseProgram);
		mShaderList.push_back(&gDeferredBumpProgram);
		mShaderList.push_back(&gDeferredPBROpaqueProgram);
		mShaderList.push_back(&gGLTFPBRMetallicRoughnessProgram);
		mShaderList.push_back(&gDeferredAvatarProgram);
		mShaderList.push_back(&gDeferredPBRTerrainProgram);
		mShaderList.push_back(&gDeferredDiffuseAlphaMaskProgram);
		mShaderList.push_back(&gDeferredNonIndexedDiffuseAlphaMaskProgram);
		mShaderList.push_back(&gDeferredTreeProgram);
	}
}

//static
void LLViewerShaderMgr::createInstance()
{
	if (gViewerShaderMgrp)
	{
		llwarns << "Instance already exists !" << llendl;
		llassert(false);
		return;
	}
	gViewerShaderMgrp = new LLViewerShaderMgr();
}

//static
void LLViewerShaderMgr::releaseInstance()
{
	sInitialized = false;
	if (gViewerShaderMgrp)
	{
		delete gViewerShaderMgrp;
		gViewerShaderMgrp = NULL;
	}
}

void LLViewerShaderMgr::initAttribsAndUniforms()
{
	if (sReservedUniforms.empty())
	{
		LLShaderMgr::initAttribsAndUniforms();
	}
}

// Note: Windows cannot reuse its splash screen after the main viewer window
// has been created. HB
#if LL_LINUX
class HBUpdateSplashScreen
{
public:
	HBUpdateSplashScreen(const std::string& message)
	:	mSplashScreenExists(LLSplashScreen::isVisible())
	{
		if (!mSplashScreenExists)
		{
			// Allow to disable the splash screen on shaders re-compilation
			// after viewer startup, just in case it would cause issues on some
			// systems (unlikely, but better safe than sorry). HB
			if (getenv("LL_DISABLE_SHADER_COMPILING_SPLASH"))
			{
				mSplashScreenExists = true;	// Nothing to do in the destructor.
				return;
			}
			LLSplashScreen::show();
		}
		LLSplashScreen::update(message);
	}

	~HBUpdateSplashScreen()
	{
		if (!mSplashScreenExists)
		{
			LLSplashScreen::hide();
		}
	}

private:
	bool mSplashScreenExists;
};
#endif

// Shader Management
void LLViewerShaderMgr::setShaders()
{
	// We get called recursively by gSavedSettings callbacks, so return on
	// reentrance
	static bool reentrance = false;
	if (reentrance)
	{
		// Always refresh cached settings however
		LLPipeline::refreshCachedSettings();
		return;
	}
	if (!gPipeline.isInit() || !sInitialized || sSkipReload ||
		!LLGLManager::sHasRequirements)
	{
		return;
	}
	reentrance = true;

	// Try and temporarily change the window title. Note that depending on how
	// and how fast (compared with the shaders compilation dusration) the
	// window manager propagates this change, it might not result in a visible
	// change of the window title... HB
	HBTempWindowTitle temp_title("Compiling shaders");
#if LL_LINUX
	// Note: for Windows, LLSplashScreen::update("Compiling shaders...")
	// happens in LLAppViewer::initWindow() on viewer launch, because the
	// splash screen cannot be (re)used once the viewer main window created. HB
	HBUpdateSplashScreen splash("Compiling shaders...");
#endif

	bool can_do_reflections = false;
	S32 reserved_units = 0;
	if (LLGLManager::sGLSLVersionMajor < 2 &&
		LLGLManager::sGLSLVersionMinor <= 20)
	{
		// NEVER use indexed texture rendering when GLSL version is 1.20 or
		// earlier
		sIndexedTextureChannels = 1;
	}
	else
	{
		S32 max_units = LLGLManager::sNumTextureImageUnits;
		if (gUsePBRShaders)
		{
			// Image units needed for PBR with reflection maps.
			constexpr S32 MIN_PBR_UNITS = 8;
			reserved_units = MIN_PBR_UNITS;
			if (max_units > reserved_units)
			{
				max_units -= reserved_units;
				can_do_reflections = true;
			}
			else
			{
				max_units = 1;
				llwarns << "Not enough available tex units for PBR reflection maps."
						<< llendl;
			}
		}
		// 1 texture unit at the minimum, 4 at the maximum...
		sIndexedTextureChannels = llmin(4, max_units);
	}
	llinfos << "Using up to " << sIndexedTextureChannels + reserved_units
			<< " image units for shaders";
	if (gUsePBRShaders && reserved_units)
	{
		llcont << " (" << reserved_units
			   << " reserved units for PBR shadows and reflections plus up to "
			   << sIndexedTextureChannels << " indexed texture channels)";
	}
	llcont << "." << llendl;

	// Make sure the compiled shader maps are cleared before we recompile
	// shaders, and set the shaders directory prefix depending whether we are
	// going to use the legacy EE/WL renderer or the PBR one. HB
	init();

	initAttribsAndUniforms();
	gPipeline.releaseGLBuffers();

	// *HACK: to reset buffers that change behavior with shaders
	gPipeline.resetVertexBuffers();

	unloadShaders();

	sCanDoRP = can_do_reflections && LLGLManager::sGLVersion >= 4.f &&
			   gFeatureManager.isFeatureAvailable("RenderReflectionProbes");
	sHasRP = sCanDoRP && gSavedSettings.getU32("RenderReflectionProbes");
	LLPipeline::refreshCachedSettings();
	LLShaderMgr::sMirrorsEnabled = LLPipeline::sRenderMirrors;

	if (gViewerWindowp)
	{
		gViewerWindowp->setCursor(UI_CURSOR_WAIT);
	}

	// Shaders
	llinfos << "\n~~~~~~~~~~~~~~~~~~\n Loading Shaders:\n~~~~~~~~~~~~~~~~~~"
			<< llendl;
	llinfos << llformat("Using GLSL %d.%d", LLGLManager::sGLSLVersionMajor,
						LLGLManager::sGLSLVersionMinor) << llendl;

	for (S32 i = 0; i < SHADER_COUNT; ++i)
	{
		mShaderLevel[i] = 0;
	}
	mMaxAvatarShaderLevel = 0;

	LLVertexBuffer::unbind();

	// GL_ARB_depth_clamp was so far always disabled because of an issue with
	// projectors... Let's use it when available depending on a debug setting,
	// to see how it fares... HB
	LLGLManager::sUseDepthClamp =
		LLGLManager::sHasDepthClamp &&
		gSavedSettings.getBool("RenderUseDepthClamp");
	if (!LLGLManager::sHasDepthClamp)
	{
		llinfos << "Missing feature GL_ARB_depth_clamp. Void water might disappear in rare cases."
				<< llendl;
	}
	else if (LLGLManager::sUseDepthClamp)
	{
		llinfos << "Depth clamping usage is enabled for shaders, which may possibly cause issues with projectors. Change RenderDepthClampShadows and/or RenderUseDepthClamp to FALSE (in this order of preference) if you wish to disable it, and please report successful combination(s) of those settings on the Cool VL Viewer support forum."
				<< llendl;
	}

	bool use_deferred = gUsePBRShaders ||
						(gFeatureManager.isFeatureAvailable("RenderDeferred") &&
						 gSavedSettings.getBool("RenderDeferred"));

	S32 interface_class, env_class, obj_class, effect_class, water_class,
		deferred_class;

	interface_class = env_class = obj_class = effect_class = water_class = 2;

	if (gUsePBRShaders)
	{
		deferred_class = water_class = 3;
	}
	else if (!use_deferred)
	{
		deferred_class = 0;
	}
	else if (gSavedSettings.getU32("RenderShadowDetail"))
	{
		// Shadows on
		deferred_class = 2;
	}
	else
	{
		// No shadows
		deferred_class = 1;
	}

	// Load lighting shaders
	mShaderLevel[SHADER_LIGHTING] = 3;
	mShaderLevel[SHADER_INTERFACE] = interface_class;
	mShaderLevel[SHADER_ENVIRONMENT] = env_class;
	mShaderLevel[SHADER_WATER] = water_class;
	mShaderLevel[SHADER_OBJECT] = obj_class;
	mShaderLevel[SHADER_EFFECT] = effect_class;
	mShaderLevel[SHADER_WINDLIGHT] = 2;
	mShaderLevel[SHADER_DEFERRED] = deferred_class;

	bool loaded = loadBasicShaders();
	if (!loaded)
	{
		sInitialized = false;
		reentrance = false;
		mShaderLevel[SHADER_LIGHTING] = 0;
		mShaderLevel[SHADER_INTERFACE] = 0;
		mShaderLevel[SHADER_ENVIRONMENT] = 0;
		mShaderLevel[SHADER_WATER] = 0;
		mShaderLevel[SHADER_OBJECT] = 0;
		mShaderLevel[SHADER_EFFECT] = 0;
		mShaderLevel[SHADER_WINDLIGHT] = 0;
		mShaderLevel[SHADER_DEFERRED] = 0;
		gPipeline.mVertexShadersLoaded = -1;
		llwarns << "Failed to load the basic shaders !" << llendl;
		return;
	}
	gPipeline.mVertexShadersLoaded = 1;

	// Load all shaders to set max levels
	loaded = loadShadersEnvironment();
	if (loaded)
	{
		loaded = loadShadersWater();
	}

	if (loaded)
	{
		loaded = loadShadersWindLight();
	}

	if (loaded)
	{
		loaded = loadShadersEffects();
	}

	if (loaded)
	{
		loaded = loadShadersInterface();
	}

	LLPipeline::sRenderAvatarCloth = false;
	if (loaded)
	{
		// Load max avatar shaders to set the max level
		mShaderLevel[SHADER_AVATAR] = 3;
		mMaxAvatarShaderLevel = 3;

		if (loadShadersObject())
		{
			// At this point, skinning shader is enabled and rigged attachment
			// shaders loaded correctly.
			bool avatar_cloth = gSavedSettings.getBool("RenderAvatarCloth");
			// Cloth is a class3 EE shader, class 1 PBR
			S32 avatar_class = avatar_cloth && !gUsePBRShaders ? 3 : 1;
			// Set the actual level
			mShaderLevel[SHADER_AVATAR] = avatar_class;
			loaded = loadShadersAvatar();
			if (!gUsePBRShaders && mShaderLevel[SHADER_AVATAR] != avatar_class)
			{
				if (llmax(mShaderLevel[SHADER_AVATAR] - 1, 0) >= 3)
				{
					avatar_cloth = true;
				}
				else
				{
					avatar_cloth = false;
				}
				gSavedSettings.setBool("RenderAvatarCloth", avatar_cloth);
			}
			LLPipeline::sRenderAvatarCloth = avatar_cloth;
		}
		else if (!gUsePBRShaders)
		{
			// Skinning shader not possible, neither is deferred rendering
			mShaderLevel[SHADER_AVATAR] = 0;
			mShaderLevel[SHADER_DEFERRED] = 0;

			gSavedSettings.setBool("RenderDeferred", false);
			gSavedSettings.setBool("RenderAvatarCloth", false);

			loadShadersAvatar(); // unloads

			loaded = loadShadersObject();
		}
	}

	// Some required shader could not load.
	if (!loaded &&
		(gUsePBRShaders || !gSavedSettings.getBool("RenderDeferred")))
	{
		sInitialized = false;
		reentrance = false;
		// In the PBR case, do not even bother trying to load deferred shaders
		// at this point. HB
		mShaderLevel[SHADER_DEFERRED] = 0;
		llwarns << "Failed to load required shaders !" << llendl;
		return;
	}
	if (loaded && !loadShadersDeferred())
	{
		if (gUsePBRShaders)
		{
			// PBR needs deferred shaders, so... HB
			sInitialized = false;
			reentrance = false;
			mShaderLevel[SHADER_DEFERRED] = 0;
			llwarns << "Failed to load the deferred shaders !" << llendl;
			return;
		}
		// Everything else succeeded but deferred failed, disable deferred and
		// try again.
		gSavedSettings.setBool("RenderDeferred", false);
		reentrance = false;
		setShaders();
		return;
	}

	if (gViewerWindowp)
	{
		gViewerWindowp->setCursor(UI_CURSOR_ARROW);
	}

	LLPipeline::refreshCachedSettings();

	gPipeline.createGLBuffers();

	reentrance = false;
}

void LLViewerShaderMgr::unloadShaders()
{
	while (!LLGLSLShader::sInstances.empty())
	{
		LLGLSLShader* shaderp = *(LLGLSLShader::sInstances.begin());
		shaderp->unload();
	}

	mShaderLevel[SHADER_LIGHTING] = 0;
	mShaderLevel[SHADER_OBJECT] = 0;
	mShaderLevel[SHADER_AVATAR] = 0;
	mShaderLevel[SHADER_ENVIRONMENT] = 0;
	mShaderLevel[SHADER_WATER] = 0;
	mShaderLevel[SHADER_INTERFACE] = 0;
	mShaderLevel[SHADER_EFFECT] = 0;
	mShaderLevel[SHADER_WINDLIGHT] = 0;

	gPipeline.mVertexShadersLoaded = -1;
}

// Loads basic dependency shaders first. All of these have to load for any
// shaders to function.
bool LLViewerShaderMgr::loadBasicShaders()
{
	// Use the feature table to mask out the max light level to use. Also make
	// sure it is at least 1.
	S32 max_class = gSavedSettings.getU32("RenderShaderLightingMaxLevel");
	S32 sum_lights_class = llclamp(max_class, 1, 3);

	// Load the Basic Vertex Shaders at the appropriate level (in order of
	// shader function call depth for reference purposes, deepest level first).

	S32 wl_level = mShaderLevel[SHADER_WINDLIGHT];
	S32 light_level = mShaderLevel[SHADER_LIGHTING];

	std::vector<std::pair<std::string, S32> > shaders;
	shaders.emplace_back("windlight/atmosphericsVarsV.glsl", wl_level);
	if (!gUsePBRShaders)
	{
		shaders.emplace_back("windlight/atmosphericsVarsWaterV.glsl",
							 wl_level);
	}
	shaders.emplace_back("windlight/atmosphericsHelpersV.glsl", wl_level);
	shaders.emplace_back("lighting/lightFuncV.glsl", light_level);
	shaders.emplace_back("lighting/sumLightsV.glsl", sum_lights_class);
	shaders.emplace_back("lighting/lightV.glsl", light_level);
	shaders.emplace_back("lighting/lightFuncSpecularV.glsl", light_level);
	shaders.emplace_back("lighting/sumLightsSpecularV.glsl", sum_lights_class);
	shaders.emplace_back("lighting/lightSpecularV.glsl", light_level);
	shaders.emplace_back("windlight/atmosphericsFuncs.glsl", wl_level);
	shaders.emplace_back("windlight/atmosphericsV.glsl", wl_level);
	if (gUsePBRShaders)
	{
		shaders.emplace_back("environment/srgbF.glsl", 1);
	}
	shaders.emplace_back("avatar/avatarSkinV.glsl", 1);
	shaders.emplace_back("avatar/objectSkinV.glsl", 1);
	if (gUsePBRShaders)
	{
		shaders.emplace_back("deferred/textureUtilV.glsl", 1);
	}
	if (LLGLManager::sGLSLVersionMajor > 1 ||
		LLGLManager::sGLSLVersionMinor >= 30)
	{
		shaders.emplace_back("objects/indexedTextureV.glsl", 1);
	}
	shaders.emplace_back("objects/nonindexedTextureV.glsl", 1);

	strings_map_t attribs;
	attribs["MAX_JOINTS_PER_MESH_OBJECT"] =
		llformat("%d", LL_MAX_JOINTS_PER_MESH_OBJECT);

	if (gUsePBRShaders && gSavedSettings.getBool("RenderEnableEmissiveBuffer"))
	{
		attribs["HAS_EMISSIVE"] = "1";
	}

	bool has_ssr = false;
	if (gUsePBRShaders)
	{
		U32 shadow_detail = gSavedSettings.getU32("RenderShadowDetail");
		if (shadow_detail >= 1)
		{
			attribs["SUN_SHADOW"] = "1";
			if (shadow_detail >= 2)
			{
				attribs["SPOT_SHADOW"] = "1";
			}
		}
		if (LLPipeline::sRenderSSR)
		{
			has_ssr = true;
			attribs["SSR"] = "1";
		}
		llinfos << "Screen space reflections "
				<< (has_ssr ? "enabled" : "disabled") << llendl;
		if (sHasRP)
		{
			U32 probe_level =
				llmin(3, gSavedSettings.getU32("RenderReflectionProbeLevel"));
			llinfos << "Reflection probe level: " << probe_level << llendl;
			attribs["REFMAP_LEVEL"] = llformat("%u", probe_level);
			attribs["REF_SAMPLE_COUNT"] = "32";
			if (gSavedSettings.getU32("RenderReflectionProbeQuality") > 0)
			{
				attribs["REFLECTION_PROBE_MED_QUALITY"] = "1";
			}
		}
		else
		{
			llinfos << "Reflection probes disabled." << llendl;
		}
		// PBR terrain
		bool triplanar = gSavedSettings.getBool("RenderTerrainPBRTriplanar");
		if (triplanar && LLGLManager::sMaxVaryingVectors <= 16)
		{
			gSavedSettings.setBool("RenderTerrainPBRTriplanar", false);
			triplanar = false;
			llinfos << "Not enough varying vectors with this GPU/driver: triplanar PBR terrain disabled."
					<< llendl;
		}

		S32 d = -4;
		S32 s = 1;
		F32 f = gSavedSettings.getF32("RenderTerrainPBRBlendFactor");
		if (LLPipeline::sRenderPBRTerrain)
		{
			d = LLPipeline::sRenderPBRTerrainDetail;
			s = triplanar ? 3 : 1;
		}
		attribs["TERRAIN_PBR_DETAIL"] = llformat("%d", d);
		attribs["TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT"] = llformat("%d", s);
		attribs["TERRAIN_TRIPLANAR_BLEND_FACTOR"] =
			llformat("%.2f", llclamp(f, 1.f, 32.f));
	}

	stop_glerror();

	// We no longer have to bind the shaders to global GLhandles, they are
	// automatically added to a map now.
	for (U32 i = 0, count = shaders.size(); i < count; ++i)
	{
		// Note usage of GL_VERTEX_SHADER
		if (loadShaderFile(shaders[i].first, shaders[i].second,
						   GL_VERTEX_SHADER, &attribs) == 0)
		{
			return false;
		}
	}

	// Load the Basic Fragment Shaders at the appropriate level (in order of
	// shader function call depth for reference purposes, deepest level first).

	shaders.clear();

	S32 ch = 1;
	if (LLGLManager::sGLSLVersionMajor > 1 ||
		LLGLManager::sGLSLVersionMinor >= 30)
	{
		// Use indexed texture rendering for GLSL >= 1.30
		if (gUsePBRShaders)
		{
			ch = llmax(sIndexedTextureChannels, 1);
		}
		else
		{
			ch = llmax(sIndexedTextureChannels - 1, 1);
		}
	}

	std::vector<S32> index_channels;
	index_channels.push_back(-1);
	shaders.emplace_back("windlight/atmosphericsVarsF.glsl", wl_level);
	if (!gUsePBRShaders)
	{
		index_channels.push_back(-1);
		shaders.emplace_back("windlight/atmosphericsVarsWaterF.glsl",
							 wl_level);
	}
	index_channels.push_back(-1);
	shaders.emplace_back("windlight/atmosphericsHelpersF.glsl", wl_level);
	index_channels.push_back(-1);
	shaders.emplace_back("windlight/gammaF.glsl", wl_level);
	index_channels.push_back(-1);
	shaders.emplace_back("windlight/atmosphericsFuncs.glsl", wl_level);
	index_channels.push_back(-1);
	shaders.emplace_back("windlight/atmosphericsF.glsl", wl_level);
	if (!gUsePBRShaders)
	{
		index_channels.push_back(-1);
		shaders.emplace_back("windlight/transportF.glsl", wl_level);
	}
	index_channels.push_back(-1);
	shaders.emplace_back("environment/waterFogF.glsl",
						 mShaderLevel[SHADER_WATER]);
	S32 env_level = mShaderLevel[SHADER_ENVIRONMENT];
	if (!gUsePBRShaders)
	{
		index_channels.push_back(-1);
		shaders.emplace_back("environment/encodeNormF.glsl", env_level);
	}
	index_channels.push_back(-1);
	shaders.emplace_back("environment/srgbF.glsl", env_level);
	index_channels.push_back(-1);
	shaders.emplace_back("deferred/deferredUtil.glsl", 1);
	if (gUsePBRShaders)
	{
		index_channels.push_back(-1);
		shaders.emplace_back("deferred/gbufferUtil.glsl", 1);
		index_channels.push_back(-1);
		shaders.emplace_back("deferred/globalF.glsl", 1);
	}
	index_channels.push_back(-1);
	shaders.emplace_back("deferred/shadowUtil.glsl", 1);
	index_channels.push_back(-1);
	shaders.emplace_back("deferred/aoUtil.glsl", 1);
	if (gUsePBRShaders)
	{
		index_channels.push_back(-1);
		shaders.emplace_back("deferred/pbrterrainUtilF.glsl", 1);
		index_channels.push_back(-1);
		shaders.emplace_back("deferred/reflectionProbeF.glsl", sHasRP ? 3 : 2);
		index_channels.push_back(-1);
		shaders.emplace_back("deferred/screenSpaceReflUtil.glsl",
							 has_ssr ? 3 : 1);
	}
	index_channels.push_back(-1);
	shaders.emplace_back("lighting/lightNonIndexedF.glsl", light_level);
	index_channels.push_back(-1);
	shaders.emplace_back("lighting/lightAlphaMaskNonIndexedF.glsl",
						 light_level);
	if (!gUsePBRShaders)
	{
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightFullbrightNonIndexedF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightFullbrightNonIndexedAlphaMaskF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightWaterNonIndexedF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightWaterAlphaMaskNonIndexedF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightFullbrightWaterNonIndexedF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightFullbrightWaterNonIndexedAlphaMaskF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightShinyNonIndexedF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightFullbrightShinyNonIndexedF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightShinyWaterNonIndexedF.glsl",
							 light_level);
		index_channels.push_back(-1);
		shaders.emplace_back("lighting/lightFullbrightShinyWaterNonIndexedF.glsl",
							 light_level);
	}
	index_channels.push_back(ch);
	shaders.emplace_back("lighting/lightF.glsl", light_level);
	index_channels.push_back(ch);
	shaders.emplace_back("lighting/lightAlphaMaskF.glsl", light_level);
	if (!gUsePBRShaders)
	{
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightFullbrightF.glsl", light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightFullbrightAlphaMaskF.glsl",
							 light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightWaterF.glsl", light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightWaterAlphaMaskF.glsl",
							 light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightFullbrightWaterF.glsl",
							 light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightFullbrightWaterAlphaMaskF.glsl",
							 light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightShinyF.glsl", light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightFullbrightShinyF.glsl",
							 light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightShinyWaterF.glsl", light_level);
		index_channels.push_back(ch);
		shaders.emplace_back("lighting/lightFullbrightShinyWaterF.glsl",
							 light_level);
	}

	for (U32 i = 0; i < shaders.size(); ++i)
	{
		// Note usage of GL_FRAGMENT_SHADER
		if (loadShaderFile(shaders[i].first, shaders[i].second,
						   GL_FRAGMENT_SHADER, &attribs,
						   index_channels[i]) == 0)
		{
			return false;
		}
	}

	llinfos << "Basic shaders loaded." << llendl;

	return true;
}

bool LLViewerShaderMgr::loadShadersEnvironment()
{
	S32 shader_level = mShaderLevel[SHADER_ENVIRONMENT];

	if (shader_level == 0 || gUsePBRShaders)
	{
		gTerrainProgram.unload();
		gMoonProgram.unload();
		gStarsProgram.unload();
		return true;
	}

	gTerrainProgram.setup("Terrain shader", shader_level,
						  "environment/terrainV.glsl",
						  "environment/terrainF.glsl");
	gTerrainProgram.mFeatures.mIndexedTextureChannels = 0;
	gTerrainProgram.mFeatures.calculatesLighting = true;
	gTerrainProgram.mFeatures.calculatesAtmospherics = true;
	gTerrainProgram.mFeatures.hasAtmospherics = true;
	gTerrainProgram.mFeatures.disableTextureIndex = true;
	gTerrainProgram.mFeatures.hasGamma = true;
	gTerrainProgram.mFeatures.hasTransport = true;
	gTerrainProgram.mFeatures.hasSrgb = true;
	bool success = gTerrainProgram.createShader();

	if (success)
	{
		gStarsProgram.setup("Environment stars shader", shader_level,
							"environment/starsV.glsl",
							"environment/starsF.glsl");
		gStarsProgram.addConstant(LLGLSLShader::CONST_STAR_DEPTH);
		success = gStarsProgram.createShader();
	}

	if (success)
	{
		gMoonProgram.setup("Environment Moon shader", shader_level,
						   "environment/moonV.glsl", "environment/moonF.glsl");
		gMoonProgram.addConstant(LLGLSLShader::CONST_CLOUD_MOON_DEPTH);
		success = gMoonProgram.createShader();
		if (success)
		{
			gMoonProgram.bind();
			gMoonProgram.uniform1i(sTex0, 0);
			gMoonProgram.unbind();
		}
	}

	if (success)
	{
		gWorld.updateWaterObjects();
		llinfos << "Environment shaders loaded at level: " << shader_level
				<< llendl;
		return true;
	}

	mShaderLevel[SHADER_ENVIRONMENT] = 0;
	return false;
}

bool LLViewerShaderMgr::loadShadersWater()
{
	S32 shader_level = mShaderLevel[SHADER_WATER];

	if (shader_level == 0)
	{
		gWaterProgram.unload();
		gWaterEdgeProgram.unload();
		gUnderWaterProgram.unload();
		gTerrainWaterProgram.unload();
		return true;
	}

	bool use_sun_shadow = false;
	if (gUsePBRShaders)
	{
		use_sun_shadow = mShaderLevel[SHADER_DEFERRED] > 1 &&
						 gSavedSettings.getU32("RenderShadowDetail");
	}

	// Load water shader
	gWaterProgram.setup("Water shader", shader_level,
						"environment/waterV.glsl",
						"environment/waterF.glsl");
	gWaterProgram.mFeatures.calculatesAtmospherics = true;
	gWaterProgram.mFeatures.hasGamma = true;
	gWaterProgram.mFeatures.hasSrgb = true;
	if (gUsePBRShaders)
	{
		gWaterProgram.mFeatures.hasAtmospherics = true;
		gWaterProgram.mFeatures.hasReflectionProbes = true;
		gWaterProgram.mFeatures.hasShadows = use_sun_shadow;
		if (use_sun_shadow)
		{
			gWaterProgram.addPermutation("HAS_SUN_SHADOW", "1");
		}
		if (LLPipeline::RenderTransparentWater)
		{
			gWaterProgram.addPermutation("TRANSPARENT_WATER", "1");
		}
		if (sHasRP)
		{
			if (LLPipeline::sRenderSSR)
			{
				gWaterProgram.mFeatures.hasScreenSpaceReflections = true;
				gWaterProgram.addPermutation("SSR", "1");
			}
			else if (gSavedSettings.getBool("RenderPBRWaterReflections"))
			{
				gWaterProgram.addPermutation("WATER_MINIMAL_PLUS", "1");
			}
		}
	}
	else
	{
		gWaterProgram.mFeatures.hasTransport = true;
	}
	gWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
	bool success = gWaterProgram.createShader();

	if (success)
	{
		// Load water edge shader
		gWaterEdgeProgram.setup("Water edge shader", shader_level,
								"environment/waterV.glsl",
								"environment/waterF.glsl");
		gWaterEdgeProgram.mFeatures.calculatesAtmospherics = true;
		gWaterEdgeProgram.mFeatures.hasGamma = true;
		gWaterEdgeProgram.mFeatures.hasSrgb = true;
		if (gUsePBRShaders)
		{
			gWaterEdgeProgram.mFeatures.hasAtmospherics = true;
			gWaterEdgeProgram.mFeatures.hasReflectionProbes = true;
		}
		else
		{
			gWaterEdgeProgram.mFeatures.hasTransport = true;
		}
		gWaterEdgeProgram.addPermutation("WATER_EDGE", "1");
		gWaterEdgeProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gWaterEdgeProgram.createShader();
	}

	if (success)
	{
		// Load under water vertex shader
		gUnderWaterProgram.setup("Underwater shader", shader_level,
								 "environment/waterV.glsl",
								 "environment/underWaterF.glsl");
		gUnderWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gUnderWaterProgram.mFeatures.calculatesAtmospherics = true;
		if (gUsePBRShaders)
		{
			gUnderWaterProgram.mFeatures.hasAtmospherics = true;
		}
		else
		{
			gUnderWaterProgram.mFeatures.hasWaterFog = true;
		}
		if (gUsePBRShaders && LLPipeline::RenderTransparentWater)
		{
			gUnderWaterProgram.addPermutation("TRANSPARENT_WATER", "1");
		}
		success = gUnderWaterProgram.createShader();
	}

	bool terrain_water_success = true;
	if (success && !gUsePBRShaders)
	{
		// Load terrain water shader
		gTerrainWaterProgram.setup("Terrain water shader",
								   mShaderLevel[SHADER_ENVIRONMENT],
								   "environment/terrainWaterV.glsl",
								   "environment/terrainWaterF.glsl");
		gTerrainWaterProgram.mFeatures.calculatesLighting = true;
		gTerrainWaterProgram.mFeatures.calculatesAtmospherics = true;
		gTerrainWaterProgram.mFeatures.hasAtmospherics = true;
		gTerrainWaterProgram.mFeatures.hasWaterFog = true;
		gTerrainWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gTerrainWaterProgram.mFeatures.disableTextureIndex = true;
		gTerrainWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		if (LLPipeline::sRenderDeferred)
		{
			gTerrainWaterProgram.addPermutation("ALM", "1");
		}
		terrain_water_success = gTerrainWaterProgram.createShader();
	}

	// Keep track of water shader levels
	if (gWaterProgram.mShaderLevel != shader_level ||
		gUnderWaterProgram.mShaderLevel != shader_level)
	{
		mShaderLevel[SHADER_WATER] =
			llmin(gWaterProgram.mShaderLevel, gUnderWaterProgram.mShaderLevel);
	}

	if (!success)
	{
		mShaderLevel[SHADER_WATER] = 0;
		return false;
	}

	// If we failed to load the terrain water shaders and we need them (using
	// class2 water), then drop down to class1 water.
	if (mShaderLevel[SHADER_WATER] > 1 && !terrain_water_success)
	{
		--mShaderLevel[SHADER_WATER];
		return loadShadersWater();
	}

	gWorld.updateWaterObjects();

	llinfos << "Water shaders loaded at level: " << mShaderLevel[SHADER_WATER]
			<< llendl;

	return true;
}

bool LLViewerShaderMgr::loadShadersEffects()
{
	S32 shader_level = mShaderLevel[SHADER_EFFECT];

	if (shader_level == 0)
	{
		gGlowProgram.unload();
		gGlowExtractProgram.unload();
		return true;
	}

	gGlowProgram.setup("Glow shader (post)", shader_level,
					   "effects/glowV.glsl", "effects/glowF.glsl");
	bool success = gGlowProgram.createShader();
	if (success)
	{
		gGlowExtractProgram.setup("Glow extract shader (post)", shader_level,
								  "effects/glowExtractV.glsl",
								  "effects/glowExtractF.glsl");
		if (gUsePBRShaders && gSavedSettings.getBool("RenderGlowNoise"))
		{
			gGlowExtractProgram.addPermutation("HAS_NOISE", "1");
		}
		success = gGlowExtractProgram.createShader();
	}

	LLPipeline::sCanRenderGlow = success;

	if (success)
	{
		llinfos << "Effects shaders loaded at level: " << shader_level
				<< llendl;
	}

	return success;
}

bool LLViewerShaderMgr::loadShadersDeferred()
{
	sHasFXAA = sHasSMAA = sHasCAS = sHasLut = false;

	S32 shader_level = mShaderLevel[SHADER_DEFERRED];
	bool use_sun_shadow = shader_level > 1;
	if (gUsePBRShaders)
	{
		use_sun_shadow &= gSavedSettings.getU32("RenderShadowDetail") > 0;
	}

	if (shader_level == 0)
	{
		gDeferredTreeProgram.unload();
		gDeferredTreeShadowProgram.unload();
		gDeferredSkinnedTreeShadowProgram.unload();
		gDeferredHighlightProgram.unload();
		gDeferredDiffuseProgram.unload();
		gDeferredSkinnedDiffuseProgram.unload();
		gDeferredDiffuseAlphaMaskProgram.unload();
		gDeferredSkinnedDiffuseAlphaMaskProgram.unload();
		gDeferredNonIndexedDiffuseAlphaMaskProgram.unload();
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.unload();
		gDeferredBumpProgram.unload();
		gDeferredSkinnedBumpProgram.unload();
		gDeferredImpostorProgram.unload();
		gDeferredTerrainProgram.unload();
		gDeferredTerrainWaterProgram.unload();
		gDeferredLightProgram.unload();
		for (U32 i = 0; i < LL_DEFERRED_MULTI_LIGHT_COUNT; ++i)
		{
			gDeferredMultiLightProgram[i].unload();
		}
		gDeferredSpotLightProgram.unload();
		gDeferredMultiSpotLightProgram.unload();
		gDeferredSunProgram.unload();
		gDeferredBlurLightProgram.unload();
		gSSRFilterProgram.unload();
		gHazeProgram.unload();
		gHazeWaterProgram.unload();
		gDeferredSoftenProgram.unload();
		gDeferredSoftenCubeProgram.unload();
		gDeferredSoftenWaterProgram.unload();
		gDeferredShadowProgram.unload();
		gDeferredSkinnedShadowProgram.unload();
		gDeferredShadowCubeProgram.unload();
		gDeferredShadowAlphaMaskProgram.unload();
		gDeferredSkinnedShadowAlphaMaskProgram.unload();
		gDeferredShadowGLTFAlphaMaskProgram.unload();
		gDeferredSkinnedShadowGLTFAlphaMaskProgram.unload();
		gDeferredShadowGLTFAlphaBlendProgram.unload();
		gDeferredSkinnedShadowGLTFAlphaBlendProgram.unload();
		gDeferredShadowFullbrightAlphaMaskProgram.unload();
		gDeferredSkinnedShadowFullbrightAlphaMaskProgram.unload();
		gDeferredAvatarShadowProgram.unload();
		gDeferredAvatarAlphaShadowProgram.unload();
		gDeferredAvatarAlphaMaskShadowProgram.unload();
		gDeferredAvatarProgram.unload();
		gDeferredAvatarAlphaProgram.unload();
		gDeferredAlphaProgram.unload();
		gHUDAlphaProgram.unload();
		gDeferredSkinnedAlphaProgram.unload();
		gDeferredAlphaWaterProgram.unload();
		gDeferredSkinnedAlphaWaterProgram.unload();
		gDeferredFullbrightProgram.unload();
		gDeferredFullbrightAlphaMaskProgram.unload();
		gDeferredFullbrightAlphaMaskAlphaProgram.unload();
		gHUDFullbrightProgram.unload();
		gHUDFullbrightAlphaMaskProgram.unload();
		gHUDFullbrightAlphaMaskAlphaProgram.unload();
		gDeferredFullbrightWaterProgram.unload();
		gDeferredSkinnedFullbrightWaterProgram.unload();
		gDeferredFullbrightAlphaMaskWaterProgram.unload();
		gDeferredSkinnedFullbrightAlphaMaskWaterProgram.unload();
		gDeferredEmissiveProgram.unload();
		gDeferredSkinnedEmissiveProgram.unload();
		gDeferredAvatarEyesProgram.unload();
		gDeferredPostProgram.unload();
		gDeferredCoFProgram.unload();
		gDeferredDoFCombineProgram.unload();
		gDeferredPostGammaCorrectProgram.unload();
		gExposureProgram.unload();
		gExposureProgramNoFade.unload();
		gLuminanceProgram.unload();
		gDeferredPostTonemapProgram.unload();
		gNoPostTonemapProgram.unload();
		gLegacyPostGammaCorrectProgram.unload();
		gDeferredPostNoDoFNoiseProgram.unload();
		for (U32 i = 0; i < 4; ++i)
		{
			gFXAAProgram[i].unload();
			gPostSMAAEdgeDetect[i].unload();
			gPostSMAABlendWeights[i].unload();
			gPostSMAANeighborhoodBlend[i].unload();
		}
		gPostCASProgram.unload();
		gDeferredWaterProgram.unload();
		gDeferredUnderWaterProgram.unload();
		gEnvironmentMapProgram.unload();
		gDeferredWLSkyProgram.unload();
		gDeferredWLCloudProgram.unload();
		gDeferredWLSunProgram.unload();
		gDeferredWLMoonProgram.unload();
		gDeferredStarProgram.unload();
		gDeferredFullbrightShinyProgram.unload();
		gHUDFullbrightShinyProgram.unload();
		gDeferredSkinnedFullbrightShinyProgram.unload();
		gDeferredSkinnedFullbrightProgram.unload();
		gDeferredSkinnedFullbrightAlphaMaskProgram.unload();
		gDeferredSkinnedFullbrightAlphaMaskAlphaProgram.unload();

		gNormalMapGenProgram.unload();
		gDeferredGenBrdfLutProgram.unload();
		gDeferredBufferVisualProgram.unload();
		gScreenSpaceReflTraceProgram.unload();
		gSSRAlphaProgram.unload();
		gSkinnedSSRAlphaProgram.unload();
		gSSRWaterProgram.unload();

		for (U32 i = 0; i < LLMaterial::SHADER_COUNT * 2; ++i)
		{
			gDeferredMaterialProgram[i].unload();
			gDeferredMaterialWaterProgram[i].unload();
		}

		gHUDPBROpaqueProgram.unload();
		gPBRGlowProgram.unload();
		gPBRGlowSkinnedProgram.unload();
		gDeferredPBROpaqueProgram.unload();
		gGLTFPBRMetallicRoughnessProgram.unload();
		gDeferredSkinnedPBROpaqueProgram.unload();
		gDeferredPBRAlphaProgram.unload();
		gDeferredSkinnedPBRAlphaProgram.unload();
		gDeferredPBRTerrainProgram.unload();

		return true;
	}

	bool emissive = gUsePBRShaders &&
					gSavedSettings.getBool("RenderEnableEmissiveBuffer");

	LLGLSLShader* shaderp = &gDeferredDiffuseProgram;
	shaderp->setup("Deferred diffuse shader", shader_level,
				   "deferred/diffuseV.glsl", "deferred/diffuseIndexedF.glsl");
	shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
	shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
	shaderp->mFeatures.hasSrgb = true;
	if (emissive)
	{
		shaderp->addPermutation("HAS_EMISSIVE", "1");
	}
	bool success = create_with_rigged(*shaderp,
									  gDeferredSkinnedDiffuseProgram);

	if (success && gUsePBRShaders)
	{
		gDeferredHighlightProgram.setup("Deferred highlight shader",
										mShaderLevel[SHADER_INTERFACE],
										"interface/highlightV.glsl",
										"deferred/highlightF.glsl");
		if (emissive)
		{
			gDeferredHighlightProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredHighlightProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredDiffuseAlphaMaskProgram;
		shaderp->setup("Deferred diffuse alpha mask shader", shader_level,
					   "deferred/diffuseV.glsl",
					   "deferred/diffuseAlphaMaskIndexedF.glsl");
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedDiffuseAlphaMaskProgram);
	}

	if (success)
	{
		shaderp = &gDeferredNonIndexedDiffuseAlphaMaskProgram;
		shaderp->setup("Deferred diffuse non-indexed alpha mask shader",
					   shader_level, "deferred/diffuseV.glsl",
					   "deferred/diffuseAlphaMaskF.glsl");
		shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		shaderp = &gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
		shaderp->setup("Deferred diffuse non-indexed alpha mask no color shader",
					   shader_level, "deferred/diffuseNoColorV.glsl",
					   "deferred/diffuseAlphaMaskNoColorF.glsl");
		shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		gDeferredBumpProgram.setup("Deferred bump shader", shader_level,
								   "deferred/bumpV.glsl",
								   "deferred/bumpF.glsl");
		// Note: ignored by PBR
		gDeferredBumpProgram.mFeatures.encodesNormal = true;
		if (emissive)
		{
			gDeferredBumpProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(gDeferredBumpProgram,
									 gDeferredSkinnedBumpProgram);
	}

	std::string name;
	for (U32 i = 0; success && i < LLMaterial::SHADER_COUNT * 2; ++i)
	{
		U32 alpha_mode = i & 0x3;
		std::string alpha_mode_str = llformat("%d", alpha_mode);
		bool has_specular_map = i & 0x4;
		bool has_normal_map = i & 0x8;
		bool has_skin = i & 0x10;

		shaderp = &gDeferredMaterialProgram[i];
		name = llformat("Deferred material shader %d", i);
		shaderp->setup(name.c_str(), shader_level, "deferred/materialV.glsl",
					   "deferred/materialF.glsl");
		shaderp->addPermutation("DIFFUSE_ALPHA_MODE", alpha_mode_str);
		if (alpha_mode)
		{
			shaderp->mFeatures.hasAlphaMask = true;
			shaderp->addPermutation("HAS_ALPHA_MASK", "1");
		}
		shaderp->mFeatures.hasShadows = use_sun_shadow;
		if (use_sun_shadow)
		{
			shaderp->addPermutation("HAS_SUN_SHADOW", "1");
		}
		if (has_normal_map)
		{
			shaderp->addPermutation("HAS_NORMAL_MAP", "1");
		}
		if (has_specular_map)
		{
			shaderp->addPermutation("HAS_SPECULAR_MAP", "1");
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
		if (alpha_mode == 1 || gUsePBRShaders)
		{
			if (gUsePBRShaders)
			{
				shaderp->mFeatures.hasReflectionProbes = true;
			}
			else
			{
				shaderp->mFeatures.hasTransport = true;
			}
			// *TODO: verify this is needed for PBR when alpha_mode != 1. HB
			shaderp->mFeatures.calculatesAtmospherics = true;
			shaderp->mFeatures.hasAtmospherics = true;
			shaderp->mFeatures.hasGamma = true;
		}
		if (has_skin)
		{
			shaderp->addPermutation("HAS_SKIN", "1");
			shaderp->mFeatures.hasObjectSkinning = true;
		}
		else
		{
			shaderp->mRiggedVariant = &gDeferredMaterialProgram[i + 0x10];
		}

		success = shaderp->createShader();
		if (!success) break;

		if (!has_skin || !gUsePBRShaders)
		{
			mShaderList.push_back(shaderp);
		}

		if (gUsePBRShaders)
		{
			continue;	// No water shader needed any more for PBR
		}

		shaderp = &gDeferredMaterialWaterProgram[i];
		name = llformat("Deferred underwater material shader %d", i);
		shaderp->setup(name.c_str(), shader_level, "deferred/materialV.glsl",
					  "deferred/materialF.glsl");
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		shaderp->addPermutation("WATER_FOG", "1");
		shaderp->addPermutation("DIFFUSE_ALPHA_MODE", alpha_mode_str);
		if (alpha_mode)
		{
			shaderp->mFeatures.hasAlphaMask = true;
			shaderp->addPermutation("HAS_ALPHA_MASK", "1");
		}
		shaderp->mFeatures.hasShadows = use_sun_shadow;
		if (use_sun_shadow)
		{
			shaderp->addPermutation("HAS_SUN_SHADOW", "1");
		}
		if (has_normal_map)
		{
			shaderp->addPermutation("HAS_NORMAL_MAP", "1");
		}
		if (has_specular_map)
		{
			shaderp->addPermutation("HAS_SPECULAR_MAP", "1");
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
		if (alpha_mode == 1)
		{
			shaderp->mFeatures.hasTransport = true;
			shaderp->mFeatures.hasWaterFog = true;
			shaderp->mFeatures.calculatesAtmospherics = true;
			shaderp->mFeatures.hasAtmospherics = true;
			shaderp->mFeatures.hasGamma = true;
		}
		if (has_skin)
		{
			shaderp->addPermutation("HAS_SKIN", "1");
			shaderp->mFeatures.hasObjectSkinning = true;
		}
		else
		{
			shaderp->mRiggedVariant = &gDeferredMaterialWaterProgram[i + 0x10];
		}

		success = shaderp->createShader();
		if (success)
		{
			 mShaderList.push_back(shaderp);
		}
	}

	gDeferredMaterialProgram[1].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[5].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[9].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[13].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[1 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[5 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[9 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[13 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;

	if (!gUsePBRShaders)
	{
		gDeferredMaterialWaterProgram[1].mFeatures.hasLighting = true;
		gDeferredMaterialWaterProgram[5].mFeatures.hasLighting = true;
		gDeferredMaterialWaterProgram[9].mFeatures.hasLighting = true;
		gDeferredMaterialWaterProgram[13].mFeatures.hasLighting = true;
		gDeferredMaterialWaterProgram[1 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
		gDeferredMaterialWaterProgram[5 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
		gDeferredMaterialWaterProgram[9 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
		gDeferredMaterialWaterProgram[13 + LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	}

	if (success && gUsePBRShaders)
	{
		gDeferredPBROpaqueProgram.setup("Deferred PBR opaque shader",
										shader_level,
										"deferred/pbropaqueV.glsl",
										"deferred/pbropaqueF.glsl");
		gDeferredPBROpaqueProgram.mFeatures.hasSrgb = true;
		if (emissive)
		{
			gDeferredPBROpaqueProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(gDeferredPBROpaqueProgram,
									 gDeferredSkinnedPBROpaqueProgram);
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gGLTFPBRMetallicRoughnessProgram;
		shaderp->setup("glTF PBR metallic roughness shader",
					   shader_level, "gltf/pbrmetallicroughnessV.glsl",
					   "gltf/pbrmetallicroughnessF.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mGLTFVariants.resize(LLGLSLShader::NUM_GLTF_VARIANTS);
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		for (U8 i = 0; i < LLGLSLShader::NUM_GLTF_VARIANTS; ++i)
		{
			if (!make_gltf_variant(*shaderp, i, use_sun_shadow))
			{
				success = false;
				break;
			}
		}
		sCanRenderGLTFScene = success;
		success = true;	// We can do without it for now...
    }

	if (success && gUsePBRShaders)
	{
		gPBRGlowProgram.setup("Deferred PBR glow shader", shader_level,
							  "deferred/pbrglowV.glsl",
							  "deferred/pbrglowF.glsl");
		gPBRGlowProgram.mFeatures.hasSrgb = true;
		if (emissive)
		{
			gPBRGlowProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(gPBRGlowProgram, gPBRGlowSkinnedProgram);
	}

	if (success && gUsePBRShaders)
	{
		gHUDPBROpaqueProgram.setup("Deferred HUD PBR opaque shader",
								   shader_level, "deferred/pbropaqueV.glsl",
								   "deferred/pbropaqueF.glsl");
		gHUDPBROpaqueProgram.mFeatures.hasSrgb = true;
		gHUDPBROpaqueProgram.addPermutation("IS_HUD", "1");
		if (emissive)
		{
			gHUDPBROpaqueProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gHUDPBROpaqueProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredPBRAlphaProgram;
		shaderp->setup("Deferred PBR alpha shader", shader_level,
					   "deferred/pbralphaV.glsl", "deferred/pbralphaF.glsl");
		shaderp->mFeatures.isAlphaLighting = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		// Includes deferredUtils:
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.hasReflectionProbes = shader_level;
		if (use_sun_shadow)
		{
			shaderp->mFeatures.hasShadows = true;
			shaderp->addPermutation("HAS_SUN_SHADOW", "1");
		}
		std::string dam = llformat("%d", LLMaterial::DIFFUSE_ALPHA_MODE_BLEND);
		shaderp->addPermutation("DIFFUSE_ALPHA_MODE", dam);
		shaderp->addPermutation("HAS_NORMAL_MAP", "1");
		// Note: SPECULAR_MAP = packed vector (Occlusion, Metal, Roughness).
		shaderp->addPermutation("HAS_SPECULAR_MAP", "1");
		shaderp->addPermutation("HAS_EMISSIVE_MAP", "1");
		shaderp->addPermutation("USE_VERTEX_COLOR", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedPBRAlphaProgram);
		// *HACK: set after creation to disable auto-setup of texture channels
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.hasLighting = true;
		shaderp->mRiggedVariant->mFeatures.calculatesLighting = true;
		shaderp->mRiggedVariant->mFeatures.hasLighting = true;
	}

	if (success && gUsePBRShaders)
	{
		gHUDPBRAlphaProgram.setup("Deferred HUD PBR alpha shader",
								  shader_level, "deferred/pbralphaV.glsl",
								  "deferred/pbralphaF.glsl");
		gHUDPBRAlphaProgram.mFeatures.hasSrgb = true;
		gHUDPBRAlphaProgram.addPermutation("IS_HUD", "1");
		if (emissive)
		{
			gHUDPBRAlphaProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gHUDPBRAlphaProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredPBRTerrainProgram;
		S32 d = -4;
		S32 s = 1;
		if (LLPipeline::sRenderPBRTerrain)
		{
			d = LLPipeline::sRenderPBRTerrainDetail;
			s = gSavedSettings.getBool("RenderTerrainPBRTriplanar") ? 3 : 1;
		}
		std::string name = llformat("Deferred PBR terrain shader %d %s", d,
									(s == 1 ? "flat" : "trilinear"));
		shaderp->setup(name.c_str(), shader_level, "deferred/pbrterrainV.glsl",
					   "deferred/pbrterrainF.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isAlphaLighting = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.isPBRTerrain = true;
		shaderp->addPermutation("TERRAIN_PBR_DETAIL", llformat("%d", d));
		shaderp->addPermutation("TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT",
								llformat("%d", s));
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		gDeferredTreeProgram.setup("Deferred tree shader", shader_level,
								   "deferred/treeV.glsl",
								   "deferred/treeF.glsl");
		// Note: ignored by PBR
		gDeferredTreeProgram.mFeatures.encodesNormal = true;
		if (emissive)
		{
			gDeferredTreeProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredTreeProgram.createShader();
	}

	if (success)
	{
		gDeferredTreeShadowProgram.setup("Deferred tree shadow shader",
										 shader_level,
										 "deferred/treeShadowV.glsl",
										 "deferred/treeShadowF.glsl");
		if (!gUsePBRShaders)
		{
			gDeferredTreeShadowProgram.mFeatures.hasShadows = true;
			gDeferredTreeShadowProgram.mFeatures.isDeferred = true;
		}
		gDeferredTreeShadowProgram.mRiggedVariant =
			&gDeferredSkinnedTreeShadowProgram;
		success = gDeferredTreeShadowProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredSkinnedTreeShadowProgram;
		shaderp->setup("Skinned deferred tree shadow shader", shader_level,
					   "deferred/treeShadowSkinnedV.glsl",
					   "deferred/treeShadowF.glsl");
		if (!gUsePBRShaders)
		{
			shaderp->mFeatures.hasShadows = true;
			shaderp->mFeatures.isDeferred = true;
		}
		shaderp->mFeatures.hasObjectSkinning = true;
		success = shaderp->createShader();
	}

	if (success)
	{
		gDeferredImpostorProgram.setup("Deferred impostor shader",
									   shader_level, "deferred/impostorV.glsl",
									   "deferred/impostorF.glsl");
		gDeferredImpostorProgram.mFeatures.hasSrgb = true;
		// Note: ignored by PBR
		gDeferredImpostorProgram.mFeatures.encodesNormal = true;
		if (emissive)
		{
			gDeferredImpostorProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredImpostorProgram.createShader();
	}

	if (success)
	{
		gDeferredLightProgram.setup("Deferred light shader", shader_level,
									"deferred/pointLightV.glsl",
									"deferred/pointLightF.glsl");
		gDeferredLightProgram.mFeatures.hasShadows = true;
		gDeferredLightProgram.mFeatures.isDeferred = true;
		gDeferredLightProgram.mFeatures.hasSrgb = true;
		if (gUsePBRShaders)
		{
			gDeferredLightProgram.mFeatures.hasFullGBuffer = true;
		}
		if (emissive)
		{
			gDeferredLightProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredLightProgram.createShader();
	}

	for (U32 i = 0; success && i < LL_DEFERRED_MULTI_LIGHT_COUNT; ++i)
	{
		shaderp = &gDeferredMultiLightProgram[i];
		name = llformat("Deferred multilight shader %d", i);
		shaderp->setup(name.c_str(), shader_level,
					  "deferred/multiPointLightV.glsl",
					  "deferred/multiPointLightF.glsl");
		shaderp->mFeatures.hasShadows = true;
		shaderp->mFeatures.isDeferred = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasFullGBuffer = true;
		}
		shaderp->mFeatures.hasSrgb = true;
		shaderp->addPermutation("LIGHT_COUNT", llformat("%d", i + 1));
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		gDeferredSpotLightProgram.setup("Deferred spotlight shader",
										shader_level,
										"deferred/pointLightV.glsl",
										"deferred/spotLightF.glsl");
		gDeferredSpotLightProgram.mFeatures.hasShadows = true;
		gDeferredSpotLightProgram.mFeatures.hasSrgb = true;
		gDeferredSpotLightProgram.mFeatures.isDeferred = true;
		if (gUsePBRShaders)
		{
			gDeferredSpotLightProgram.mFeatures.hasFullGBuffer = true;
		}
		if (emissive)
		{
			gDeferredSpotLightProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredSpotLightProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredMultiSpotLightProgram;
		if (gUsePBRShaders)
		{
			shaderp->setup("Deferred multispotlight shader", shader_level,
						   "deferred/multiPointLightV.glsl",
						   "deferred/spotLightF.glsl");
			shaderp->addPermutation("MULTI_SPOTLIGHT", "1");
		}
		else
		{
			shaderp->setup("Deferred multispotlight shader", shader_level,
						   "deferred/multiPointLightV.glsl",
						   "deferred/multiSpotLightF.glsl");
		}
		shaderp->mFeatures.hasShadows = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isDeferred = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasFullGBuffer = true;
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	U32 ssao = gSavedSettings.getU32("RenderDeferredSSAO");
	bool use_ao = ssao > 1 ||
				 (ssao == 1 && gSavedSettings.getU32("RenderShadowDetail"));

	if (success)
	{
		const char* fragment;
		const char* vertex = "deferred/sunLightV.glsl";
		if (use_ao)
		{
			fragment = "deferred/sunLightSSAOF.glsl";
		}
		else
		{
			fragment = "deferred/sunLightF.glsl";
			if (shader_level == 1 && !gUsePBRShaders)
			{
				// No shadows, no SSAO, no frag coord
				vertex = "deferred/sunLightNoFragCoordV.glsl";
			}
		}
		gDeferredSunProgram.setup("Deferred Sun shader", shader_level,
								  vertex, fragment);
		gDeferredSunProgram.mFeatures.hasAmbientOcclusion = use_ao;
		gDeferredSunProgram.mFeatures.hasShadows = true;
		gDeferredSunProgram.mFeatures.isDeferred = true;
		if (emissive)
		{
			gDeferredSunProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredSunProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gDeferredSunProbeProgram.setup("Deferred Sun probe shader",
									   shader_level, "deferred/sunLightV.glsl",
									   "deferred/sunLightF.glsl");
		gDeferredSunProbeProgram.mFeatures.hasShadows = true;
		gDeferredSunProbeProgram.mFeatures.isDeferred = true;
		if (emissive)
		{
			gDeferredSunProbeProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredSunProbeProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gDeferredBlurLightProgram.setup("Deferred blur light shader",
										shader_level,
										"deferred/blurLightV.glsl",
										"deferred/blurLightF.glsl");
		gDeferredBlurLightProgram.mFeatures.isDeferred = true;
		if (emissive)
		{
			gDeferredBlurLightProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredBlurLightProgram.createShader();
	}

	bool pbr_ssr = gUsePBRShaders && LLPipeline::sRenderSSR;
	if (success && pbr_ssr)
	{
		gSSRFilterProgram.setup("SSR filter shader", shader_level,
								"deferred/blurLightV.glsl",
								"deferred/screenSpaceReflFilterF.glsl");
		gSSRFilterProgram.mFeatures.isDeferred = true;
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = gSSRFilterProgram.createShader();	
	}

	if (success)
	{
		std::string name;
		// type 0 is simple deferred alpha, 1 is skinned, 2 is HUD. HB
		U32 max_type = gUsePBRShaders ? 2 : 1;
		for (U32 type = 0; type <= max_type; ++type)
		{
			if (type == 0)
			{
				name = "Deferred alpha shader";
				shaderp = &gDeferredAlphaProgram;
				shaderp->mRiggedVariant = &gDeferredSkinnedAlphaProgram;
			}
			else if (type == 1)
			{
				name = "Skinned deferred alpha shader";
				shaderp = &gDeferredSkinnedAlphaProgram;
			}
			else
			{
				name = "Deferred HUD alpha shader";
				shaderp = &gHUDAlphaProgram;
			}
			shaderp->setup(name.c_str(), shader_level, "deferred/alphaV.glsl",
						  "deferred/alphaF.glsl");
			if (shader_level < 1 || gUsePBRShaders)
			{
				shaderp->mFeatures.mIndexedTextureChannels =
					sIndexedTextureChannels;
			}
			else
			{
				// Shave off some texture units for shadow maps
				shaderp->mFeatures.mIndexedTextureChannels =
					llmax(sIndexedTextureChannels - 6, 1);
			}
			shaderp->mFeatures.isAlphaLighting = true;
			// *HACK: to disable auto-setup of texture channels
			shaderp->mFeatures.disableTextureIndex = true;
			shaderp->mFeatures.hasShadows = use_sun_shadow;
			shaderp->mFeatures.hasSrgb = true;
			shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
			shaderp->mFeatures.calculatesAtmospherics = true;
			shaderp->mFeatures.hasAtmospherics = true;
			shaderp->mFeatures.hasGamma = true;
			if (gUsePBRShaders)
			{
				shaderp->mFeatures.hasReflectionProbes = true;
			}
			else
			{
				shaderp->mFeatures.hasTransport = true;
			}
			if (use_sun_shadow)
			{
				if (gUsePBRShaders)
				{
					shaderp->addPermutation("HAS_SUN_SHADOW", "1");
				}
				else
				{
					shaderp->addPermutation("HAS_SHADOW", "1");
				}
			}
			if (type == 1)
			{
				shaderp->mFeatures.hasObjectSkinning = true;
				shaderp->addPermutation("HAS_SKIN", "1");
			}
			else if (type == 2)
			{
				shaderp->addPermutation("IS_HUD", "1");
			}
			shaderp->addPermutation("USE_INDEXED_TEX", "1");
			shaderp->addPermutation("USE_VERTEX_COLOR", "1");
			if (gUsePBRShaders)
			{
				shaderp->addPermutation("HAS_ALPHA_MASK", "1");
			}
			if (emissive)
			{
				shaderp->addPermutation("HAS_EMISSIVE", "1");
			}
			success = shaderp->createShader();
			// *HACK: set after creation to disable auto-setup of texture
			// channels
			shaderp->mFeatures.calculatesLighting = true;
			shaderp->mFeatures.hasLighting = true;
		}
	}

	if (success)
	{
		LLGLSLShader* shaders[] = { &gDeferredAlphaImpostorProgram,
									&gDeferredSkinnedAlphaImpostorProgram };
		std::string name;
		for (U32 rigged = 0; success && rigged < 2; ++rigged)
		{
			shaderp = shaders[rigged];
			if (rigged)
			{
				name = "Skinned deferred alpha impostor shader";
			}
			else
			{
				name = "Deferred alpha impostor shader";
			}
			shaderp->setup(name.c_str(), shader_level, "deferred/alphaV.glsl",
						   "deferred/alphaF.glsl");

			if (shader_level < 1 || gUsePBRShaders)
			{
				shaderp->mFeatures.mIndexedTextureChannels =
					sIndexedTextureChannels;
			}
			else
			{
				// Shave off some texture units for shadow maps
				shaderp->mFeatures.mIndexedTextureChannels =
					llmax(sIndexedTextureChannels - 6, 1);
			}
			shaderp->mFeatures.isAlphaLighting = true;
			shaderp->mFeatures.hasSrgb = true;
			shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
			if (gUsePBRShaders)
			{
				shaderp->mFeatures.hasReflectionProbes = true;
				shaderp->addPermutation("HAS_ALPHA_MASK", "1");
			}
			if (use_sun_shadow)
			{
				shaderp->mFeatures.hasShadows = true;
				if (gUsePBRShaders)
				{
					shaderp->addPermutation("HAS_SUN_SHADOW", "1");
				}
				else
				{
					shaderp->addPermutation("HAS_SHADOW", "1");
				}
			}
			if (emissive)
			{
				shaderp->addPermutation("HAS_EMISSIVE", "1");
			}
			shaderp->addPermutation("USE_INDEXED_TEX", "1");
			shaderp->addPermutation("USE_VERTEX_COLOR", "1");
			shaderp->addPermutation("FOR_IMPOSTOR", "1");
			if (gUsePBRShaders)
			{
				shaderp->addPermutation("HAS_ALPHA_MASK", "1");
			}
			if (rigged)
			{
				shaderp->mFeatures.hasObjectSkinning = true;
				shaderp->addPermutation("HAS_SKIN", "1");
			}
			else
			{
				shaderp->mRiggedVariant = shaders[1];
			}
			success = shaderp->createShader();
			// *HACK: set after creation to disable auto-setup of texture channels
			shaderp->mFeatures.calculatesLighting = true;
			shaderp->mFeatures.hasLighting = true;
		}
	}

	if (success && !gUsePBRShaders)
	{
		LLGLSLShader* shaders[] = { &gDeferredAlphaWaterProgram,
									&gDeferredSkinnedAlphaWaterProgram };
		std::string name;
		for (U32 rigged = 0; success && rigged < 2; ++rigged)
		{
			shaderp = shaders[rigged];
			if (rigged)
			{
				name = "Skinned deferred alpha underwater shader";
			}
			else
			{
				name = "Deferred alpha underwater shader";
			}
			shaderp->setup(name.c_str(), shader_level, "deferred/alphaV.glsl",
						   "deferred/alphaF.glsl");

			shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
			if (shader_level < 1 || gUsePBRShaders)
			{
				shaderp->mFeatures.mIndexedTextureChannels =
					sIndexedTextureChannels;
			}
			else
			{
				// Shave off some texture units for shadow maps
				shaderp->mFeatures.mIndexedTextureChannels =
					llmax(sIndexedTextureChannels - 6, 1);
			}
			shaderp->mFeatures.isAlphaLighting = true;
			// *HACK: to disable auto-setup of texture channels
			shaderp->mFeatures.disableTextureIndex = true;
			shaderp->mFeatures.hasWaterFog = true;
			shaderp->mFeatures.hasSrgb = true;
			shaderp->mFeatures.encodesNormal = true;
			shaderp->mFeatures.calculatesAtmospherics = true;
			shaderp->mFeatures.hasAtmospherics = true;
			shaderp->mFeatures.hasGamma = true;
			if (gUsePBRShaders)
			{
				shaderp->mFeatures.hasReflectionProbes = true;
				shaderp->addPermutation("HAS_ALPHA_MASK", "1");
			}
			else
			{
				shaderp->mFeatures.hasTransport = true;
			}
			if (use_sun_shadow)
			{
				shaderp->mFeatures.hasShadows = true;
				if (gUsePBRShaders)
				{
					shaderp->addPermutation("HAS_SUN_SHADOW", "1");
				}
				else
				{
					shaderp->addPermutation("HAS_SHADOW", "1");
				}
			}
			shaderp->addPermutation("USE_INDEXED_TEX", "1");
			shaderp->addPermutation("WATER_FOG", "1");
			shaderp->addPermutation("USE_VERTEX_COLOR", "1");
			if (rigged)
			{
				shaderp->mFeatures.hasObjectSkinning = true;
				shaderp->addPermutation("HAS_SKIN", "1");
			}
			else
			{
				shaderp->mRiggedVariant = shaders[1];
			}
			success = shaderp->createShader();
			// *HACK: set after creation to disable auto-setup of texture channels
			shaderp->mFeatures.calculatesLighting = true;
			shaderp->mFeatures.hasLighting = true;
		}
	}

	if (success)
	{
		gDeferredAvatarEyesProgram.setup("Deferred alpha eyes shader",
										 shader_level,
										 "deferred/avatarEyesV.glsl",
										 "deferred/diffuseF.glsl");
		gDeferredAvatarEyesProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAvatarEyesProgram.mFeatures.hasGamma = true;
		if (gUsePBRShaders)
		{
			// *TODO: verify PBR really needs this. HB
			gDeferredAvatarEyesProgram.mFeatures.hasAtmospherics = true;
		}
		else
		{
			gDeferredAvatarEyesProgram.mFeatures.hasTransport = true;
		}
		gDeferredAvatarEyesProgram.mFeatures.disableTextureIndex = true;
		gDeferredAvatarEyesProgram.mFeatures.hasSrgb = true;
		// Note: ignored by PBR
		gDeferredAvatarEyesProgram.mFeatures.encodesNormal = true;
		gDeferredAvatarEyesProgram.mFeatures.hasShadows = true;
		if (emissive)
		{
			gDeferredAvatarEyesProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredAvatarEyesProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredFullbrightProgram;
		shaderp->setup("Deferred full bright shader", shader_level,
					   "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasAtmospherics = true;
		}
		else
		{
			shaderp->mFeatures.hasTransport = true;
		}
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedFullbrightProgram);
	}

 	if (success && gUsePBRShaders)
	{
		shaderp = &gHUDFullbrightProgram;
		shaderp->setup("Deferred HUD full bright shader", shader_level,
					   "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->addPermutation("IS_HUD", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		shaderp = &gDeferredFullbrightAlphaMaskProgram;
		shaderp->setup("Deferred full bright alpha masking shader",
					   shader_level, "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasAtmospherics = true;
		}
		else
		{
			shaderp->mFeatures.hasTransport = true;
		}
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->addPermutation("HAS_ALPHA_MASK", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedFullbrightAlphaMaskProgram);
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gHUDFullbrightAlphaMaskProgram;
		shaderp->setup("Deferred HUD full bright alpha masking shader",
					   shader_level, "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->addPermutation("HAS_ALPHA_MASK", "1");
		shaderp->addPermutation("IS_HUD", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredFullbrightAlphaMaskAlphaProgram;
		shaderp->setup("Deferred full bright alpha masking alpha shader",
					   shader_level, "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->addPermutation("HAS_ALPHA_MASK", "1");
		shaderp->addPermutation("IS_ALPHA", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedFullbrightAlphaMaskAlphaProgram);
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gHUDFullbrightAlphaMaskAlphaProgram;
		shaderp->setup("Deferred HUD full bright alpha masking alpha shader",
					   shader_level, "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->addPermutation("HAS_ALPHA_MASK", "1");
		shaderp->addPermutation("IS_ALPHA", "1");
		shaderp->addPermutation("IS_HUD", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gDeferredFullbrightWaterProgram;
		shaderp->setup("Deferred full bright underwater shader", shader_level,
					   "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		shaderp->addPermutation("WATER_FOG", "1");
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedFullbrightWaterProgram);
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gDeferredFullbrightAlphaMaskWaterProgram;
		shaderp->setup("Deferred full bright underwater alpha masking shader",
					   shader_level, "deferred/fullbrightV.glsl",
					   "deferred/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		shaderp->addPermutation("HAS_ALPHA_MASK","1");
		shaderp->addPermutation("WATER_FOG","1");
		success =
			create_with_rigged(*shaderp,
							   gDeferredSkinnedFullbrightAlphaMaskWaterProgram);
	}

	if (success)
	{
		shaderp = &gDeferredFullbrightShinyProgram;
		shaderp->setup("Deferred fullbrightshiny shader", shader_level,
					   "deferred/fullbrightShinyV.glsl",
					   "deferred/fullbrightShinyF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasSrgb = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasReflectionProbes = true;
			shaderp->mFeatures.mIndexedTextureChannels =
				sIndexedTextureChannels;
		}
		else
		{
			shaderp->mFeatures.hasTransport = true;
			shaderp->mFeatures.mIndexedTextureChannels =
				sIndexedTextureChannels - 1;
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedFullbrightShinyProgram);
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gHUDFullbrightShinyProgram;
		shaderp->setup("Deferred HUD full bright shiny shader", shader_level,
					   "deferred/fullbrightShinyV.glsl",
					   "deferred/fullbrightShinyF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		// *TODO: verify PBR really needs this. HB
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.hasReflectionProbes = true;
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->addPermutation("IS_HUD", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		shaderp = &gDeferredEmissiveProgram;
		shaderp->setup("Deferred emissive shader", shader_level,
					   "deferred/emissiveV.glsl", "deferred/emissiveF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		if (gUsePBRShaders)
		{
			// *TODO: verify PBR really needs this. HB
			shaderp->mFeatures.hasAtmospherics = true;
		}
		else
		{
			shaderp->mFeatures.hasTransport = true;
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedEmissiveProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gDeferredWaterProgram.setup("Deferred water shader", shader_level,
									"deferred/waterV.glsl",
									"deferred/waterF.glsl");
		gDeferredWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredWaterProgram.mFeatures.hasGamma = true;
		gDeferredWaterProgram.mFeatures.hasTransport = true;
		gDeferredWaterProgram.mFeatures.encodesNormal = true;
		gDeferredWaterProgram.mFeatures.hasSrgb = true;
		gDeferredWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gDeferredWaterProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gDeferredUnderWaterProgram.setup("Deferred under water shader",
										 shader_level,
										 "deferred/waterV.glsl",
										 "deferred/underWaterF.glsl");
		gDeferredUnderWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredUnderWaterProgram.mFeatures.hasGamma = true;
		gDeferredUnderWaterProgram.mFeatures.hasTransport = true;
		gDeferredUnderWaterProgram.mFeatures.hasWaterFog = true;
		gDeferredUnderWaterProgram.mFeatures.hasSrgb = true;
		gDeferredUnderWaterProgram.mFeatures.encodesNormal = true;
		gDeferredUnderWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gDeferredUnderWaterProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gHazeProgram.setup("Deferred haze shader", shader_level,
					 	   "deferred/softenLightV.glsl",
						   "deferred/hazeF.glsl");
		gHazeProgram.mFeatures.hasSrgb = true;
		gHazeProgram.mFeatures.calculatesAtmospherics = true;
		gHazeProgram.mFeatures.hasAtmospherics = true;
		gHazeProgram.mFeatures.hasGamma = true;
		gHazeProgram.mFeatures.isDeferred = true;
		gHazeProgram.mFeatures.hasShadows = use_sun_shadow;
		gHazeProgram.mFeatures.hasReflectionProbes = shader_level > 2;
		if (emissive)
		{
			gHazeProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gHazeProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gHazeWaterProgram.setup("Deferred water haze shader", shader_level,
					 			"deferred/waterHazeV.glsl",
								"deferred/waterHazeF.glsl");
		gHazeWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gHazeWaterProgram.mFeatures.hasSrgb = true;
		gHazeWaterProgram.mFeatures.calculatesAtmospherics = true;
		gHazeWaterProgram.mFeatures.hasAtmospherics = true;
		gHazeWaterProgram.mFeatures.hasGamma = true;
		gHazeWaterProgram.mFeatures.isDeferred = true;
		gHazeWaterProgram.mFeatures.hasShadows = use_sun_shadow;
		gHazeWaterProgram.mFeatures.hasReflectionProbes = shader_level > 2;
		if (emissive)
		{
			gHazeWaterProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gHazeWaterProgram.createShader();
	}

	S32 soften_level = shader_level;
	if (use_ao)
	{
		// When using SSAO, take screen space light map into account as if
		// shadows are enabled
		soften_level = llmax(soften_level, 2);
	}

	if (success)
	{
		shaderp = &gDeferredSoftenProgram;
		shaderp->setup("Deferred soften shader", soften_level,
					   "deferred/softenLightV.glsl",
					   "deferred/softenLightF.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.hasShadows = use_sun_shadow;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasFullGBuffer = true;
			shaderp->mFeatures.hasReflectionProbes = shader_level > 2;
			if (use_sun_shadow)
			{
				shaderp->addPermutation("HAS_SUN_SHADOW", "1");
			}
			if (use_ao)
			{
				shaderp->addPermutation("HAS_SSAO", "1");
			}
			if (emissive)
			{
				shaderp->addPermutation("HAS_EMISSIVE", "1");
			}
		}
		else
		{
			shaderp->mFeatures.hasTransport = true;
		}
		success = shaderp->createShader();
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredSoftenCubeProgram;
		shaderp->setup("Deferred soften cube shader", soften_level,
					   "deferred/softenLightV.glsl",
					   "deferred/softenLightF.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.hasShadows = use_sun_shadow;
		shaderp->mFeatures.hasFullGBuffer = true;
		shaderp->mFeatures.hasReflectionProbes = shader_level > 2;
		if (use_sun_shadow)
		{
			shaderp->addPermutation("HAS_SUN_SHADOW", "1");
		}
		if (use_ao)
		{
			shaderp->addPermutation("HAS_SSAO", "1");
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		shaderp->addPermutation("CUBE_SNAPSHOT", "1");
		success = shaderp->createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gDeferredSoftenWaterProgram;
		shaderp->setup("Deferred soften underwater shader", soften_level,
					   "deferred/softenLightV.glsl",
					   "deferred/softenLightF.glsl");
		shaderp->addPermutation("WATER_FOG", "1");
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.hasShadows = use_sun_shadow;
		success = shaderp->createShader();
	}

	std::string depth_clamp = LLGLManager::sUseDepthClamp ? "1" : "0";

	if (success)
	{
		gDeferredShadowProgram.setup("Deferred shadow shader", shader_level,
									 "deferred/shadowV.glsl",
									 "deferred/shadowF.glsl");
		if (!gUsePBRShaders)
		{
			gDeferredShadowProgram.mFeatures.isDeferred = true;
			gDeferredShadowProgram.mFeatures.hasShadows = true;
			if (LLGLManager::sUseDepthClamp)
			{
				gDeferredShadowProgram.addPermutation("DEPTH_CLAMP", "1");
			}
		}
		gDeferredShadowProgram.mRiggedVariant = &gDeferredSkinnedShadowProgram;
		success = gDeferredShadowProgram.createShader();
	}

	if (success)
	{
		gDeferredSkinnedShadowProgram.setup("Skinned deferred shadow shader",
											shader_level,
											"deferred/shadowSkinnedV.glsl",
											"deferred/shadowF.glsl");
		gDeferredSkinnedShadowProgram.mFeatures.isDeferred = true;
		gDeferredSkinnedShadowProgram.mFeatures.hasShadows = true;
		gDeferredSkinnedShadowProgram.mFeatures.hasObjectSkinning = true;
		if (LLGLManager::sUseDepthClamp && !gUsePBRShaders)
		{
			gDeferredSkinnedShadowProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		if (emissive)
		{
			gDeferredSkinnedShadowProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredSkinnedShadowProgram.createShader();
	}

	if (success)
	{
		gDeferredShadowCubeProgram.setup("Deferred shadow cube shader",
										 shader_level,
										 "deferred/shadowCubeV.glsl",
										 "deferred/shadowF.glsl");
		gDeferredShadowCubeProgram.mFeatures.isDeferred = true;
		gDeferredShadowCubeProgram.mFeatures.hasShadows = true;
		if (LLGLManager::sUseDepthClamp && !gUsePBRShaders)
		{
			gDeferredShadowCubeProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		success = gDeferredShadowCubeProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredShadowFullbrightAlphaMaskProgram;
		shaderp->setup("Deferred shadow full bright alpha mask shader",
					   shader_level, "deferred/shadowAlphaMaskV.glsl",
					   "deferred/shadowAlphaMaskF.glsl");
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		if (LLGLManager::sUseDepthClamp || gUsePBRShaders)
		{
			shaderp->addPermutation("DEPTH_CLAMP", "1");
		}
		shaderp->addPermutation("IS_FULLBRIGHT", "1");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		if (gUsePBRShaders)
		{
			success =
				create_with_rigged(*shaderp,
								   gDeferredSkinnedShadowFullbrightAlphaMaskProgram);
		}
		else
		{
			shaderp->mRiggedVariant =
				&gDeferredSkinnedShadowFullbrightAlphaMaskProgram;
			success = shaderp->createShader();
		}
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gDeferredSkinnedShadowFullbrightAlphaMaskProgram;
		shaderp->setup("Skinned deferred shadow full bright alpha mask shader",
					   shader_level, "deferred/shadowAlphaMaskSkinnedV.glsl",
					   "deferred/shadowAlphaMaskF.glsl");
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		shaderp->mFeatures.hasObjectSkinning = true;
		if (LLGLManager::sUseDepthClamp)
		{
			shaderp->addPermutation("DEPTH_CLAMP", "1");
		}
		shaderp->addPermutation("IS_FULLBRIGHT", "1");
		success = shaderp->createShader();
	}

	if (success)
	{
		shaderp = &gDeferredShadowAlphaMaskProgram;
		shaderp->setup("Deferred shadow alpha mask shader", shader_level,
					   "deferred/shadowAlphaMaskV.glsl",
					   "deferred/shadowAlphaMaskF.glsl");
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		if (LLGLManager::sUseDepthClamp && !gUsePBRShaders)
		{
			shaderp->addPermutation("DEPTH_CLAMP", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedShadowAlphaMaskProgram);
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredShadowGLTFAlphaMaskProgram;
		shaderp->setup("Deferred glTF shadow alpha mask shader", shader_level,
					   "deferred/pbrShadowAlphaMaskV.glsl",
					   "deferred/pbrShadowAlphaMaskF.glsl");
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedShadowGLTFAlphaMaskProgram);
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredShadowGLTFAlphaBlendProgram;
		shaderp->setup("Deferred glTF shadow alpha blend shader", shader_level,
					   "deferred/pbrShadowAlphaMaskV.glsl",
					   "deferred/pbrShadowAlphaBlendF.glsl");
		shaderp->mFeatures.mIndexedTextureChannels = sIndexedTextureChannels;
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = create_with_rigged(*shaderp,
									 gDeferredSkinnedShadowGLTFAlphaBlendProgram);
	}

	if (success)
	{
		gDeferredAvatarShadowProgram.setup("Deferred avatar shadow shader",
										   shader_level,
										   "deferred/avatarShadowV.glsl",
										   "deferred/avatarShadowF.glsl");
		gDeferredAvatarShadowProgram.mFeatures.hasSkinning = true;
		if (LLGLManager::sUseDepthClamp && !gUsePBRShaders)
		{
			gDeferredAvatarShadowProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		success = gDeferredAvatarShadowProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredAvatarAlphaShadowProgram;
		shaderp->setup("Deferred avatar alpha shadow shader", shader_level,
					   "deferred/avatarAlphaShadowV.glsl",
					   "deferred/avatarAlphaShadowF.glsl");
		shaderp->mFeatures.hasSkinning = true;
		if (!gUsePBRShaders)
		{
			shaderp->addPermutation("DEPTH_CLAMP", depth_clamp);
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		shaderp = &gDeferredAvatarAlphaMaskShadowProgram;
		shaderp->setup("Deferred avatar alpha mask shadow shader",
					   shader_level, "deferred/avatarAlphaShadowV.glsl",
					   "deferred/avatarAlphaMaskShadowF.glsl");
		shaderp->mFeatures.hasSkinning = true;
		if (!gUsePBRShaders)
		{
			shaderp->addPermutation("DEPTH_CLAMP", depth_clamp);
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		gDeferredTerrainProgram.setup("Deferred terrain shader", shader_level,
									  "deferred/terrainV.glsl",
									  "deferred/terrainF.glsl");
		// Note: ignored by PBR
		gDeferredTerrainProgram.mFeatures.encodesNormal = true;
		gDeferredTerrainProgram.mFeatures.hasSrgb = true;
		if (gUsePBRShaders)
		{
			gDeferredTerrainProgram.mFeatures.isAlphaLighting = true;
		}
		else
		{
			gDeferredTerrainProgram.mFeatures.hasWaterFog = true;
			gDeferredTerrainProgram.mFeatures.hasTransport = true;
		}
		gDeferredTerrainProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredTerrainProgram.mFeatures.hasAtmospherics = true;
		gDeferredTerrainProgram.mFeatures.hasGamma = true;
		// *HACK: to disable auto-setup of texture channels
		gDeferredTerrainProgram.mFeatures.disableTextureIndex = true;
		if (emissive)
		{
			gDeferredTerrainProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredTerrainProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gDeferredTerrainWaterProgram;
		shaderp->setup("Deferred terrain underwater shader", shader_level,
					   "deferred/terrainV.glsl", "deferred/terrainF.glsl");
		shaderp->mFeatures.encodesNormal = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasTransport = true;
		// *HACK: to disable auto-setup of texture channels
		shaderp->mFeatures.disableTextureIndex = true;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		shaderp->addPermutation("WATER_FOG", "1");
		success = shaderp->createShader();
	}

	if (success)
	{
		shaderp = &gDeferredAvatarProgram;
		shaderp->setup("Deferred avatar shader", shader_level,
					   "deferred/avatarV.glsl", "deferred/avatarF.glsl");
		shaderp->mFeatures.hasSkinning = true;
		shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
		if (LLPipeline::sRenderAvatarCloth)
		{
			shaderp->addPermutation("AVATAR_CLOTH", "1");
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		shaderp = &gDeferredAvatarAlphaProgram;
		shaderp->setup("Deferred avatar alpha shader", shader_level,
					   "deferred/alphaV.glsl", "deferred/alphaF.glsl");
		shaderp->mFeatures.hasSkinning = true;
		shaderp->mFeatures.isAlphaLighting = true;
		shaderp->mFeatures.disableTextureIndex = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.encodesNormal = true;	// Note: ignored by PBR
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasAtmospherics = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasReflectionProbes = true;
		}
		else
		{
			shaderp->mFeatures.hasTransport = true;
		}
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.hasShadows = true;
		if (use_sun_shadow)
		{
			if (gUsePBRShaders)
			{
				shaderp->addPermutation("HAS_SUN_SHADOW", "1");
			}
			else
			{
				shaderp->addPermutation("HAS_SHADOW", "1");
			}
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		shaderp->addPermutation("USE_DIFFUSE_TEX", "1");
		shaderp->addPermutation("IS_AVATAR_SKIN", "1");
		success = shaderp->createShader();
		// *HACK: set after creation to disable auto-setup of texture channels
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.hasLighting = true;
	}

	if (success && gUsePBRShaders)
	{
		gExposureProgram.setup("Deferred exposure shader", shader_level,
							   "deferred/postDeferredNoTCV.glsl",
							   "deferred/exposureF.glsl");
		gExposureProgram.mFeatures.hasSrgb = true;
		gExposureProgram.mFeatures.isDeferred = true;
		gExposureProgram.addPermutation("USE_LAST_EXPOSURE", "1");
		success = gExposureProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gExposureProgramNoFade.setup("Deferred exposure (no fade) shader",
									 shader_level,
									 "deferred/postDeferredNoTCV.glsl",
									 "deferred/exposureF.glsl");
		gExposureProgramNoFade.mFeatures.hasSrgb = true;
		gExposureProgramNoFade.mFeatures.isDeferred = true;
		success = gExposureProgramNoFade.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gLuminanceProgram.setup("Deferred luminance shader", shader_level,
							   "deferred/postDeferredNoTCV.glsl",
							   "deferred/luminanceF.glsl");
		success = gLuminanceProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredPostGammaCorrectProgram;
		shaderp->setup("Deferred gamma correction post process", shader_level,
					   "deferred/postDeferredNoTCV.glsl",
					   "deferred/postDeferredGammaCorrect.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isDeferred = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasGamma = true;
		}
		success = shaderp->createShader();
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gLegacyPostGammaCorrectProgram;
		shaderp->setup("Deferred legacy gamma correction post process",
					   shader_level, "deferred/postDeferredNoTCV.glsl",
					   "deferred/postDeferredGammaCorrect.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->addPermutation("LEGACY_GAMMA", "1");
		success = shaderp->createShader();
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredPostTonemapProgram;
		shaderp->setup("Deferred tone map post process",
					   shader_level, "deferred/postDeferredNoTCV.glsl",
					   "deferred/postDeferredTonemap.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isDeferred = true;
		success = shaderp->createShader();
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gNoPostTonemapProgram;
		shaderp->setup("No post tone map post process",
					   shader_level, "deferred/postDeferredNoTCV.glsl",
					   "deferred/postDeferredTonemap.glsl");
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.isDeferred = true;
		shaderp->addPermutation("NO_POST", "1");
		success = shaderp->createShader();
	}

	static const char* levels[4] = { " low quality shader",
									 " medium quality shader",
									 " high quality shader",
									 " ultra quality shader" };

	if (success)
	{
		static const char* qualities[4] = { "12", "23", "29", "39" };
		sHasFXAA = true;	// Would be set to false on error
		for (U32 i = 0; sHasFXAA && i < 4; ++i)
		{
			shaderp = &gFXAAProgram[i];
			shaderp->setup(llformat("FXAA%s", levels[i]).c_str(), shader_level,
						   "deferred/postDeferredV.glsl",
						   "deferred/fxaaF.glsl");
			shaderp->mFeatures.isDeferred = true;
			shaderp->addPermutation("FXAA_QUALITY__PRESET", qualities[i]);
			sHasFXAA = shaderp->createShader();
		}
		if (!sHasFXAA)
		{
			llwarns << "Failed to compile FXAA shaders. Not a show stopper; ignored."
					<<llendl;
		}
	}

	if (success)
	{
		static const char* qualities[4] = { "SMAA_PRESET_LOW",
											"SMAA_PRESET_MEDIUM",
											"SMAA_PRESET_HIGH",
											"SMAA_PRESET_ULTRA" };
		strings_map_t defines;
		if (LLGLManager::sGLVersion >= 4.f)
		{
			defines.emplace("SMAA_GLSL_4", "1");
		}
		else if (LLGLManager::sGLVersion >= 3.1f)
		{
			defines.emplace("SMAA_GLSL_3", "1");
		}
		else
		{
			defines.emplace("SMAA_GLSL_2", "1");
		}
		defines.emplace("SMAA_PREDICATION", "0");
		defines.emplace("SMAA_REPROJECTION", "0");
		sHasSMAA = true;	// Would be set to false on error

		for (U32 i = 0; sHasSMAA && i < 4; ++i)
		{
			shaderp = &gPostSMAAEdgeDetect[i];
			shaderp->setup(llformat("SMAA edge detection%s", levels[i]).c_str(),
						   shader_level, "deferred/SMAAEdgeDetectV.glsl",
						   "deferred/SMAAEdgeDetectF.glsl");
			shaderp->mShaderFiles.emplace_back("deferred/SMAAV.glsl",
											   GL_VERTEX_SHADER);
			shaderp->mShaderFiles.emplace_back("deferred/SMAAF.glsl",
											   GL_FRAGMENT_SHADER);
			shaderp->mFeatures.isDeferred = true;
			shaderp->addPermutation(qualities[i], "1");
			shaderp->addPermutations(defines);
			sHasSMAA = shaderp->createShader();
			if (sHasSMAA && !gUsePBRShaders)
			{
				shaderp->bind();
				shaderp->uniform1i(sTex0, 0);
				shaderp->uniform1i(sTex1, 1);
				shaderp->unbind();
			}
		}

		for (U32 i = 0; sHasSMAA && i < 4; ++i)
		{
			shaderp = &gPostSMAABlendWeights[i];
			shaderp->setup(llformat("SMAA weights blending%s",
									levels[i]).c_str(),
						   shader_level, "deferred/SMAABlendWeightsV.glsl",
						   "deferred/SMAABlendWeightsF.glsl");
			shaderp->mShaderFiles.emplace_back("deferred/SMAAV.glsl",
											   GL_VERTEX_SHADER);
			shaderp->mShaderFiles.emplace_back("deferred/SMAAF.glsl",
											   GL_FRAGMENT_SHADER);
			shaderp->mFeatures.isDeferred = true;
			shaderp->addPermutation(qualities[i], "1");
			shaderp->addPermutations(defines);
			sHasSMAA = shaderp->createShader();
			if (sHasSMAA && !gUsePBRShaders)
			{
				shaderp->bind();
				shaderp->uniform1i(sTex0, 0);
				shaderp->uniform1i(sTex1, 1);
				shaderp->uniform1i(sTex2, 2);
				shaderp->unbind();
			}
		}

		for (U32 i = 0; sHasSMAA && i < 4; ++i)
		{
			shaderp = &gPostSMAANeighborhoodBlend[i];
			shaderp->setup(llformat("SMAA neighborhood blending%s",
									levels[i]).c_str(),
						   shader_level,
						   "deferred/SMAANeighborhoodBlendV.glsl",
						   "deferred/SMAANeighborhoodBlendF.glsl");
			shaderp->mShaderFiles.emplace_back("deferred/SMAAV.glsl",
											   GL_VERTEX_SHADER);
			shaderp->mShaderFiles.emplace_back("deferred/SMAAF.glsl",
											   GL_FRAGMENT_SHADER);
			shaderp->mFeatures.isDeferred = true;
			shaderp->addPermutation(qualities[i], "1");
			shaderp->addPermutations(defines);
			sHasSMAA = shaderp->createShader();
			if (sHasSMAA && !gUsePBRShaders)
			{
				shaderp->bind();
				shaderp->uniform1i(sTex0, 0);
				shaderp->uniform1i(sTex1, 1);
				shaderp->uniform1i(sTex2, 2);
				shaderp->unbind();
			}
		}
		if (!sHasSMAA)
		{
			llwarns << "Failed to compile SMAA shaders. Not a show stopper; ignored."
					<<llendl;
		}
	}

	if (success)
	{
		gPostCASProgram.setup("Contrast adaptive sharpen shader", shader_level,
							  "deferred/postNoTCV.glsl", "deferred/CASF.glsl");
		sHasCAS = gPostCASProgram.createShader();
		if (!sHasCAS)
		{
			llwarns << "Failed to compile CAS shader. Not a show stopper; ignored."
					<<llendl;
		}
	}

	if (success)
	{
		gDeferredPostProgram.setup("Deferred post shader", shader_level,
								   "deferred/postDeferredNoTCV.glsl",
								   "deferred/postDeferredF.glsl");
		gDeferredPostProgram.mFeatures.isDeferred = true;
		success = gDeferredPostProgram.createShader();
	}

	if (success)
	{
		gDeferredCoFProgram.setup("Deferred CoF shader", shader_level,
								  "deferred/postDeferredNoTCV.glsl",
								  "deferred/cofF.glsl");
		gDeferredCoFProgram.mFeatures.isDeferred = true;
		success = gDeferredCoFProgram.createShader();
	}

	if (success)
	{
		gDeferredDoFCombineProgram.setup("Deferred DoF combine shader",
										 shader_level,
										 "deferred/postDeferredNoTCV.glsl",
										 "deferred/dofCombineF.glsl");
		gDeferredDoFCombineProgram.mFeatures.isDeferred = true;
		success = gDeferredDoFCombineProgram.createShader();
	}

	if (success)
	{
		gDeferredPostNoDoFProgram.setup("Deferred post no DoF shader",
										shader_level,
										"deferred/postDeferredNoTCV.glsl",
										"deferred/postDeferredNoDoFF.glsl");
		gDeferredPostNoDoFProgram.mFeatures.isDeferred = true;
		success = gDeferredPostNoDoFProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredPostNoDoFNoiseProgram;
		shaderp->setup("Deferred post no DoF noise shader", shader_level,
					   "deferred/postDeferredNoTCV.glsl",
					   "deferred/postDeferredNoDoFF.glsl");
		gDeferredPostNoDoFNoiseProgram.mFeatures.isDeferred = true;
		gDeferredPostNoDoFNoiseProgram.addPermutation("HAS_NOISE", "1");
		success = gDeferredPostNoDoFNoiseProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gEnvironmentMapProgram.setup("Deferred environment map shader",
									 shader_level,
									 "deferred/skyV.glsl",
									 "deferred/skyF.glsl");
		gEnvironmentMapProgram.mFeatures.calculatesAtmospherics = true;
		gEnvironmentMapProgram.mFeatures.hasAtmospherics = true;
		gEnvironmentMapProgram.mFeatures.hasGamma = true;
		gEnvironmentMapProgram.mFeatures.hasSrgb = true;
		gEnvironmentMapProgram.addPermutation("HAS_HDRI", "1");
		gEnvironmentMapProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		if (emissive)
		{
			gEnvironmentMapProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gEnvironmentMapProgram.createShader();
	}

	if (success)
	{
		gDeferredWLSkyProgram.setup("Deferred Windlight sky shader",
									shader_level,
									"deferred/skyV.glsl",
									"deferred/skyF.glsl");
		gDeferredWLSkyProgram.mFeatures.calculatesAtmospherics = true;
		if (gUsePBRShaders)
		{
			gDeferredWLSkyProgram.mFeatures.hasAtmospherics = true;
		}
		else
		{
			gDeferredWLSkyProgram.mFeatures.hasTransport = true;
		}
		gDeferredWLSkyProgram.mFeatures.hasGamma = true;
		gDeferredWLSkyProgram.mFeatures.hasSrgb = true;
		gDeferredWLSkyProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		if (emissive)
		{
			gDeferredWLSkyProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredWLSkyProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredWLCloudProgram;
		shaderp->setup("Deferred Windlight cloud shader", shader_level,
					   "deferred/cloudsV.glsl", "deferred/cloudsF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasSrgb = true;
		if (gUsePBRShaders)
		{
			shaderp->mFeatures.hasAtmospherics = true;
		}
		else
		{
			shaderp->mFeatures.hasTransport = true;
		}
		shaderp->addConstant(LLGLSLShader::CONST_CLOUD_MOON_DEPTH);
		shaderp->mShaderGroup = LLGLSLShader::SG_SKY;
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		gDeferredWLSunProgram.setup("Deferred Windlight Sun program",
									shader_level,
									"deferred/sunDiscV.glsl",
									"deferred/sunDiscF.glsl");
		gDeferredWLSunProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredWLSunProgram.mFeatures.hasAtmospherics = true;
		gDeferredWLSunProgram.mFeatures.hasGamma = true;
		gDeferredWLSunProgram.mFeatures.disableTextureIndex = true;
		gDeferredWLSunProgram.mFeatures.hasSrgb = true;
		if (!gUsePBRShaders)
		{
			gDeferredWLSunProgram.mFeatures.hasTransport = true;
			gDeferredWLSunProgram.mFeatures.isFullbright = true;
		}
		gDeferredWLSunProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		if (emissive)
		{
			gDeferredWLSunProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredWLSunProgram.createShader();
	}

	if (success)
	{
		shaderp = &gDeferredWLMoonProgram;
		shaderp->setup("Deferred Windlight Moon program", shader_level,
					   "deferred/moonV.glsl", "deferred/moonF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.disableTextureIndex = true;
		if (!gUsePBRShaders)
		{
			shaderp->mFeatures.hasTransport = true;
			shaderp->mFeatures.isFullbright = true;
		}
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		shaderp->mShaderGroup = LLGLSLShader::SG_SKY;
		shaderp->addConstant(LLGLSLShader::CONST_CLOUD_MOON_DEPTH);
		success = shaderp->createShader();
	}

	if (success)
	{
		gDeferredStarProgram.setup("Deferred star program", shader_level,
								   "deferred/starsV.glsl",
								   "deferred/starsF.glsl");
		gDeferredStarProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		gDeferredStarProgram.addConstant(LLGLSLShader::CONST_STAR_DEPTH);
		if (emissive)
		{
			gDeferredStarProgram.addPermutation("HAS_EMISSIVE", "1");
		}
		success = gDeferredStarProgram.createShader();
	}

	if (success)
	{
		gNormalMapGenProgram.setup("Normal map generation program",
								   shader_level, "deferred/normgenV.glsl",
								   "deferred/normgenF.glsl");
		gNormalMapGenProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gNormalMapGenProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gDeferredGenBrdfLutProgram.setup("Brdf generation program",
								   shader_level, "deferred/genbrdflutV.glsl",
								   "deferred/genbrdflutF.glsl");
		sHasLut = gDeferredGenBrdfLutProgram.createShader();
		if (!sHasLut)
		{
			llwarns << "Failed to compile Lut shader. Not a show stopper; ignored."
					<<llendl;
		}
	}

	if (success && pbr_ssr)
	{
		shaderp = &gScreenSpaceReflTraceProgram;
		shaderp->setup("SSR trace shader", 3,
					   "deferred/screenSpaceReflPostV.glsl",
					   "deferred/screenSpaceReflTraceF.glsl");
		shaderp->mFeatures.isDeferred = true;
		shaderp->mFeatures.hasScreenSpaceReflections = true;
		shaderp->mFeatures.hasFullGBuffer = true;
		success = shaderp->createShader();	
	}

	if (success && pbr_ssr)
	{
		gSSRAlphaProgram.setup("SSR alpha shader", 3,
							   "deferred/screenSpaceReflAlphaV.glsl",
							   "deferred/screenSpaceReflAlphaF.glsl");
		gSSRAlphaProgram.mFeatures.isDeferred = true;
		gSSRAlphaProgram.mFeatures.hasScreenSpaceReflections = true;
		success = create_with_rigged(gSSRAlphaProgram,
									 gSkinnedSSRAlphaProgram);
	}

	if (success && pbr_ssr)
	{
		gSSRWaterProgram.setup("SSR water shader", 3,
							   "environment/waterV.glsl",
							   "deferred/screenSpaceReflWaterF.glsl");
		gSSRWaterProgram.mFeatures.isDeferred = true;
		gSSRWaterProgram.mFeatures.hasScreenSpaceReflections = true;
		gSSRWaterProgram.mFeatures.hasAtmospherics = true;
		gSSRWaterProgram.mFeatures.calculatesAtmospherics = true;
		gSSRWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gSSRWaterProgram.createShader();	
	}

	if (success && gUsePBRShaders)
	{
		shaderp = &gDeferredBufferVisualProgram;
		shaderp->setup("Deferred buffer visualization shader", shader_level,
					   "deferred/postDeferredNoTCV.glsl",
					   "deferred/postDeferredVisualizeBuffers.glsl");
		if (emissive)
		{
			shaderp->addPermutation("HAS_EMISSIVE", "1");
		}
		success = shaderp->createShader();
	}

	if (success)
	{
		llinfos << "Deferred shaders loaded at level: " << shader_level
				<< llendl;
	}

	return success;
}

bool LLViewerShaderMgr::loadShadersObject()
{
	S32 shader_level = mShaderLevel[SHADER_OBJECT];

	LLGLSLShader* shaderp;
	bool success = true;
	if (!gUsePBRShaders)
	{
		shaderp = &gObjectSimpleNonIndexedTexGenProgram;
		shaderp->setup("Non indexed tex-gen shader", shader_level,
					   "objects/simpleTexGenV.glsl", "objects/simpleF.glsl");
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasLighting = true;
		shaderp->mFeatures.disableTextureIndex = true;
		success = shaderp->createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectSimpleNonIndexedTexGenWaterProgram;
		shaderp->setup("Non indexed tex-gen water shader", shader_level,
					   "objects/simpleTexGenV.glsl",
					   "objects/simpleWaterF.glsl");
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasLighting = true;
		shaderp->mFeatures.disableTextureIndex = true;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		success = shaderp->createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectAlphaMaskNonIndexedProgram;
		shaderp->setup("Non indexed alpha mask shader", shader_level,
					   "objects/simpleNonIndexedV.glsl",
					   "objects/simpleF.glsl");
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasLighting = true;
		shaderp->mFeatures.disableTextureIndex = true;
		shaderp->mFeatures.hasAlphaMask = true;
		success = shaderp->createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectAlphaMaskNonIndexedWaterProgram;
		shaderp->setup("Non indexed alpha mask water shader", shader_level,
					   "objects/simpleNonIndexedV.glsl",
					   "objects/simpleWaterF.glsl");
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasLighting = true;
		shaderp->mFeatures.disableTextureIndex = true;
		shaderp->mFeatures.hasAlphaMask = true;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		success = shaderp->createShader();
	}

	if (success)
	{
		gObjectAlphaMaskNoColorProgram.setup("No color alpha mask shader",
											 shader_level,
											 "objects/simpleNoColorV.glsl",
											 "objects/simpleF.glsl");
		gObjectAlphaMaskNoColorProgram.mFeatures.calculatesLighting = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.calculatesAtmospherics = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasGamma = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasAtmospherics = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasLighting = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.disableTextureIndex = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasAlphaMask = true;
		success = gObjectAlphaMaskNoColorProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectAlphaMaskNoColorWaterProgram;
		shaderp->setup("No color alpha mask water shader", shader_level,
					   "objects/simpleNoColorV.glsl",
					   "objects/simpleWaterF.glsl");
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasLighting = true;
		shaderp->mFeatures.disableTextureIndex = true;
		shaderp->mFeatures.hasAlphaMask = true;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		success = shaderp->createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gTreeProgram.setup("Tree shader", shader_level,
						   "objects/treeV.glsl", "objects/simpleF.glsl");
		gTreeProgram.mFeatures.calculatesLighting = true;
		gTreeProgram.mFeatures.calculatesAtmospherics = true;
		gTreeProgram.mFeatures.hasGamma = true;
		gTreeProgram.mFeatures.hasAtmospherics = true;
		gTreeProgram.mFeatures.hasLighting = true;
		gTreeProgram.mFeatures.disableTextureIndex = true;
		gTreeProgram.mFeatures.hasAlphaMask = true;
		success = gTreeProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gTreeWaterProgram.setup("Tree water shader", shader_level,
								"objects/treeV.glsl",
								"objects/simpleWaterF.glsl");
		gTreeWaterProgram.mFeatures.calculatesLighting = true;
		gTreeWaterProgram.mFeatures.calculatesAtmospherics = true;
		gTreeWaterProgram.mFeatures.hasWaterFog = true;
		gTreeWaterProgram.mFeatures.hasAtmospherics = true;
		gTreeWaterProgram.mFeatures.hasLighting = true;
		gTreeWaterProgram.mFeatures.disableTextureIndex = true;
		gTreeWaterProgram.mFeatures.hasAlphaMask = true;
		gTreeWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gTreeWaterProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectFullbrightNoColorWaterProgram;
		shaderp->setup("Non indexed no color full bright water shader",
					   shader_level, "objects/fullbrightNoColorV.glsl",
					   "objects/fullbrightWaterF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.isFullbright = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.disableTextureIndex = true;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		success = shaderp->createShader();
	}

	if (success)
	{
		gImpostorProgram.setup("Impostor shader", shader_level,
							   "objects/impostorV.glsl",
							   "objects/impostorF.glsl");
		gImpostorProgram.mFeatures.disableTextureIndex = true;
		gImpostorProgram.mFeatures.hasSrgb = true;
		success = gImpostorProgram.createShader();
	}

	if (success)
	{
		gObjectPreviewProgram.setup("Preview shader", shader_level,
								    "objects/previewV.glsl",
								    "objects/previewF.glsl");
		gObjectPreviewProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectPreviewProgram.mFeatures.disableTextureIndex = true;
		if (gUsePBRShaders)
		{
			success = create_with_rigged(gObjectPreviewProgram,
										 gSkinnedObjectPreviewProgram);
			gSkinnedObjectPreviewProgram.mFeatures.hasLighting = true;
		}
		else
		{
			success = gObjectPreviewProgram.createShader();
		}
		// *HACK: set after creation to disable auto-setup of texture channels
		gObjectPreviewProgram.mFeatures.hasLighting = true;
	}

	if (success && gUsePBRShaders)
	{
		gPhysicsPreviewProgram.setup("Preview physics shader", shader_level,
									 "objects/previewPhysicsV.glsl",
									 "objects/previewPhysicsF.glsl");
		gPhysicsPreviewProgram.mFeatures.mIndexedTextureChannels = 0;
		success = gPhysicsPreviewProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gObjectSimpleProgram.setup("Simple shader", shader_level,
								   "objects/simpleV.glsl",
								   "objects/simpleF.glsl");
		gObjectSimpleProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleProgram.mFeatures.hasGamma = true;
		gObjectSimpleProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleProgram.mFeatures.hasLighting = true;
		gObjectSimpleProgram.mFeatures.mIndexedTextureChannels = 0;
		success = create_with_rigged(gObjectSimpleProgram,
									 gSkinnedObjectSimpleProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectSimpleImpostorProgram.setup("Simple impostor shader",
										   shader_level,
										   "objects/simpleV.glsl",
										   "objects/simpleF.glsl");
		gObjectSimpleImpostorProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleImpostorProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleImpostorProgram.mFeatures.hasGamma = true;
		gObjectSimpleImpostorProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleImpostorProgram.mFeatures.hasLighting = true;
		gObjectSimpleImpostorProgram.mFeatures.mIndexedTextureChannels = 0;
		// Force alpha mask version of lighting so we can weed out transparent
		// pixels from impostor temp buffer:
		gObjectSimpleImpostorProgram.mFeatures.hasAlphaMask = true;
		success = create_with_rigged(gObjectSimpleImpostorProgram,
									 gSkinnedObjectSimpleImpostorProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectSimpleWaterProgram.setup("Simple water shader", shader_level,
									    "objects/simpleV.glsl",
									    "objects/simpleWaterF.glsl");
		gObjectSimpleWaterProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleWaterProgram.mFeatures.hasWaterFog = true;
		gObjectSimpleWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleWaterProgram.mFeatures.hasLighting = true;
		gObjectSimpleWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectSimpleWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = create_with_rigged(gObjectSimpleWaterProgram,
									 gSkinnedObjectSimpleWaterProgram);
	}

	if (success)
	{
		gObjectBumpProgram.setup("Bump shader", shader_level,
								 "objects/bumpV.glsl", "objects/bumpF.glsl");
		// Note: ignored by PBR
		gObjectBumpProgram.mFeatures.encodesNormal = true;
		success = create_with_rigged(gObjectBumpProgram,
									 gSkinnedObjectBumpProgram);
		// LLDrawpoolBump assumes "texture0" has channel 0 and "texture1" has
		// channel 1
		LLGLSLShader* shader[] = { &gObjectBumpProgram,
								   &gSkinnedObjectBumpProgram };
		for (U32 rigged = 0; success && rigged < 2; ++rigged)
		{
			shader[rigged]->bind();
			shader[rigged]->uniform1i(sTexture0, 0);
			shader[rigged]->uniform1i(sTexture1, 1);
			shader[rigged]->unbind();
		}
	}

	if (success && !gUsePBRShaders)
	{
		gObjectSimpleAlphaMaskProgram.setup("Simple alpha mask shader",
											shader_level,
											"objects/simpleV.glsl",
											"objects/simpleF.glsl");
		gObjectSimpleAlphaMaskProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasGamma = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasLighting = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasAlphaMask = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.mIndexedTextureChannels = 0;
		success = create_with_rigged(gObjectSimpleAlphaMaskProgram,
									 gSkinnedObjectSimpleAlphaMaskProgram);
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectSimpleWaterAlphaMaskProgram;
		shaderp->setup("Simple water alpha mask shader", shader_level,
					   "objects/simpleV.glsl", "objects/simpleWaterF.glsl");
		shaderp->mFeatures.calculatesLighting = true;
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasAtmospherics = true;
		shaderp->mFeatures.hasLighting = true;
		shaderp->mFeatures.hasAlphaMask = true;
		shaderp->mFeatures.mIndexedTextureChannels = 0;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		success =
			create_with_rigged(*shaderp,
							   gSkinnedObjectSimpleWaterAlphaMaskProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectFullbrightProgram.setup("Fullbright shader", shader_level,
									   "objects/fullbrightV.glsl",
									   "objects/fullbrightF.glsl");
		gObjectFullbrightProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightProgram.mFeatures.hasGamma = true;
		gObjectFullbrightProgram.mFeatures.hasTransport = true;
		gObjectFullbrightProgram.mFeatures.isFullbright = true;
		gObjectFullbrightProgram.mFeatures.hasSrgb = true;
		gObjectFullbrightProgram.mFeatures.mIndexedTextureChannels = 0;
		success = create_with_rigged(gObjectFullbrightProgram,
									 gSkinnedObjectFullbrightProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectFullbrightWaterProgram.setup("Fullbright water shader",
											shader_level,
											"objects/fullbrightV.glsl",
											"objects/fullbrightWaterF.glsl");
		gObjectFullbrightWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightWaterProgram.mFeatures.isFullbright = true;
		gObjectFullbrightWaterProgram.mFeatures.hasWaterFog = true;
		gObjectFullbrightWaterProgram.mFeatures.hasTransport = true;
		gObjectFullbrightWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = create_with_rigged(gObjectFullbrightWaterProgram,
									 gSkinnedObjectFullbrightWaterProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectEmissiveProgram.setup("Emissive shader", shader_level,
									 "objects/emissiveV.glsl",
									 "objects/fullbrightF.glsl");
		gObjectEmissiveProgram.mFeatures.calculatesAtmospherics = true;
		gObjectEmissiveProgram.mFeatures.hasGamma = true;
		gObjectEmissiveProgram.mFeatures.hasTransport = true;
		gObjectEmissiveProgram.mFeatures.isFullbright = true;
		gObjectEmissiveProgram.mFeatures.hasSrgb = true;
		gObjectEmissiveProgram.mFeatures.mIndexedTextureChannels = 0;
		success = create_with_rigged(gObjectEmissiveProgram,
									 gSkinnedObjectEmissiveProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectEmissiveWaterProgram.setup("Emissive water shader",
										  shader_level,
										  "objects/emissiveV.glsl",
										  "objects/fullbrightWaterF.glsl");
		gObjectEmissiveWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectEmissiveWaterProgram.mFeatures.isFullbright = true;
		gObjectEmissiveWaterProgram.mFeatures.hasWaterFog = true;
		gObjectEmissiveWaterProgram.mFeatures.hasTransport = true;
		gObjectEmissiveWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectEmissiveWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = create_with_rigged(gObjectEmissiveWaterProgram,
									 gSkinnedObjectEmissiveWaterProgram);
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectFullbrightAlphaMaskProgram;
		shaderp->setup("Fullbright alpha mask shader", shader_level,
					   "objects/fullbrightV.glsl", "objects/fullbrightF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.isFullbright = true;
		shaderp->mFeatures.hasAlphaMask = true;
		shaderp->mFeatures.hasSrgb = true;
		shaderp->mFeatures.mIndexedTextureChannels = 0;
		success = create_with_rigged(*shaderp,
									 gSkinnedObjectFullbrightAlphaMaskProgram);
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectFullbrightWaterAlphaMaskProgram;
		shaderp->setup("Fullbright water alpha mask shader", shader_level,
					   "objects/fullbrightV.glsl",
					   "objects/fullbrightWaterF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.isFullbright = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.hasAlphaMask = true;
		shaderp->mFeatures.mIndexedTextureChannels = 0;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		success =
			create_with_rigged(*shaderp,
							   gSkinnedObjectFullbrightWaterAlphaMaskProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectShinyProgram.setup("Shiny shader", shader_level,
								  "objects/shinyV.glsl",
								  "objects/shinyF.glsl");
		gObjectShinyProgram.mFeatures.calculatesAtmospherics = true;
		gObjectShinyProgram.mFeatures.calculatesLighting = true;
		gObjectShinyProgram.mFeatures.hasGamma = true;
		gObjectShinyProgram.mFeatures.hasAtmospherics = true;
		gObjectShinyProgram.mFeatures.isShiny = true;
		gObjectShinyProgram.mFeatures.mIndexedTextureChannels = 0;
		success = create_with_rigged(gObjectShinyProgram,
									 gSkinnedObjectShinyProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectShinyWaterProgram.setup("Shiny water shader", shader_level,
									   "objects/shinyV.glsl",
									   "objects/shinyWaterF.glsl");
		gObjectShinyWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectShinyWaterProgram.mFeatures.calculatesLighting = true;
		gObjectShinyWaterProgram.mFeatures.isShiny = true;
		gObjectShinyWaterProgram.mFeatures.hasWaterFog = true;
		gObjectShinyWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectShinyWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectShinyWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = create_with_rigged(gObjectShinyWaterProgram,
									 gSkinnedObjectShinyWaterProgram);
	}

	if (success && !gUsePBRShaders)
	{
		gObjectFullbrightShinyProgram.setup("Fullbright shiny shader",
											shader_level,
											"objects/fullbrightShinyV.glsl",
											"objects/fullbrightShinyF.glsl");
		gObjectFullbrightShinyProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightShinyProgram.mFeatures.isFullbright = true;
		gObjectFullbrightShinyProgram.mFeatures.isShiny = true;
		gObjectFullbrightShinyProgram.mFeatures.hasGamma = true;
		gObjectFullbrightShinyProgram.mFeatures.hasTransport = true;
		gObjectFullbrightShinyProgram.mFeatures.mIndexedTextureChannels = 0;
		success = create_with_rigged(gObjectFullbrightShinyProgram,
									 gSkinnedObjectFullbrightShinyProgram);
	}

	if (success && !gUsePBRShaders)
	{
		shaderp = &gObjectFullbrightShinyWaterProgram;
		shaderp->setup("Fullbright shiny water shader", shader_level,
					   "objects/fullbrightShinyV.glsl",
					   "objects/fullbrightShinyWaterF.glsl");
		shaderp->mFeatures.calculatesAtmospherics = true;
		shaderp->mFeatures.isFullbright = true;
		shaderp->mFeatures.isShiny = true;
		shaderp->mFeatures.hasGamma = true;
		shaderp->mFeatures.hasTransport = true;
		shaderp->mFeatures.hasWaterFog = true;
		shaderp->mFeatures.mIndexedTextureChannels = 0;
		shaderp->mShaderGroup = LLGLSLShader::SG_WATER;
		success =
			create_with_rigged(*shaderp,
							   gSkinnedObjectFullbrightShinyWaterProgram);
	}

	if (success && gUsePBRShaders)
	{
		gDrawColorProgram.setup("Draw color shader", shader_level,
						 		"objects/simpleNoAtmosV.glsl",
								"objects/simpleColorF.glsl");
		success = gDrawColorProgram.createShader();
	}

	if (success)
	{
		llinfos << "Object shaders loaded at level: " << shader_level
				<< llendl;
	}
	else
	{
		mShaderLevel[SHADER_OBJECT] = 0;
	}

	return success;
}

bool LLViewerShaderMgr::loadShadersAvatar()
{
	S32 shader_level = mShaderLevel[SHADER_AVATAR];
	if (shader_level == 0 || gUsePBRShaders)
	{
		gAvatarWaterProgram.unload();
	}
	if (shader_level == 0)
	{
		gAvatarProgram.unload();
		gAvatarEyeballProgram.unload();
		return true;
	}

	gAvatarProgram.setup("Avatar shader", shader_level,
						 "avatar/avatarV.glsl", "avatar/avatarF.glsl");
	gAvatarProgram.mFeatures.hasSkinning = true;
	gAvatarProgram.mFeatures.calculatesAtmospherics = true;
	gAvatarProgram.mFeatures.calculatesLighting = true;
	gAvatarProgram.mFeatures.hasGamma = true;
	gAvatarProgram.mFeatures.hasAtmospherics = true;
	gAvatarProgram.mFeatures.hasLighting = true;
	gAvatarProgram.mFeatures.hasAlphaMask = true;
	gAvatarProgram.mFeatures.disableTextureIndex = true;
	bool success = gAvatarProgram.createShader();

	if (success && !gUsePBRShaders)
	{
		gAvatarWaterProgram.setup("Avatar water shader",
								  // Note: no cloth under water:
								  llmin(shader_level, 1),
								  "avatar/avatarV.glsl",
								  "objects/simpleWaterF.glsl");
		gAvatarWaterProgram.mFeatures.hasSkinning = true;
		gAvatarWaterProgram.mFeatures.calculatesAtmospherics = true;
		gAvatarWaterProgram.mFeatures.calculatesLighting = true;
		gAvatarWaterProgram.mFeatures.hasWaterFog = true;
		gAvatarWaterProgram.mFeatures.hasAtmospherics = true;
		gAvatarWaterProgram.mFeatures.hasLighting = true;
		gAvatarWaterProgram.mFeatures.hasAlphaMask = true;
		gAvatarWaterProgram.mFeatures.disableTextureIndex = true;
		gAvatarWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gAvatarWaterProgram.createShader();
	}

	// Keep track of avatar levels
	if (gAvatarProgram.mShaderLevel != mShaderLevel[SHADER_AVATAR])
	{
		shader_level = mMaxAvatarShaderLevel = mShaderLevel[SHADER_AVATAR] =
					   gAvatarProgram.mShaderLevel;
	}

	if (success)
	{
		gAvatarEyeballProgram.setup("Avatar eyeball program", shader_level,
									"avatar/eyeballV.glsl",
									"avatar/eyeballF.glsl");
		gAvatarEyeballProgram.mFeatures.calculatesLighting = true;
		gAvatarEyeballProgram.mFeatures.isSpecular = true;
		gAvatarEyeballProgram.mFeatures.calculatesAtmospherics = true;
		gAvatarEyeballProgram.mFeatures.hasGamma = true;
		gAvatarEyeballProgram.mFeatures.hasAtmospherics = true;
		gAvatarEyeballProgram.mFeatures.hasLighting = true;
		gAvatarEyeballProgram.mFeatures.hasAlphaMask = true;
		gAvatarEyeballProgram.mFeatures.disableTextureIndex = true;
		success = gAvatarEyeballProgram.createShader();
	}

	if (success)
	{
		llinfos << "Avatar shaders loaded at level: " << shader_level
				<< llendl;
	}
	else
	{
		mShaderLevel[SHADER_AVATAR] = 0;
		mMaxAvatarShaderLevel = 0;
	}

	return success;
}

bool LLViewerShaderMgr::loadShadersInterface()
{
	sHasIrrandiance = false;

	S32 shader_level = mShaderLevel[SHADER_INTERFACE];

	gHighlightProgram.setup("Highlight shader", shader_level,
							"interface/highlightV.glsl",
							"interface/highlightF.glsl");
	bool success = create_with_rigged(gHighlightProgram,
									  gSkinnedHighlightProgram);

	if (success)
	{
		gHighlightNormalProgram.setup("Highlight normals shader", shader_level,
									  "interface/highlightNormV.glsl",
									  "interface/highlightF.glsl");
		success = gHighlightNormalProgram.createShader();
	}

	if (success)
	{
		gHighlightSpecularProgram.setup("Highlight specular shader",
										shader_level,
										"interface/highlightSpecV.glsl",
										"interface/highlightF.glsl");
		success = gHighlightSpecularProgram.createShader();
	}

	if (success)
	{
		gUIProgram.setup("UI shader", shader_level,
						 "interface/uiV.glsl", "interface/uiF.glsl");
		success = gUIProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gSplatTextureRectProgram.setup("Splat texture rect shader",
									   shader_level,
									   "interface/splattexturerectV.glsl",
									   "interface/splattexturerectF.glsl");
		success = gSplatTextureRectProgram.createShader();
		if (success)
		{
			gSplatTextureRectProgram.bind();
			gSplatTextureRectProgram.uniform1i(sScreenMap, 0);
			gSplatTextureRectProgram.unbind();
		}
	}

	if (success)
	{
		gGlowCombineProgram.setup("Glow combine shader", shader_level,
								  "interface/glowcombineV.glsl",
								  "interface/glowcombineF.glsl");
		success = gGlowCombineProgram.createShader();
		if (success)
		{
			gGlowCombineProgram.bind();
			gGlowCombineProgram.uniform1i(sGlowMap, 0);
			gGlowCombineProgram.uniform1i(sScreenMap, 1);
			gGlowCombineProgram.unbind();
		}
	}

	if (success)
	{
		gGlowCombineFXAAProgram.setup("Glow combine FXAA shader",
									  shader_level,
									  "interface/glowcombineFXAAV.glsl",
									  "interface/glowcombineFXAAF.glsl");
		success = gGlowCombineFXAAProgram.createShader();
		if (success)
		{
			gGlowCombineFXAAProgram.bind();
			gGlowCombineFXAAProgram.uniform1i(sGlowMap, 0);
			gGlowCombineFXAAProgram.uniform1i(sScreenMap, 1);
			gGlowCombineFXAAProgram.unbind();
		}
	}

	if (success && !gUsePBRShaders)
	{
		gOneTextureNoColorProgram.setup("One texture no color shader",
										shader_level,
										"interface/onetexturenocolorV.glsl",
										"interface/onetexturenocolorF.glsl");
		success = gOneTextureNoColorProgram.createShader();
		if (success)
		{
			gOneTextureNoColorProgram.bind();
			gOneTextureNoColorProgram.uniform1i(sTex0, 0);
			gOneTextureNoColorProgram.unbind();
		}
	}

	if (success)
	{
		gSolidColorProgram.setup("Solid color shader", shader_level,
								 "interface/solidcolorV.glsl",
								 "interface/solidcolorF.glsl");
		success = gSolidColorProgram.createShader();
		if (success)
		{
			gSolidColorProgram.bind();
			gSolidColorProgram.uniform1i(sTex0, 0);
			gSolidColorProgram.unbind();
		}
	}

	if (success)
	{
		gOcclusionProgram.setup("Occlusion shader", shader_level,
								"interface/occlusionV.glsl",
								"interface/occlusionF.glsl");
		gOcclusionProgram.mRiggedVariant = &gSkinnedOcclusionProgram;
		success = gOcclusionProgram.createShader();
	}

	if (success)
	{
		gSkinnedOcclusionProgram.setup("Skinned occlusion shader",
									   shader_level,
									   "interface/occlusionSkinnedV.glsl",
									   "interface/occlusionF.glsl");
		gSkinnedOcclusionProgram.mFeatures.hasObjectSkinning = true;
		success = gSkinnedOcclusionProgram.createShader();
	}

	if (success)
	{
		gOcclusionCubeProgram.setup("Occlusion cube shader", shader_level,
									"interface/occlusionCubeV.glsl",
									"interface/occlusionF.glsl");
		success = gOcclusionCubeProgram.createShader();
	}

	if (success)
	{
		gDebugProgram.setup("Debug shader", shader_level,
							"interface/debugV.glsl", "interface/debugF.glsl");
		success = create_with_rigged(gDebugProgram, gSkinnedDebugProgram);
	}

	if (success && gUsePBRShaders)
	{
		static const char* normal = "Normal debug shader";
		static const char* tangents = "Normal debug shader with tangents";
		for (S32 i = 0; success && i < NORMAL_DEBUG_SHADER_COUNT; ++i)
		{
			bool with_tangents = i == NORMAL_DEBUG_SHADER_WITH_TANGENTS;
			LLGLSLShader& shader = gNormalDebugProgram[i];
			shader.setup(with_tangents ? tangents : normal, shader_level,
						 "interface/normaldebugV.glsl",
						 "interface/normaldebugF.glsl",
						 "interface/normaldebugG.glsl");	// Geometry shader
			if (with_tangents)
			{
				shader.addPermutation("HAS_ATTRIBUTE_TANGENT", "1");
			}
			success = create_with_rigged(shader,
										 gSkinnedNormalDebugProgram[i]);
		}
	}

	if (success)
	{
		gClipProgram.setup("Clip shader", shader_level,
						   "interface/clipV.glsl", "interface/clipF.glsl");
		success = gClipProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gDownsampleDepthProgram.setup("Downsample depth shader", shader_level,
									  "interface/downsampleDepthV.glsl",
									  "interface/downsampleDepthF.glsl");
		success = gDownsampleDepthProgram.createShader();
	}

	if (success)
	{
		gBenchmarkProgram.setup("Benchmark shader", shader_level,
								"interface/benchmarkV.glsl",
								"interface/benchmarkF.glsl");
		success = gBenchmarkProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gReflectionProbeDisplayProgram.setup("Reflection probe display shader",
											 shader_level,
											 "interface/reflectionprobeV.glsl",
											 "interface/reflectionprobeF.glsl");
		gReflectionProbeDisplayProgram.mFeatures.hasReflectionProbes = true;
		gReflectionProbeDisplayProgram.mFeatures.hasSrgb = true;
		gReflectionProbeDisplayProgram.mFeatures.calculatesAtmospherics = true;
		gReflectionProbeDisplayProgram.mFeatures.hasAtmospherics = true;
		gReflectionProbeDisplayProgram.mFeatures.hasGamma = true;
		gReflectionProbeDisplayProgram.mFeatures.isDeferred = true;
		success = gReflectionProbeDisplayProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gCopyProgram.setup("Copy shader", shader_level,
						   "interface/copyV.glsl", "interface/copyF.glsl");
		success = gCopyProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gCopyDepthProgram.setup("Copy depth shader", shader_level,
						 		"interface/copyV.glsl",
								"interface/copyF.glsl");
		gCopyDepthProgram.addPermutation("COPY_DEPTH", "1");
		success = gCopyDepthProgram.createShader();
	}

	if (success && !gUsePBRShaders)
	{
		gDownsampleDepthRectProgram.setup("Downsample depth rect shader",
										  shader_level,
										  "interface/downsampleDepthV.glsl",
										  "interface/downsampleDepthRectF.glsl");
		success = gDownsampleDepthRectProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gHiZReduceProgram.setup("Hi-Z reduce shader", shader_level,
								"interface/copyV.glsl",
								"interface/hiZReduceF.glsl");
		success = gHiZReduceProgram.createShader();
	}

	if (success)
	{
		gAlphaMaskProgram.setup("Alpha mask shader", shader_level,
								"interface/alphamaskV.glsl",
								"interface/alphamaskF.glsl");
		success = gAlphaMaskProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gReflectionMipProgram.setup("Reflection mip shader", shader_level,
									"interface/splattexturerectV.glsl",
									"interface/reflectionmipF.glsl");
		gReflectionMipProgram.mFeatures.isDeferred = true;
		gReflectionMipProgram.mFeatures.hasGamma = true;
		gReflectionMipProgram.mFeatures.hasAtmospherics = true;
		gReflectionMipProgram.mFeatures.calculatesAtmospherics = true;
		success = gReflectionMipProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		gGaussianProgram.setup("Reflection gaussian shader", shader_level,
							   "interface/splattexturerectV.glsl",
							   "interface/gaussianF.glsl");
		gGaussianProgram.mFeatures.isDeferred = true;
		gGaussianProgram.mFeatures.hasGamma = true;
		gGaussianProgram.mFeatures.hasAtmospherics = true;
		gGaussianProgram.mFeatures.calculatesAtmospherics = true;
		success = gGaussianProgram.createShader();
	}

	if (success && gUsePBRShaders)
	{
		sHasIrrandiance = LLGLManager::sHasCubeMapArray;
		if (sHasIrrandiance)
		{
			gRadianceGenProgram.setup("Radiance gen shader", shader_level,
									  "interface/radianceGenV.glsl",
									  "interface/radianceGenF.glsl");
			gRadianceGenProgram.addPermutation("PROBE_FILTER_SAMPLES", "32");
			sHasIrrandiance = gRadianceGenProgram.createShader();
		}
		if (sHasIrrandiance)
		{
			gHeroRadianceGenProgram.setup("Hero radiance gen shader",
										  shader_level,
										  "interface/radianceGenV.glsl",
										  "interface/radianceGenF.glsl");
			//gHeroRadianceGenProgram.addPermutation("HERO_PROBES", "1");
			gHeroRadianceGenProgram.addPermutation("PROBE_FILTER_SAMPLES",
												   "4");
			sHasIrrandiance = gHeroRadianceGenProgram.createShader();
		}

		if (sHasIrrandiance)
		{
			gIrradianceGenProgram.setup("Irradiance gen shader", shader_level,
										"interface/irradianceGenV.glsl",
										"interface/irradianceGenF.glsl");
			sHasIrrandiance = gIrradianceGenProgram.createShader();
		}
		if (!sHasIrrandiance)
		{
			llwarns << "No cube map array support: refection maps will not render."
					<< llendl;
		}
	}

	if (success)
	{
		llinfos << "Interface shaders loaded at level: " << shader_level
				<< llendl;
	}
	else
	{
		mShaderLevel[SHADER_INTERFACE] = 0;
	}

	return success;
}

bool LLViewerShaderMgr::loadShadersWindLight()
{
	S32 shader_level = mShaderLevel[SHADER_WINDLIGHT];

	if (shader_level < 2 || gUsePBRShaders)
	{
		gWLSkyProgram.unload();
		gWLCloudProgram.unload();
		gWLSunProgram.unload();
		gWLMoonProgram.unload();
		return true;
	}

	gWLSkyProgram.setup("Windlight sky shader", shader_level,
						"windlight/skyV.glsl", "windlight/skyF.glsl");
	gWLSkyProgram.mFeatures.calculatesAtmospherics = true;
	gWLSkyProgram.mFeatures.hasTransport = true;
	gWLSkyProgram.mFeatures.hasGamma = true;
	gWLSkyProgram.mFeatures.hasSrgb = true;
	gWLSkyProgram.mShaderGroup = LLGLSLShader::SG_SKY;
	bool success = gWLSkyProgram.createShader();

	if (success)
	{
		gWLCloudProgram.setup("Windlight cloud program", shader_level,
							  "windlight/cloudsV.glsl",
							  "windlight/cloudsF.glsl");
		gWLCloudProgram.mFeatures.calculatesAtmospherics = true;
		gWLCloudProgram.mFeatures.hasTransport = true;
		gWLCloudProgram.mFeatures.hasGamma = true;
		gWLCloudProgram.mFeatures.hasSrgb = true;
		gWLCloudProgram.addConstant(LLGLSLShader::CONST_CLOUD_MOON_DEPTH);
		gWLCloudProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gWLCloudProgram.createShader();
	}

	if (success)
	{
		gWLSunProgram.setup("Windlight Sun program", shader_level,
							"windlight/sunDiscV.glsl",
							"windlight/sunDiscF.glsl");
		gWLSunProgram.mFeatures.calculatesAtmospherics = true;
		gWLSunProgram.mFeatures.hasTransport = true;
		gWLSunProgram.mFeatures.hasGamma = true;
		gWLSunProgram.mFeatures.hasAtmospherics = true;
		gWLSunProgram.mFeatures.isFullbright = true;
		gWLSunProgram.mFeatures.disableTextureIndex = true;
		gWLSunProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gWLSunProgram.createShader();
	}

	if (success)
	{
		gWLMoonProgram.setup("Windlight Moon program", shader_level,
							 "windlight/moonV.glsl",
							 "windlight/moonF.glsl");
		gWLMoonProgram.mFeatures.calculatesAtmospherics = true;
		gWLMoonProgram.mFeatures.hasTransport = true;
		gWLMoonProgram.mFeatures.hasGamma = true;
		gWLMoonProgram.mFeatures.hasAtmospherics = true;
		gWLMoonProgram.mFeatures.isFullbright = true;
		gWLMoonProgram.mFeatures.disableTextureIndex = true;
		gWLMoonProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		gWLMoonProgram.addConstant(LLGLSLShader::CONST_CLOUD_MOON_DEPTH);
		success = gWLMoonProgram.createShader();
	}

	if (success)
	{
		llinfos << "Windlight shaders loaded at level: " << shader_level
				<< llendl;
	}

	return success;
}

//virtual
void LLViewerShaderMgr::updateShaderUniforms(LLGLSLShader* shaderp)
{
	LL_TRACY_TIMER(TRC_UPD_SHADER_UNIFORMS);
	gEnvironment.updateShaderUniforms(shaderp);
}
