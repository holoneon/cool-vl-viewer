/**
 * @file lllocalgltfmaterials.h
 * @brief LLLocalGLTFMaterial and HBFloaterLocalMaterial classes declaration
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 *
 * Copyright (c) 2022, Linden Research, Inc. (LLLocalGLTFMaterial)
 * Copyright (c) 2023, Henri Beauchamp. (HBFloaterLocalMaterial)
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

#include <list>
#include <time.h>						// For time_t

#include "lleventtimer.h"
#include "hbfileselector.h"
#include "llpointer.h"

#include "llgltfmateriallist.h"

class LLButton;
class LLCheckBoxCtrl;
class LLScrollListCtrl;
class LLTextureEntry;

class LLLocalGLTFMaterialTimer final : public LLEventTimer
{
public:
	LLLocalGLTFMaterialTimer();

	bool tick() override;

	void startTimer();
	void stopTimer();
	bool isRunning();
};

class LLLocalGLTFMaterial : public LLFetchedGLTFMaterial
{
protected:
	LOG_CLASS(LLLocalGLTFMaterial);

public:
	LLLocalGLTFMaterial(const std::string& filename, S32 index);

	LL_INLINE const std::string& getFilename() const	{ return mFilename; }
	LL_INLINE const std::string& getShortName() const	{ return mShortName; }
	LL_INLINE const LLUUID& getTrackingID() const		{ return mTrackingID; }
	LL_INLINE const LLUUID& getWorldID() const			{ return mWorldID; }
	LL_INLINE S32 getIndexInFile() const				{ return mMaterialIndex; }

	bool updateSelf();

	typedef std::list<LLPointer<LLLocalGLTFMaterial> > list_t;
	LL_INLINE static const list_t& getMaterialList()	{ return sMaterialList; }

	static void addUnits();
	static void delUnit(const LLUUID& tracking_id);

	static const LLUUID& getWorldID(const LLUUID& tracking_id);
	static bool isLocal(const LLUUID& world_id);
	static const std::string& getFilenameAndIndex(const LLUUID& tracking_id,
												  S32& index);
	static void associate(const LLUUID& tracking_id, LLGLTFMaterial* matp);
	static void doUpdates();

	// To be called on viewer shutdown in LLAppViewer::cleanup(). HB
	static void cleanupClass();

	LL_INLINE static S32 getMaterialListVersion()		{ return sMaterialsListVersion; }

private:
	bool loadMaterial();

	static S32 addUnit(const std::string& filename);
	static void addUnitsCallback(HBFileSelector::ELoadFilter type,
								 std::deque<std::string>& files, void*);

private:
	LLUUID							mTrackingID;
	LLUUID							mWorldID;

	std::string						mFilename;
	std::string						mShortName;

	time_t							mLastModified;

	// A single file can have more than one material, so we use an index
	S32								mMaterialIndex;

	U32								mUpdateRetries;

	enum EExtension
	{
		ET_MATERIAL_GLTF,
		ET_MATERIAL_GLB,
	};
	EExtension						mExtension;

	enum ELinkStatus
	{
		LS_ON,
		LS_BROKEN,
	};
	ELinkStatus						mLinkStatus;

private:
	static list_t					sMaterialList;
	static LLLocalGLTFMaterialTimer	sTimer;
	static S32						sMaterialsListVersion;
};

///////////////////////////////////////////////////////////////////////////////
// HBFloaterLocalMaterial class
//
// Implements the user interface to LLLocalGLTFMaterial as a floater allowing
// to select/add/remove/upload local materials.
///////////////////////////////////////////////////////////////////////////////

class HBFloaterLocalMaterial final : public LLFloater
{
protected:
	LOG_CLASS(HBFloaterLocalMaterial);

public:
	typedef void(*callback_t)(const LLUUID&, void*);

	// Call this to create a local glTF material floater. The callback function
	// will be passed the selected material UUID, if any, or a null UUID on
	// Cancel action.
	// The material picker floater will automatically become dependent on the
	// parent floater of 'ownerp', if there is one (and if owner is not NULL,
	// of course), else it will stay independent.
	HBFloaterLocalMaterial(LLView* ownerp, callback_t cb, void* userdata);

	~HBFloaterLocalMaterial() override;

private:
	bool postBuild() override;
	void draw() override;

	static void onBtnAdd(void*);
	static void onBtnRemove(void* userdata);
	static void onBtnUpload(void* userdata);
	static void onBtnSelect(void* userdata);
	static void onBtnCancel(void* userdata);
	static void onMaterialListCommit(LLUICtrl*, void* userdata);

private:
	LLScrollListCtrl*	mMaterialsList;
	LLCheckBoxCtrl*		mApplyImmediatelyCheck;
	LLButton*			mSelectButton;
	LLButton*			mRemoveButton;
	LLButton*			mUploadButton;

	void				(*mCallback)(const LLUUID& world_id, void* userdata);
	void*				mCallbackUserdata;

	S32					mLastListVersion;
};
