/**
 * @file llvolumemgr.h
 * @brief LLVolumeMgr class.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llmutex.h"
#include "llpointer.h"
#include "llthread.h"
#include "llvolume.h"

class LLVolumeParams;
class LLVolumeLODGroup;

class LLVolumeLODGroup
{
protected:
	LOG_CLASS(LLVolumeLODGroup);

public:
	enum
	{
		NUM_LODS = 4
	};

	LLVolumeLODGroup(const LLVolumeParams& params);
	~LLVolumeLODGroup();
	bool cleanupRefs();

	static S32 getDetailFromTan(F32 tan_angle);
	static void getDetailProximity(F32 tan_angle, F32& to_lower,
								   F32& to_higher);
	static F32 getVolumeScaleFromDetail(S32 detail);
	static S32 getVolumeDetailFromScale(F32 scale);

	LLVolume* refLOD(S32 detail);
	bool derefLOD(LLVolume* volumep);
	LL_INLINE S32 getNumRefs() const						{ return mRefs; }

	LL_INLINE const LLVolumeParams* getVolumeParams() const	{ return &mVolumeParams; }

	F32	dump();
	friend std::ostream& operator<<(std::ostream& s,
									const LLVolumeLODGroup& volgroup);

protected:
	LLVolumeParams		mVolumeParams;

	S32					mRefs;

	S32					mAccessCount[NUM_LODS];
	S32					mLODRefs[NUM_LODS];
	LLPointer<LLVolume>	mVolumeLODs[NUM_LODS];

	static F32			sDetailThresholds[NUM_LODS];
	static F32			sDetailScales[NUM_LODS];
};

class LLVolumeMgr
{
protected:
	LOG_CLASS(LLVolumeMgr);

public:
	static void initClass();
	static void cleanupClass();

	LLVolumeLODGroup* getGroup(const LLVolumeParams& vparams);

	// Whatever calls getVolume() never owns the LLVolume* and cannot keep
	// references for long since it may be deleted later. For best results hold
	// it in an LLPointer<LLVolume>.
	LLVolume* refVolume(const LLVolumeParams& volume_params, S32 detail);
	void unrefVolume(LLVolume* volumep);

	void dump();

	friend std::ostream& operator<<(std::ostream& s,
									const LLVolumeMgr& volume_mgr);

private:
	// Use initclass() and cleanupClass()
	LLVolumeMgr() = default;
	~LLVolumeMgr();

	void insertGroup(LLVolumeLODGroup* volgroup);

	LLVolumeLODGroup* createNewGroup(const LLVolumeParams& vparams);

private:
	typedef std::map<const LLVolumeParams*, LLVolumeLODGroup*,
					 LLVolumeParams::compare> volume_lod_group_map_t;
	volume_lod_group_map_t	mVolumeLODGroups;

	LLMutex					mDataMutex;
};

extern LLVolumeMgr* gVolumeMgrp;
