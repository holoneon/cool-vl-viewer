/**
 * @file hbobjectbackup.h
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 *
 * Original implementation Copyright (c) 2008 Merkat viewer authors.
 * Debugged/rewritten/augmented code Copyright (c) 2008-2023 Henri Beauchamp.
 *
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

#include "hbfastmap.h"
#include "hbfileselector.h"

#include "llselectmgr.h"
#include "llviewerinventory.h"

class LLSelectNode;
class LLViewerObject;

enum export_states {
	EXPORT_INIT,
	EXPORT_CHECK_PERMS,
	EXPORT_FETCH_PHYSICS,
	EXPORT_STRUCTURE,
	EXPORT_TEXTURES,
	EXPORT_LLSD,
	EXPORT_DONE,
	EXPORT_FAILED,
	EXPORT_ABORTED
};

class HBObjectBackup final : public LLFloater,
							 public LLFloaterSingleton<HBObjectBackup>
{
	friend class LLUISingleton<HBObjectBackup, VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBObjectBackup);

public:
	~HBObjectBackup() override;

	///////////////////////////////////////////////////////////////////////////
	// LLFloater interface

	void onClose(bool app_quitting) override;

	///////////////////////////////////////////////////////////////////////////
	// Public interface for invoking the object backup feature

	// Import entry point
	static void importObject(bool upload = false);

	// Export entry point
	static void exportObject();

	///////////////////////////////////////////////////////////////////////////
	// Public methods used in callbacks, workers and responders

	// Update map from texture worker
	void updateMap(const LLUUID& uploaded_asset);

	// Export idle callback
	static void exportWorker(void* userdata);

	// Prim updated callback, called in llviewerobjectlist.cpp
	static void primUpdate(LLViewerObject* object);

	// New prim callback, called in llviewerobjectlist.cpp
	static void newPrim(LLViewerObject* object);

	// Folder public getter, used by the texture cache responder
	std::string getFolder()						{ return mFolder; }

	///////////////////////////////////////////////////////////////////////////
	// Public static methods, re-used in llviewerobjectexport.cpp

	static void setDefaultTextures();

	// Permissions checking (also used for export as DAE/OBJ).
	static bool validatePerms(const LLPermissions* item_permissions,
							  bool strict = false);	// true for meshes
	static bool validateAssetPerms(const LLUUID& asset_id,
								   bool strict = false);
	static bool validateNode(LLSelectNode* node);

private:
	// Open only via the importObject() and exportObject() methods defined
	// above.
	HBObjectBackup(const LLSD&);

	void showFloater(bool exporting);

	static bool confirmCloseCallback(const LLSD& notification,
									 const LLSD& response);

	// Update the floater with status numbers
	void updateImportNumbers();
	void updateExportNumbers();

	// Permissions stuff.
	LLUUID validateTextureID(const LLUUID& asset_id);

	// Convert a selection list of objects to LLSD
	LLSD primsToLLSD(LLViewerObject::child_list_t child_list,
					 bool is_attachment);

	// Move to next texture upload
	void uploadNextAsset();
	static void uploadNextAssetCallback(const LLSD& result, void*);

	// Start the export process
	void doExportObject(std::string filename);
	static void exportCallback(HBFileSelector::ESaveFilter type,
							   std::string& filename, void*);

	// Returns true if we need to register this texture id for upload.
	bool uploadNeeded(const LLUUID& id);

	// Start the import process
	void doImportObject(std::string filename);
	static void importCallback(HBFileSelector::ELoadFilter type,
							   std::string& filename, void* data);
	void importFirstObject();

	// Move to the next import group
	void importNextObject();

	// Export the next texture in list
	void exportNextTexture();

	// Apply LLSD to object
	void xmlToPrim(LLSD prim_llsd, LLViewerObject* object);

	// Rez a prim at a given position
	void rezAgentOffset(LLVector3 offset);

	// Get an offset from the agent based on rotation and current pos
	LLVector3 offsetAgent(LLVector3 offset);

public:
	// Public static constants, used in callbacks, workers and responders
	static constexpr U32 TEXTURE_OK = 0x00;
	static constexpr U32 TEXTURE_BAD_PERM = 0x01;
	static constexpr U32 TEXTURE_MISSING = 0x02;
	static constexpr U32 TEXTURE_BAD_ENCODING = 0x04;
	static constexpr U32 TEXTURE_IS_NULL = 0x08;
	static constexpr U32 TEXTURE_SAVED_FAILED = 0x10;

	// Export state machine
	enum export_states			mExportState;

	// Export result flags for textures.
	U32							mNonExportedTextures;

	// Set when the region supports the extra physics flags
	bool						mGotExtraPhysics;

	// Are we ready to check for next texture ?
	bool						mCheckNextTexture;

private:
	// Are we active flag
	bool						mRunning;

	// True if we need to rebase the assets
	bool						mRetexture;

	// Counts of import and export objects and prims
	U32							mObjects;
	U32							mCurObject;
	U32							mPrims;
	U32							mCurPrim;

	// Number of rezzed prims
	U32							mRezCount;

	// Root pos and rotation and central root pos for link set
	LLVector3					mRootPos;
	LLQuaternion				mRootRot;
	LLVector3					mRootRootPos;
	LLVector3					mGroupOffset;

	// Agent inital pos and rot when starting import
	LLVector3					mAgentPos;
	LLQuaternion				mAgentRot;

	LLUUID						mCurrentAsset;
	LLUUID						mExpectingUpdate;

	// Safe handle to selected objects, held throughout export.
	LLObjectSelectionHandle		mSelectedObjects;

	// Working llsd iterators for objects and linksets
	LLSD::map_const_iterator	mPrimImportIter;
	LLSD::array_const_iterator	mGroupPrimImportIter;

	// File and folder name control
	std::string					mFileName;
	std::string					mFolder;

	// Export texture list
	uuid_list_t					mTexturesList;
	uuid_list_t					mBadPermsTexturesList;

	// Import object tracking
	std::vector<LLViewerObject*> mToSelect;
	std::vector<LLViewerObject*>::iterator mProcessIter;

	// Working LLSD holders
	LLSD						mLLSD;
	LLSD						mThisGroup;

public:
	// Rebase asset map. Static to keep the memory of the assets we already
	// uploaded during the viewer session (avoids superfluous re-uploads).
	typedef fast_hmap<LLUUID, LLUUID> rebase_map_t;
	static rebase_map_t			sAssetMap;
};

extern LLUUID gTextureTransparent;
