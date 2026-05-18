/**
 * @file   llconvexdecompvhacd.cpp
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

#include "linden_common.h"

// This causes #include "VHACD.h" in "llconvexdecompvhacd.h" to expand to the
// full implementation.
#define ENABLE_VHACD_IMPLEMENTATION 1
#include "llconvexdecompvhacd.h"

#include "llmath.h"

constexpr S32 MAX_HULLS = 256;
constexpr S32 MAX_VERTICES_PER_HULL = 256;
constexpr S32 INVALID_DECOMP_ID = -1;

///////////////////////////////////////////////////////////////////////////////
// LLConvexDecompVHACD::VHACDCallback sub-class

LLConvexDecompVHACD::VHACDCallback::VHACDCallback()
:	mCallbackFunc(NULL)
{
}

//virtual
void LLConvexDecompVHACD::VHACDCallback::Update(const double total_progress,
												const double stage_pogress,
												const char* const stage,
												const char* operation)
{
	std::string out_msg = llformat("Stage: %s - Operation: %s", stage,
								   operation);
	if (mCurrentStage != stage && mCurrentOperation != operation)
	{
		mCurrentStage = stage;
		mCurrentOperation = operation;
		llinfos << out_msg << llendl;
	}

	if (mCallbackFunc)
	{
		mCallbackFunc(out_msg.c_str(), ll_round((F32)stage_pogress),
					  ll_round((F32)total_progress));
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLConvexDecompVHACD::VHACDLogger sub-class

//virtual
void LLConvexDecompVHACD::VHACDLogger::Log(const char* const msg)
{
	llinfos << msg << llendl;
}

///////////////////////////////////////////////////////////////////////////////
// LLConvexDecompVHACD::LLVHACDMesh sub-class

void LLConvexDecompVHACD::LLVHACDMesh::setVertices(const F32* datap,
												   S32 num_vertices,
												   S32 vert_stride_bytes)
{
	LL_DEBUGS("Decomp") << "Setting " << num_vertices
						<< " vertices with stride = " << vert_stride_bytes
						<< LL_ENDL;
	mVertices.clear();
	mVertices.reserve(num_vertices);

	const S32 stride = vert_stride_bytes / sizeof(F32);
	for (S32 i = 0; i < num_vertices; ++i)
	{
		mVertices.emplace_back(datap[i * stride], datap[i * stride + 1],
							   datap[i * stride + 2]);
	}
}

void LLConvexDecompVHACD::LLVHACDMesh::setIndices(const void* datap,
												  S32 num_indices,
												  S32 idx_stride_bytes,
												  LLCDMeshData::IndexType type)
{
	LL_DEBUGS("Decomp") << "Setting " << num_indices << " indices of type INT_"
						<< (type == LLCDMeshData::INT_16 ? "16" : "32")
						<< " with stride = " << idx_stride_bytes << LL_ENDL;
	mIndices.clear();
	mIndices.reserve(num_indices);

	if (type == LLCDMeshData::INT_16)
	{
		const U16* indexdatap = (const U16*)datap;
		const S32 stride = idx_stride_bytes / sizeof(U16);
		for (S32 i = 0; i < num_indices; ++i)
		{
			mIndices.emplace_back(indexdatap[i * stride],
								  indexdatap[i * stride + 1],
								  indexdatap[i * stride + 2]);
		}
	}
	else
	{
		const U32* indexdatap = (const U32*)datap;
		const S32 stride = idx_stride_bytes / sizeof(U32);
		for (S32 i = 0; i < num_indices; ++i)
		{
			mIndices.emplace_back(indexdatap[i * stride],
								  indexdatap[i * stride + 1],
								  indexdatap[i * stride + 2]);
		}
	}
}

LLCDResult LLConvexDecompVHACD::LLVHACDMesh::from(const LLCDHull* hullinp)
{
	clear();

	if (!hullinp || !hullinp->mVertexBase || hullinp->mNumVertices < 3 ||
		(hullinp->mVertexStrideBytes != 12 &&
		 hullinp->mVertexStrideBytes != 16))
	{
		llwarns << "Invalid hull data" << llendl;
		return LLCD_INVALID_HULL_DATA;
	}

	setVertices(hullinp->mVertexBase, hullinp->mNumVertices,
				hullinp->mVertexStrideBytes);

	return LLCD_OK;
}

LLCDResult LLConvexDecompVHACD::LLVHACDMesh::from(const LLCDMeshData* meshinp,
												  bool vertex_based)
{
	clear();

	if (!meshinp || !meshinp->mVertexBase || meshinp->mNumVertices < 3 ||
		(meshinp->mVertexStrideBytes != 12 &&
		 meshinp->mVertexStrideBytes != 16))
	{
		llwarns << "Invalid mesh data" << llendl;
		return LLCD_INVALID_MESH_DATA;
	}

	if (!vertex_based && (meshinp->mNumTriangles < 1 || !meshinp->mIndexBase))
	{
		llwarns << "Invalid mesh data" << llendl;
		return LLCD_INVALID_MESH_DATA;
	}

	setVertices(meshinp->mVertexBase, meshinp->mNumVertices,
				meshinp->mVertexStrideBytes);
	if (!vertex_based)
	{
		setIndices(meshinp->mIndexBase, meshinp->mNumTriangles,
				   meshinp->mIndexStrideBytes, meshinp->mIndexType);
	}

	return LLCD_OK;
}

///////////////////////////////////////////////////////////////////////////////
// LLConvexDecompVHACD::LLConvexMesh sub-class

void LLConvexDecompVHACD::LLConvexMesh::setVertices(const vhacd_vertex_vec_t& verts)
{
	mVertices.clear();
	size_t count = verts.size();
	mVertices.reserve(count);
	LL_DEBUGS("Decomp") << "Setting " << count << " vertices." << LL_ENDL;
	for (size_t i = 0; i < count; ++i)
	{
		const VHACD::Vertex& v = verts[i];
		mVertices.emplace_back(F32(v.mX), F32(v.mY), F32(v.mZ));
	}
}

void LLConvexDecompVHACD::LLConvexMesh::setIndices(const vhacd_index_vec_t& indices)
{
	mIndices.clear();
	size_t count = indices.size();
	mIndices.reserve(count);
	LL_DEBUGS("Decomp") << "Setting " << count << " indices." << LL_ENDL;
	for (size_t i = 0; i < count; ++i)
	{
		const VHACD::Triangle& t = indices[i];
		mIndices.emplace_back(U32(t.mI0), U32(t.mI1), U32(t.mI2));
	}
}

void LLConvexDecompVHACD::LLConvexMesh::to(LLCDHull* hulloutp) const
{
	hulloutp->mVertexBase = (F32*)mVertices.data();
	hulloutp->mVertexStrideBytes = sizeof(vertex_t);
	hulloutp->mNumVertices = (S32)mVertices.size();
	LL_DEBUGS("Decomp") << "Set " << hulloutp->mNumVertices << " vertices."
						<< " with stride = " << hulloutp->mVertexStrideBytes
						<< LL_ENDL;
}

void LLConvexDecompVHACD::LLConvexMesh::to(LLCDMeshData* meshoutp) const
{
	meshoutp->mVertexBase = (F32*)mVertices.data();
	meshoutp->mVertexStrideBytes = sizeof(vertex_t);
	meshoutp->mNumVertices = (S32)mVertices.size();
	LL_DEBUGS("Decomp") << "Set " << meshoutp->mNumVertices << " vertices."
						<< " with stride = " << meshoutp->mVertexStrideBytes
						<< LL_ENDL;
	meshoutp->mIndexType = LLCDMeshData::INT_32;
	meshoutp->mIndexBase = mIndices.data();
	meshoutp->mIndexStrideBytes = sizeof(index_t);
	meshoutp->mNumTriangles = (S32)mIndices.size();
	LL_DEBUGS("Decomp") << "Set " << meshoutp->mNumTriangles << " indices."
						<< " with stride = " << meshoutp->mIndexStrideBytes
						<< LL_ENDL;
}

///////////////////////////////////////////////////////////////////////////////
// LLConvexDecompVHACD class proper

LLConvexDecompVHACD::LLConvexDecompVHACD()
:	mBoundDecompID(INVALID_DECOMP_ID),
	mNextDecompID(0)
{
	mVHACDParameters.m_logger = &mVHACDLogger;

	mDecompStages[0].mName = "Analyze";
	mDecompStages[0].mDescription = NULL;

	LLCDParam param;
	param.mName = "Fill Mode";
	param.mDescription = NULL;
	param.mType = LLCDParam::LLCD_ENUM;
	param.mDetails.mEnumValues.mNumEnums = 3;

	static LLCDParam::LLCDEnumItem fill_enums[3];
	fill_enums[(size_t)VHACD::FillMode::FLOOD_FILL].mName = "Flood";
	fill_enums[(size_t)VHACD::FillMode::FLOOD_FILL].mValue =
		(S32)VHACD::FillMode::FLOOD_FILL;
	fill_enums[(size_t)VHACD::FillMode::SURFACE_ONLY].mName = "Surface Only";
	fill_enums[(size_t)VHACD::FillMode::SURFACE_ONLY].mValue =
		(S32)VHACD::FillMode::SURFACE_ONLY;
	fill_enums[(size_t)VHACD::FillMode::RAYCAST_FILL].mName = "Raycast";
	fill_enums[(size_t)VHACD::FillMode::RAYCAST_FILL].mValue =
		(S32)VHACD::FillMode::RAYCAST_FILL;

	param.mDetails.mEnumValues.mEnumsArray = fill_enums;
	param.mDefault.mIntOrEnumValue = (S32)VHACD::FillMode::FLOOD_FILL;
	param.mStage = 0;
	param.mReserved = -1;
	mDecompParams.push_back(param);

	enum EVoxelQualityLevels
	{
		E_LOW_QUALITY = 0,
		E_NORMAL_QUALITY,
		E_HIGH_QUALITY,
		E_VERY_HIGH_QUALITY,
		E_ULTRA_QUALITY,
		E_MAX_QUALITY,
		E_NUM_QUALITY_LEVELS
	};

	param.mName = "Voxel Resolution";
	param.mDescription = NULL;
	param.mType = LLCDParam::LLCD_ENUM;
	param.mDetails.mEnumValues.mNumEnums = E_NUM_QUALITY_LEVELS;

	static LLCDParam::LLCDEnumItem voxel_quality_enums[E_NUM_QUALITY_LEVELS];
	voxel_quality_enums[E_LOW_QUALITY].mName = "Low";
	voxel_quality_enums[E_LOW_QUALITY].mValue = 200000;
	voxel_quality_enums[E_NORMAL_QUALITY].mName = "Normal";
	voxel_quality_enums[E_NORMAL_QUALITY].mValue = 400000;
	voxel_quality_enums[E_HIGH_QUALITY].mName = "High";
	voxel_quality_enums[E_HIGH_QUALITY].mValue = 800000;
	voxel_quality_enums[E_VERY_HIGH_QUALITY].mName = "Very High";
	voxel_quality_enums[E_VERY_HIGH_QUALITY].mValue = 1200000;
	voxel_quality_enums[E_ULTRA_QUALITY].mName = "Ultra";
	voxel_quality_enums[E_ULTRA_QUALITY].mValue = 1600000;
	voxel_quality_enums[E_MAX_QUALITY].mName = "Maximum";
	voxel_quality_enums[E_MAX_QUALITY].mValue = 2000000;

	param.mDetails.mEnumValues.mEnumsArray = voxel_quality_enums;
	param.mDefault.mIntOrEnumValue = 400000;
	param.mStage = 0;
	param.mReserved = -1;
	mDecompParams.push_back(param);

	param.mName = "Num Hulls";
	param.mDescription = NULL;
	param.mType = LLCDParam::LLCD_FLOAT;
	param.mDetails.mRange.mLow.mFloat = 1.f;
	param.mDetails.mRange.mHigh.mFloat = MAX_HULLS;
	param.mDetails.mRange.mDelta.mFloat = 1.f;
	param.mDefault.mFloat = 8.f;
	param.mStage = 0;
	param.mReserved = -1;
	mDecompParams.push_back(param);

	param.mName = "Num Vertices";
	param.mDescription = NULL;
	param.mType = LLCDParam::LLCD_FLOAT;
	param.mDetails.mRange.mLow.mFloat = 3.f;
	param.mDetails.mRange.mHigh.mFloat = MAX_VERTICES_PER_HULL;
	param.mDetails.mRange.mDelta.mFloat = 1.f;
	param.mDefault.mFloat = 32.f;
	param.mStage = 0;
	param.mReserved = -1;
	mDecompParams.push_back(param);

	param.mName = "Error Tolerance";
	param.mDescription = NULL;
	param.mType = LLCDParam::LLCD_FLOAT;
	param.mDetails.mRange.mLow.mFloat = 0.01f;
	param.mDetails.mRange.mHigh.mFloat = 99.f;
	param.mDetails.mRange.mDelta.mFloat = 0.01f;
	param.mDefault.mFloat = 1.f;
	param.mStage = 0;
	param.mReserved = -1;
	mDecompParams.push_back(param);

	for (const LLCDParam& param : mDecompParams)
	{
		const char* const name = param.mName;

		switch (param.mType)
		{
			case LLCDParam::LLCD_FLOAT:
				setParam(name, param.mDefault.mFloat);
				break;

			case LLCDParam::LLCD_ENUM:
			case LLCDParam::LLCD_INTEGER:
				setParam(name, param.mDefault.mIntOrEnumValue);
				break;

			case LLCDParam::LLCD_BOOLEAN:
				setParam(name, (param.mDefault.mBool != 0));
				break;

			default:
				break;
		}
	}
}

//virtual
LLConvexDecompVHACD::~LLConvexDecompVHACD()
{
	mDecompDataMutex.lock();
	mBoundDecompID = INVALID_DECOMP_ID;
	mDecompData.clear();
	mDecompDataMutex.unlock();
}

//virtual
void LLConvexDecompVHACD::genDecomposition(S32& decomp)
{
	mDecompDataMutex.lock();
	mDecompData[mNextDecompID] = std::make_shared<LLDecompData>();
	decomp = mNextDecompID++;
	mDecompDataMutex.unlock();
	LL_DEBUGS("Decomp") << "Generated new decomposition: " << decomp
						<< LL_ENDL;
}

//virtual
void LLConvexDecompVHACD::deleteDecomposition(S32 decomp)
{
	mDecompDataMutex.lock();
	auto iter = mDecompData.find(decomp);
	if (iter != mDecompData.end())
	{
		if (mBoundDecompID == decomp)
		{
			mBoundDecompID = INVALID_DECOMP_ID;
		}
		mDecompData.erase(iter);
		LL_DEBUGS("Decomp") << "Deleted decomposition: " << decomp << LL_ENDL;
	}
	mDecompDataMutex.unlock();
}

//virtual
void LLConvexDecompVHACD::bindDecomposition(S32 decomp)
{
	mDecompDataMutex.lock();
	if (mDecompData.contains(decomp))
	{
		mBoundDecompID = decomp;
		LL_DEBUGS("Decomp") << "Bound decomposition: " << decomp << LL_ENDL;
	}
	else
	{
		mBoundDecompID = INVALID_DECOMP_ID;
		llwarns << "Failed to bind unknown decomposition: " << decomp
				<< llendl;
	}
	mDecompDataMutex.unlock();
}

LLConvexDecompVHACD::data_ptr_t LLConvexDecompVHACD::getBoundDecomp()
{
	data_ptr_t decompp;
	mDecompDataMutex.lock();
	auto it = mDecompData.find(mBoundDecompID);
	if (it != mDecompData.end())
	{
		// Take a copy of the shared_ptr to avoid potential deletion
		decompp = it->second;
	}
	mDecompDataMutex.unlock();
	return decompp;
}

//virtual
LLCDResult LLConvexDecompVHACD::setParam(const char* name, F32 val)
{
	LLCDResult res = LLCD_UNKNOWN_PARAM;
	if (!name || !*name)
	{
		llwarns << "Empty parameter name" << llendl;
		return res;
	}
	mParamsMutex.lock();
	if (strcmp("Num Hulls", name) == 0)
	{
		U32 hulls = llclamp(U32(val), 1, MAX_HULLS);
		mVHACDParameters.m_maxConvexHulls = hulls;
		LL_DEBUGS("Decomp") << "Set number of hulls to: " << hulls << LL_ENDL;
		res = LLCD_OK;
	}
	else if (strcmp("Num Vertices", name) == 0)
	{
		U32 verts = llclamp(U32(val), 3, MAX_VERTICES_PER_HULL);
		mVHACDParameters.m_maxNumVerticesPerCH = verts;
		LL_DEBUGS("Decomp") << "Set number of vertices to: " << verts
							<< LL_ENDL;
		res = LLCD_OK;
	}
	else if (strcmp("Error Tolerance", name) == 0)
	{
		mVHACDParameters.m_minimumVolumePercentErrorAllowed = val;
		LL_DEBUGS("Decomp") << "Set error tolerance to: " << val << LL_ENDL;
		res = LLCD_OK;
	}
	mParamsMutex.unlock();
	if (res == LLCD_UNKNOWN_PARAM)
	{
		llwarns << "Unknown parameter name: " << name << llendl;
	}
	return res;
}

//virtual
LLCDResult LLConvexDecompVHACD::setParam(const char* name, bool)
{
	llwarns << "Unknown parameter name: " << name << llendl;
	return LLCD_UNKNOWN_PARAM;
}

//virtual
LLCDResult LLConvexDecompVHACD::setParam(const char* name, S32 val)
{
	LLCDResult res = LLCD_UNKNOWN_PARAM;
	if (!name || !*name)
	{
		llwarns << "Empty parameter name" << llendl;
		return res;
	}
	mParamsMutex.lock();
	if (strcmp("Fill Mode", name) == 0)
	{
		if (val < 0 || val > 2)
		{
			llwarns << "Invalid fill mode requested: " << val << llendl;
			res = LLCD_BAD_VALUE;
		}
		else
		{
			mVHACDParameters.m_fillMode = (VHACD::FillMode)val;
			LL_DEBUGS("Decomp") << "Set fill mode to: " << val << LL_ENDL;
			res = LLCD_OK;
		}
	}
	if (strcmp("Voxel Resolution", name) == 0)
	{
		if (val < 0)
		{
			llwarns << "Invalid voxel resolution requested: " << val << llendl;
			res = LLCD_BAD_VALUE;
		}
		else
		{
			mVHACDParameters.m_resolution = val;
			LL_DEBUGS("Decomp") << "Set voxel resolution to: " << val << LL_ENDL;
			res = LLCD_OK;
		}
	}
	mParamsMutex.unlock();
	if (res == LLCD_UNKNOWN_PARAM)
	{
		llwarns << "Unknown parameter name: " << name << llendl;
	}
	return res;
}

//virtual
LLCDResult LLConvexDecompVHACD::setMeshData(const LLCDMeshData* datap,
											bool vertex_based)
{
	data_ptr_t decompp = getBoundDecomp();
	if (decompp)
	{
		LL_DEBUGS("Decomp") << "Setting mesh data" << LL_ENDL;
		return decompp->mSourceMesh.from(datap, vertex_based);
	}
	llwarns << "No bound decomposition: cannot set mesh data." << llendl;
	return LLCD_NULL_PTR;
}

//virtual
LLCDResult LLConvexDecompVHACD::registerCallback(S32 stage,
												 llcd_callback_t cb)
{
	if (stage == 0)
	{
		mParamsMutex.lock();
		mCurrentCallbackFunc = cb;
		mParamsMutex.unlock();
		LL_DEBUGS("Decomp") << "Registered callback" << LL_ENDL;
		return LLCD_OK;
	}
	llwarns << "Cannot register a callback at stage " << stage << llendl;
	return LLCD_INVALID_STAGE;
}

//virtual
LLCDResult LLConvexDecompVHACD::executeStage(S32 stage)
{
	if (stage != 0)
	{
		llwarns << "Cannot execute stage " << stage << llendl;
		return LLCD_INVALID_STAGE;
	}

	data_ptr_t decompp = getBoundDecomp();
	if (!decompp)
	{
		llwarns << "No bound decomposition: cannot execute stage " << stage
				<< llendl;
		return LLCD_NULL_PTR;
	}

	LL_DEBUGS("Decomp") << "Executing stage " << stage << LL_ENDL;

	decompp->mDecomposedHulls.clear();

	const auto& decomp_mesh = decompp->mSourceMesh;

	VHACDCallback callbacks;
	VHACD::IVHACD::Parameters current_params;
	mParamsMutex.lock();
	current_params = mVHACDParameters;
	callbacks.setCallbackFunc(mCurrentCallbackFunc);
	current_params.m_callback = &callbacks;
	mParamsMutex.unlock();

	VHACD::IVHACD* vhacdp = VHACD::CreateVHACD();
	if (!vhacdp)
	{
		llwarns << "Failed to create VHACD instance" << llendl;
		return LLCD_NULL_PTR;
	}

	if (!vhacdp->Compute((const double* const)decomp_mesh.mVertices.data(),
						 (U32)decomp_mesh.mVertices.size(),
						 (const U32* const)decomp_mesh.mIndices.data(),
						 (U32)decomp_mesh.mIndices.size(),
						 current_params))
	{
		vhacdp->Release();
		llwarns << "Invalid hull data (1)" << llendl;
		return LLCD_INVALID_HULL_DATA;
	}

	U32 num_nulls = vhacdp->GetNConvexHulls();
	if (num_nulls == 0)
	{
		vhacdp->Release();
		llwarns << "Invalid hull data (2)" << llendl;
		return LLCD_INVALID_HULL_DATA;
	}

	for (U32 i = 0; num_nulls > i; ++i)
	{
		VHACD::IVHACD::ConvexHull ch;
		if (!vhacdp->GetConvexHull(i, ch))
		{
			continue;
		}

		LLConvexMesh out_mesh;
		out_mesh.setVertices(ch.m_points);
		out_mesh.setIndices(ch.m_triangles);
		decompp->mDecomposedHulls.emplace_back(out_mesh);
	}

	vhacdp->Release();

	LL_DEBUGS("Decomp") << "Stage success" << LL_ENDL;
	return LLCD_OK;
}

//virtual
LLCDResult LLConvexDecompVHACD::buildSingleHull()
{
	data_ptr_t decompp = getBoundDecomp();
	if (!decompp || decompp->mSourceMesh.mVertices.empty())
	{
		return LLCD_NULL_PTR;
	}
	LL_DEBUGS("Decomp") << "Building single hull mesh..." << LL_ENDL;

	decompp->mSingleHullMesh.clear();

	VHACD::QuickHull quickhull;
	U32 num_tris =
		quickhull.ComputeConvexHull(decompp->mSourceMesh.mVertices,
									MAX_VERTICES_PER_HULL);
	if (num_tris > 0)
	{
		decompp->mSingleHullMesh.setVertices(quickhull.GetVertices());
		decompp->mSingleHullMesh.setIndices(quickhull.GetIndices());
		LL_DEBUGS("Decomp") << "Single hull mesh built" << LL_ENDL;
		return LLCD_OK;
	}

	llwarns << "Invalid mesh data" << llendl;
	return LLCD_INVALID_MESH_DATA;
}

//virtual
S32 LLConvexDecompVHACD::getNumHullsFromStage(S32 stage)
{
	if (stage != 0)
	{
		llwarns << "Cannot get hulls from stage " << stage << llendl;
		return 0;
	}
	data_ptr_t decompp = getBoundDecomp();
	if (!decompp)
	{
		llwarns << "No bound decomposition; cannot get hulls." << llendl;
		return 0;
	}
	S32 hulls = decompp->mDecomposedHulls.size();
	LL_DEBUGS("Decomp") << "Number of decomposed hulls: " << hulls << LL_ENDL;
	return hulls;
}

//virtual
LLCDResult LLConvexDecompVHACD::getSingleHull(LLCDHull* hulloutp)
{
	memset((void*)hulloutp, 0, sizeof(LLCDHull));

	data_ptr_t decompp = getBoundDecomp();
	if (!decompp)
	{
		llwarns << "No bound decomposition; cannot get hulls." << llendl;
		return LLCD_NULL_PTR;
	}

	if (decompp->mSingleHullMesh.mVertices.empty())
	{
		llwarns << "Invalid hull data (no vertices)" << llendl;
		return LLCD_INVALID_HULL_DATA;
	}

	decompp->mSingleHullMesh.to(hulloutp);
	LL_DEBUGS("Decomp") << "Got single hull from mesh." << LL_ENDL;
	return LLCD_OK;
}

//virtual
LLCDResult LLConvexDecompVHACD::getHullFromStage(S32 stage, S32 hull,
												 LLCDHull* hulloutp)
{
	memset((void*)hulloutp, 0, sizeof(LLCDHull));

	data_ptr_t decompp = getBoundDecomp();
	if (!decompp)
	{
		llwarns << "No bound decomposition; cannot get hulls." << llendl;
		return LLCD_NULL_PTR;
	}

	if (stage != 0)
	{
		llwarns << "Request done at invalid stage: " << stage << llendl;
		return LLCD_INVALID_STAGE;
	}

	if (decompp->mDecomposedHulls.empty() ||
		(S32)decompp->mDecomposedHulls.size() <= hull)
	{
		llwarns << "Hull number out of range: " << hull << llendl;
		return LLCD_REQUEST_OUT_OF_RANGE;
	}

	decompp->mDecomposedHulls[hull].to(hulloutp);
	LL_DEBUGS("Decomp") << "Got decomposed hull #" << hull << LL_ENDL;
	return LLCD_OK;
}

//virtual
LLCDResult LLConvexDecompVHACD::getMeshFromStage(S32 stage, S32 hull,
												 LLCDMeshData* dataoutp)
{
	memset((void*)dataoutp, 0, sizeof(LLCDMeshData));

	data_ptr_t decompp = getBoundDecomp();
	if (!decompp)
	{
		llwarns << "No bound decomposition; cannot get hulls." << llendl;
		return LLCD_NULL_PTR;
	}

	if (stage != 0)
	{
		llwarns << "Request done at invalid stage: " << stage << llendl;
		return LLCD_INVALID_STAGE;
	}

	if (decompp->mDecomposedHulls.empty() ||
		(S32)decompp->mDecomposedHulls.size() <= hull)
	{
		llwarns << "Hull number out of range: " << hull << llendl;
		return LLCD_REQUEST_OUT_OF_RANGE;
	}

	decompp->mDecomposedHulls[hull].to(dataoutp);
	LL_DEBUGS("Decomp") << "Got mesh from stage " << stage << " for hull #"
						<< hull << LL_ENDL;
	return LLCD_OK;
}

//virtual
LLCDResult LLConvexDecompVHACD::getMeshFromHull(LLCDHull* hullinp,
												LLCDMeshData* meshoutp)
{
	memset((void*)meshoutp, 0, sizeof(LLCDMeshData));

	LLVHACDMesh input_mesh(hullinp);
	VHACD::QuickHull quickhull;
	U32 num_tris = quickhull.ComputeConvexHull(input_mesh.mVertices,
											   MAX_VERTICES_PER_HULL);
	if (num_tris > 0)
	{
		mMeshFromHullData.setVertices(quickhull.GetVertices());
		mMeshFromHullData.setIndices(quickhull.GetIndices());

		mMeshFromHullData.to(meshoutp);
		LL_DEBUGS("Decomp") << "Successfully set mesh from hull" << LL_ENDL;
		return LLCD_OK;
	}

	llwarns << "Invalid hull data" << llendl;
	return LLCD_INVALID_HULL_DATA;
}

//virtual
LLCDResult LLConvexDecompVHACD::generateSingleHullMeshFromMesh(LLCDMeshData* meshinp,
															   LLCDMeshData* meshoutp)
{
	memset((void*)meshoutp, 0, sizeof(LLCDMeshData));

	LLVHACDMesh input_mesh(meshinp, true);
	VHACD::QuickHull quickhull;
	U32 num_tris = quickhull.ComputeConvexHull(input_mesh.mVertices,
											   MAX_VERTICES_PER_HULL);
	if (num_tris > 0)
	{
		mSingleHullMeshFromMeshData.setVertices(quickhull.GetVertices());
		mSingleHullMeshFromMeshData.setIndices(quickhull.GetIndices());

		mSingleHullMeshFromMeshData.to(meshoutp);
		LL_DEBUGS("Decomp") << "Successfully set single hull mesh from mesh"
							<< LL_ENDL;
		return LLCD_OK;
	}

	llwarns << "Invalid mesh data" << llendl;
	return LLCD_INVALID_MESH_DATA;
}
