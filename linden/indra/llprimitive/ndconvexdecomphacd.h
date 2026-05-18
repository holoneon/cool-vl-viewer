/**
 * @file ndconvexdecomphacd.h
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 *
 * Copyright (c) 2011, Nicky Dasmijn <sl.nicky.ml@googlemail.com>
 * Copyright (c) 2026, Henri Beauchamp
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

#include <map>
#include <vector>

#include "llconvexdecomposition.h"

#define HACD_NUM_STAGES 1

class HACDDecoder;

class NDConvexDecompHACD final : public LLConvexDecomposition
{
protected:
	LOG_CLASS(NDConvexDecompHACD);

public:
	NDConvexDecompHACD();

	// Generate a decomposition object handle
	void genDecomposition(S32& decomp) override;
	// Delete decomposition object handle
	void deleteDecomposition(S32 decomp) override;
	// Bind given decomposition handle
	// Commands operate on currently bound decomposition
	void bindDecomposition(S32 decomp) override;

	// Sets *params_outp to the address of the LLCDParam array and returns
	// the length of the array
	S32 getParameters(const LLCDParam** params_outp) override;
	S32 getStages(const LLCDStageData** stages_outp) override;

	// Set a parameter by name. Returns false if out of bounds or unsupported
	// parameter
	LLCDResult setParam(const char* name, F32 val) override;
	LLCDResult setParam(const char* name, S32 val) override;
	LLCDResult setParam(const char* name, bool val) override;

	LLCDResult setMeshData(const LLCDMeshData* datap,
						   bool vertex_based) override;
	LLCDResult registerCallback(S32 stage, llcd_callback_t callback) override;

	LLCDResult executeStage(S32 stage) override;
	LLCDResult buildSingleHull() override;

	S32 getNumHullsFromStage(S32 stage) override;

	LLCDResult getHullFromStage(S32 stage, S32 hull,
								LLCDHull* hulloutp) override;
	LLCDResult getSingleHull(LLCDHull* hulloutp) override;

	// *TODO: Implement lock of some kind to disallow this call if data not yet
	// ready
	LLCDResult getMeshFromStage(S32 stage, S32 hull,
								LLCDMeshData* dataoutp) override;
	LLCDResult getMeshFromHull(LLCDHull* hullinp,
							   LLCDMeshData* meshoutp) override;

	// For visualizing convex hull shapes in the viewer physics shape display
	LLCDResult generateSingleHullMeshFromMesh(LLCDMeshData* meshinp,
											  LLCDMeshData* meshoutp) override;

private:
	HACDDecoder*					mSingleHullMeshFromMesh;
	std::map<S32, HACDDecoder*>		mDecoders;
	S32								mCurrentDecoder;
	S32								mNextId;

	std::vector<F32>				mMeshToHullVertices;
	std::vector<S32>				mMeshToHullTriangles;

	static LLCDStageData			mStages[HACD_NUM_STAGES];
	static LLCDParam				mParams[4];
	static LLCDParam::LLCDEnumItem	mMethods[HACD_NUM_STAGES];
	static LLCDParam::LLCDEnumItem	mQuality[HACD_NUM_STAGES];
	static LLCDParam::LLCDEnumItem	mSimplify[HACD_NUM_STAGES];
};
