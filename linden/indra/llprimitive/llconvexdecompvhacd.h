/**
 * @file   llconvexdecompvhacd.h
 * @author rye@alchemyviewer.org
 * @brief  A VHACD based implementation of LLConvexDecomposition
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 *
 * Copyright (c) 2025, Linden Research, Inc.
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

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "glm/vec3.hpp"
#include "glm/gtx/type_aligned.hpp"

#include "VHACD.h"

#include "llconvexdecomposition.h"
#include "hbfastmap.h"
#include "llmemory.h"				//  For LL_ALIGNED16_NEW_DELETE
#include "llmutex.h"

class LLConvexDecompVHACD final : public LLConvexDecomposition
{
protected:
	LOG_CLASS(LLConvexDecompVHACD);

public:
	LLConvexDecompVHACD();
	~LLConvexDecompVHACD() override;

	void genDecomposition(S32& decomp) override;
	void deleteDecomposition(S32 decomp) override;
	void bindDecomposition(S32 decomp) override;

	// Sets *params_out to the address of the LLCDParam array and returns
	// the length of the array
	LL_INLINE S32 getParameters(const LLCDParam** params_out) override
	{
		*params_out = mDecompParams.data();
		return U32(mDecompParams.size());
	}

	LL_INLINE S32 getStages(const LLCDStageData** stages_out) override
	{
		*stages_out = mDecompStages.data();
		return U32(mDecompStages.size());
	}

	// Set a parameter by name. Returns false if out of bounds or unsupported
	// parameter
	LLCDResult setParam(const char* name, F32 val) override;
	LLCDResult setParam(const char* name, S32 val) override;
	LLCDResult setParam(const char* name, bool val) override;
	LLCDResult setMeshData(const LLCDMeshData* datap,
						   bool vertex_based) override;
	LLCDResult registerCallback(S32 stage, llcd_callback_t cb) override;

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
	class VHACDCallback : public VHACD::IVHACD::IUserCallback
	{
	protected:
		LOG_CLASS(VHACDCallback);

	public:
		VHACDCallback();

		void Update(const double total_progress, const double stage_pogress,
					const char* const stage, const char* operation) override;

		LL_INLINE void setCallbackFunc(llcd_callback_t cb)
		{
			mCallbackFunc = cb;
		}

	private:
		llcd_callback_t	mCallbackFunc;
		std::string		mCurrentStage;
		std::string		mCurrentOperation;
	};

	class VHACDLogger : public VHACD::IVHACD::IUserLogger
	{
	protected:
		LOG_CLASS(VHACDLogger);

		void Log(const char* const msg) override;
	};

	typedef std::vector<VHACD::Vertex> vhacd_vertex_vec_t;
	typedef std::vector<VHACD::Triangle> vhacd_index_vec_t;

	class LLVHACDMesh
	{
	public:
		LLVHACDMesh() = default;
		LL_INLINE LLVHACDMesh(const LLCDHull* hullinp)
		{
			if (hullinp)
			{
				from(hullinp);
			}
		}

		LL_INLINE LLVHACDMesh(const LLCDMeshData* meshinp, bool vertex_based)
		{
			if (meshinp)
			{
				from(meshinp, vertex_based);
			}
		}

		LL_INLINE void clear()
		{
			mVertices.clear();
			mIndices.clear();
		}

		void setVertices(const F32* datap, S32 num_vertices,
						 S32 vert_stride_bytes);
		void setIndices(const void* datap, S32 num_indices,
						S32 idx_stride_bytes, LLCDMeshData::IndexType type);

		LLCDResult from(const LLCDHull* hullinp);
		LLCDResult from(const LLCDMeshData* meshinp, bool vertex_based);

	public:
		vhacd_vertex_vec_t	mVertices;
		vhacd_index_vec_t	mIndices;
	};

	// 16-bytes-aligned, since storing aligned glm vectors. HB
	class alignas(16) LLConvexMesh
	{
	public:
		LL_ALIGNED16_NEW_DELETE

		LLConvexMesh() = default;

		LL_INLINE void clear()
		{
			mVertices.clear();
			mIndices.clear();
		}

		void setVertices(const vhacd_vertex_vec_t& in_verts);
		void setIndices(const vhacd_index_vec_t& in_indices);

		void to(LLCDHull* hulloutp) const;
		void to(LLCDMeshData* meshoutp) const;

	public:
		using vertex_t = glm::vec3;
		using index_t = glm::u32vec3;
		using vertex_array_t = std::vector<vertex_t>;
		using index_array_t = std::vector<index_t>;

		alignas(16) vertex_array_t	mVertices;
		alignas(16) index_array_t	mIndices;
	};

	struct alignas(16) LLDecompData
	{
		LL_ALIGNED16_NEW_DELETE

		// Aligned members first. HB
		alignas(16) LLConvexMesh				mSingleHullMesh;
		alignas(16) std::vector<LLConvexMesh>	mDecomposedHulls;
		LLVHACDMesh								mSourceMesh;
	};

	typedef std::shared_ptr<LLDecompData> data_ptr_t;
	data_ptr_t getBoundDecomp();

private:
	alignas(16) LLConvexMesh		mMeshFromHullData;
	alignas(16) LLConvexMesh		mSingleHullMeshFromMeshData;

	data_ptr_t						mBoundDecomp;

	std::vector<LLCDParam>			mDecompParams;
	std::array<LLCDStageData, 1>	mDecompStages;

	flat_hmap<S32, data_ptr_t>		mDecompData;

	LLMutex							mDecompDataMutex;
	LLMutex							mParamsMutex;

	S32								mBoundDecompID;
	// Only for use inside genDecomposition().
	S32								mNextDecompID;

	VHACDLogger						mVHACDLogger;
	VHACD::IVHACD::Parameters		mVHACDParameters;
	llcd_callback_t					mCurrentCallbackFunc;
};
