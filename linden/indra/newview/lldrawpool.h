/**
 * @file lldrawpool.h
 * @brief LLDrawPool class definition
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

#include "llcolor4u.h"
#include "llvector2.h"
#include "llvector3.h"
#include "llvertexbuffer.h"

#include "llviewertexturelist.h"

class LLDrawInfo;
class LLFace;
class LLMatrix4;
class LLMeshSkinInfo;
class LLSpatialGroup;
class LLViewerTexture;
class LLViewerFetchedTexture;
class LLVOAvatar;

class LLDrawPool
{
protected:
	LOG_CLASS(LLDrawPool);

public:
	enum : U32
	{
		// Correspond to LLPipeline render type (and to gPoolNames).
		// Also controls render order, so passes that do not use alpha masking
		// or blending should come before other passes to preserve hierarchical
		// Z for occlusion queries. Occlusion queries happen just before grass,
		// so grass should be the first alpha masked pool. Other ordering
		// should be done based on fill rate and likelihood to occlude future
		// passes (faster, large occluders first).
		POOL_SKY = 1,
		POOL_WATEREXCLUSION,
		POOL_WL_SKY,
		POOL_SIMPLE,
		POOL_FULLBRIGHT,
		POOL_BUMP,
		POOL_MATERIALS,
		POOL_MAT_PBR,				// PBR only
		POOL_TERRAIN,
		POOL_GRASS,
		POOL_MAT_PBR_ALPHA_MASK,	// PBR only
		POOL_TREE,
		POOL_ALPHA_MASK,
		POOL_FULLBRIGHT_ALPHA_MASK,
		POOL_INVISIBLE,				// EE only (*)
		POOL_AVATAR,
		POOL_PUPPET,				// Animesh
		POOL_GLOW,
		POOL_ALPHA_PRE_WATER,		// PBR only
		POOL_VOIDWATER,
		POOL_WATER,
		POOL_ALPHA_POST_WATER,		// PBR only
		// Note: for PBR, there is no actual "POOL_ALPHA" but pre-water and
		// post-water pools consume POOL_ALPHA faces.
		POOL_ALPHA,
		NUM_POOL_TYPES,
		// (*) Invisiprims work by rendering to the depth buffer but not the
		//     color buffer, occluding anything rendered after them and the
		//     LLDrawPool types enum controls what order things are rendered
		//     in so, it has absolute control over what invisprims block,
		//     invisiprims being rendered in pool_invisible shiny/bump mapped
		//     objects in rendered in POOL_BUMP.
	};

	LLDrawPool(U32 type);
	virtual ~LLDrawPool() = default;

	virtual bool isDead() = 0;

	LL_INLINE S32 getId() const								{ return mId; }
	LL_INLINE U32 getType() const							{ return mType; }
	LL_INLINE S32 getShaderLevel() const					{ return mShaderLevel; }

	// No more in use with the PBR renderer
	virtual U32 getVertexDataMask() = 0;

	LL_INLINE virtual void prerender()						{}

	// Unless overridden, returns 1 in EE rendering mode and 0 in PBR mode
	// (no forward rendering available for the latter). HB
	virtual S32 getNumPasses();
	LL_INLINE virtual void beginRenderPass(S32 pass)		{}
	virtual void endRenderPass(S32 pass);
	LL_INLINE virtual void render(S32 pass = 0)				{}

	LL_INLINE virtual S32 getNumDeferredPasses()			{ return 0; }
	LL_INLINE virtual void beginDeferredPass(S32 pass)		{}
	LL_INLINE virtual void endDeferredPass(S32 pass)		{}
	LL_INLINE virtual void renderDeferred(S32 pass = 0)		{}

	LL_INLINE virtual S32 getNumPostDeferredPasses()		{ return 0; }
	LL_INLINE virtual void beginPostDeferredPass(S32 pass)	{}
	LL_INLINE virtual void endPostDeferredPass(S32 pass)	{}
	LL_INLINE virtual void renderPostDeferred(S32 p = 0)	{}

	LL_INLINE virtual S32 getNumShadowPasses()				{ return 0; }
	LL_INLINE virtual void beginShadowPass(S32 pass)		{}
	LL_INLINE virtual void endShadowPass(S32 pass)			{}
	LL_INLINE virtual void renderShadow(S32 pass = 0)		{}

	// Verifies that all data in the draw pool is correct.
	LL_INLINE virtual bool verify() const					{ return true; }

	LL_INLINE virtual bool isFacePool()						{ return false; }
	LL_INLINE virtual bool isTerrainPool()					{ return false; }

	// Overridden in LLDrawPoolTerrain and LLDrawPoolTree.
	LL_INLINE virtual LLViewerTexture* getTexture()			{ return NULL; }

	// Overridden in LLFacePool only.

	LL_INLINE virtual void pushFaceGeometry()				{}
	LL_INLINE virtual void resetDrawOrders()				{}

	static LLDrawPool* createPool(U32 type, LLViewerTexture* tex0p = NULL);

	static std::string getPoolName(U32 type);

protected:
	U32			mType;				// Type of draw pool
	S32			mId;
	S32			mShaderLevel;

private:
	static S32	sNumDrawPools;
};

class LLRenderPass : public LLDrawPool
{
public:
	// List of possible LLRenderPass types to assign a render batch to.
	// IMPORTANT: the "rigged" variant MUST be non-rigged variant + 1 !
	enum : U32
	{
		PASS_SIMPLE = NUM_POOL_TYPES,
		PASS_SIMPLE_RIGGED,
		PASS_GRASS,
		PASS_FULLBRIGHT,
		PASS_FULLBRIGHT_RIGGED,
		PASS_INVISIBLE,				// Also for PBR water exclusion surfaces.
		PASS_INVISIBLE_RIGGED,
		PASS_INVISI_SHINY,
		PASS_INVISI_SHINY_RIGGED,
		PASS_FULLBRIGHT_SHINY,
		PASS_FULLBRIGHT_SHINY_RIGGED,
		PASS_SHINY,
		PASS_SHINY_RIGGED,
		PASS_BUMP,
		PASS_BUMP_RIGGED,
		PASS_POST_BUMP,
		PASS_POST_BUMP_RIGGED,
		PASS_MATERIAL,
		PASS_MATERIAL_RIGGED,
		PASS_MATERIAL_ALPHA,
		PASS_MATERIAL_ALPHA_RIGGED,
		PASS_MATERIAL_ALPHA_MASK,
		PASS_MATERIAL_ALPHA_MASK_RIGGED,
		PASS_MATERIAL_ALPHA_EMISSIVE,
		PASS_MATERIAL_ALPHA_EMISSIVE_RIGGED,
		PASS_SPECMAP,
		PASS_SPECMAP_RIGGED,
		PASS_SPECMAP_BLEND,
		PASS_SPECMAP_BLEND_RIGGED,
		PASS_SPECMAP_MASK,
		PASS_SPECMAP_MASK_RIGGED,
		PASS_SPECMAP_EMISSIVE,
		PASS_SPECMAP_EMISSIVE_RIGGED,
		PASS_NORMMAP,
		PASS_NORMMAP_RIGGED,
		PASS_NORMMAP_BLEND,
		PASS_NORMMAP_BLEND_RIGGED,
		PASS_NORMMAP_MASK,
		PASS_NORMMAP_MASK_RIGGED,
		PASS_NORMMAP_EMISSIVE,
		PASS_NORMMAP_EMISSIVE_RIGGED,
		PASS_NORMSPEC,
		PASS_NORMSPEC_RIGGED,
		PASS_NORMSPEC_BLEND,
		PASS_NORMSPEC_BLEND_RIGGED,
		PASS_NORMSPEC_MASK,
		PASS_NORMSPEC_MASK_RIGGED,
		PASS_NORMSPEC_EMISSIVE,
		PASS_NORMSPEC_EMISSIVE_RIGGED,
		PASS_GLOW,
		PASS_GLOW_RIGGED,
		PASS_PBR_GLOW,
		PASS_PBR_GLOW_RIGGED,
		PASS_ALPHA,
		PASS_ALPHA_RIGGED,
		PASS_ALPHA_MASK,
		PASS_ALPHA_MASK_RIGGED,
		PASS_FULLBRIGHT_ALPHA_MASK,
		PASS_FULLBRIGHT_ALPHA_MASK_RIGGED,
		PASS_ALPHA_INVISIBLE,
		PASS_ALPHA_INVISIBLE_RIGGED,
		PASS_MAT_PBR,
		PASS_MAT_PBR_RIGGED,
		PASS_MAT_PBR_ALPHA_MASK,
		PASS_MAT_PBR_ALPHA_MASK_RIGGED,
		NUM_RENDER_TYPES,
	};

	LL_INLINE LLRenderPass(U32 type)
	:	LLDrawPool(type)
	{
	}

	LL_INLINE bool isDead() override					{ return false; }

	// NOTE: in the following methods, the 'mask' parameter is ignored for the
	// PBR renderer. I kept them "as is" to avoid duplicating too much code for
	// the dual EE-PBR renderer. HB

	void pushBatches(U32 type, U32 mask, bool texture = true,
					 bool batch_textures = false);
	void pushRiggedBatches(U32 type, U32 mask, bool texture = true,
						   bool batch_textures = false);
	void pushMaskBatches(U32 type, U32 mask, bool texture = true,
						 bool batch_textures = false);
	void pushRiggedMaskBatches(U32 type, U32 mask, bool texture = true,
							   bool batch_textures = false);
	// Overridden in LLDrawPoolBump only. HB
	virtual void pushBatch(LLDrawInfo& params, U32 mask, bool texture,
						   bool batch_textures = false);
	// For PBR rendering
	static void pushUntexturedBatches(U32 type);
	static void pushUntexturedRiggedBatches(U32 type);
	static void pushUntexturedBatch(LLDrawInfo& params);
	static void pushGLTFBatches(U32 type);
	static void pushGLTFBatch(LLDrawInfo& params);
	// Like pushGLTFBatches, but will not bind textures or set up texture
	// transforms.
	static void pushUntexturedGLTFBatches(U32 type);
	// Rigged variants of above
	static void pushRiggedGLTFBatches(U32 type);
	static void pushUntexturedGLTFBatch(LLDrawInfo& params);
	static void pushUntexturedRiggedGLTFBatches(U32 type);

	// Helper methods for dispatching to textured or untextured pass based on
	// boolean 'textured' (declared const and methods inlined to try and hint
	// the compiler to optimize it out - HB).

	LL_INLINE static void pushGLTFBatches(U32 type, const bool textured)
	{
		if (textured)
		{
			pushGLTFBatches(type);
		}
		else
		{
			pushUntexturedGLTFBatches(type);
		}
	}

	LL_INLINE static void pushRiggedGLTFBatches(U32 type, const bool textured)
	{
		if (textured)
		{
			pushRiggedGLTFBatches(type);
		}
		else
		{
			pushUntexturedRiggedGLTFBatches(type);
		}
	}

	virtual void renderGroup(LLSpatialGroup* groupp, U32 type, U32 mask,
							 bool texture = true);
	virtual void renderRiggedGroup(LLSpatialGroup* groupp, U32 type, U32 mask,
								   bool texture = true);

	static void applyModelMatrix(LLDrawInfo& params);
	// For rendering that does not use LLDrawInfo for some reason.
	static void applyModelMatrix(const LLMatrix4* model_matp);
	static bool uploadMatrixPalette(const LLDrawInfo& params);
	static bool uploadMatrixPalette(LLVOAvatar* avp, LLMeshSkinInfo* skinp);
};

class LLFacePool : public LLDrawPool
{
protected:
	LOG_CLASS(LLFacePool);

public:
	typedef std::vector<LLFace*> face_vec_t;

	enum
	{
		SHADER_LEVEL_SCATTERING = 2
	};

public:
	LLFacePool(U32 type);
	~LLFacePool() override;

	LL_INLINE bool isDead() override					{ return mReferences.empty(); }

	virtual void enqueue(LLFace* facep);
	virtual bool addFace(LLFace* facep);
	virtual bool removeFace(LLFace* facep);

	// Verifies that all data in the draw pool is correct.
	bool verify() const override;

	void pushFaceGeometry() override;

	void resetDrawOrders() override;
	void resetAll();

	void destroy();

	void buildEdges();

	void addFaceReference(LLFace* facep);
	void removeFaceReference(LLFace* facep);

	void printDebugInfo() const;

	LL_INLINE bool isFacePool() override				{ return true; }

	friend class LLFace;
	friend class LLPipeline;

public:
	face_vec_t	mDrawFace;
	face_vec_t	mMoveFace;
	face_vec_t	mReferences;

public:
	class LLOverrideFaceColor
	{
	public:
		LL_INLINE LLOverrideFaceColor(LLDrawPool* pool)
		:	mOverride(sOverrideFaceColor),
			mPool(pool)
		{
			sOverrideFaceColor = true;
		}

		LL_INLINE LLOverrideFaceColor(LLDrawPool* pool, const LLColor4& color)
		:	mOverride(sOverrideFaceColor),
			mPool(pool)
		{
			sOverrideFaceColor = true;
			setColor(color);
		}

		LL_INLINE LLOverrideFaceColor(LLDrawPool* pool, const LLColor4U& color)
		:	mOverride(sOverrideFaceColor),
			mPool(pool)
		{
			sOverrideFaceColor = true;
			setColor(color);
		}

		LL_INLINE LLOverrideFaceColor(LLDrawPool* pool,
									  F32 r, F32 g, F32 b, F32 a)
		:	mOverride(sOverrideFaceColor),
			mPool(pool)
		{
			sOverrideFaceColor = true;
			setColor(r, g, b, a);
		}

		LL_INLINE ~LLOverrideFaceColor()
		{
			sOverrideFaceColor = mOverride;
		}

		void setColor(const LLColor4& color);
		void setColor(const LLColor4U& color);
		void setColor(F32 r, F32 g, F32 b, F32 a);

	public:
		LLDrawPool*	mPool;
		bool		mOverride;
		static bool	sOverrideFaceColor;
	};
};
