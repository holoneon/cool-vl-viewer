/**
 * @file llgltfloader.cpp
 * @brief LLGLTFLoader implementation
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

#include "linden_common.h"

// Work-around for a spurious and bogus warning in glm headers seen with GCC 14
// (and maybe 12 and 13, but definitely not with 11 and 15). HB
#if LL_GNUC
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "glm/ext/vector_uint4_sized.hpp"		// For glm::u16vec4
#include "glm/gtc/packing.hpp"					// For glm::unpackUint4x16
#include "meshoptimizer.h"

#include "llgltfloader.h"

#include "lldir.h"
#include "llgltexture.h"
#include "llgltfprimitive.h"
#include "lljoint.h"
#include "lluri.h"

// 10 vertices: 3 complete triangles plus remapping overhead
constexpr size_t VERTEX_SPLIT_SAFETY_MARGIN = 3 * 3 + 1;
constexpr size_t VERTEX_LIMIT = USHRT_MAX - VERTEX_SPLIT_SAFETY_MARGIN;

static const std::string lod_suffix[LLModel::NUM_LODS] =
{
	"_LOD0",
	"_LOD1",
	"_LOD2",
	"",
	"_PHYS",
};

// Premade rotation matrices, glTF is Y-up while SL is Z-up
static const mat4 coord_system_rotation(1.f, 0.f, 0.f, 0.f,
										0.f, 0.f, 1.f, 0.f,
										0.f, -1.f, 0.f, 0.f,
										0.f, 0.f, 0.f, 1.f);
static const mat4 coord_system_rotationxy(0.f, 1.f, 0.f, 0.f,
										  -1.f, 0.f, 0.f, 0.f,
										  0.f, 0.f, 1.f, 0.f,
										  0.f, 0.f, 0.f, 1.f);

// Helper function
static std::string get_lod_less_label(const Node& node)
{
	const std::string& label = node.mName;
	size_t ext_pos = std::string::npos;
	if (label.find("_LOD") != std::string::npos ||
		label.find("_PHYS") != std::string::npos)
	{
		ext_pos = label.rfind('_');
	}
	if (ext_pos != std::string::npos)
	{
		return label.substr(0, ext_pos);
	}
	return label;
}

LLGLTFLoader::LLGLTFLoader(const std::string& filename, S32 lod,
						   load_callback_t load_cb,
						   joint_lookup_func_t joint_lookup_func,
						   texture_load_func_t texture_load_func,
						   state_callback_t state_cb, void* userdata,
						   JointTransformMap& joint_transform_map,
						   JointNameSet& joints_from_nodes,
						   strings_map_t& legal_joint_names,
						   U32 max_joints_per_mesh, U32 model_limit)
:	LLModelLoader(filename, lod, load_cb, joint_lookup_func, texture_load_func,
				  state_cb, userdata, joint_transform_map, joints_from_nodes,
				  legal_joint_names, max_joints_per_mesh, model_limit),
	mGltfLoaded(false),
	mApplyXYRotation(false)
{
	JointData::getJointMatricesAndHierarhy(mViewerJointData);
}

bool LLGLTFLoader::openFile(const std::string& filename)
{
	// Clear the material cache for new file
	mMaterialCache.clear();

	std::string error;
	try
	{
		mGltfLoaded = mGLTFAsset.load(filename, false);
	}
	catch (const std::exception& e)
	{
		error = e.what();
		if (error.empty())
		{
			error = "Unknown exception";
		}
	}
	catch (...)
	{
		error = "Unknown exception";
	}
	if (!error.empty())
	{
		llwarns << "Exception encountered while loading glTF asset for file: "
				<< filename << "- Exception: " << error << llendl;
		LLSD args;
		args["Message"] = "ParsingErrorException";
		args["FILENAME"] = mFilename;
		args["EXCEPTION"] = error;
		mWarningsArray.append(args);
		mGltfLoaded = false;
		setLoadState(ERROR_PARSING);
		return false;
	}

	if (!mGltfLoaded)
	{
		notifyUnsupportedExtension(true);
		for (const auto& buffer : mGLTFAsset.mBuffers)
		{
			if (buffer.mByteLength > 0 && buffer.mData.empty())
			{
				bool bin_file =
					buffer.mUri.rfind(".bin") == buffer.mUri.size() - 4;
				LLSD args;
				args["Message"] = bin_file ? "ParsingErrorMissingBufferBin"
										   : "ParsingErrorMissingBuffer";
				args["BUFFER_NAME"] = buffer.mName;
				args["BUFFER_URI"] = buffer.mUri;
				mWarningsArray.append(args);
			}
		}
		setLoadState(ERROR_PARSING);
		return false;
	}
	notifyUnsupportedExtension(false);

	bool mesh_loaded = parseMeshes();
	setLoadState(DONE);
	return mesh_loaded;
}

void LLGLTFLoader::notifyUnsupportedExtension(bool unsupported)
{
	const std::vector<std::string>& extensions =
		unsupported ? mGLTFAsset.mUnsupportedExtensions
					: mGLTFAsset.mIgnoredExtensions;
	size_t count = extensions.size();
	if (!count)
	{
		return;
	}
	LLSD args;
	args["Message"] = unsupported ? "UnsupportedExtension"
								  : "IgnoredExtension";
	std::string ext;
	for (size_t i = 0; i < count; ++i)
	{
		if (i)
		{
			ext += ',';
		}
		ext += extensions[i];
	}
	args["EXT"] = ext;
	mWarningsArray.append(args);
	llwarns << "Model uses unsupported extension: " << ext << llendl;
}

void LLGLTFLoader::checkGlobalJointUsage()
{
	// Check if some joints remained unused
	for (S32 skin_idx = 0, count = mGLTFAsset.mSkins.size(); skin_idx < count;
		 ++skin_idx)
	{
		const Skin& gltf_skin = mGLTFAsset.mSkins[skin_idx];
		S32 joint_count = gltf_skin.mJoints.size();
		S32 used_joints = 0;
		for (S32 i = 0; i < joint_count; ++i)
		{
			if (mJointUsage[skin_idx][i])
			{
				++used_joints;
			}
			else
			{
				LL_DEBUGS("MeshUpload") << "Joint "
										<< mJointNames[skin_idx][i]
										<< " in skin " << skin_idx
										<< " is unused." << LL_ENDL;
			}
		}

		S32 valid_joints = mValidJointsCount[skin_idx];
		if (valid_joints > used_joints)
		{
			LLSD args;
			args["Message"] = "SkinUnusedJoints";
			args["SKIN_INDEX"] = skin_idx;
			args["JOINT_COUNT"] = valid_joints;
			args["USED_COUNT"] = used_joints;
			mWarningsArray.append(args);
		}
	}
}

bool LLGLTFLoader::parseMeshes()
{
	if (!mGltfLoaded) return false;

	// 2022-04 DJH Volume params from dae example. *TODO understand PCODE
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

	mTransform.setIdentity();

	for (auto& node : mGLTFAsset.mNodes)
	{
		// Make node matrix valid for correct transformation
		node.makeMatrixValid();
	}

	if (mGLTFAsset.mSkins.size() > 0)
	{
		checkForXYRotation(mGLTFAsset.mSkins[0]);
		populateJointGroups();
	}

	// Populate the joints from skins first. Multiple meshes can share the same
	// skin, so preparing skins beforehand.
	for (S32 i = 0, count = mGLTFAsset.mSkins.size(); i < count; ++i)
	{
		populateJointsFromSkin(i);
	}

	// Track how many times each mesh name has been used
	mesh_count_mat_t mesh_name_counts;
	// For now use mesh count, but might be better to do mNodes.size() minus
	// joints count...
	U32 meshes = mGLTFAsset.mNodes.size();
	U32 submodel_limit = meshes > 0 ? mGeneratedModelLimit / meshes : 0;

	// Check if we have scenes defined
	S32 scenes_size = mGLTFAsset.mScenes.size();
	if (scenes_size <= 0)
	{
		LLSD args;
		args["Message"] = "NoScenesFound";
		mWarningsArray.append(args);
		return false;
	}

	// Process the default scene (or first scene if no default)
	S32 scene_idx = mGLTFAsset.mScene >= 0 ? mGLTFAsset.mScene : 0;
	if (scene_idx < scenes_size)
	{
		const Scene& scene = mGLTFAsset.mScenes[scene_idx];
		llinfos << "Processing scene: " << scene_idx << " with "
				<< scene.mNodes.size() << " root nodes" << llendl;
		// Process all root nodes defined in the scene
		S32 nodes_size = mGLTFAsset.mNodes.size();
		for (S32 root_idx : scene.mNodes)
		{
			if (root_idx >= 0 && root_idx < nodes_size)
			{
				processNodeHierarchy(root_idx, mesh_name_counts,
									 submodel_limit, volume_params);
			}
		}
	}

	checkGlobalJointUsage();

	return true;
}

void LLGLTFLoader::buildOverrideMatrix(const JointData& viewer_data,
									   joints_data_map_t& gltf_nodes,
									   joints_name_to_node_map_t& names_to_nodes,
									   mat4& parent_rest,
									   mat4& parent_support_rest) const
{
	mat4 rest(1.f);
	auto it = names_to_nodes.find(viewer_data.mName);
	if (it != names_to_nodes.end())
	{
		S32 gltf_node_idx = it->second;
		JointNodeData& node = gltf_nodes[gltf_node_idx];
		node.mIsOverrideValid = true;
		node.mViewerRestMatrix = viewer_data.mRestMatrix;

		mat4 gltf_joint_rest_pose = coord_system_rotation *
									node.mGltfRestMatrix;
		if (mApplyXYRotation)
		{
			gltf_joint_rest_pose = coord_system_rotationxy *
								   gltf_joint_rest_pose;
		}

		mat4 translated_joint;
		// Example:
		// Viewer has pelvis->spine1->spine2->torso while glTF example model
		// has pelvis->torso. By doing:
		// glm::inverse(transalted_rest_spine2) * gltf_rest_torso
		// We get what torso would have looked like if glTF had a spine2
		if (viewer_data.mIsJoint)
		{
			translated_joint = glm::inverse(parent_rest) *
							   gltf_joint_rest_pose;
		}
		else
		{
			translated_joint = glm::inverse(parent_support_rest) *
							   gltf_joint_rest_pose;
		}

		vec3 translation_override, skew, scale;
		vec4 perspective;
		quat rotation;
		glm::decompose(translated_joint, scale, rotation, translation_override,
					   skew, perspective);

		// Viewer allows overrides, which are base joint with applied
		// translation override; fortunately normal bones use only translation,
		// without rotation or scale.
		node.mOverrideMatrix =
			glm::recompose(vec3(1.f, 1.f, 1.f), glm::identity<quat>(),
						   translation_override, vec3(0.f, 0.f, 0.f),
						   vec4(0.f, 0.f, 0.f, 1.f));
		mat4 override_joint = node.mOverrideMatrix;

		// *TODO: if glTF bones had rotation or scale, they probably should be
		// saved here.
		rest = parent_rest * override_joint;
		if (viewer_data.mIsJoint)
		{
			node.mOverrideRestMatrix = rest;
		}
		else
		{
			// Collision volumes need the imported translation override, but
			// their local rotation-scale basis must come from the raw viewer
			// skeleton XML values. For non-uniform torso volumes, matching DAE
			// requires viewer rotation followed by viewer scale.
			mat4 rot_scale(1.f);
			rot_scale = glm::rotate(rot_scale,
									glm::radians(viewer_data.mRotation[0]),
									vec3(1.f, 0.f, 0.f));
			rot_scale = glm::rotate(rot_scale,
									glm::radians(viewer_data.mRotation[1]),
									vec3(0.f, 1.f, 0.f));
			rot_scale = glm::rotate(rot_scale,
									glm::radians(viewer_data.mRotation[2]),
									vec3(0.f, 0.f, 1.f));
			rot_scale = glm::scale(rot_scale, viewer_data.mScale);
			override_joint = rot_scale;
			override_joint[3][0] = translation_override.x;
			override_joint[3][1] = translation_override.y;
			override_joint[3][2] = translation_override.z;
			node.mOverrideRestMatrix = parent_support_rest * override_joint;
		}
	}
	else
	{
		// No override for this joint
		rest = parent_rest * viewer_data.mJointMatrix;
	}

	mat4 support_rest(1.f);
	if (viewer_data.mSupport == JointData::SUPPORT_BASE)
	{
		support_rest = rest;
	}
	else
	{
		support_rest = parent_support_rest;
	}

	for (const JointData& child_data : viewer_data.mChildren)
	{
		buildOverrideMatrix(child_data, gltf_nodes, names_to_nodes, rest,
							support_rest);
	}
}

// This is inefficient since we are recalculating some joints multiple times
// over. *TODO: cache it ?
mat4 LLGLTFLoader::buildGltfRestMatrix(S32 joint_node_idx,
									   const Skin& gltf_skin) const
{
	if (joint_node_idx < 0 ||
		joint_node_idx >= S32(mGLTFAsset.mNodes.size()))
	{
		return mat4(1.f);
	}

	const auto& node = mGLTFAsset.mNodes[joint_node_idx];
	// Find and apply parent transform if it exists
	for (S32 i = 0, count = mGLTFAsset.mNodes.size(); i < count; ++i)
	{
		const auto& potential_parent = mGLTFAsset.mNodes[i];
		auto children_end = potential_parent.mChildren.end();
		auto it = std::find(potential_parent.mChildren.begin(), children_end,
							joint_node_idx);
		if (it == children_end)
		{
			continue;
		}
		// Found a parent...
		auto joints_end = gltf_skin.mJoints.end();
		if (std::find(gltf_skin.mJoints.begin(), joints_end,
					  joint_node_idx) != joints_end)
		{
			return buildGltfRestMatrix(i, gltf_skin) * node.mMatrix;
		}
	}

	// Should we return the armature or stop earlier ?
	return node.mMatrix;
}

bool LLGLTFLoader::checkForXYRotation(const Skin& gltf_skin, S32 joint_idx,
									  S32 bind_indx)
{
	mat4 gltf_joint_rest = buildGltfRestMatrix(joint_idx, gltf_skin);
	mat4 test_mat = glm::inverse(gltf_joint_rest) *
					gltf_skin.mInverseBindMatricesData[bind_indx];
	// Normally for shoulders it should be something close to
	// {1,0,0,0;0,-1,0,0;0,0,-1,0;0,0,0,1}, rotated one will look like
	// {0,0,0,-1;1,0,0,0;0,-1,0,0;0,0,0,1}. This is a cheap hack.
	// *TODO: figure out how rotation is supposed to work
	return fabs(test_mat[0][0]) < 0.5f && fabs(test_mat[1][1]) < 0.5f &&
		   fabs(test_mat[2][2]) < 0.5f;
}

void LLGLTFLoader::checkForXYRotation(const Skin& gltf_skin)
{
	// *HACK: figure out the model rotation from shoulders matrix. This is
	// is wrong on many levels: too limited (only models that have shoulders),
	// does not work well with things that emulate 3 hands in some manner, only
	// supports x/y 90 degrees rotation.
	// *TODO: figure out how to find the skeleton orientation correctly when
	// the model is rotated at a triangle level.
	static const std::string right_shoulder = "mShoulderRight";
	static const std::string left_shoulder = "mShoulderLeft";

	S32 joints_found = 0;
	for (S32 i = 0, count = gltf_skin.mJoints.size(); i < count; ++i)
	{
		S32 joint = gltf_skin.mJoints[i];
		const Node& joint_node = mGLTFAsset.mNodes[joint];
		auto it = mJointMap.find(joint_node.mName);
		if (it == mJointMap.end())
		{
			// Unsupported joint
			continue;
		}
		if (it->second == right_shoulder || it->second == left_shoulder)
		{
			if (!checkForXYRotation(gltf_skin, joint, i))
			{
				return;
			}
			++joints_found;
		}
	}
	if (joints_found == 2)
	{
		// Both joints in a weird position/rotation: assume rotated model.
		mApplyXYRotation = true;
	}
}

void LLGLTFLoader::buildJointGroup(JointData& viewer_data,
								   const std::string& parent_group)
{
	JointGroups& jount_group_data = mJointGroups[viewer_data.mName];
	jount_group_data.mGroup = viewer_data.mGroup;
	jount_group_data.mParentGroup = parent_group;
	for (size_t i = 0, count = viewer_data.mChildren.size(); i < count; ++i)
	{
		buildJointGroup(viewer_data.mChildren[i], viewer_data.mGroup);
	}
}

void LLGLTFLoader::populateJointGroups()
{
	std::string parent;
	for (size_t i = 0, count = mViewerJointData.size(); i < count; ++i)
	{
		buildJointGroup(mViewerJointData[i], parent);
	}
}

mat4 LLGLTFLoader::computeGltfToViewerSkeletonTransform(const joints_data_map_t& data_map,
														S32 gltf_node_index,
														const std::string& joint_name) const
{
	const JointNodeData& node_data = data_map.at(gltf_node_index);
	if (!node_data.mIsOverrideValid)
	{
		// For now assume they are identical and return an identity (for ease
		// of debuging).
		return mat4(1.f);
	}

	// Get the glTF joint rest pose (in glTF coordinate system)
	const mat4& gltf_joint_rest_pose = node_data.mGltfRestMatrix;
	mat4 rest_pose = coord_system_rotation * gltf_joint_rest_pose;
	LLMatrix4 transform(glm::value_ptr(rest_pose));
	// Compute transformation from glTF space to viewer space:
	// this assumes both skeletons are in rest pose initially.
	return node_data.mOverrideRestMatrix * glm::inverse(rest_pose);
}

void LLGLTFLoader::populateJointsFromSkin(S32 skin_idx)
{
	const Skin& skin = mGLTFAsset.mSkins[skin_idx];
	S32 joint_count = skin.mJoints.size();
	llinfos << "Processing skin #" << skin_idx << " with "
			<< joint_count << " joints" << llendl;
	S32 inverse_count = skin.mInverseBindMatricesData.size();
	if (skin.mInverseBindMatrices > 0 && inverse_count != joint_count)
	{
		llwarns << "Bind matrices count (" << inverse_count
				<< ") mismatches joints count (" << joint_count << ")"
				<< llendl;
		LLSD args;
		args["Message"] = "InvBindCountMismatch";
		mWarningsArray.append(args);
	}
	if ((S32)mInverseBindMatrices.size() <= skin_idx)
	{
		mInverseBindMatrices.resize(skin_idx + 1);
		mAlternateBindMatrices.resize(skin_idx + 1);
		mJointNames.resize(skin_idx + 1);
		mJointUsage.resize(skin_idx + 1);
		mValidJointsCount.resize(skin_idx + 1, 0);
	}

	// Fill up joints related data
	joints_data_map_t joints_data;
	joints_name_to_node_map_t names_to_nodes;
	for (S32 i = 0; i < joint_count; ++i)
	{
		S32 joint = skin.mJoints[i];
		const Node& joint_node = mGLTFAsset.mNodes[joint];
		JointNodeData& data = joints_data[joint];
		data.mNodeIdx = joint;
		data.mJointListIdx = i;
		data.mGltfRestMatrix = buildGltfRestMatrix(joint, skin);
		data.mGltfMatrix = joint_node.mMatrix;
		data.mOverrideMatrix = mat4(1.f);

		if (mJointMap.count(joint_node.mName))
		{
			data.mName = mJointMap[joint_node.mName];
			data.mIsValidViewerJoint = true;
			++mValidJointsCount[skin_idx];
		}
		else
		{
			data.mName = joint_node.mName;
			data.mIsValidViewerJoint = false;
		}
		names_to_nodes[data.mName] = joint;

		for (size_t j = 0, chid_count = joint_node.mChildren.size();
			 j < chid_count; ++j)
		{
			JointNodeData& child_data = joints_data[joint_node.mChildren[j]];
			child_data.mParentNodeIdx = joint;
			child_data.mIsParentValidViewerJoint = data.mIsValidViewerJoint;
		}
	}

	// Go over viewer joints and build overrides. This is needed because the
	// glTF skeleton does not necessarily match viewer skeleton.
	mat4 ident(1.f);
	for (size_t i = 0, count = mViewerJointData.size(); i < count; ++i)
	{
		buildOverrideMatrix(mViewerJointData[i], joints_data, names_to_nodes,
							ident, ident);
	}

	std::string legal_name;
	for (S32 i = 0; i < joint_count; ++i)
	{
		S32 joint = skin.mJoints[i];
		const Node& joint_node = mGLTFAsset.mNodes[joint];
		legal_name = joint_node.mName;
		bool legal_joint = false;
		// Viewer supports a limited set of joints, mark them as legal
		if (mJointMap.count(legal_name))
		{
			legal_joint = true;
			legal_name = mJointMap[legal_name];
			mJointNames[skin_idx].emplace_back(legal_name);
		}
		else
		{
			mJointNames[skin_idx].emplace_back();
		}
		mJointUsage[skin_idx].push_back(0);

		// Compute bind matrices
		if (!legal_joint)
		{
			// Add placeholder to not break index (not going to be used by
			// the viewer: will be stripped from skin_info).
			mInverseBindMatrices[skin_idx].emplace_back(LLMatrix4());
		}
		else if (inverse_count > i)
		{
			// Translate existing bind matrix to the viewer overriden
			// skeleton.
			mat4 orig_bind_matrix =
				glm::inverse(skin.mInverseBindMatricesData[i]);
			mat4 rotated_orig = coord_system_rotation * orig_bind_matrix;
			mat4 skeleton_transform =
				computeGltfToViewerSkeletonTransform(joints_data, joint,
													 legal_name);
			mat4 tranlated_orig = skeleton_transform * rotated_orig;
			mat4 final_inv_bind_matrix = glm::inverse(tranlated_orig);
			LLMatrix4 gltf_transform(glm::value_ptr(final_inv_bind_matrix));
			mInverseBindMatrices[skin_idx].emplace_back(gltf_transform);
		}
		else
		{
			// If bind matrices are not present (they are optional in gltf),
			// assume an identy matrix. *TODO: find a model with this, might
			// need to use Y/Z rotated matrix.
			mat4 inv_bind(1.f);
			mat4 skeleton_transform =
				computeGltfToViewerSkeletonTransform(joints_data, joint,
													 legal_name);
			inv_bind = glm::inverse(skeleton_transform * inv_bind);
			LLMatrix4 gltf_transform = LLMatrix4(glm::value_ptr(inv_bind));
			mInverseBindMatrices[skin_idx].emplace_back(gltf_transform);
		}

		// Compute Alternative matrices also known as overrides
		LLMatrix4 orig_joint_tf(glm::value_ptr(joints_data[joint].mOverrideMatrix));
		// The viewer seems to care only about the translation part, but for
		// parity with Collada let's take the original value.
		LLMatrix4 new_inverse = mInverseBindMatrices[skin_idx].back();
		new_inverse.setTranslation(orig_joint_tf.getTranslation());
		mAlternateBindMatrices[skin_idx].emplace_back(new_inverse);

		if (legal_joint)
		{
			// Might be needed for uploader UI to correctly identify overriden
			// joints but going to be incorrect if multiple skins are present.
			mJointList[legal_name] = new_inverse;
			mJointsFromNode.emplace_back(legal_name);
		}
	}

	S32 valid_joints = mValidJointsCount[skin_idx];
	if (valid_joints < joint_count)
	{
		LLSD args;
		args["Message"] = "SkinUsupportedJoints";
		args["SKIN_INDEX"] = skin_idx;
		args["JOINT_COUNT"] = joint_count;
		args["LEGAL_COUNT"] = valid_joints;
		mWarningsArray.append(args);
	}
}

void LLGLTFLoader::computeCombinedNodeTransform(const Asset& asset,
												S32 node_index,
												mat4& combined_transform) const
{
	if (node_index < 0 || node_index >= S32(asset.mNodes.size()))
	{
		combined_transform = mat4(1.f);
		return;
	}

	const auto& node = asset.mNodes[node_index];
	// Ensure the node's matrix is valid
	const_cast<Node&>(node).makeMatrixValid();

	// Start with this node's transform
	combined_transform = node.mMatrix;

	// Find and apply parent transform if it exists
	for (S32 i = 0, count = asset.mNodes.size(); i < count; ++i)
	{
		const auto& potential_parent = asset.mNodes[i];
		auto children_end = potential_parent.mChildren.end();
		auto it = std::find(potential_parent.mChildren.begin(), children_end,
							node_index);
		if (it != children_end)
		{
			// Found parent: recursively get its combined transform and apply
			// it.
			mat4 parent_transform;
			computeCombinedNodeTransform(asset, i, parent_transform);
			combined_transform = parent_transform * combined_transform;
			return;	// Early exit since a node can only have one parent.
		}
	}
}

bool LLGLTFLoader::addJointToModelSkin(LLMeshSkinInfo& skin_info,
									   S32 gltf_skin_idx,
									   S32 gltf_joint_idx)
{
	const std::string& legal_name = mJointNames[gltf_skin_idx][gltf_joint_idx];
	if (legal_name.empty())
	{
		// This should have been stopped by gltf_joint_index_use[i] == -1
		llassert(false);
		return false;
	}
	skin_info.mJointNames.emplace_back(legal_name);
	skin_info.mJointKeys.push_back(LLJoint::getKey(legal_name));
	// In scope of same skin multiple meshes reuse same bind matrices
	skin_info.mInvBindMatrix.emplace_back(mInverseBindMatrices[gltf_skin_idx][gltf_joint_idx]);
	skin_info.mAlternateBindMatrix.emplace_back(mAlternateBindMatrices[gltf_skin_idx][gltf_joint_idx]);
	// Track joint usage for this skin, for the sake of unused joints detection
	++mJointUsage[gltf_skin_idx][gltf_joint_idx];
	return true;
}

std::string LLGLTFLoader::generateMaterialName(S32 mat_idx,
											   S32 fallback_idx) const
{
	if (mat_idx >= 0 && mat_idx < (S32)mGLTFAsset.mMaterials.size())
	{
		const Material& mat = mGLTFAsset.mMaterials[mat_idx];
		if (!mat.mName.empty())
		{
			return mat.mName;
		}
		fallback_idx = mat_idx;	// Return "mat_default<mat_idx>"
	}
	if (fallback_idx < 0)
	{
		return "mat_default";
	}
	return llformat("mat_default%d", fallback_idx);
}

bool LLGLTFLoader::validateTextureIndex(S32 tex_idx, S32& src_idx) const
{
	if (tex_idx < 0 || tex_idx >= (S32)mGLTFAsset.mTextures.size())
	{
		return false;
	}

	src_idx = mGLTFAsset.mTextures[tex_idx].mSource;
	return src_idx >= 0 && src_idx < (S32)mGLTFAsset.mImages.size();
}

std::string LLGLTFLoader::processTexture(std::string& full_path, S32 tex_idx,
										 const char* tex_type,
										 const std::string& mat_name)
{
	S32 src_idx;
	if (!validateTextureIndex(tex_idx, src_idx))
	{
		return "";
	}

	const Image& image = mGLTFAsset.mImages[src_idx];
	if (!image.mUri.empty())
	{
		// URI might be a remote URL or a local path. Extract just the filename
		// from the URI.
		std::string filename = image.mUri;
		size_t pos = filename.find_last_of("/\\");
		if (pos != std::string::npos)
		{
			filename = filename.substr(pos + 1);
		}

		std::string dir = LLDir::getDirName(mFilename);
		full_path = dir + LL_DIR_DELIM_STR + filename;
		if (filename.find("data:") == std::string::npos &&
			!LLFile::exists(full_path))
		{
			// Characters might be escaped in the URI
			filename = LLURI::unescape(filename);
			full_path = dir + LL_DIR_DELIM_STR + filename;
			if (!LLFile::exists(full_path))
			{
				full_path.clear();
			}
		}

		LLSD args;
		args["Message"] = "TextureFound";
		args["TEXTURE_NAME"] = filename;
		args["MATERIAL_NAME"] = mat_name;
		mWarningsArray.append(args);

		return filename;
	}

	// For embedded textures (no URI but has buffer data)
	if (image.mBufferView >= 0)
	{
		std::string temp_path = extractTextureToTempFile(tex_idx, tex_type);
		if (!temp_path.empty())
		{
			full_path = temp_path;
		}
		return temp_path;
	}

	return "";
}

LLImportMaterial& LLGLTFLoader::processMaterial(S32 mat_idx, S32 fallback_idx)
{
	// Check cache first
	cached_mat_map_t::iterator it = mMaterialCache.find(mat_idx);
	if (it != mMaterialCache.end())
	{
		return it->second;
	}

	LLImportMaterial imat;
	imat.mDiffuseColor = LLColor4::white; // Default color
	// Generate material name
	imat.setName(generateMaterialName(mat_idx, fallback_idx));

	// Process material if available
	if (mat_idx >= 0 && mat_idx < (S32)mGLTFAsset.mMaterials.size())
	{
		Material& mat = mGLTFAsset.mMaterials[mat_idx];
		// Set diffuse color from base color factor
		imat.mDiffuseColor =
			LLColor4(mat.mPbrMetallicRoughness.mBaseColorFactor[0],
					 mat.mPbrMetallicRoughness.mBaseColorFactor[1],
					 mat.mPbrMetallicRoughness.mBaseColorFactor[2],
					 mat.mPbrMetallicRoughness.mBaseColorFactor[3]);

		// Process base color texture if it exists
		S32 tex_idx = mat.mPbrMetallicRoughness.mBaseColorTexture.mIndex;
		if (tex_idx >= 0)
		{
			std::string full_path;
			std::string filename = processTexture(full_path, tex_idx,
												  "base_color", mat.mName);
			if (!filename.empty())
			{
				imat.mDiffuseMapFilename = full_path;
				imat.mDiffuseMapLabel = mat.mName.empty() ? filename
														  : mat.mName;
				// Check if the texture is already loaded
				S32 src_idx;
				if (validateTextureIndex(tex_idx, src_idx))
				{
					Image& image = mGLTFAsset.mImages[src_idx];
					llinfos << "Found texture: " << filename
							<< " - For material: " << mat.mName;
					if (image.mTexture.notNull())
					{
						const LLUUID& tex_id = image.mTexture->getID();
						// If the image has a texture loaded already, use it
						imat.setDiffuseMap(tex_id);
						llcont << " - Texture Id: " << tex_id;
						if (image.mHeight > gMaxImageSizeDefault ||
							image.mWidth > gMaxImageSizeDefault)
						{
							mTexturesNeedScaling = true;
							llcont << " (will be scaled down on upload)";
						}
					}
					else
					{
						// Texture will be loaded later through the callback
						// system.
						llcont << " - Texture needs loading.";
					}
					llcont << llendl;
				}
			}
		}
	}

	mMaterialCache[mat_idx] = imat;
	return mMaterialCache[mat_idx];
}

struct GLTFVertex
{
	vec3			position;
	vec3			normal;
	vec2			uv0;
	glm::u16vec4	joints;
	vec4			weights;
};

bool LLGLTFLoader::populateModelFromMesh(LLModel* modelp,
										 const std::string& base_name,
										 const Mesh& mesh, const Node& nodeno,
										 mats_map_t& mats,
										 S32 instance_count)
{
	modelp->mRequestedLabel = LLDir::getBaseFileName(mFilename, true);
	// Set only the abse name, suffix will be added later
	modelp->mLabel = base_name;

	modelp->clearFacesAndMaterials();

	S32 skin_idx = nodeno.mSkin;

	// Compute final combined transform matrix (hierarchy + coordinate rotation)
	S32 node_index = S32(&nodeno - &mGLTFAsset.mNodes[0]);
	mat4 hierarchy_transform;
	computeCombinedNodeTransform(mGLTFAsset, node_index, hierarchy_transform);

	// Combine transforms: coordinate rotation applied to hierarchy transform
	mat4 final_transform = coord_system_rotation * hierarchy_transform;
	if (mApplyXYRotation)
	{
		final_transform = coord_system_rotationxy * final_transform;
	}

	// Check if we have a negative scale (flipped coordinate system)
	bool has_negative_scale = glm::determinant(final_transform) < 0.f;

	// Pre-compute normal transform matrix (transpose of inverse of upper-left 3x3)
	const glm::mat3 normal_transform =
		glm::transpose(glm::inverse(glm::mat3(final_transform)));

	// Mark unsuported joints with '-1' so that they won't get added into
	// weights glTF maps all joints onto all meshes. Gather use count per mesh
	// to cut unused ones.
	std::vector<S32> gltf_joint_index_use;
	if (skin_idx >= 0 && skin_idx < (S32)mGLTFAsset.mSkins.size())
	{
		Skin& gltf_skin = mGLTFAsset.mSkins[skin_idx];
		size_t count = gltf_skin.mJoints.size();
		gltf_joint_index_use.resize(count, 0);
		for (size_t i = 0; i < count; ++i)
		{
			if (mJointNames[skin_idx][i].empty())
			{
				// This might need to hold a substitute index. Mark as
				// unsupported.
				gltf_joint_index_use[i] = -1;
			}
		}
	}

	std::string tempname;
	for (size_t prim_idx = 0, prim_count = mesh.mPrimitives.size();
		 prim_idx < prim_count; ++prim_idx)
	{
		const Primitive& prim = mesh.mPrimitives[prim_idx];
		if (prim.getIndexCount() % 3 != 0)
		{
			LLSD args;
			args["Message"] = "InvalidGeometryNonTriangulated";
			args["MESH_NAME"] = mesh.mName;
			args["PRIMITIVE_INDEX"] = S32(prim_idx);
			args["INDEX_COUNT"] = S32(prim.getIndexCount());
			mWarningsArray.append(args);
			return false;	// Skip this primitive
		}

		// Use cached material processing
		LLImportMaterial imat =
			processMaterial(prim.mMaterial, modelp->getNumVolumeFaces() - 1);
		tempname = imat.getName();
		mats[tempname] = imat;

		// So primitives already have all of the data we need for a given face
		// in SL land. Primitives may only ever have a single material assigned
		// to them as the relation is 1:1 in terms of intended draw call count.
		// Just go ahead and populate faces direct from the glTF primitives
		// here. -Geenz 2025-04-07
		LLVolumeFace face;
		std::vector<GLTFVertex> vertices;

		// Apply the global scale and center offset to all vertices
		bool missing_normals = false;
		for (U32 i = 0, verts = prim.getVertexCount(); i < verts; ++i)
		{
			// Use pre-computed final_transform
			vec4 pos(prim.mPositions[i][0], prim.mPositions[i][1],
					 prim.mPositions[i][2], 1.f);
			vec4 transformed_pos = final_transform * pos;

			GLTFVertex vert;
			vert.position = vec3(transformed_pos);

			if (prim.mNormals.empty())
			{
				// Use default normal (pointing up in model space)
				missing_normals = true;
				vert.normal = glm::normalize(normal_transform *
											 vec3(0.f, 0.f, 1.f));
			}
			else
			{
				// Use pre-computed normal_transform
				vec3 normal_vec(prim.mNormals[i][0], prim.mNormals[i][1],
								prim.mNormals[i][2]);
				vert.normal = glm::normalize(normal_transform * normal_vec);
			}

			vert.uv0 = vec2(prim.mTexCoords0[i][0],
							// Flip texture V coordinate
							1.f - prim.mTexCoords0[i][1]);

			if (skin_idx >= 0)
			{
				vert.weights = vec4(prim.mWeights[i][0], prim.mWeights[i][1],
									prim.mWeights[i][2], prim.mWeights[i][3]);
				auto accessor_idx = prim.mAttributes.at("JOINTS_0");
				auto comp_type = Accessor::ComponentType::UNSIGNED_BYTE;
				if (accessor_idx >= 0)
				{
					auto accessor = mGLTFAsset.mAccessors[accessor_idx];
					comp_type = accessor.mComponentType;
				}
				// The glTF spec allows for either an unsigned byte for joint
				// indices, or an unsigned short. Detect and unpack
				// accordingly.
				if (comp_type == Accessor::ComponentType::UNSIGNED_BYTE)
				{
					vert.joints = glm::unpackUint4x16(prim.mJoints[i]);
				}
				else
				{
					vert.joints = glm::zero<glm::u16vec4>();
					vert.weights = glm::zero<glm::vec4>();
				}
			}

			vertices.emplace_back(vert);
		}
		if (missing_normals)
		{
			LL_DEBUGS("MeshUpload") << "No normal found for some vertices in primitive "
									<< prim_idx
									<< ": default normal used instead."
									<< LL_ENDL;
		}

		// Check for empty vertex array before processing
		if (vertices.empty())
		{
			LLSD args;
			args["Message"] = "EmptyVertexArray";
			args["MESH_NAME"] = mesh.mName;
			args["PRIMITIVE_INDEX"] = S32(prim_idx);
			args["INDEX_COUNT"] = S32(prim.getIndexCount());
			mWarningsArray.append(args);
			return false;	// Skip this primitive
		}

		std::vector<LLVolumeFace::VertexData> face_verts;
		vec3 min(FLT_MAX);
		vec3 max(-FLT_MAX);

		for (size_t i = 0, count = vertices.size(); i < count; ++i)
		{
			LLVolumeFace::VertexData vert;

			// Update min/max bounds
			if (i)
			{
				min.x = llmin(min.x, vertices[i].position.x);
				min.y = llmin(min.y, vertices[i].position.y);
				min.z = llmin(min.z, vertices[i].position.z);
				max.x = llmax(max.x, vertices[i].position.x);
				max.y = llmax(max.y, vertices[i].position.y);
				max.z = llmax(max.z, vertices[i].position.z);
			}
			else
			{
				min = max = vertices[i].position;
			}

			LLVector4a position(vertices[i].position.x, vertices[i].position.y,
								vertices[i].position.z);
			LLVector4a normal(vertices[i].normal.x, vertices[i].normal.y,
							  vertices[i].normal.z);
			vert.setPosition(position);
			vert.setNormal(normal);
			vert.mTexCoord.set(vertices[i].uv0.x, vertices[i].uv0.y);
			face_verts.emplace_back(vert);

			if (skin_idx >= 0)
			{
				// Create list of weights that influence this vertex
				LLModel::weight_list weights;

				// Drop joints that viewer does not support (negative in
				// gltf_joint_index_use); do not re-index them yet, more
				// indexes will be removed. Also drop joints that have no
				// weight. glTF stores 4 per vertex, so there might be 'empty'
				// ones.
				if (gltf_joint_index_use[vertices[i].joints.x] >= 0 &&
					vertices[i].weights.x > 0.f)
				{
					weights.emplace_back(vertices[i].joints.x,
										 vertices[i].weights.x);
					++gltf_joint_index_use[vertices[i].joints.x];
				}
				if (gltf_joint_index_use[vertices[i].joints.y] >= 0 &&
					vertices[i].weights.y > 0.f)
				{
					weights.emplace_back(vertices[i].joints.y,
										 vertices[i].weights.y);
					++gltf_joint_index_use[vertices[i].joints.y];
				}
				if (gltf_joint_index_use[vertices[i].joints.z] >= 0 &&
					vertices[i].weights.z > 0.f)
				{
					weights.emplace_back(vertices[i].joints.z,
										 vertices[i].weights.z);
					++gltf_joint_index_use[vertices[i].joints.z];
				}
				if (gltf_joint_index_use[vertices[i].joints.w] >= 0 &&
					vertices[i].weights.w > 0.f)
				{
					weights.emplace_back(vertices[i].joints.w,
										 vertices[i].weights.w);
					++gltf_joint_index_use[vertices[i].joints.w];
				}
				std::sort(weights.begin(), weights.end(),
						  LLModel::CompareWeightGreater());

				std::vector<LLModel::JointWeight> wght;
				F32 total = 0.f;
				for (size_t j = 0; j < llmin(weights.size(), 4); ++j)
				{
					wght.emplace_back(weights[j]);
					total += weights[j].mWeight;
				}

				if (total != 0.f)
				{
					F32 scale = 1.f / total;
					if (scale != 1.f)
					{
						// Normalize weights
						for (size_t j = 0, wcount = wght.size(); j < wcount; ++j)
						{
							wght[j].mWeight *= scale;
						}
					}
				}

				if (!wght.empty())
				{
					LLVector3 pos(vertices[i].position[0], vertices[i].position[1],
								  vertices[i].position[2]);
					modelp->mSkinWeights[pos] = wght;
				}
			}
		}

		// Indices handling
		if (face_verts.size() >= VERTEX_LIMIT)
		{
			// Will have to remap 32 bits indices into 16 bits ones.
			// For the sake of simplicity build vector of 32 bits indices first
			std::vector<U32> indices_32;
			for (U32 i = 0, icount = prim.getIndexCount(); i < icount; i += 3)
			{
				// When processing indices, flip winding order if needed
				indices_32.push_back(prim.mIndexArray[i]);
				if (has_negative_scale)
				{
					// Flip winding order for negative scale
					indices_32.push_back(prim.mIndexArray[i + 2]);
					indices_32.push_back(prim.mIndexArray[i + 1]);
				}
				else
				{
					indices_32.push_back(prim.mIndexArray[i + 1]);
					indices_32.push_back(prim.mIndexArray[i + 2]);
				}
			}

			// Generates a vertex remap table with no gaps in the resulting
			// sequence.
			std::vector<U32> remap(face_verts.size());
			size_t vert_count =
				meshopt_generateVertexRemap(&remap[0], &indices_32[0],
											indices_32.size(), &face_verts[0],
											face_verts.size(),
											sizeof(LLVolumeFace::VertexData));
			// Manually remap vertices
			std::vector<LLVolumeFace::VertexData> opt_verts(vert_count);
			for (size_t i = 0; i < vert_count; ++i)
			{
				opt_verts[i] = face_verts[remap[i]];
			}

			std::vector<U32> opt_idx(indices_32.size());
			meshopt_remapIndexBuffer(&opt_idx[0], &indices_32[0],
									 indices_32.size(), &remap[0]);

			// Sort indices to improve mesh splits (reducing amount of
			// duplicated indices).
			meshopt_optimizeVertexCache(&opt_idx[0], &opt_idx[0],
										indices_32.size(), vert_count);

			// Remap 32 bit into multiple 16 bit ones
			std::vector<U16> indices_16;
			std::vector<S64> vertices_remap;
			vertices_remap.resize(vert_count, -1);
			std::vector<LLVolumeFace::VertexData> face_verts2;
			S32 created_faces = 0;
			min = vec3(FLT_MAX);
			max = vec3(-FLT_MAX);
			for (size_t idx = 0, ixcnt = opt_idx.size(); idx < ixcnt; ++idx)
			{
				S32 vert_index = opt_idx[idx];
				if (vertices_remap[vert_index] == -1)
				{
					// First encounter, add it
					S32 new_vert_idx = face_verts2.size();
					vertices_remap[vert_index] = (S64)new_vert_idx;
					face_verts2.push_back(opt_verts[vert_index]);
					vert_index = new_vert_idx;

					// Update min/max bounds
					const LLVector4a& vec =
						face_verts2[new_vert_idx].getPosition();
					if (new_vert_idx == 0)
					{
						min.x = vec[0];
						min.y = vec[1];
						min.z = vec[2];
						max = min;
					}
					else
					{
						min.x = llmin(min.x, vec[0]);
						min.y = llmin(min.y, vec[1]);
						min.z = llmin(min.z, vec[2]);
						max.x = llmax(max.x, vec[0]);
						max.y = llmax(max.y, vec[1]);
						max.z = llmax(max.z, vec[2]);
					}
				}
				else
				{
					// Already in vector, get position
					vert_index = vertices_remap[vert_index];
				}
				indices_16.push_back((U16)vert_index);

				if (indices_16.size() % 3 == 0 &&
					face_verts2.size() >= VERTEX_LIMIT)
				{
					LLVolumeFace face;
					face.fillFromLegacyData(face_verts2, indices_16);
					face.mExtents[0] = LLVector4a(min.x, min.y, min.z, 0);
					face.mExtents[1] = LLVector4a(max.x, max.y, max.z, 0);
					modelp->getVolumeFaces().emplace_back(face);
					modelp->getMaterialList().emplace_back(tempname);
					++created_faces;

					std::fill(vertices_remap.begin(), vertices_remap.end(), -1);
					indices_16.clear();
					face_verts2.clear();
					min = vec3(FLT_MAX);
					max = vec3(-FLT_MAX);
				}
			}
			if (indices_16.size() > 0 && face_verts2.size() > 0)
			{
				LLVolumeFace face;
				face.fillFromLegacyData(face_verts2, indices_16);
				face.mExtents[0] = LLVector4a(min.x, min.y, min.z, 0);
				face.mExtents[1] = LLVector4a(max.x, max.y, max.z, 0);
				modelp->getVolumeFaces().emplace_back(face);
				modelp->getMaterialList().emplace_back(tempname);
				++created_faces;
			}
			LL_DEBUGS("MeshUpload") << "Primitive " << modelp->mLabel
									<< " is over vertices limit, it was split into "
									<< created_faces << " faces" << LL_ENDL;
			LLSD args;
			args["Message"] = "ModelSplitPrimitive";
			args["MODEL_NAME"] = modelp->mLabel;
			args["FACE_COUNT"] = created_faces;
			mWarningsArray.append(args);
		}
		else
		{
			// Can use indices directly
			std::vector<U16> indices;
			for (U32 i = 0, icount = prim.getIndexCount(); i < icount; i += 3)
			{
				// When processing indices, flip winding order if needed
				indices.push_back(prim.mIndexArray[i]);
				if (has_negative_scale)
				{
					// Flip winding order for negative scale
					indices.push_back(prim.mIndexArray[i + 2]);
					indices.push_back(prim.mIndexArray[i + 1]);
				}
				else
				{
					indices.push_back(prim.mIndexArray[i + 1]);
					indices.push_back(prim.mIndexArray[i + 2]);
				}
			}

			face.fillFromLegacyData(face_verts, indices);
			face.mExtents[0] = LLVector4a(min.x, min.y, min.z, 0);
			face.mExtents[1] = LLVector4a(max.x, max.y, max.z, 0);
			modelp->getVolumeFaces().emplace_back(face);
			modelp->getMaterialList().emplace_back(tempname);
		}
	}

	// Call normalizeVolumeFacesAndWeights to compute proper extents
	modelp->normalizeVolumeFacesAndWeights();

	// Fill joint names, bind matrices and remap weight indices
	if (skin_idx >= 0)
	{
		Skin& gltf_skin = mGLTFAsset.mSkins[skin_idx];
		LLMeshSkinInfo& skin_info = modelp->mSkinInfo;
		S32 valid_joints_count = mValidJointsCount[skin_idx];
		S32 replacement_index = 0;
		std::vector<S32> gltfindex_to_jointindex_map;
		size_t joint_cnt = gltf_skin.mJoints.size();
		gltfindex_to_jointindex_map.resize(joint_cnt, -1);

		if (valid_joints_count > (S32)mMaxJointsPerMesh)
		{
			mesh_count_mat_t group_use_count;
			for (const auto& elem : mJointGroups)
			{
				group_use_count[elem.second.mGroup] = 0;
				group_use_count[elem.second.mParentGroup] = 0;
			}
			// Assume that 'Torso' group is always in use since that is what
			// everything else is attached to.
			group_use_count["Torso"] = 1;
			// Note that Collisions and Extra groups are all over the place,
			// might want to include them from the start or add individual when
			// parents are added.

			// Check which groups are in use
			for (size_t i = 0; i < joint_cnt; ++i)
			{
				std::string& joint_name = mJointNames[skin_idx][i];
				if (!joint_name.empty() && gltf_joint_index_use[i] > 0)
				{
					const JointGroups& group = mJointGroups[joint_name];
					// Joint in use, increment its groups
					++group_use_count[group.mGroup];
					++group_use_count[group.mParentGroup];
				}
			}

			// 1. Add joints that are in use directly
			for (size_t i = 0; i < joint_cnt; ++i)
			{
				// Process joint name and index
				if (gltf_joint_index_use[i] <= 0)
				{
					// Unsupported (-1) or unused (0) joint, drop it.
					continue;
				}
				if (addJointToModelSkin(skin_info, skin_idx, i))
				{
					gltfindex_to_jointindex_map[i] = replacement_index++;
				}
			}

			// 2. add joints from groups that this model's joints belong to.
			// it is perfectly valid to have more joints than is in use.
			// E.g. sandals that make your legs digitigrade despite not skining
			// to knees or the like. *TODO: sort and add by usecount.
			for (size_t i = 0; i < joint_cnt; ++i)
			{
				if (gltf_joint_index_use[i])
				{
					// This step needs only joints that have zero uses
					continue;
				}
				if (skin_info.mInvBindMatrix.size() >
						(size_t)mMaxJointsPerMesh)
				{
					break;
				}
				const std::string& legal_name = mJointNames[skin_idx][i];
				const std::string& group_name =
					mJointGroups[legal_name].mGroup;
				if (group_use_count[group_name] > 0)
				{
					if (addJointToModelSkin(skin_info, skin_idx, i))
					{
						gltfindex_to_jointindex_map[i] = replacement_index++;
					}
				}
			}
		}
		else
		{
			// Less than 110, just add every valid joint
			for (size_t i = 0; i < joint_cnt; ++i)
			{
				if (gltf_joint_index_use[i] < 0)
				{
					continue;	// Unsupported joint
				}
				if (addJointToModelSkin(skin_info, skin_idx, i))
				{
					gltfindex_to_jointindex_map[i] = replacement_index++;
				}
			}
		}

		// Note: usually mMaxJointsPerMesh == LL_MAX_JOINTS_PER_MESH_OBJECT
		if (skin_info.mInvBindMatrix.size() > (size_t)mMaxJointsPerMesh)
		{
			LLSD args;
			args["Message"] = "ModelTooManyJoints";
			args["MODEL_NAME"] = modelp->mLabel;
			args["JOINT_COUNT"] = (S32)skin_info.mInvBindMatrix.size();
			args["MAX"] = (S32)mMaxJointsPerMesh;
			mWarningsArray.append(args);
		}

		// Remap indices for modelp->mSkinWeights
		for (auto& weights : modelp->mSkinWeights)
		{
			for (auto& weight : weights.second)
			{
				weight.mJointIdx =
					gltfindex_to_jointindex_map[weight.mJointIdx];
			}
		}
	}

	return true;
}

void LLGLTFLoader::addModelToScene(LLModel* modelp,
								   const std::string& model_name,
								   U32 submodel_limit,
								   const LLMatrix4& transformation,
								   const LLVolumeParams& volume_params,
								   const mats_map_t& mats)
{
	U32 volume_faces = (U32)modelp->getNumVolumeFaces();

	// Side-steps all manner of issues when splitting models and matching lower
	// LOD materials to base models

	modelp->sortVolumeFacesByMaterialName();

	// Remove all faces that definitely would not fit into one model and
	// submodel limit
	U32 face_limit = (submodel_limit + 1) * LL_SCULPT_MESH_MAX_FACES;
	if (volume_faces > face_limit)
	{
		LLSD args;
		args["Message"] = "ModelTooManySubmodels";
		args["MODEL_NAME"] = modelp->mLabel;
		args["SUBMODEL_COUNT"] = S32(F32(volume_faces) /
									 F32(LL_SCULPT_MESH_MAX_FACES));
		args["SUBMODEL_LIMIT"] = (S32)submodel_limit;
		mWarningsArray.append(args);
		modelp->setNumVolumeFaces(face_limit);
	}

	S32 submodel_id = 0;
	LLVolume::face_list_t remainder;
	std::vector<LLModel*> ready_models;
	std::string instance_name;
	LLModel* cur_modelp = modelp;
	do
	{
		cur_modelp->trimVolumeFacesToSize(LL_SCULPT_MESH_MAX_FACES, &remainder);
		volume_faces = (U32)remainder.size();
		// Do not add to scene yet because weights and materials are not ready:
		// just save it.
		ready_models.push_back(cur_modelp);

		if (!volume_faces)
		{
			break;
		}

		// If we have left-over volume faces, create another model to absorb
		// them.
		LLModel* next_modep = new LLModel(volume_params, 0.f);
		next_modep->clearFacesAndMaterials();
		next_modep->mSubmodelID = ++submodel_id;
		instance_name = model_name;
		if (next_modep->mSubmodelID > 0)
		{
			instance_name += (char)((int)'a' + next_modep->mSubmodelID);
		}
		// Check for duplicates and add copy suffix if needed
		S32 dup_count = 0;
		for (const auto& inst : mScene[transformation])
		{
			if (inst.mLabel == instance_name)
			{
				++dup_count;
			}
		}
		if (dup_count)
		{
			instance_name += llformat("_copy_%d", dup_count);
		}
		next_modep->mLabel = instance_name;
		next_modep->getVolumeFaces() = remainder;
		next_modep->mNormalizedScale = cur_modelp->mNormalizedScale;
		next_modep->mNormalizedTranslation = cur_modelp->mNormalizedTranslation;
		next_modep->mSkinWeights = cur_modelp->mSkinWeights;
		next_modep->mPosition = cur_modelp->mPosition;

		const LLMeshSkinInfo& cur_skin_info = cur_modelp->mSkinInfo;
		LLMeshSkinInfo& next_skin_info = next_modep->mSkinInfo;
		next_skin_info.mJointNames = cur_skin_info.mJointNames;
		next_skin_info.mJointKeys = cur_skin_info.mJointKeys;
		next_skin_info.mBindShapeMatrix = cur_skin_info.mBindShapeMatrix;
		next_skin_info.mAlternateBindMatrix = cur_skin_info.mAlternateBindMatrix;
		next_skin_info.mInvBindShapeMatrix = cur_skin_info.mInvBindShapeMatrix;
		next_skin_info.mPelvisOffset = cur_skin_info.mPelvisOffset;
		next_skin_info.updateHash();

		if (cur_modelp->mMaterialList.size() > LL_SCULPT_MESH_MAX_FACES)
		{
			next_modep->mMaterialList.assign(cur_modelp->mMaterialList.begin() +
											 LL_SCULPT_MESH_MAX_FACES,
											 cur_modelp->mMaterialList.end());
			cur_modelp->mMaterialList.resize(LL_SCULPT_MESH_MAX_FACES);
		}

		cur_modelp = next_modep;
		remainder.clear();
	}
	while (volume_faces);

	bool first_transform;
	for (size_t i = 0, count = ready_models.size(); i < count; ++i)
	{
		LLModel* cur_modelp = ready_models[i];

		// Remove unused/redundant vertices
		cur_modelp->remapVolumeFaces();

		mModelList.push_back(cur_modelp);

		mats_map_t materials;
		for (U32 i = 0, mcnt = cur_modelp->mMaterialList.size(); i < mcnt; ++i)
		{
			auto it = mats.find(cur_modelp->mMaterialList[i]);
			if (it != mats.end())
			{
				materials[cur_modelp->mMaterialList[i]] = it->second;
			}
			else
			{
				materials[cur_modelp->mMaterialList[i]] = LLImportMaterial();
			}
		}
		// Keep base name for scene instance, add LOD suffix to model label for
		// matching.
		instance_name = cur_modelp->mLabel;
		cur_modelp->mLabel += lod_suffix[mLod];
		mScene[transformation].emplace_back(cur_modelp, instance_name,
											transformation, materials);
		stretch_extents(cur_modelp, transformation, mExtents[0], mExtents[1],
						first_transform);
	}
}
						
void LLGLTFLoader::processNodeHierarchy(S32 node_idx,
										mesh_count_mat_t& mesh_name_counts,
										U32 submodel_limit,
										const LLVolumeParams& volume_params)
{
	if (node_idx < 0 || node_idx >= (S32)mGLTFAsset.mNodes.size())
	{
		return;
	}

	Node& node = mGLTFAsset.mNodes[node_idx];
	LL_DEBUGS("MeshUpload") << "Processing node: " << node_idx << "("
							<< node.mName << ") - Has mesh: "
							<< (node.mMesh >= 0 ? "yes" : "no")
							<< " - Number of children: "
							<< node.mChildren.size() << LL_ENDL;

	if (node.mMesh >= (S32)mGLTFAsset.mMeshes.size())
	{
		LLSD args;
		args["Message"] = "InvalidMeshReference";
		args["NODE_NAME"] = node.mName;
		args["MESH_INDEX"] = node.mMesh;
		args["TOTAL_MESHES"] = (S32)mGLTFAsset.mMeshes.size();
		mWarningsArray.append(args);
		return;
	}

	// Process this node's mesh if it has one
	if (node.mMesh >= 0)
	{
		// Get base mesh name and track usage
		// Potentially multiple nodes can reuse the same mesh and Collada used
		// node name instead of mesh name, so for consistency use node name if
		// avaliable, node index otherwise.
		std::string base_name = get_lod_less_label(node);
		if (base_name.empty())
		{
			base_name = llformat("node_%d", node_idx);
		}

		S32 instance_count = mesh_name_counts[base_name]++;
		// Make name unique
		if (instance_count > 0)
		{
			base_name += llformat("_copy_%d", instance_count);
		}

		mats_map_t mats;
		LLModel* modelp = new LLModel(volume_params, 0.f);
		Mesh& mesh = mGLTFAsset.mMeshes[node.mMesh];
		if (populateModelFromMesh(modelp, base_name, mesh, node, mats,
								  instance_count) &&
			modelp->getStatus() == LLModel::NO_ERRORS && modelp->validate())
		{
			LLMatrix4 transformation;	// Initialized at identify
			mTransform.setIdentity();

			// Adjust the transformation to compensate for mesh normalization
			LLVector3 scale_vec, trans_vec;
			modelp->getNormalizedScaleTranslation(scale_vec, trans_vec);

			LLMatrix4 mesh_translation;
			mesh_translation.setTranslation(trans_vec);
			mesh_translation *= transformation;

			LLMatrix4 mesh_scale;
			mesh_scale.initScale(scale_vec);
			mesh_scale *= mesh_translation;

			transformation = mesh_scale;

			if (node.mSkin >= 0)
			{
				// "Bind Shape Matrix" is supposed to transform the geometry of
				// the skinned mesh into the coordinate space of the joints.
				// For glTF, this matrix is omitted, and it is assumed that this
				// transform is either pre-multiplied with the mesh data, or
				// post-multiplied to the inverse bind matrices.
				// *TODO: it appears to be missing rotation when joints rotate
				// the model or inverted bind matrices are missing inherited
				// rotation (based of values the 'bento shoes' mesh might be
				// missing 90 degrees horizontaly prior to skinning).
				modelp->mSkinInfo.mBindShapeMatrix = mesh_scale;
			}

			if (transformation.determinant() < 0)
			{
				// Negative scales are not supported
				LLSD args;
				args["Message"] = "NegativeScaleNormTrans";
				args["LABEL"] = modelp->mLabel;
				mWarningsArray.append(args);
			}

			addModelToScene(modelp, base_name, submodel_limit, transformation,
							volume_params, mats);
		}
		else
		{
			setLoadState((U32)ERROR_MODEL + (U32)modelp->getStatus());
			delete modelp;
			return;
		}
	}

	// Process all children recursively
	for (S32 i = 0, count = node.mChildren.size(); i < count; ++i)
	{
		processNodeHierarchy(node.mChildren[i], mesh_name_counts,
							 submodel_limit, volume_params);
	}
}

std::string LLGLTFLoader::extractTextureToTempFile(S32 tex_idx,
												   const char* tex_type)
{
	if (tex_idx < 0 || tex_idx >= (S32)mGLTFAsset.mTextures.size())
	{
		return LLStringUtil::null;
	}

	S32 src_idx = mGLTFAsset.mTextures[tex_idx].mSource;
	if (src_idx < 0 || src_idx >= (S32)mGLTFAsset.mImages.size())
	{
		return LLStringUtil::null;
	}

	Image& image = mGLTFAsset.mImages[src_idx];
	// Handle URI-based textures
	if (!image.mUri.empty())
	{
		llinfos << "Found image URI: " << image.mUri << llendl;
		return image.mUri; // Return URI directly
	}

	S32 img_idx = image.mBufferView;
	if (img_idx < 0 || img_idx >= (S32)mGLTFAsset.mBufferViews.size())
	{
		LL_DEBUGS("MeshUpload") << "Image index (" << img_idx
								<< ") out of range [0-"
								<< mGLTFAsset.mBufferViews.size() << "["
								<< LL_ENDL;
		return LLStringUtil::null;
	}

	BufferView& buffer_view = mGLTFAsset.mBufferViews[img_idx];
	if (buffer_view.mBuffer >= (S32)mGLTFAsset.mBuffers.size())
	{
		llwarns << "Buffer view (" << buffer_view.mBuffer << ") of range [0-"
				<< mGLTFAsset.mBuffers.size() << "[" << llendl;
		return LLStringUtil::null;
	}

	Buffer& buffer = mGLTFAsset.mBuffers[buffer_view.mBuffer];
	S32 buffer_end = buffer_view.mByteOffset + buffer_view.mByteLength;
	if (buffer_end > (S32)buffer.mData.size())
	{
		llwarns << "Buffer view end (" << buffer_end
				<< ") pointing beyond buffer size (" << buffer.mData.size()
				<< ")" << llendl;
		return LLStringUtil::null;
	}

	// Extract image data
	const U8* datap = &buffer.mData[buffer_view.mByteOffset];
	U32 data_size = buffer_view.mByteLength;

	// Determine the file extension
	std::string extension = ".png";	// Default
	if (!image.mMimeType.empty())
	{
		if (image.mMimeType == "image/jpeg")
		{
			extension = ".jpg";
		}
		// Else, use default ".png"... Do not bother testing
	}
	else if (data_size >= 4)
	{
		if (datap[0] == 0xFF && datap[1] == 0xD8)
		{
			extension = ".jpg";
		}
		// Else, use default ".png"... Do not bother testing
	}

	// Create a temporary file
	std::string temp_dir = gDirUtil.getTempDir();
	std::string filename = temp_dir + LL_DIR_DELIM_STR + "gltf_embedded_";
	filename.append(tex_type);
	filename += llformat("%d", src_idx) + extension;
	LLFILE* fp = LLFile::open(filename, "wb");
	if (!fp)
	{
		llwarns << "Failed to create temporary file: " << filename << llendl;
		LLSD args;
		args["Message"] = "FailedToCreateTempFile";
		args["TEXTURE_INDEX"] = src_idx;
		args["TEXTURE_TYPE"] = LLSD::String(tex_type);
		args["TEMP_FILE"] = filename;
		mWarningsArray.append(args);
		return LLStringUtil::null;
	}
	if (fwrite(datap, 1, data_size, fp) == data_size)
	{
		llinfos << "Extracted embedded " << tex_type << " texture as: "
				<< filename << llendl;
	}
	else
	{
		llwarns << "Short write on temporary file: " << filename << llendl;
	}
	LLFile::close(fp);
	return filename;
}
