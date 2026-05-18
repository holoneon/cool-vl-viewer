/**
 * @file llgltfasset.h
 * @brief LLGLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 *
 * Copyright (c) 2024, Linden Research, Inc.
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

#include "llgltfaccessor.h"
#include "llgltfglm.h"
#include "llpointer.h"
#include "llglslshader.h"
#include "llmemory.h"			// For LL_ALIGNED16_NEW_DELETE
#include "llpointer.h"
#include "llvertexbuffer.h"

class LLGLTexture;
class LLVector2;
class LLVector4a;

// wingdi defines OPAQUE, which conflicts with our enum
#if LL_WINDOWS && defined(OPAQUE)
# undef OPAQUE
#endif

namespace LLGLTF
{
	class Asset;
	class Animation;
	class Primitive;

	class alignas(16) TextureTransform
	{
	protected:
		LOG_CLASS(LLGLTF::TextureTransform);

	public:
		LL_ALIGNED16_NEW_DELETE

		TextureTransform();

		const TextureTransform& operator=(const lljson& src);
		void serialize(lljson& dst) const;

		// Gets the texture transform as a packed array of vec4's. 'dst' *must*
		// point to at least 2 vec4's.
		void getPacked(vec4* dst) const;

	public:
		// Aligned members first...
		vec2	mOffset;
		vec2	mScale;
		// ... then the rest.
		F32		mRotation;
		S32		mTexCoord;
		bool	mPresent;
	};

	class alignas(16) TextureInfo
	{
	protected:
		LOG_CLASS(LLGLTF::TextureInfo);

	public:
		LL_ALIGNED16_NEW_DELETE

		LL_INLINE TextureInfo()
		:	mIndex(INVALID_INDEX),
			mTexCoord(0)
		{
		}

		virtual ~TextureInfo() = default;

		bool operator==(const TextureInfo& rhs) const;
		bool operator!=(const TextureInfo& rhs) const;

		const TextureInfo& operator=(const lljson& src);
		virtual void serialize(lljson& dst) const;

		// Gets the UV channel that should be used for sampling this texture.
		// Returns mTextureTransform.mTexCoord if present and valid, otherwise
		// mTexCoord.
		S32 getTexCoord() const;

	public:
		// Aligned member first...
		TextureTransform	mTextureTransform;
		// ... then the rest.
		S32					mIndex;
		S32					mTexCoord;
	};

	class alignas(16) NormalTextureInfo final : public TextureInfo
	{
	protected:
		LOG_CLASS(LLGLTF::NormalTextureInfo);

	public:
		LL_ALIGNED16_NEW_DELETE

		LL_INLINE NormalTextureInfo()
		:	mScale(1.f)
		{
		}

		const NormalTextureInfo& operator=(const lljson& src);
		void serialize(lljson& dst) const override;

	public:
		F32 mScale;
	};

	class alignas(16) OcclusionTextureInfo final : public TextureInfo
	{
	protected:
		LOG_CLASS(LLGLTF::OcclusionTextureInfo);

	public:
		LL_ALIGNED16_NEW_DELETE

		LL_INLINE OcclusionTextureInfo()
		:	mStrength(1.f)
		{
		}

		const OcclusionTextureInfo& operator=(const lljson& src);
		void serialize(lljson& dst) const override;

	public:
		F32 mStrength;
	};

	class alignas(16) Material
	{
	protected:
		LOG_CLASS(LLGLTF::Material);

	public:
		LL_ALIGNED16_NEW_DELETE

		Material();

		const Material& operator=(const lljson& src);
		void serialize(lljson& dst) const;

		bool isMultiUV() const;

		enum class AlphaMode { OPAQUE, MASK, BLEND };

		class Unlit
		{
		protected:
			LOG_CLASS(LLGLTF::Material::Unlit);

		public:
			LL_INLINE Unlit()
			:	mPresent(false)
			{
			}

			LL_INLINE const Unlit& operator=(const lljson&)
			{
				mPresent = true;
				return *this;
			}

			// No members and object has already been created: nothing to do.
			LL_INLINE void serialize(lljson& dst) const	{}

		public:
			bool mPresent;
		};

		class alignas(16) PbrMetallicRoughness
		{
		protected:
			LOG_CLASS(LLGLTF::Material::PbrMetallicRoughness);

		public:
			LL_ALIGNED16_NEW_DELETE

			PbrMetallicRoughness();

			bool operator==(const PbrMetallicRoughness& rhs) const;
			bool operator!=(const PbrMetallicRoughness& rhs) const;

			const PbrMetallicRoughness& operator=(const lljson& src);
			void serialize(lljson& dst) const;

		public:
			// Aligned members first...
			vec4				mBaseColorFactor;
			TextureInfo			mBaseColorTexture;
			TextureInfo			mMetallicRoughnessTexture;
			// ... then the rest.
			F32					mMetallicFactor;
			F32					mRoughnessFactor;
		};

	public:
		// Aligned members first...
		NormalTextureInfo					mNormalTexture;
		OcclusionTextureInfo				mOcclusionTexture;
		PbrMetallicRoughness				mPbrMetallicRoughness;
		TextureInfo							mEmissiveTexture;
		vec3								mEmissiveFactor;
		// ... then the rest.
		std::string							mName;
		AlphaMode							mAlphaMode;
		F32									mAlphaCutoff;
		Unlit								mUnlit;
		bool								mDoubleSided;
	};

	class alignas(16) Mesh
	{
	protected:
		LOG_CLASS(LLGLTF::Mesh);

	public:
		LL_ALIGNED16_NEW_DELETE

		const Mesh& operator=(const lljson& src);
		void serialize(lljson& dst) const;

		bool prep(Asset& asset);

	public:
		// Aligned member first...
		alignas(16) std::vector<Primitive>	mPrimitives;
		// ... then the rest.
		std::vector<F64>					mWeights;
		std::string							mName;
	};

	class alignas(16) Node
	{
	protected:
		LOG_CLASS(LLGLTF::Node);

	public:
		LL_ALIGNED16_NEW_DELETE

		Node();

		const Node& operator=(const lljson& src);
		void serialize(lljson& dst) const;

		// Updates mAssetMatrix and mAssetMatrixInv
		void updateTransforms(Asset& asset, const mat4& parent_mat);

		// Ensures mMatrix is valid; if mMatrixValid is false and mTRSValid is
		// true, will update mMatrix to match Translation/Rotation/Scale
		void makeMatrixValid();

		// Ensures Translation/Rotation/Scale are valid; if mTRSValid is false
		// and mMatrixValid is true, will update Translation/Rotation/Scale to
		// match mMatrix.
		void makeTRSValid();

		// Sets rotation of this node
		// SIDE EFFECT: invalidates mMatrix
		void setRotation(const quat& rotation);

		// Sets translation of this node
		// SIDE EFFECT: invalidates mMatrix
		void setTranslation(const vec3& translation);

		// Sets scale of this node
		// SIDE EFFECT: invalidates mMatrix
		void setScale(const vec3& scale);

	public:
		// Aligned members first...
		mat4				mMatrix;			// Local transform
		mat4				mRenderMatrix;		// Transform for rendering
		mat4				mAssetMatrix;		// Local to asset space transf.
		mat4				mAssetMatrixInv;	// Asset to local space transf.
		quat				mRotation;
		vec3				mTranslation;
		vec3				mScale;

		// ... then the rest.

		std::string			mName;

		std::vector<S32>	mChildren;
		S32					mParent;
		S32					mMesh;
		S32					mSkin;

		// If true, mMatrix is valid and up to date
		bool				mMatrixValid;

		// If true, translation/rotation/scale are valid and up to date
		bool				mTRSValid;

		bool				mNeedsApplyMatrix;
	};

	class alignas(16) Skin
	{
	protected:
		LOG_CLASS(LLGLTF::Skin);

	public:
		LL_ALIGNED16_NEW_DELETE

		Skin();
		~Skin();

		bool prep(Asset& asset);
		void uploadMatrixPalette(Asset& asset);

		const Skin& operator=(const lljson& src);
		void serialize(lljson& dst) const;

	public:
		// Aligned member first...
		alignas(16) std::vector<mat4>	mInverseBindMatricesData;
		// ... then the rest.
		std::vector<S32>				mJoints;
		std::string						mName;
		S32								mInverseBindMatrices;
		S32								mSkeleton;
		U32								mUBO;
	};

	class Scene
	{
	protected:
		LOG_CLASS(LLGLTF::Scene);

	public:
		const Scene& operator=(const lljson& src);
		void serialize(lljson& dst) const;

		void updateTransforms(Asset& asset);

	public:
		std::string			mName;
		std::vector<S32>	mNodes;
	};

	class Texture
	{
	protected:
		LOG_CLASS(LLGLTF::Texture);

	public:
		Texture();
		
		const Texture& operator=(const lljson& src);
		void serialize(lljson& dst) const;

	public:
		std::string	mName;
		S32			mSampler;
		S32			mSource;
	};

	class Sampler
	{
	protected:
		LOG_CLASS(LLGLTF::Sampler);

	public:
		const Sampler& operator=(const lljson& src);
		void serialize(lljson& dst) const;

	public:
		std::string	mName;
		S32			mMagFilter;
		S32			mMinFilter;
		S32			mWrapS;
		S32			mWrapT;
	};

	class Image
	{
	protected:
		LOG_CLASS(LLGLTF::Image);

	public:
		Image();

		const Image& operator=(const lljson& src);
		void serialize(lljson& dst) const;

		// Saves image to disk; may remove image data from buffer views and
		// convert to file URI if necessary.
		bool save(Asset& asset, const std::string& filename);

		// Erases the buffer view associated with this image and frees any
		// associated GLTF resources. Preserves only the URI and name.
		void clearData(Asset& asset);

		bool prep(Asset& asset, bool load_into_vram);

	public:
		std::string				mName;
		std::string				mUri;
		std::string				mMimeType;
		LLPointer<LLGLTexture>	mTexture;
		S32						mBufferView;
		S32						mWidth;
		S32						mHeight;
		S32						mComponent;
		S32						mBits;
		S32						mPixelType;
		bool					mLoadIntoTexturePipe;
	};

	class RenderBatch
	{
	public:
		struct PrimitiveData
		{
			LL_INLINE PrimitiveData(S32 prim_idx, S32 node_idx)
			:	mPrimitiveIndex(prim_idx),
				mNodeIndex(node_idx)
			{
			}

			S32 mPrimitiveIndex = INVALID_INDEX;
			S32 mNodeIndex = INVALID_INDEX;
		};

	public:
		LLPointer<LLVertexBuffer>	mVertexBuffer;
		std::vector<PrimitiveData>	mPrimitives;
	};

	struct RenderData
	{
		std::vector<RenderBatch> mBatches[LLGLSLShader::NUM_GLTF_VARIANTS];
	};

	class alignas(16) Asset : public LLRefCount
	{
	protected:
		LOG_CLASS(LLGLTF::Asset);

	public:
		LL_ALIGNED16_NEW_DELETE

		Asset();
		Asset(const lljson& src);

		~Asset() override;

		// Prepares the asset for rendering.
		// Returns true on success, or false on failure (at which point this
		// asset must be destroyed and never used). HB
		bool prep();

		// Called periodically (typically once per frame). Any ongoing work
		// (such as animations) should be handled here NOT guaranteed to be
		// called every frame. MAY be called more than once per frame. Upon
		// return, all Node Matrix transforms should be up to date.
		void update();

		// Updates asset-to-node and node-to-asset transforms
		void updateTransforms();

		// Uploads matrices to UBO
		void uploadTransforms();

		// Uploads materials to UBO
		void uploadMaterials();

		// Return the index of the node that the line segment intersects with,
		// or -1 if no hit input and output values must be in this asset's
		// local coordinate frame
		S32 lineSegmentIntersect(const LLVector4a& start,
								 const LLVector4a& end,
								 LLVector4a* intersectp = NULL,
								 LLVector2* tcoordp = NULL,
								 LLVector4a* normp = NULL,
								 LLVector4a* tangentp = NULL,
								 S32* prim_hitp = NULL);

		const Asset& operator=(const lljson& src);
		void serialize(lljson& dst) const;

		// Loads from given file (.gltf or .glb). Any existing data will be
		// lost. Returns result of prep() on success.
		bool load(std::string_view filename, bool load_into_vram = true);
		// Loads from a .glb file or from binary contents of data. Returns
		// result of prep() on success.
		bool loadBinary(const std::string& data, bool load_into_vram =  true);
		// Saves the asset to the given .gltf file with images and binaries
		// along it.
		bool save(const std::string& filename);

		// Removes the bufferview at the given index and updates all bufferview
		// indices in this Asset as needed.
		void eraseBufferView(S32 bufferview);

		LL_INLINE bool isLocalPreview() const	{ return !mFilename.empty(); }

		// These two methods are here to avoid having to include fson.hpp from
		// llgltfscenemanager.cpp. HB
		void serializeToString(std::string& buffer);
		static Asset* createFromJsonData(const std::string& data);

		// Hooks into functions provided by newview. HB
		typedef LLGLTexture* (*gltex_from_fetched_fn_t)(const LLUUID& id);
		typedef LLGLTexture* (*gltex_from_file_fn_t)(const std::string& fname);
		typedef LLGLTexture* (*gltex_from_memory_fn_t)(U8* datap, S32 size,
													   const std::string& mime);
		typedef const std::string& (*localtex_filename_fn_t)(const LLUUID& id);
		// This must be called on viewer initalization, before using any glTF
		// 'Asset'. HB
		static void setHooks(gltex_from_fetched_fn_t fn1,
							 gltex_from_file_fn_t fn2,
							 gltex_from_memory_fn_t fn3,
							 localtex_filename_fn_t fn4);

	public:
		// Note: since this class is LLRefCounted, there is a 32 bits (4 bytes)
		// counter before the following members: let's avoid 12 padding bytes
		// and use the corresponding space for three 32 bits member variables.
		// HB
		
		// The last time update() was called according to
		// LLFrameTimer::getElapsedSeconds()
		F32									mLastUpdateTime;
		// UBO for storing node transforms
		U32									mNodesUBO;
		// UBO for storing material data
		U32									mMaterialsUBO;

		// Aligned members...
		alignas(16) std::vector<Node>		mNodes;
		alignas(16) std::vector<Mesh>		mMeshes;
		alignas(16) std::vector<Skin>		mSkins;
		alignas(16) std::vector<Material>	mMaterials;
		alignas(16) std::vector<Animation>	mAnimations;
		// ... then the rest.
		std::vector<Scene>					mScenes;
		std::vector<Texture>				mTextures;
		std::vector<Sampler>				mSamplers;
		std::vector<Image>					mImages;
		std::vector<Buffer>					mBuffers;
		std::vector<BufferView>				mBufferViews;
		std::vector<Accessor>				mAccessors;
		std::vector<std::string>			mExtensionsUsed;
		std::vector<std::string>			mExtensionsRequired;
		std::vector<std::string>			mUnsupportedExtensions;
		std::vector<std::string>			mIgnoredExtensions;

		// Local file this asset was loaded from (if any)
		std::string							mFilename;
		std::string							mVersion;
		std::string							mGenerator;
		std::string							mMinVersion;
		std::string							mCopyright;
#if 0	// Not used (for now ?)...
		lljson								mExtras;
#endif
		// Data used for rendering. Index is used 0 for single-sided and 1 for
		// double-sided faces.
		RenderData							mRenderData[2];

		S32									mScene;

		// Used during asset data upload
		U32									mPendingBuffers;

		bool								mLoadIntoVRAM;

		static gltex_from_fetched_fn_t		sTexFromFetchedFn;
		static gltex_from_file_fn_t			sTexFromFileFn;
		static gltex_from_memory_fn_t		sTexFromMemoryFn;
		static localtex_filename_fn_t		sGetLocaTexFilenameFn;
	};

	Material::AlphaMode gltf_alpha_mode_to_enum(const std::string& alpha_mode);
	const std::string& enum_to_gltf_alpha_mode(Material::AlphaMode alpha_mode);
}
