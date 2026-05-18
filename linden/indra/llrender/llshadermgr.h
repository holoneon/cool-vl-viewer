/**
 * @file llshadermgr.h
 * @brief Shader Manager
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llglslshader.h"

class LLShaderMgr
{
protected:
	LOG_CLASS(LLShaderMgr);

	LLShaderMgr();
	virtual ~LLShaderMgr();

public:
	typedef enum
	{
		MODELVIEW_MATRIX = 0,
		PROJECTION_MATRIX,
		INVERSE_PROJECTION_MATRIX,
		MODELVIEW_PROJECTION_MATRIX,
		INVERSE_MODELVIEW_MATRIX,
		NORMAL_MATRIX,
		TEXTURE_MATRIX0,
		// Actually never used by shaders, but currently needed by the C++ code
		// due to NUM_MATRIX_MODES.
		// *TODO: cleanup the code and get rid of this. HB 
		TEXTURE_MATRIX1,
		TEXTURE_MATRIX2,
		TEXTURE_MATRIX3,

		OBJECT_PLANE_S,
		OBJECT_PLANE_T,
		TEXTURE_BASE_COLOR_TRANSFORM,	// For PBR shaders only
		TEXTURE_NORMAL_TRANSFORM,		// For PBR shaders only
		TEXTURE_ROUGHNESS_TRANSFORM,	// For PBR shaders only
		TEXTURE_EMISSIVE_TRANSFORM,		// For PBR shaders only
		BASE_COLOR_TEXCOORD,			// For PBR shaders only
		EMISSIVE_TEXCOORD,				// For PBR shaders only
		NORMAL_TEXCOORD,				// For PBR shaders only
		METALLIC_ROUGHNESS_TEXCOORD,	// For PBR shaders only
		OCCLUSION_TEXCOORD,				// For PBR shaders only
		GLTF_NODE_ID,					// For PBR shaders only
		GLTF_MATERIAL_ID,				// For PBR shaders only
		TERRAIN_TEXTURE_TRANSFORMS,		// For PBR shaders only
		VIEWPORT,
		LIGHT_POSITION,
		LIGHT_DIRECTION,
		LIGHT_ATTENUATION,
		LIGHT_DEFERRED_ATTENUATION,		// For PBR shaders only
		LIGHT_DIFFUSE,
		LIGHT_AMBIENT,
		MULTI_LIGHT_COUNT,
		MULTI_LIGHT,
		MULTI_LIGHT_COL,
		MULTI_LIGHT_FAR_Z,
		PROJECTOR_MATRIX,
		PROJECTOR_P,
		PROJECTOR_N,
		PROJECTOR_ORIGIN,
		PROJECTOR_RANGE,
		PROJECTOR_AMBIANCE,
		PROJECTOR_SHADOW_INDEX,
		PROJECTOR_SHADOW_FADE,
		PROJECTOR_FOCUS,
		PROJECTOR_LOD,
		DIFFUSE_COLOR,
		EMISSIVE_COLOR,					// For PBR shaders only
		METALLIC_FACTOR,				// For PBR shaders only
		ROUGHNESS_FACTOR,				// For PBR shaders only
		MIRROR_FLAG,					// For PBR shaders only
		CLIP_PLANE,						// For PBR shaders only
		DIFFUSE_MAP,
		ALTERNATE_DIFFUSE_MAP,
		SPECULAR_MAP,
		METALLIC_ROUGHNESS_MAP,			// For PBR shaders only
		NORMAL_MAP,
		OCCLUSION_MAP,					// For PBR shaders only
		EMISSIVE_MAP,					// For PBR shaders only
		BUMP_MAP,
		BUMP_MAP2,
		ENVIRONMENT_MAP,
		SCENE_MAP,						// For PBR shaders only
		SCENE_DEPTH,					// For PBR shaders only
		REFLECTION_PROBES,				// For PBR shaders only
		IRRADIANCE_PROBES,				// For PBR shaders only
		HERO_PROBE,						// For PBR shaders only
		CLOUD_NOISE_MAP,
		CLOUD_NOISE_MAP_NEXT,
		FULLBRIGHT,						// For EE shaders only
		LIGHTNORM,
		SUNLIGHT_COLOR,
		AMBIENT,
		SKY_HDR_SCALE,					// For PBR shaders only
		SKY_SUNLIGHT_SCALE,				// For PBR shaders only
		SKY_AMBIENT_SCALE,				// For PBR shaders only
		CLASSIC_MODE,					// For PBR shaders only
		BLUE_HORIZON,
		BLUE_DENSITY,
		HAZE_HORIZON,
		HAZE_DENSITY,
		CLOUD_SHADOW,
		DENSITY_MULTIPLIER,
		DISTANCE_MULTIPLIER,
		MAX_Y,
		GLOW,
		CLOUD_COLOR,
		CLOUD_POS_DENSITY1,
		CLOUD_POS_DENSITY2,
		CLOUD_SCALE,
		GAMMA,
		SCENE_LIGHT_STRENGTH,
		LIGHT_CENTER,
		LIGHT_SIZE,
		LIGHT_FALLOFF,
		BOX_CENTER,
		BOX_SIZE,

		GLOW_MIN_LUMINANCE,
		GLOW_MAX_EXTRACT_ALPHA,
		GLOW_LUM_WEIGHTS,
		GLOW_WARMTH_WEIGHTS,
		GLOW_WARMTH_AMOUNT,
		GLOW_STRENGTH,
		GLOW_DELTA,
		GLOW_NOISE_MAP,					// For PBR shaders only

		MINIMUM_ALPHA,
		EMISSIVE_BRIGHTNESS,
		ALPHA_GAMMA,					// For PBR shaders only

		DEFERRED_SHADOW_MATRIX,
		DEFERRED_ENV_MAT,
		DEFERRED_SHADOW_CLIP,
		DEFERRED_SUN_WASH,
		DEFERRED_SHADOW_NOISE,
		DEFERRED_BLUR_SIZE,
		DEFERRED_SSAO_RADIUS,
		DEFERRED_SSAO_MAX_RADIUS,
		DEFERRED_SSAO_FACTOR,
		DEFERRED_SSAO_EFFECT_MAT,
		DEFERRED_SCREEN_RES,
		DEFERRED_NEAR_CLIP,
		DEFERRED_SHADOW_OFFSET,
		DEFERRED_SHADOW_BIAS,
		DEFERRED_SPOT_SHADOW_BIAS,
		DEFERRED_SPOT_SHADOW_OFFSET,
		DEFERRED_SUN_DIR,
		DEFERRED_MOON_DIR,
		DEFERRED_SHADOW_RES,
		DEFERRED_PROJ_SHADOW_RES,
		DEFERRED_SHADOW_TARGET_WIDTH,

		DEFERRED_SSR_ITR_COUNT,			// For PBR shaders only
		DEFERRED_SSR_MAX_THICKNESS,		// For PBR shaders only
		DEFERRED_SSR_DEPTH_BIAS,		// For PBR shaders only
		DEFERRED_SSR_GLOSSY_SAMPLES,	// For PBR shaders only
		DEFERRED_SSR_NOISE_SINE,		// For PBR shaders only
		DEFERRED_SSR_MAX_Z,				// For PBR shaders only
		DEFERRED_SSR_MAX_ROUGHNESS,		// For PBR shaders only

		MODELVIEW_DELTA_MATRIX,			// For PBR shaders only
		INVERSE_MODELVIEW_DELTA_MATRIX,	// For PBR shaders only
		CUBE_SNAPSHOT,					// For PBR shaders only
		DEFAULT_PROBE_RENDER,			// For PBR shaders only
		REFLECTION_PROBE_QUALITY,		// For PBR shaders only

		FXAA_TC_SCALE,
		FXAA_RCP_SCREEN_RES,
		FXAA_RCP_FRAME_OPT,
		FXAA_RCP_FRAME_OPT2,

		DOF_FOCAL_DISTANCE,
		DOF_BLUR_CONSTANT,
		DOF_TAN_PIXEL_ANGLE,
		DOF_MAGNIFICATION,
		DOF_MAX_COF,
		DOF_RES_SCALE,
		DOF_WIDTH,
		DOF_HEIGHT,

		DEFERRED_DEPTH,
		DEFERRED_SHADOW0,
		DEFERRED_SHADOW1,
		DEFERRED_SHADOW2,
		DEFERRED_SHADOW3,
		DEFERRED_SHADOW4,
		DEFERRED_SHADOW5,

		DEFERRED_POSITION,
		DEFERRED_DIFFUSE,
		DEFERRED_SPECULAR,
		DEFERRED_EMISSIVE,				// For PBR shaders only
		EXPOSURE_MAP,					// For PBR shaders only
		DEFERRED_BRDF_LUT,				// For PBR shaders only
		DEFERRED_NOISE,
		DEFERRED_LIGHTFUNC,
		DEFERRED_LIGHT,
		DEFERRED_BLOOM,					// For EE shaders only
		DEFERRED_PROJECTION,
		DEFERRED_NORM_MATRIX,

		TEXTURE_GAMMA,					// For EE shaders only

		SPECULAR_COLOR,
		ENVIRONMENT_INTENSITY,

		AVATAR_MATRIX,

		WATER_SCREENTEX,
		WATER_SCREENDEPTH,				// For PBR shaders only
		WATER_REFTEX,
		WATER_EXCLUSIONTEX,				// For PBR shaders only
		WATER_EYEVEC,
		WATER_TIME,
		WATER_WAVE_DIR1,
		WATER_WAVE_DIR2,
		WATER_LIGHT_DIR,
		WATER_SPECULAR,
		WATER_FOGCOLOR,
		WATER_FOGCOLOR_LINEAR,			// For PBR shaders only
		WATER_FOGDENSITY,
		WATER_FOGKS,
		WATER_REFSCALE,
		WATER_WATERHEIGHT,
		WATER_WATERPLANE,
		WATER_NORM_SCALE,
		WATER_FRESNEL_SCALE,
		WATER_FRESNEL_OFFSET,
		WATER_BLUR_MULTIPLIER,
		WATER_SUN_ANGLE,

		WL_CAMPOSLOCAL,

		AVATAR_WIND,					// For EE shaders only
		AVATAR_SINWAVE,					// For EE shaders only
		AVATAR_GRAVITY,					// For EE shaders only

		TERRAIN_DETAIL0,
		TERRAIN_DETAIL1,
		TERRAIN_DETAIL2,
		TERRAIN_DETAIL3,
		TERRAIN_ALPHARAMP,

		TERRAIN_DETAIL0_BASE_COLOR,		// For PBR shaders only
		TERRAIN_DETAIL1_BASE_COLOR,		// For PBR shaders only
		TERRAIN_DETAIL2_BASE_COLOR,		// For PBR shaders only
		TERRAIN_DETAIL3_BASE_COLOR,		// For PBR shaders only
		TERRAIN_DETAIL0_NORMAL,			// For PBR shaders only
		TERRAIN_DETAIL1_NORMAL,			// For PBR shaders only
		TERRAIN_DETAIL2_NORMAL,			// For PBR shaders only
		TERRAIN_DETAIL3_NORMAL,			// For PBR shaders only
		TERRAIN_DETAIL0_MROUGHNESS,		// For PBR shaders only
		TERRAIN_DETAIL1_MROUGHNESS,		// For PBR shaders only
		TERRAIN_DETAIL2_MROUGHNESS,		// For PBR shaders only
		TERRAIN_DETAIL3_MROUGHNESS,		// For PBR shaders only
		TERRAIN_DETAIL0_EMISSIVE,		// For PBR shaders only
		TERRAIN_DETAIL1_EMISSIVE,		// For PBR shaders only
		TERRAIN_DETAIL2_EMISSIVE,		// For PBR shaders only
		TERRAIN_DETAIL3_EMISSIVE,		// For PBR shaders only
		TERRAIN_BASE_COLOR_FACTORS,		// For PBR shaders only
		TERRAIN_METALLIC_FACTORS,		// For PBR shaders only
		TERRAIN_ROUGHNESS_FACTORS,		// For PBR shaders only
		TERRAIN_EMISSIVE_COLORS,		// For PBR shaders only
		TERRAIN_MINIMUM_ALPHAS,			// For PBR shaders only

		SHINY_ORIGIN,

		DISPLAY_GAMMA,

		SUN_SIZE,
		FOG_COLOR,

		BLEND_FACTOR,

		NO_ATMO,						// For EE shaders only
		MOISTURE_LEVEL,
		DROPLET_RADIUS,
		ICE_LEVEL,
		RAINBOW_MAP,
		HALO_MAP,

		MOON_BRIGHTNESS,

		CLOUD_VARIANCE,

		REFLECTION_PROBE_AMBIANCE,		// For PBR shaders only
		REFLECTION_PROBE_MAX_LOD,		// For PBR shaders only
		REFLECTION_PROBE_STRENGTH,		// For PBR shaders only

		// Used only be the EE shaders, but not in the C++ renderer code.
		// *TODO: check for a possible bug or eliminate if actually useless. HB
		SH_INPUT_L1R,
		SH_INPUT_L1G,
		SH_INPUT_L1B,

		SUN_MOON_GLOW_FACTOR,
		SUN_UP_FACTOR,
		MOONLIGHT_COLOR,

		DEBUG_NORMAL_DRAW_LENGTH,		// For PBR shaders only

		// For the SMAA shaders
		SMAA_EDGE_TEX,
		SMAA_AREA_TEX,
		SMAA_SEARCH_TEX,
		SMAA_BLEND_TEX,

		END_RESERVED_UNIFORMS
	} eGLSLReservedUniforms;

	// Singleton pattern implementation
	static LLShaderMgr* getInstance();

	virtual void initAttribsAndUniforms();

	bool attachShaderFeatures(LLGLSLShader* shader);
	bool linkProgramObject(GLuint obj, bool suppress_errors = false);
	GLuint loadShaderFile(const std::string& filename, S32& shader_level,
						  U32 type, strings_map_t* defines = NULL,
						  S32 texture_index_channels = -1);

	// Implemented in the application to actually point to the shader directory
	virtual const std::string& getShaderDirPrefix() const = 0;

	// Implemented in the application to actually update out of date uniforms
	// for a particular shader
	virtual void updateShaderUniforms(LLGLSLShader* shader) = 0;

private:
	void dumpObjectLog(bool is_program, GLuint ret, bool warns = true);
	void dumpShaderSource(U32 shader_count, GLchar** shader_text);

protected:
	// Our parameter manager singleton instance
	static LLShaderMgr*			sInstance;

public:
	// Map of shader names to compiled
	typedef std::map<std::string, GLuint, std::less<> > shaders_map_t;
	typedef shaders_map_t::const_iterator map_citer_t;
	static shaders_map_t	sVertexShaderObjects;
	static shaders_map_t	sFragmentShaderObjects;

	// Global (reserved slot) shader parameters
	static strings_vec_t	sReservedAttribs;

	static strings_vec_t	sReservedUniforms;

	static S32				sIndexedTextureChannels;

	static bool				sMirrorsEnabled;
};
