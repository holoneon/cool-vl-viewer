/**
 * @file ndconvexdecomphacd.cpp
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

#include "linden_common.h"

#include <memory>

#include "hacdHACD.h"

#include "ndconvexdecomphacd.h"

///////////////////////////////////////////////////////////////////////////////
// Type definitions and constants (initially from nd_hacdDefines.h)

typedef HACD::HACD hacd_inst_t;
typedef HACD::Vec3<double> vec_dbl_t;
typedef HACD::Vec3<long> vec_lng_t;
typedef vec_lng_t (*from_ixx_fn_t)(void const*&, S32);

// See http://wiki.secondlife.com/wiki/Mesh/Mesh_physics
constexpr S32 MAX_VERTICES_PER_HULL  = 256;
constexpr S32 MIN_NUMBER_OF_CLUSTERS = 1;
constexpr S32 TO_SINGLE_HULL_TRIES = 10;

// Use a high value so HACD will generate just one hull. For now we use the
// same concavity for each run.
int const CONCAVITY_FOR_SINGLE_HULL[TO_SINGLE_HULL_TRIES] =
	{ 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000 };

// Max distance to connect CC. Increase this each run.
double const CONNECT_DISTS[TO_SINGLE_HULL_TRIES] =
	{ 30, 60, 120, 240, 480, 960, 1920, 3840, 7680, 10000 };

///////////////////////////////////////////////////////////////////////////////
// HACD from/to LLCD conversion classes (initially from nd_hacdStructs.h/cpp)
// Note: I turned all structures into classes; this is especially important for
// HACDDecoder which derivates for HACD's HACD::ICallback *class*. HB

class DecompHull
{
public:
	void clear()
	{
		mVertices.clear();
		mTriangles.clear();
		mLLVertices.clear();
		mLLTriangles.clear();
	}

	void computeLLVertices()
	{
		size_t num_verts = mVertices.size();
		if (num_verts != mLLVertices.size() * 3)
		{
			mLLVertices.clear();
			mLLVertices.reserve(num_verts * 3);
			for (size_t i = 0; i < num_verts; ++i)
			{
				mLLVertices.push_back((F32)mVertices[i].X());
				mLLVertices.push_back((F32)mVertices[i].Y());
				mLLVertices.push_back((F32)mVertices[i].Z());
			}
		}
	}

	void computeLLTriangles()
	{
		size_t num_tris = mTriangles.size();
		if (num_tris != mLLTriangles.size() * 3)
		{
			mLLTriangles.clear();
			mLLTriangles.reserve(num_tris * 3);
			for (size_t i = 0; i < num_tris; ++i)
			{
				mLLTriangles.push_back(mTriangles[i].X());
				mLLTriangles.push_back(mTriangles[i].Y());
				mLLTriangles.push_back(mTriangles[i].Z());
			}
		}
	}

	void toLLHull(LLCDHull* hulloutp)
	{
		computeLLVertices();

		hulloutp->mVertexBase = mLLVertices.data();
		hulloutp->mVertexStrideBytes = sizeof(F32) * 3;
		hulloutp->mNumVertices = mVertices.size();
	}

	void toLLMesh(LLCDMeshData* meshoutp)
	{
		computeLLVertices();
		computeLLTriangles();

		meshoutp->mIndexType = LLCDMeshData::INT_32;
		meshoutp->mVertexBase = mLLVertices.data();
		meshoutp->mNumVertices = mVertices.size();
		meshoutp->mVertexStrideBytes = sizeof(F32) * 3;

		meshoutp->mIndexBase = mLLTriangles.data();
		meshoutp->mIndexStrideBytes = sizeof(U32) * 3;
		meshoutp->mNumTriangles = mTriangles.size();
	}

public:
	std::vector<vec_dbl_t>	mVertices;
	std::vector<vec_lng_t>	mTriangles;

	std::vector<F32>		mLLVertices;
	std::vector<U32>		mLLTriangles;
};

class DecompData
{
public:
	void clear()
	{
		mHulls.clear();
	}

public:
	std::vector<DecompHull> mHulls;
};

class HACDDecoder final : public HACD::ICallback
{
public:
	HACDDecoder()
	:	mCallback(NULL)
	{
		mStages.resize(HACD_NUM_STAGES);
	}

	// Note: not declared override, because HACD's HACD::ICallback does not
	// declare its own destructor... HB
	~HACDDecoder() = default;

	void operator()(const char* msg, double progress, double concavity,
					size_t vertices) override
	{
		if (mCallback)
		{
			(*mCallback)(msg, (S32)progress, vertices);
		}
	}

	void clear()
	{
		mCallback = NULL;

		mVertices.clear();
		mTriangles.clear();

		for (size_t i = 0;  i < mStages.size(); ++i)
		{
			mStages[i].clear();
		}

		mSingleHull.clear();
	}

public:
	std::vector<vec_dbl_t>	mVertices;
	std::vector<vec_lng_t>	mTriangles;

	std::vector<DecompData>	mStages;
	DecompHull				mSingleHull;

	llcd_callback_t			mCallback;
};

///////////////////////////////////////////////////////////////////////////////
// Utility functions (initially from nd_hacdUtils.h/cpp)

static hacd_inst_t* init(S32 concavity, S32 clusters, S32 max_verts_per_hull,
						 F64 max_conn_dist, HACDDecoder* decoderp)
{
	hacd_inst_t* hacdp = HACD::CreateHACD(0);
	hacdp->SetPoints(&decoderp->mVertices[0]);
	hacdp->SetNPoints(decoderp->mVertices.size());

	if (decoderp->mTriangles.size())
	{
		hacdp->SetTriangles(&decoderp->mTriangles[0]);
		hacdp->SetNTriangles(decoderp->mTriangles.size());
	}

	hacdp->SetCompacityWeight(0.1f);
	hacdp->SetVolumeWeight(0);
	hacdp->SetNClusters(clusters);
	hacdp->SetAddExtraDistPoints(true);
	hacdp->SetAddFacesPoints(true);
	hacdp->SetNVerticesPerCH(max_verts_per_hull);
	hacdp->SetConcavity(concavity);
	hacdp->SetConnectDist(max_conn_dist);

	hacdp->SetCallBack(decoderp);

	return hacdp;
}

static DecompData decompose(hacd_inst_t* hacdp)
{
	hacdp->Compute();

	DecompData outdata;

	S32 clusters = hacdp->GetNClusters();

	for (S32 i = 0; i < clusters; ++i)
	{
		S32 num_verts = hacdp->GetNPointsCH(i);
		S32 nTriangles = hacdp->GetNTrianglesCH(i);
		DecompHull outhull;

		outhull.mVertices.resize(num_verts);
		outhull.mTriangles.resize(nTriangles);
		hacdp->GetCH(i, &outhull.mVertices[0], &outhull.mTriangles[0]);

		outdata.mHulls.push_back(outhull);
	}

	return outdata;
}

static vec_lng_t fromI16(void const*& datap, int stride)
{
	const U16* valp = reinterpret_cast<const U16*>(datap);
	vec_lng_t v(valp[0], valp[1], valp[2]);
	valp += stride / sizeof(U16);
	datap = valp;
	return v;
}

static vec_lng_t fromI32(void const*& datap, int stride)
{
	const U32* valp = reinterpret_cast<const U32*>(datap);
	vec_lng_t v(valp[0], valp[1], valp[2]);
	valp += stride / sizeof(U32);
	datap = valp;
	return v;
}

static LLCDResult set_mesh_data(const LLCDMeshData* meshp, bool vertex_based,
								HACDDecoder* decoderp)
{
	if (!meshp || !meshp->mVertexBase || meshp->mNumVertices < 3 ||
		 (meshp->mVertexStrideBytes != 12 && meshp->mVertexStrideBytes != 16))
	{
		llwarns << "Invalid mesh data" << llendl;
		return LLCD_INVALID_MESH_DATA;
	}

	if (!vertex_based && (meshp->mNumTriangles < 1 || !meshp->mIndexBase))
	{
		llwarns << "Invalid mesh data" << llendl;
		return LLCD_INVALID_MESH_DATA;
	}

	decoderp->clear();
	S32 count = meshp->mNumVertices;
	const F32* vertdatap = meshp->mVertexBase;
	S32 stride = meshp->mVertexStrideBytes / sizeof(F32);

	for (S32 i = 0; i < count; ++i)
	{
		vec_dbl_t vert(vertdatap[0], vertdatap[1], vertdatap[2]);
		decoderp->mVertices.push_back(vert);
		vertdatap += stride;
	}

	if (!vertex_based)
	{
		count = meshp->mNumTriangles;
		stride = meshp->mIndexStrideBytes;
		const void* idxdatap = meshp->mIndexBase;
		from_ixx_fn_t fn = meshp->mIndexType == LLCDMeshData::INT_16 ? fromI16
																	 : fromI32;
		for (S32 i = 0; i < count; ++i)
		{
			vec_lng_t tri((*fn)(idxdatap, stride));
			decoderp->mTriangles.push_back(tri);
		}
	}

	return LLCD_OK;
}

static LLCDResult hull_to_mesh(const LLCDHull* hullp,
							   std::vector<F32>& vertices,
							   std::vector<S32>& triangles)
{
	if (!hullp || !hullp->mVertexBase)
	{
		LL_DEBUGS("Decomp") << "No hull data" << LL_ENDL;
		return LLCD_NULL_PTR;
	}

	if (hullp->mVertexStrideBytes < S32(3 * sizeof(F32)) ||
		hullp->mNumVertices < 3)
	{
		LL_DEBUGS("Decomp") << "Invalid hull data (no vertices or invalid stride)"
				<< LL_ENDL;
		return LLCD_INVALID_HULL_DATA;
	}

	HACD::ICHull outhull;

	S32 num_verts = hullp->mNumVertices;
	const F32* vertdatap = hullp->mVertexBase;
	S32 stride = hullp->mVertexStrideBytes / sizeof(F32);
	std::vector<vec_dbl_t> verts;
	for (S32 i = 0; i < num_verts; ++i)
	{
		verts.push_back(vec_dbl_t(vertdatap[0], vertdatap[1], vertdatap[2])); 
		vertdatap += stride;
	}

	outhull.AddPoints(verts.data(), verts.size());
	if (outhull.Process(MAX_VERTICES_PER_HULL) != HACD::ICHullErrorOK)
	{
		LL_DEBUGS("Decomp") << "Invalid hull data" << LL_ENDL;
		return LLCD_INVALID_HULL_DATA;
	}

	HACD::TMMesh& outmesh = outhull.GetMesh();

	std::vector<vec_dbl_t> out_verts;
	std::vector<vec_lng_t> out_tris;

	out_verts.resize(outmesh.GetNVertices());
	out_tris.resize(outmesh.GetNTriangles());

	outmesh.GetIFS(&out_verts[0], &out_tris[0]);

	for (size_t i = 0; i < out_verts.size(); ++i)
	{
		vertices.push_back((F32)out_verts[i].X());
		vertices.push_back((F32)out_verts[i].Y());
		vertices.push_back((F32)out_verts[i].Z());
	}

	for (size_t i = 0; i < out_tris.size(); ++i)
	{
		triangles.push_back(out_tris[i].X());
		triangles.push_back(out_tris[i].Y());
		triangles.push_back(out_tris[i].Z());
	}

	return LLCD_OK;
}

static DecompData to_single_hull(HACDDecoder* decoderp, LLCDResult& result)
{
	result = LLCD_REQUEST_OUT_OF_RANGE;

	for (S32 i = 0; i < TO_SINGLE_HULL_TRIES; ++i)
	{
		hacd_inst_t* hacdp = init(CONCAVITY_FOR_SINGLE_HULL[i], 1,
							MAX_VERTICES_PER_HULL, CONNECT_DISTS[i], decoderp);
		DecompData outdata = decompose(hacdp);
		delete hacdp;

		if (outdata.mHulls.size() == 1)
		{
			result = LLCD_OK;
			return outdata;
		}
	}

	return DecompData();
}

///////////////////////////////////////////////////////////////////////////////
// NDConvexDecompHACD class proper

LLCDStageData NDConvexDecompHACD::mStages[1];
LLCDParam NDConvexDecompHACD::mParams[4];
LLCDParam::LLCDEnumItem NDConvexDecompHACD::mMethods[1];
LLCDParam::LLCDEnumItem NDConvexDecompHACD::mQuality[1];
LLCDParam::LLCDEnumItem NDConvexDecompHACD::mSimplify[1];

NDConvexDecompHACD::NDConvexDecompHACD()
:	mSingleHullMeshFromMesh(new HACDDecoder()),
	mCurrentDecoder(0),
	mNextId(0)
{
	memset((void*)&mStages[0], 0, sizeof(mStages));
	memset((void*)&mParams[0], 0, sizeof(mParams));
	memset((void*)&mMethods[0], 0, sizeof(mMethods));
	memset((void*)&mQuality[0], 0, sizeof(mQuality));
	memset((void*)&mSimplify[0], 0, sizeof(mSimplify));

	// Stage name changed from "Decompose" to "Analyze" to match VHACD's. HB
	mStages[0].mName = "Analyze";
	mStages[0].mDescription = NULL;

	mMethods[0].mName = "Default";
	mMethods[0].mValue = 0;

	mQuality[0].mName = "Default";
	mQuality[0].mValue = 0;

	mSimplify[0].mName = "None";
	mSimplify[0].mValue = 0;

	mParams[0].mName = "nd_AlwaysNeedTriangles";
	mParams[0].mDescription = 0;
	mParams[0].mType = LLCDParam::LLCD_BOOLEAN;
	mParams[0].mDefault.mBool = true;

	mParams[1].mName = "Method";
	mParams[1].mType = LLCDParam::LLCD_ENUM;
	mParams[1].mDetails.mEnumValues.mNumEnums =
		sizeof(mMethods) / sizeof(LLCDParam::LLCDEnumItem);
	mParams[1].mDetails.mEnumValues.mEnumsArray = mMethods;
	mParams[1].mDefault.mIntOrEnumValue = 0;

	mParams[2].mName = "Decompose Quality";
	mParams[2].mType = LLCDParam::LLCD_ENUM;
	mParams[2].mDetails.mEnumValues.mNumEnums =
		sizeof(mQuality) / sizeof(LLCDParam::LLCDEnumItem);
	mParams[2].mDetails.mEnumValues.mEnumsArray = mQuality;
	mParams[2].mDefault.mIntOrEnumValue = 0;

	mParams[3].mName = "Simplify Method";
	mParams[3].mType = LLCDParam::LLCD_ENUM;
	mParams[3].mDetails.mEnumValues.mNumEnums =
		sizeof(mSimplify) / sizeof(LLCDParam::LLCDEnumItem);
	mParams[3].mDetails.mEnumValues.mEnumsArray = mSimplify;
	mParams[3].mDefault.mIntOrEnumValue = 0;
}

void NDConvexDecompHACD::genDecomposition(S32& decomp)
{
	HACDDecoder* decoderp = new HACDDecoder();
	decomp = mNextId++;
	mDecoders[decomp] = decoderp;
}

void NDConvexDecompHACD::deleteDecomposition(S32 decomp)
{
	delete mDecoders[decomp];
	mDecoders.erase(decomp);
}

void NDConvexDecompHACD::bindDecomposition(S32 decomp)
{
	mCurrentDecoder = decomp;
}

LLCDResult NDConvexDecompHACD::setParam(const char*, F32)
{
	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::setParam(const char*, bool)
{
	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::setParam(const char*, S32)
{
	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::setMeshData(const LLCDMeshData* meshp,
										   bool vertex_based)
{
	HACDDecoder* decoderp = mDecoders[mCurrentDecoder];
	return set_mesh_data(meshp, vertex_based, decoderp);
}

LLCDResult NDConvexDecompHACD::registerCallback(S32 stage, llcd_callback_t cb)
{
	if (!mDecoders.count(mCurrentDecoder) || !mDecoders[mCurrentDecoder])
	{
		llwarns << "Stage " << stage << " not ready" << llendl;
		return LLCD_STAGE_NOT_READY;
	}
	HACDDecoder* decoderp = mDecoders[mCurrentDecoder];
	decoderp->mCallback = cb;
	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::buildSingleHull()
{
	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::executeStage(S32 stage)
{
	if (stage < 0 || stage >= HACD_NUM_STAGES)
	{
		llwarns << "Unsupported stage " << stage << llendl;
		return LLCD_INVALID_STAGE;
	}

	HACDDecoder* decoderp = mDecoders[mCurrentDecoder];
	hacd_inst_t* hacdp = init(1, MIN_NUMBER_OF_CLUSTERS, MAX_VERTICES_PER_HULL,
							  CONNECT_DISTS[0], decoderp);
	DecompData outdata = decompose(hacdp);
	delete hacdp;

	decoderp->mStages[stage] = outdata;
	return LLCD_OK;
}

S32 NDConvexDecompHACD::getNumHullsFromStage(S32 stage)
{
	HACDDecoder* decoderp = mDecoders[mCurrentDecoder];
	if (!decoderp || stage < 0 || stage >= (S32)decoderp->mStages.size())
	{
		llwarns << "Cannot get hulls from stage " << stage << llendl;
		return 0;
	}
	return decoderp->mStages[stage].mHulls.size();
}

LLCDResult NDConvexDecompHACD::getSingleHull(LLCDHull* hulloutp)
{
	HACDDecoder* decoderp = mDecoders[mCurrentDecoder];

	memset((void*)hulloutp, 0, sizeof(LLCDHull));

	LLCDResult res;
	DecompData outdata = to_single_hull(decoderp, res);
	if (res != LLCD_OK || outdata.mHulls.size() != 1)
	{
		return res;
	}

	decoderp->mSingleHull = outdata.mHulls[0];
	decoderp->mSingleHull.toLLHull(hulloutp);

	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::getHullFromStage(S32 stage, S32 hull,
												LLCDHull* hulloutp)
{
	HACDDecoder* decoderp = mDecoders[mCurrentDecoder];

	memset((void*)hulloutp, 0, sizeof(LLCDHull));

	if (stage < 0 || stage >= (S32)decoderp->mStages.size())
	{
		llwarns << "Invalid stage: " << stage << llendl;
		return LLCD_INVALID_STAGE;
	}

	DecompData& decompdata = decoderp->mStages[stage];

	if (hull < 0 || hull >= (S32)decompdata.mHulls.size())
	{
		llwarns << "Invalid hull: " << hull << llendl;
		return LLCD_REQUEST_OUT_OF_RANGE;
	}

	decompdata.mHulls[hull].toLLHull(hulloutp);

	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::getMeshFromStage(S32 stage, S32 hull,
												LLCDMeshData* dataoutp)
{
	HACDDecoder* decoderp = mDecoders[mCurrentDecoder];

	memset((void*)dataoutp, 0, sizeof(LLCDHull));

	if (stage < 0 || stage >= (S32)decoderp->mStages.size())
	{
		llwarns << "Invalid stage: " << stage << llendl;
		return LLCD_INVALID_STAGE;
	}

	DecompData& decompdata = decoderp->mStages[stage];

	if (hull < 0 || hull >= (S32)decompdata.mHulls.size())
	{
		llwarns << "Invalid hull: " << hull << llendl;
		return LLCD_REQUEST_OUT_OF_RANGE;
	}

	decompdata.mHulls[hull].toLLMesh(dataoutp);
	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::getMeshFromHull(LLCDHull* hullinp,
											   LLCDMeshData* meshoutp)
{
	memset((void*)meshoutp, 0, sizeof(LLCDMeshData));

	mMeshToHullVertices.clear();
	mMeshToHullTriangles.clear();

	if (!hullinp || !hullinp->mVertexBase || !meshoutp)
	{
		llwarns << "No hull data" << llendl;
		return LLCD_NULL_PTR;
	}

	if (hullinp->mVertexStrideBytes < S32(3 * sizeof(F32)) ||
		hullinp->mNumVertices < 3)
	{
		LL_DEBUGS("Decomp") << "Invalid hull data (no vertices or invalid stride)"
							<< LL_ENDL;
		return LLCD_INVALID_HULL_DATA;
	}

	LLCDResult res = hull_to_mesh(hullinp, mMeshToHullVertices,
								  mMeshToHullTriangles);
	if (res != LLCD_OK)
	{
		return res;
	}

	meshoutp->mVertexStrideBytes = sizeof(F32) * 3;
	meshoutp->mNumVertices = mMeshToHullVertices.size() / 3;
	meshoutp->mVertexBase = &mMeshToHullVertices[0];

	meshoutp->mIndexType = LLCDMeshData::INT_32;
	meshoutp->mIndexStrideBytes = sizeof(U32) * 3;
	meshoutp->mNumTriangles = mMeshToHullTriangles.size() / 3;
	meshoutp->mIndexBase = &mMeshToHullTriangles[0];

	return LLCD_OK;
}

LLCDResult NDConvexDecompHACD::generateSingleHullMeshFromMesh(LLCDMeshData* meshinp,
															  LLCDMeshData* meshoutp)
{
	memset((void*)meshoutp, 0, sizeof(LLCDMeshData));
	mSingleHullMeshFromMesh->clear();
	LLCDResult res = set_mesh_data(meshinp, meshinp->mNumVertices > 3,
								   mSingleHullMeshFromMesh);
	if (res != LLCD_OK)
	{
		LL_DEBUGS("Decomp") << "Invalid mesh data" << LL_ENDL;
		return res;
	}

	DecompData outdata = to_single_hull(mSingleHullMeshFromMesh, res);
	if (res != LLCD_OK || outdata.mHulls.size() != 1)
	{
		LL_DEBUGS("Decomp") << "Invalid mesh data" << LL_ENDL;
		return res;
	}

	mSingleHullMeshFromMesh->mSingleHull = outdata.mHulls[0];
	mSingleHullMeshFromMesh->mSingleHull.toLLMesh(meshoutp);

	return LLCD_OK;
}

S32 NDConvexDecompHACD::getParameters(const LLCDParam** paramsoutp)
{
	*paramsoutp = mParams;
	return sizeof(mParams) / sizeof(LLCDParam);
}

S32 NDConvexDecompHACD::getStages(const LLCDStageData** stagesoutp)
{
	*stagesoutp = mStages;
	return sizeof(mStages) / sizeof(LLCDStageData);
}
