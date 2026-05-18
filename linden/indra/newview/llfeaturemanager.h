/**
 * @file llfeaturemanager.h
 * @brief The feature manager is responsible for determining what features are turned on/off in the app.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llstring.h"

typedef enum EGPUClass
{
	GPU_CLASS_UNKNOWN = -1,
	GPU_CLASS_0 = 0,
	GPU_CLASS_1,
	GPU_CLASS_2,
	GPU_CLASS_3,
	GPU_CLASS_4,
	GPU_CLASS_5
} EGPUClass;

class LLFeatureInfo
{
public:
	LLFeatureInfo()
	:	mValid(false),
		mAvailable(false),
		mRecommendedLevel(-1.f)
	{
	}

	LLFeatureInfo(const std::string& name, bool available, F32 level);

	LL_INLINE bool isValid() const				{ return mValid; }

public:
	F32			mRecommendedLevel;
	bool		mValid;
	bool		mAvailable;
	std::string	mName;
};

class LLFeatureList
{
protected:
	LOG_CLASS(LLFeatureList);

public:
	typedef std::map<std::string, LLFeatureInfo, std::less<> > feature_map_t;

	LLFeatureList(const std::string& name);
	virtual ~LLFeatureList() = default;

	bool isFeatureAvailable(const std::string& name);

	void setRecommendedLevel(const std::string& name, F32 level);

	void maskList(LLFeatureList& mask);

	void addFeature(const std::string& name, bool available, F32 level);

	LL_INLINE feature_map_t& getFeatures()		{ return mFeatures; }

	void dump();

protected:
	std::string		mName;
	feature_map_t	mFeatures;
};

class LLFeatureManager : public LLFeatureList
{
protected:
	LOG_CLASS(LLFeatureManager);

public:
	LL_INLINE LLFeatureManager()
	:	LLFeatureList("default"),
		mTableVersion(0),
		mSafe(false),
		mGPUClass(GPU_CLASS_UNKNOWN),
		mGPUSupported(false),
		mGPUMemoryBandwidth(0)
	{
	}

	LL_INLINE ~LLFeatureManager()				{ cleanupFeatureTables(); }

	// initialize this by loading feature table and gpu table
	void init();

	// Mask the current feature list with the named list
	void maskCurrentList(const std::string& name);

	bool loadFeatureTables();

	LL_INLINE EGPUClass getGPUClass() const		{ return mGPUClass; }

	LL_INLINE const std::string& getGPUString() const
	{
		return mGPUString;
	}

	LL_INLINE bool isGPUSupported() const		{ return mGPUSupported; }
	LL_INLINE F32 getGPUMemoryBandwidth() const	{ return mGPUMemoryBandwidth; }

	void cleanupFeatureTables();

	LL_INLINE S32 getVersion() const			{ return mTableVersion; }
	LL_INLINE void setSafe(bool safe)			{ mSafe = safe; }
	LL_INLINE bool isSafe() const				{ return mSafe; }

	LLFeatureList* findMask(const std::string& name);
	bool maskFeatures(const std::string& name);

	// Set the graphics to low, medium, high, or ultra. skip_features forces
	// skipping of mostly hardware settings that we don't want to change when
	// we change graphics settings.
	void setGraphicsLevel(S32 level, bool skip_features);

	void applyBaseMasks();
	void applyRecommendedSettings();

	// Apply the basic masks. Also, skip one saved in the skip list if true
	void applyFeatures(bool skip_features);

protected:
	void loadGPUClass(bool benchmark_gpu);
	void initBaseMask();
	static F32 benchmarkGPU();

protected:
	std::string		mGPUString;
	typedef std::map<std::string, LLFeatureList*, std::less<> > mask_map_t;
	mask_map_t		mMaskList;
	strings_set_t	mSkippedFeatures;
	S32				mTableVersion;
	F32				mGPUMemoryBandwidth;
	EGPUClass		mGPUClass;
	bool			mGPUSupported;
	// To reinitialize everything to the "safe" mask:
	bool			mSafe;
};

extern LLFeatureManager gFeatureManager;
