/**
 * @file llglslshader.h
 * @brief GLSL shader wrappers
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

#include <vector>

#include "llgl.h"
#include "llrender.h"
#include "llstring.h"
#include "llstringtable.h"

class LLRenderTarget;

class LLShaderFeatures
{
public:
	LLShaderFeatures();

public:
	S32 mIndexedTextureChannels;

	bool calculatesLighting;
	bool calculatesAtmospherics;
	// Implies no transport (it is possible to have neither though):
	bool hasLighting;
	// Indicates lighting shaders need not be linked in (lighting performed
	// directly in alpha shader to match deferred lighting functions):
	bool isAlphaLighting;
	bool isShiny;					// No more used by PBR shaders
	bool isFullbright;				// Implies no lighting, not used by PBR
	bool isSpecular;
	bool hasWaterFog;				// Implies no gamma, not used by PBR
	// Implies no lighting (it is possible to have neither though):
	bool hasTransport;				// No more used by PBR shaders
	bool hasSkinning;
	bool hasObjectSkinning;
	bool hasAtmospherics;
	bool hasGamma;
	bool hasSrgb;
	bool encodesNormal;				// Not used any more by PBR shaders
	bool isDeferred;
	bool hasFullGBuffer;
	bool hasShadows;
	bool hasAmbientOcclusion;
	bool disableTextureIndex;
	bool hasAlphaMask;
	bool attachNothing;
	bool hasScreenSpaceReflections;	// For PBR shaders
	bool hasReflectionProbes;		// For PBR shaders
	bool isPBRTerrain;				// For PBR shaders
};

class LLGLSLShader
{
protected:
	LOG_CLASS(LLGLSLShader);

public:
	enum
	{
		SG_DEFAULT = 0,	// Not sky or water specific
		SG_SKY,
		SG_WATER,
		SG_ANY,
		SG_COUNT
	};

	enum EShaderConsts
	{
		CONST_CLOUD_MOON_DEPTH,
		CONST_STAR_DEPTH,
		NUM_SHADER_CONSTS
	};

	enum UniformBlock : U32
	{
		UB_REFLECTION_PROBES,	// "ReflectionProbes"
		UB_GLTF_JOINTS,			// "GLTFJoints"
		UB_GLTF_NODES,			// "GLTFNodes"
		UB_GLTF_MATERIALS,		// "GLTFMaterials"
		NUM_UNIFORM_BLOCKS
	};

	// Variants for use by the glTF assets renderer
	//	- bit 0 = alpha mode blend (1) or opaque (0)
	//	- bit 1 = rigged (1) or static (0)
	//	- bit 2 = unlit (1) or lit (0)
	//	- bit 3 = single (0) or multi (1) uv coordinates
	enum EGltfVariant : U8
	{
		ALPHA_BLEND = 1,
		RIGGED = 2,
		UNLIT = 4,
		MULTI_UV = 8,
		NUM_GLTF_VARIANTS = 16	// 2 ^ 4
	};

	LLGLSLShader();

	static void initProfile();
	static void finishProfile(bool emit_report = true);

	LL_INLINE static void startProfile()
	{
		if (sProfileEnabled && sCurBoundShaderPtr)
		{
			sCurBoundShaderPtr->placeProfileQuery();
		}
	}

	LL_INLINE static void stopProfile()
	{
		if (sProfileEnabled && sCurBoundShaderPtr)
		{
			sCurBoundShaderPtr->readProfileQuery();
		}
	}

	void clearStats();
	void dumpStats();
	void placeProfileQuery();
	void readProfileQuery();

	void setup(const char* name, S32 level, const char* vertex_shader,
			   const char* fragment_shader,
			   const char* geometry_shader = NULL);

	void unload();

	typedef std::vector<LLStaticHashedString> hash_vector_t;
	bool createShader(hash_vector_t* attributes = NULL,
					  hash_vector_t* uniforms = NULL,
					  U32 varying_count = 0, const char** varyings = NULL);

	bool attachVertexObject(const char* object);
	bool attachFragmentObject(const char* object);
	void attachObject(GLuint object);
	void attachObjects(GLuint* objects = NULL, S32 count = 0);
	bool mapAttributes(const hash_vector_t* attributes);
	bool mapUniforms(const hash_vector_t* uniforms);
	void mapUniform(S32 index, const hash_vector_t* uniforms);

	void uniform1i(U32 index, S32 i);
	void uniform1f(U32 index, F32 v);
	void uniform2f(U32 index, F32 x, F32 y);
	void uniform3f(U32 index, F32 x, F32 y, F32 z);
	void uniform4f(U32 index, F32 x, F32 y, F32 z, F32 w);
	void uniform1iv(U32 index, U32 count, const S32* i);
	void uniform4iv(U32 index, U32 count, const S32* i);
	void uniform4uiv(U32 index, U32 count, const U32* i);
	void uniform1fv(U32 index, U32 count, const F32* v);
	void uniform2fv(U32 index, U32 count, const F32* v);
	void uniform3fv(U32 index, U32 count, const F32* v);
	void uniform4fv(U32 index, U32 count, const F32* v);

	void uniformMatrix2fv(U32 index, U32 count, GLboolean transpose,
						  const F32* v);
	void uniformMatrix3fv(U32 index, U32 count, GLboolean transpose,
						  const F32* v);
	void uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose,
							const F32* v);
	void uniformMatrix4fv(U32 index, U32 count, GLboolean transpose,
						  const F32* v);

	void uniform1i(const LLStaticHashedString& uniform, S32 i);
	void uniform1iv(const LLStaticHashedString& uniform, U32 count,
					const S32* v);
	void uniform4iv(const LLStaticHashedString& uniform, U32 count,
					const S32* v);
	void uniform4uiv(const LLStaticHashedString& uniform, U32 count,
					 const U32* v);
	void uniform2i(const LLStaticHashedString& uniform, S32 i, S32 j);
	void uniform1f(const LLStaticHashedString& uniform, F32 v);
	void uniform2f(const LLStaticHashedString& uniform, F32 x, F32 y);
	void uniform3f(const LLStaticHashedString& uniform, F32 x, F32 y,
				   F32 z);
	void uniform4f(const LLStaticHashedString& uniform, F32 x, F32 y, F32 z,
				    F32 w);
	void uniform1fv(const LLStaticHashedString& uniform, U32 count,
					const F32* v);
	void uniform2fv(const LLStaticHashedString& uniform, U32 count,
					const F32* v);
	void uniform3fv(const LLStaticHashedString& uniform, U32 count,
					const F32* v);
	void uniform4fv(const LLStaticHashedString& uniform, U32 count,
					const F32* v);

	void uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count,
						  GLboolean transpose, const F32* v);

	void setMinimumAlpha(F32 minimum);

	void vertexAttrib4f(U32 index, F32 x, F32 y, F32 z, F32 w);
	void vertexAttrib4fv(U32 index, F32* v);

	S32 getUniformLocation(const LLStaticHashedString& uniform);
	S32 getUniformLocation(U32 index);

	S32 getAttribLocation(U32 attrib);

	void addConstant(EShaderConsts shader_const);
	void addPermutation(const std::string& name, const std::string& value);
	void addPermutations(const strings_map_t& defines);

	// Enable/disable texture channel for specified uniform. If given texture
	// uniform is active in the shader, the corresponding channel will be
	// active upon return. Returns channel texture is enabled in from [0-MAX).
	S32 enableTexture(S32 uniform,
					  LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
	S32 disableTexture(S32 uniform,
					   LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);

	// Gets the texture channel of the given uniform, or -1 if uniform is not
	// used as a texture.
	LL_INLINE S32 getTextureChannel(S32 u) const
	{
		return u >= 0 && u < (S32)mTexture.size() ? mTexture[u] : -1;
	}

	// bindTexture returns the texture unit we have bound the texture to. You
	// can reuse the return value to unbind a texture when required.
	S32 bindTexture(S32 uniform, LLGLTexture* texp,
					LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);

	LL_INLINE S32 bindTexture(const std::string& uniform, LLGLTexture* texp,
							  LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE)
	{
		return bindTexture(getUniformLocation(uniform), texp, mode);
	}

	// Render targets binding methods, for PBR rendering only.
	S32 bindTexture(S32 uniform, LLRenderTarget* targetp, bool depth = false,
					LLTexUnit::eTextureFilterOptions mode =
						LLTexUnit::TFO_BILINEAR,
					U32 index = 0);

	LL_INLINE S32 bindTexture(const std::string& uniform,
							  LLRenderTarget* targetp, bool depth = false,
							  LLTexUnit::eTextureFilterOptions mode =
								LLTexUnit::TFO_BILINEAR, U32 index = 0)
	{
		return bindTexture(getUniformLocation(uniform), targetp, depth, mode,
						   index);
	}

	void bind();
	// Helper to conditionally bind mRiggedVariant instead of this
	void bind(bool rigged);
	// Helper to bind a glTF variant
	void bindVariant(U8 variant);

	// Unbinds any previously bound shader by explicitly binding no shader.
	static void unbind();

private:
	void unloadInternal();
	S32 mapUniformTextureChannel(S32 location, U32 type, S32 size);

	// Methods to get locations corresponding to texture and uniform indexes.
	// They check for index validity and warn in case of error (also printing
	// the line number of the faulty call when gDebugGL is true, and crashing
	// voluntarily with an assert for debug builds). HB
	S32 getTexture(S32 line, S32 index);
	S32 getUniform(S32 line, U32 index);

public:
	U32								mMatHash[LLRender::NUM_MATRIX_MODES];
	U32								mLightHash;

	GLuint							mProgramObject;

	// Lookup table of attribute enum to attribute channel
	std::vector<S32>				mAttribute;

	// Mask of which reserved attributes are set (lines up with
	// LLVertexBuffer::getTypeMask())
	U32 							mAttributeMask;

	S32								mTotalUniformSize;
	S32								mActiveTextureChannels;
	S32								mShaderLevel;
	S32								mShaderGroup;
	LLShaderFeatures				mFeatures;

	std::string 					mName;

	// Lookup map of uniform name to uniform location
	LLStaticStringTable<S32>		mUniformMap;

	// Lookup map of uniform location to uniform name
	typedef fast_hmap<S32, std::string> uniforms_map_t;
	uniforms_map_t					mUniformNameMap;

	// Lookup table of uniform enum to uniform location
	std::vector<S32>				mUniform;

	// Lookup table of uniform enum to texture channels
	std::vector<S32>				mTexture;

	// Lookup map of uniform location to last known value
	typedef fast_hmap<S32, LLVector4> uniform_value_map_t;
	uniform_value_map_t				mValue;

	typedef std::vector<std::pair<std::string, U32> > files_map_t;
	files_map_t						mShaderFiles;

	strings_map_t					mDefines;

	// This pointer should be set to whichever shader represents this shader's
	// rigged variant
	LLGLSLShader*					mRiggedVariant;

	std::vector<LLGLSLShader>		mGLTFVariants;

	// Statistics for profiling shader performance
	U32								mTimerQuery;
	U32								mSamplesQuery;
	U32								mPrimitivesQuery;
	U64								mTimeElapsed;
	U32								mTrianglesDrawn;
	U64								mSamplesDrawn;
	U32								mDrawCalls;

	bool							mUniformsDirty;

	// *HACK: flag used for optimization in LLDrawPoolAlpha and LLPipeline
	bool							mCanBindFast;

	static std::set<LLGLSLShader*>	sInstances;
	static LLGLSLShader*			sCurBoundShaderPtr;
	static GLuint					sCurBoundShader;
	static bool						sProfileEnabled;

	// Statistics for profiling shader performance
	static U64						sTotalTimeElapsed;
	static U32						sTotalTrianglesDrawn;
	static U64						sTotalSamplesDrawn;
	static U32						sTotalDrawCalls;
};

class LLShaderUniforms
{
public:
	LL_INLINE LLShaderUniforms()
	:	mActive(false)
	{
	}

	LL_INLINE void clear()
	{
		mIntegers.resize(0);
		mFloats.resize(0);
		mVectors.resize(0);
		mVector3s.resize(0);
		mActive = false;
	}

	LL_INLINE void uniform1i(S32 index, S32 value)
	{
		mIntegers.push_back({ index, value });
		mActive = true;
	}

	LL_INLINE void uniform1f(S32 index, F32 value)
	{
		mFloats.push_back({ index, value });
		mActive = true;
	}

	LL_INLINE void uniform4fv(S32 index, const LLVector4& value)
	{
		mVectors.push_back({ index, value });
		mActive = true;
	}

	LL_INLINE void uniform4fv(S32 index, const F32* value)
	{
		mVectors.push_back({ index, LLVector4(value) });
		mActive = true;
	}

	LL_INLINE void uniform3fv(S32 index, const LLVector3& value)
	{
		mVector3s.push_back({ index, value });
		mActive = true;
	}

	LL_INLINE void uniform3fv(S32 index, const F32* value)
	{
		mVector3s.push_back({ index, LLVector3(value) });
		mActive = true;
	}

	void apply(LLGLSLShader* shader);

private:
	template<typename T>
	struct UniformSetting
	{
		S32	mUniform;
		T	mValue;
	};

	typedef UniformSetting<S32> IntSetting;
	typedef UniformSetting<F32> FloatSetting;
	typedef UniformSetting<LLVector4> VectorSetting;
	typedef UniformSetting<LLVector3> Vector3Setting;

	std::vector<IntSetting>		mIntegers;
	std::vector<FloatSetting>	mFloats;
	std::vector<VectorSetting>	mVectors;
	std::vector<Vector3Setting>	mVector3s;

	bool						mActive;
};

// UI shader
extern LLGLSLShader gUIProgram;

// Output vec4(color.rgb, color.a * tex0[tc0].a)
extern LLGLSLShader	gSolidColorProgram;

// Alpha mask shader (declared here so llappearance can access properly)
extern LLGLSLShader gAlphaMaskProgram;
