/**
 * @file llavatarappearance.h
 * @brief Declaration of LLAvatarAppearance class
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

#include "llavatarappearancedefines.h"
#include "llavatarjointmesh.h"
#include "llcharacter.h"
#include "lldriverparam.h"
#include "hbfastmap.h"
#include "llstring.h"
#include "lltexlayer.h"
#include "llviewervisualparam.h"
#include "llxmltree.h"

class LLAvatarBoneInfo;
class LLAvatarSkeletonInfo;
class LLTexGlobalColor;
class LLTexGlobalColorInfo;
class LLTexLayerSet;
class LLWearableData;

class LLAvatarAppearance : public LLCharacter
{
protected:
	LOG_CLASS(LLAvatarAppearance);

public:
	//-------------------------------------------------------------------------
	// INITIALIZATION

	LLAvatarAppearance() = delete;	// No default constructor allowed

	LLAvatarAppearance(LLWearableData* wearable_data);
	~LLAvatarAppearance() override;

	// Initializes static members
	static void initClass(const std::string& lad_file = LLStringUtil::null,
						  const std::string& skel_file = LLStringUtil::null);

	// Cleanup data that is only initialised once per class.
	static void cleanupClass();

	// Called after construction to initialize the instance:
	virtual void initInstance();

	virtual bool loadSkeletonNode();
	bool loadMeshNodes();
	bool loadLayersets();

	//-------------------------------------------------------------------------
	// INHERITED (LLCharacter interface and related)
public:
	LLJoint* getCharacterJoint(U32 num) override;

	LL_INLINE const char* getAnimationPrefix() override
	{
		return "avatar";
	}

	LLVector3 getVolumePos(S32 joint_index, LLVector3& volume_offset) override;
	LLJoint* findCollisionVolume(S32 volume_id) override;
	S32 getCollisionVolumeID(std::string& name) override;
	LLPolyMesh* getHeadMesh() override;
	LLPolyMesh* getUpperBodyMesh() override;
	LLPolyMesh* getMesh(S32 which) override;

	//-------------------------------------------------------------------------
	// STATE
public:
	// True if this avatar is for this viewer's agent
	LL_INLINE virtual bool isSelf() const				{ return false; }
	// True if this avatar is for this viewer's agent and it is valid
	LL_INLINE virtual bool isValid() const				{ return false; }

	virtual bool isUsingServerBakes() const = 0;
	virtual bool isUsingLocalAppearance() const = 0;
	virtual bool isEditingAppearance() const = 0;

	LL_INLINE bool isBuilt() const						{ return mIsBuilt; }

	//-------------------------------------------------------------------------
	// SKELETON
 
protected:
	virtual LLAvatarJoint* createAvatarJoint() = 0;
	virtual LLAvatarJointMesh* createAvatarJointMesh() = 0;

	void makeJointAliases(LLAvatarBoneInfo* bone_info);

public:
	LL_INLINE F32 getPelvisToFoot() const				{ return mPelvisToFoot; }
	LL_INLINE LLJoint* getRootJoint() override			{ return mRoot; }

	virtual void computeBodySize(bool force = false);

	typedef std::vector<LLAvatarJoint*> avatar_joint_list_t;
	LL_INLINE const avatar_joint_list_t& getSkeleton()	{ return mSkeleton; }

	const strings_map_t& getJointAliases();

	LLJoint* getSkeletonJoint(S32 joint_num);

	static const std::vector<LLAvatarBoneInfo*>& getBoneInfoList();

protected:
	static bool parseSkeletonFile(const std::string& filename,
								  LLXmlTree& skel_xml_tree);
	virtual void buildCharacter();
	virtual bool loadAvatar();
	virtual void bodySizeChanged() = 0;

	bool setupBone(const LLAvatarBoneInfo* infop, LLJoint* parentp,
				   S32& current_volume_num, S32& current_joint_num);
	bool allocateCharacterJoints(U32 num);
	bool buildSkeleton(const LLAvatarSkeletonInfo* infop);
	void clearSkeleton();

	// Pelvis height adjustment members.
public:
	void addPelvisFixup(F32 fixup, const LLUUID& mesh_id);
	void removePelvisFixup(const LLUUID& mesh_id);
	bool hasPelvisFixup(F32& fixup, LLUUID& mesh_id) const;
	bool hasPelvisFixup(F32& fixup) const;

public:
	LLVector3				mHeadOffset; // current head position
	LLAvatarJoint*			mRoot;

	typedef flat_hmap<U32, LLJoint*> joint_map_t;
	joint_map_t				mJointMap;

	LLVector3				mBodySize;
	LLVector3				mAvatarOffset;

protected:
	// State of deferred character building
	bool					mIsBuilt;

	F32						mPelvisToFoot;

	avatar_joint_list_t		mSkeleton;
	strings_map_t			mJointAliasMap;

	LLVector3OverrideMap	mPelvisFixups;

	// Cached pointers to well known joints
public:
	LLJoint*				mPelvisp;
	LLJoint* 				mTorsop;
	LLJoint* 				mChestp;
	LLJoint* 				mNeckp;
	LLJoint* 				mHeadp;
	LLJoint* 				mSkullp;
	LLJoint* 				mEyeLeftp;
	LLJoint* 				mEyeRightp;
	LLJoint* 				mHipLeftp;
	LLJoint* 				mHipRightp;
	LLJoint* 				mKneeLeftp;
	LLJoint* 				mKneeRightp;
	LLJoint* 				mAnkleLeftp;
	LLJoint* 				mAnkleRightp;
	LLJoint* 				mFootLeftp;
	LLJoint* 				mFootRightp;
	LLJoint* 				mWristLeftp;
	LLJoint* 				mWristRightp;

	// XML parse tree
protected:
	static LLAvatarSkeletonInfo*	sAvatarSkeletonInfo;

	struct LLAvatarXmlInfo;
	static LLAvatarXmlInfo*			sAvatarXmlInfo;


	//-------------------------------------------------------------------------
	// RENDERING
public:
	// true when for special views and animated objects puppet avatars, which
	// are both local to the viewer.
	bool		mIsDummy;

	// Morph masks
public:
	virtual void applyMorphMask(U8* tex_data, S32 width, S32 height,
								S32 num_components,
								LLAvatarAppearanceDefines::EBakedTextureIndex index =
									LLAvatarAppearanceDefines::BAKED_NUM_INDICES) = 0;
private:
	void addMaskedMorph(LLAvatarAppearanceDefines::EBakedTextureIndex index,
						LLVisualParam* morph_target, bool invert,
						const std::string& layer);

	// Composites
public:
	virtual void	invalidateComposite(LLTexLayerSet* layerset,
										bool upload_result) = 0;

	//-------------------------------------------------------------------------
	// MESHES
public:
	virtual void updateMeshTextures() = 0;
	virtual void dirtyMesh() = 0; // Dirty the avatar mesh

	LL_INLINE avatar_joint_list_t getMeshLOD()			{ return mMeshLOD; }

protected:
	// Dirty the avatar mesh, with priority
	virtual void dirtyMesh(S32 priority) = 0;

protected:
	typedef std::multimap<std::string, LLPolyMesh*> polymesh_map_t;
	polymesh_map_t			mPolyMeshes;
	avatar_joint_list_t		mMeshLOD;

	//-------------------------------------------------------------------------
	// APPEARANCE

	// Clothing colors (convenience functions to access visual parameters)
public:
	void setClothesColor(LLAvatarAppearanceDefines::ETextureIndex te,
						 const LLColor4& new_color, bool upload_bake);
	LLColor4 getClothesColor(LLAvatarAppearanceDefines::ETextureIndex te);
	static bool teToColorParams(LLAvatarAppearanceDefines::ETextureIndex te,
								U32* param_name);

	// Global colors
public:
	LLColor4 getGlobalColor(const std::string& color_name) const;
	virtual void onGlobalColorChanged(const LLTexGlobalColor* global_color,
									  bool upload_bake) = 0;
protected:
	LLTexGlobalColor* mTexSkinColor;
	LLTexGlobalColor* mTexHairColor;
	LLTexGlobalColor* mTexEyeColor;

	// Visibility
public:
	static LLColor4 getDummyColor();

	//-------------------------------------------------------------------------
	// WEARABLES
public:
	LL_INLINE LLWearableData* getWearableData()			{ return mWearableData; }

	LL_INLINE const LLWearableData*	getWearableData() const
	{
		return mWearableData;
	}

	virtual bool isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex te,
								  U32 index = 0) const = 0;
	virtual bool isWearingWearableType(LLWearableType::EType type) const;

private:
	LLWearableData* mWearableData;

	//-------------------------------------------------------------------------
	// BAKED TEXTURES
public:
	LLTexLayerSet* getAvatarLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;

protected:
	virtual LLTexLayerSet* createTexLayerSet() = 0;

protected:
	class LLMaskedMorph;
	typedef std::deque<LLMaskedMorph*> morph_list_t;
	struct BakedTextureData
	{
		avatar_joint_mesh_list_t					mJointMeshes;
		morph_list_t								mMaskedMorphs;
		LLUUID										mLastTextureID;
		// Only exists for self
		LLTexLayerSet*		 						mTexLayerSet;
		LLAvatarAppearanceDefines::ETextureIndex 	mTextureIndex;
		U32											mMaskTexName;
		// Stores pointers to the joint meshes that this baked texture deals with
		bool										mIsLoaded;
		bool										mIsUsed;
	};
	typedef std::vector<BakedTextureData> bakedtexturedata_vec_t;
	bakedtexturedata_vec_t 							mBakedTextureDatas;

	//-------------------------------------------------------------------------
	// PHYSICS

	// Collision volumes
protected:
	void clearCollisionVolumes();
	bool allocateCollisionVolumes(U32 num);

public:
	S32							mNumBones;
	typedef std::vector<LLAvatarJointCollisionVolume*> collision_volumes_list_t;
	collision_volumes_list_t	mCollisionVolumes;

	//-------------------------------------------------------------------------
	// SUPPORT CLASSES
protected:
	struct LLAvatarXmlInfo
	{
		LLAvatarXmlInfo();
		~LLAvatarXmlInfo();

		bool parseXmlSkeletonNode(LLXmlTreeNode* root);
		bool parseXmlMeshNodes(LLXmlTreeNode* root);
		bool parseXmlColorNodes(LLXmlTreeNode* root);
		bool parseXmlLayerNodes(LLXmlTreeNode* root);
		bool parseXmlDriverNodes(LLXmlTreeNode* root);
		bool parseXmlMorphNodes(LLXmlTreeNode* root);

		struct LLAvatarMeshInfo
		{
			// LLPolyMorphTargetInfo stored here:
			typedef std::pair<LLViewerVisualParamInfo*, bool> morph_info_pair_t;

			typedef std::vector<morph_info_pair_t> morph_info_list_t;

			LLAvatarMeshInfo()
			:	mLOD(0),
				mMinPixelArea(.1f)
			{
			}

			~LLAvatarMeshInfo()
			{
				for (morph_info_list_t::iterator
						iter = mPolyMorphTargetInfoList.begin(),
						end = mPolyMorphTargetInfoList.end();
					 iter != end; ++iter)
				{
					delete iter->first;
				}
				mPolyMorphTargetInfoList.clear();
			}

			std::string mType;
			S32			mLOD;
			std::string	mMeshFileName;
			std::string	mReferenceMeshName;
			F32			mMinPixelArea;
			morph_info_list_t mPolyMorphTargetInfoList;
		};
		typedef std::vector<LLAvatarMeshInfo*> mesh_info_list_t;
		mesh_info_list_t mMeshInfoList;

		// LLPolySkeletalDistortionInfo stored here:
		typedef std::vector<LLViewerVisualParamInfo*> skeletal_distortion_info_list_t;

		skeletal_distortion_info_list_t mSkeletalDistortionInfoList;

		struct LLAvatarAttachmentInfo
		{
			LLAvatarAttachmentInfo()
			:	mGroup(-1),
				mAttachmentID(-1),
				mPieMenuSlice(-1),
				mVisibleFirstPerson(false),
				mIsHUDAttachment(false),
				mHasPosition(false),
				mHasRotation(false)
			{
			}

			std::string	mName;
			std::string	mJointName;
			U32			mJointKey;
			LLVector3	mPosition;
			LLVector3	mRotationEuler;
			S32			mGroup;
			S32			mAttachmentID;
			S32			mPieMenuSlice;
			bool		mVisibleFirstPerson;
			bool		mIsHUDAttachment;
			bool		mHasPosition;
			bool		mHasRotation;
		};
		typedef std::vector<LLAvatarAttachmentInfo*> attachment_info_list_t;
		attachment_info_list_t	mAttachmentInfoList;

		LLTexGlobalColorInfo*	mTexSkinColorInfo;
		LLTexGlobalColorInfo*	mTexHairColorInfo;
		LLTexGlobalColorInfo*	mTexEyeColorInfo;

		typedef std::vector<LLTexLayerSetInfo*> layer_info_list_t;
		layer_info_list_t		mLayerInfoList;

		typedef std::vector<LLDriverParamInfo*> driver_info_list_t;
		driver_info_list_t		mDriverInfoList;

		struct LLAvatarMorphInfo
		{
			LLAvatarMorphInfo()
			:	mInvert(false)
			{
			}

			std::string	mName;
			std::string	mRegion;
			std::string	mLayer;
			bool		mInvert;
		};

		typedef std::vector<LLAvatarMorphInfo*> morph_info_list_t;
		morph_info_list_t		mMorphMaskInfoList;
	};

	class LLMaskedMorph
	{
	public:
		LLMaskedMorph(LLVisualParam* morph_target, bool invert,
					  const std::string& layer);

	public:
		LLVisualParam*	mMorphTarget;
		std::string		mLayer;
		bool			mInvert;
	};
};
