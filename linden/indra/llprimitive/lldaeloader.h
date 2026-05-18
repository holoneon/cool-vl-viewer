/**
 * @file lldaeloader.h
 * @brief LLDAELoader class definition
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 *
 * Copyright (c) 2013, Linden Research, Inc.
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

#include "llmodelloader.h"

class DAE;
class daeElement;
class domProfile_COMMON;
class domInstance_geometry;
class domNode;
class domTranslate;
class domController;
class domSkin;
class domMesh;

class LLDAELoader final : public LLModelLoader
{
public:
	typedef std::map<daeElement*, std::vector<LLPointer<LLModel> > > dae_model_map;
	dae_model_map mModelsMap;

	LLDAELoader(const std::string& filename, S32 lod,
				LLModelLoader::load_callback_t load_cb,
				LLModelLoader::joint_lookup_func_t joint_lookup_func,
				LLModelLoader::texture_load_func_t texture_load_func,
				LLModelLoader::state_callback_t state_cb, void* userdata,
				JointTransformMap& joint_transform_map,
				JointNameSet& joints_from_nodes,
				strings_map_t& joint_alias_map, U32 max_joints_per_mesh,
				U32 model_limit, bool preprocess);

	bool openFile(const std::string& filename) override;

protected:
	void processElement(daeElement* element, bool& badElement, DAE* dae);
	void processDomModel(LLModel* modelp, DAE* dae, daeElement* rootp,
						 domMesh* meshp, domSkin* skinp);

	mats_map_t getMaterials(LLModel* model,
							domInstance_geometry* instance_geo, DAE* dae);
	LLImportMaterial profileToMaterial(domProfile_COMMON* material, DAE* dae);
	LLColor4 getDaeColor(daeElement* element);

	daeElement* getChildFromElement(daeElement* elementp,
									const std::string& name);

	bool isNodeAJoint(domNode* nodep);
	void processJointNode(domNode* nodep, JointTransformMap& jointTransforms);
	void extractTranslation(domTranslate* translatep, LLMatrix4& transform);
	void extractTranslationViaElement(daeElement* translate_elemp,
									  LLMatrix4& transform);
	void extractTranslationViaSID(daeElement* elementp, LLMatrix4& transform);
	void buildJointToNodeMappingFromScene(daeElement* rootp);
	void processJointToNodeMapping(domNode* nodep);
	void processChildJoints(domNode* parent_nodep);

	bool verifyCount(S32 expected, S32 result);

	// Verify that a controller matches vertex counts
	bool verifyController(domController* controllerp);

	static bool addVolumeFacesFromDomMesh(LLModel* modelp, domMesh* meshp,
										  LLSD& log_msg);

	// Loads a mesh breaking it into one or more models as necessary to get
	// around volume face limitations while retaining > 8 materials
	bool loadModelsFromDomMesh(domMesh* meshp,
							   std::vector<LLModel*>& models_out,
							   U32 submodel_limit);

	static std::string getElementLabel(daeElement* elementp);

	static size_t getSuffixPosition(const std::string& label);
	static std::string getLodlessLabel(daeElement* elementp);

	static std::string preprocessDAE(const std::string& filename);

private:
	bool	mPreprocessDAE;
};
