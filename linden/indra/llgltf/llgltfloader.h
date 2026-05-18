/**
 * @file llgltfloader.h
 * @brief LLGLTFLoader definition
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 *
 * Copyright (c) 2022, Linden Research, Inc.
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

#include "llgltfasset.h"
#include "llgltfjointdata.h"
#include "llmemory.h"			// For LL_ALIGNED16_NEW_DELETE
#include "llmodelloader.h"

using namespace LLGLTF;

class LLGLTFLoader : public LLModelLoader
{
protected:
	LOG_CLASS(LLGLTFLoader);

public:
	typedef std::map<std::string, glm::mat4, std::less<> > joint_rest_map_t;
	typedef std::map<S32, glm::mat4> joint_node_map_t;

	LLGLTFLoader(const std::string& filename, S32 lod,
				 load_callback_t load_cb,
				 joint_lookup_func_t joint_lookup_func,
				 texture_load_func_t texture_load_func,
				 state_callback_t state_cb, void* userdata,
				 JointTransformMap& joint_transform_map,
				 JointNameSet& joints_from_nodes,
				 strings_map_t& legal_joint_names, U32 max_joints_per_mesh,
				 U32 model_limit);

	bool openFile(const std::string& filename) override;

private:
	bool parseMeshes();
	void computeCombinedNodeTransform(const Asset& asset, S32 node_index,
									  glm::mat4& combined_transform) const;
	typedef std::map<std::string, S32, std::less<> > mesh_count_mat_t;
	void processNodeHierarchy(S32 node_idx, mesh_count_mat_t& mesh_name_counts,
							  U32 submodel_limit,
							  const LLVolumeParams& volume_params);
	bool addJointToModelSkin(LLMeshSkinInfo& skin_info, S32 gltf_skin_idx,
							 S32 gltf_joint_idx);
	bool populateModelFromMesh(LLModel* modelp, const std::string& base_name,
							   const Mesh& mesh, const Node& node,
							   mats_map_t& mats, S32 instance_count);
	void populateJointsFromSkin(S32 skin_idx);
	void populateJointGroups();
	void addModelToScene(LLModel* modelp, const std::string& base_name,
						 U32 submodel_limit, const LLMatrix4& transformation,
						 const LLVolumeParams& volume_params,
						 const mats_map_t& mats);
	void buildJointGroup(JointData& viewer_data,
						 const std::string& parent_group);

	class alignas(16) JointNodeData
	{
	public:
		LL_ALIGNED16_NEW_DELETE

		LL_INLINE JointNodeData()
		:	mJointListIdx(-1),
			mNodeIdx(-1),
			mParentNodeIdx(-1),
			mIsValidViewerJoint(false),
			mIsParentValidViewerJoint(false),
			mIsOverrideValid(false)
		{
		}

	public:
		mat4		mGltfRestMatrix;
		mat4		mViewerRestMatrix;
		mat4		mOverrideRestMatrix;
		mat4		mGltfMatrix;
		mat4		mOverrideMatrix;
		std::string	mName;
		S32			mJointListIdx;
		S32			mNodeIdx;
		S32			mParentNodeIdx;
		bool		mIsValidViewerJoint;
		bool		mIsParentValidViewerJoint;
		bool		mIsOverrideValid;
	};
	typedef std::map <S32, JointNodeData> joints_data_map_t;
	typedef std::map <std::string, S32> joints_name_to_node_map_t;
	void buildOverrideMatrix(const JointData& data,
							 joints_data_map_t& gltf_nodes,
							 joints_name_to_node_map_t& names_to_nodes,
							 mat4& parent_rest, mat4& support_rest) const;
	mat4 buildGltfRestMatrix(S32 joint_node_idx, const Skin& gltf_skin) const;
	mat4 computeGltfToViewerSkeletonTransform(const joints_data_map_t& joints,
											  S32 gltf_node_index,
											  const std::string& joint_name) const;
	bool checkForXYRotation(const Skin& gltf_skin, S32 joint_idx,
							S32 bind_idx);
	void checkForXYRotation(const Skin& gltf_skin);
	void checkGlobalJointUsage();
	std::string extractTextureToTempFile(S32 tex_idx, const char* tex_type);
	void notifyUnsupportedExtension(bool unsupported);

	LLImportMaterial& processMaterial(S32 mat_idx, S32 fallback_idx);
	std::string processTexture(std::string& full_path, S32 tex_idx,
							   const char* tex_type,
							   const std::string& mat_name);
	bool validateTextureIndex(S32 tex_idx, S32& src_idx) const;
	std::string generateMaterialName(S32 mat_idx, S32 fallback_idx = -1) const;

protected:
	Asset					mGLTFAsset;

	// glTF is not aware of the viewer skeleton and uses its own, so we need to
	// take the viewer joints and use them to recalculate inverse bind matrices
	JointData::bones_t		mViewerJointData;

	// Vector of vectors because of a posibility of having more than one skin
	typedef std::vector<LLMeshSkinInfo::matrix_list_t> bind_matrices_t;
	bind_matrices_t			mInverseBindMatrices;
	bind_matrices_t			mAlternateBindMatrices;
	typedef std::vector<std::vector<std::string> > joint_names_t;
	// Empty string when no legal name for a given index
	joint_names_t			mJointNames;
	// To detect and warn about unsed joints
	typedef std::vector<std::vector<S32> > joint_usage_vec_t;
	joint_usage_vec_t		mJointUsage;

	// What group a joint belongs to. For purpose of stripping unused groups
	// when joints are over limit.
	struct JointGroups
	{
		std::string	mGroup;
		std::string	mParentGroup;
	};
	typedef std::map<std::string, JointGroups,
					 std::less<> > joint_to_group_map_t;
	joint_to_group_map_t	mJointGroups;

	// Per skin joint count, needs to be tracked for the sake of limits check.
	std::vector<S32>		mValidJointsCount;

	// Cached material information
	typedef std::map<S32, LLImportMaterial> cached_mat_map_t;
	cached_mat_map_t		mMaterialCache;

	bool					mApplyXYRotation;
	bool					mGltfLoaded;
};
