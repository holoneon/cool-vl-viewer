/**
 * @file hbfileselector.h
 * @brief The HBFileSelector class declaration
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 *
 * Copyright (c) 2014-2025, Henri Beauchamp
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

#include <deque>
#include <set>
#include <vector>

#include "llerror.h"
#include "hbfastmap.h"
#include "llfloater.h"

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLFlyoutButton;
class LLLineEditor;
class LLScrollListCtrl;
class LLTextBox;

class HBFileSelector final : public LLFloater
{
protected:
	LOG_CLASS(HBFileSelector);

public:
	~HBFileSelector() override;

	bool postBuild() override;
	void draw() override;

	enum ELoadFilter
	{
		FFLOAD_ALL		= 1,
		FFLOAD_TEXT		= 2,
		FFLOAD_XML		= 3,
		FFLOAD_XUI		= 4,
		FFLOAD_SCRIPT	= 5,
		FFLOAD_SOUND	= 6,
		FFLOAD_ANIM		= 7,
		FFLOAD_MODEL	= 8,
		FFLOAD_OBJ		= 9,	// Not used (no loading of *.obj files)
		FFLOAD_TERRAIN	= 10,
		FFLOAD_IMAGE	= 11,
		FFLOAD_HDRI		= 12,
		FFLOAD_LUA		= 13,
		FFLOAD_GLTF		= 14,
		FFLOAD_NONE		= 255,
	};

	enum ESaveFilter
	{
		FFSAVE_ALL		= 1,
		FFSAVE_TXT		= 2,
		FFSAVE_XML		= 3,
		FFSAVE_XUI		= 4,
		FFSAVE_LSL		= 5,
		FFSAVE_SLUA		= 6,
		FFSAVE_WAV		= 7,
		FFSAVE_BVH		= 8,
		FFSAVE_DAE		= 9,
		FFSAVE_OBJ		= 10,
		FFSAVE_RAW		= 11,
		FFSAVE_TGA		= 12,
		FFSAVE_PNG		= 13,
		FFSAVE_JPG		= 14,
		FFSAVE_J2C		= 15,
		FFSAVE_BMP		= 16,
		FFSAVE_GLTF		= 17,
		FFSAVE_NONE		= 255,
	};

	typedef void (*HBLoadFileCallback)(ELoadFilter type, std::string& filename,
									   void* userdatap);

	typedef void (*HBLoadFilesCallback)(ELoadFilter type,
										std::deque<std::string>& files,
										void* userdatap);

	typedef void (*HBSaveFileCallback)(ESaveFilter type, std::string& filename,
									   void* userdatap);

	typedef void (*HBDirPickCallback)(std::string& dirname, void* userdatap);

	static void loadFile(ELoadFilter filter, HBLoadFileCallback callback,
						 void* userdatap = NULL);

	static void loadFiles(ELoadFilter filter, HBLoadFilesCallback callback,
						  void* userdatap = NULL);

	static void saveFile(ESaveFilter filter, std::string suggestion,
						 HBSaveFileCallback callback, void* userdatap = NULL);

	static void pickDirectory(std::string suggestion,
							  HBDirPickCallback callback,
							  void* userdatap = NULL);

	static bool isInUse()				{ return sInstance != NULL; }

	static void saveDefaultPaths(const std::string& filename);
	static void loadDefaultPaths(const std::string& filename);

private:
	// IMPORTANT: do not change the order of this enum: we rely on it to
	// restore user-preferred context directories; new contexts are to be
	// inserted before CONTEXT_END.
	enum EContext
	{
		CONTEXT_UNKNOWN		= 0,
		CONTEXT_DEFAULT		= 1,
		CONTEXT_TXT			= 2,
		CONTEXT_XML			= 3,
		CONTEXT_XUI			= 4,
		CONTEXT_LSL			= 5,
		CONTEXT_SOUND		= 6,
		CONTEXT_ANIM		= 7,
		CONTEXT_MODEL		= 8,
		CONTEXT_OBJ			= 9,	// Not used (using CONTEXT_MODEL for *.obj)
		CONTEXT_RAW			= 10,
		CONTEXT_IMAGE		= 11,
		CONTEXT_LUA			= 12,
		CONTEXT_MATERIAL	= 13,
		CONTEXT_END
	};

	HBFileSelector(ELoadFilter filter, HBLoadFileCallback callback,
				   void* userdatap);

	HBFileSelector(ELoadFilter filter, HBLoadFilesCallback callback,
				   void* userdatap);

	HBFileSelector(ESaveFilter filter, const std::string& suggestion,
				   HBSaveFileCallback callback, void* userdatap);

	HBFileSelector(const std::string& suggestion,
				   HBDirPickCallback callback, void* userdatap);

	void init(void* userdatap);
	void refreshPathCombo();
	void setValidExtensions();
	void setPrompt();
	void setPathFromContext();
	bool isCurrentPathAtRoot();
	bool isFileExtensionValid(const std::string& filename);
	void setSelectionData();
	void doCallback();

	static void onButtonCreate(void* userdatap);
	static void onButtonRefresh(void* userdatap);
	static void onButtonCancel(void* userdatap);
	static void onButtonOK(void* userdatap);
	static void onPathComboCommit(LLUICtrl*, void* userdatap);
	static void onButtonDirLevel(LLUICtrl* ctrlp, void* userdatap);
	static void onSelectDirectory(LLUICtrl*, void* userdatap);
	static void onLevelDown(void* userdatap);
	static void onSelectFile(LLUICtrl*, void* userdatap);
	static void onCommitCheckBox(LLUICtrl*, void* userdatap);
	static bool onHandleKeyCallback(KEY key, MASK mask, LLLineEditor* caller,
									void* userdatap);
	static void onKeystrokeCallback(LLLineEditor* caller, void* userdatap);

private:
	void					(*mLoadFileCallback)(ELoadFilter type,
												 std::string& filename,
												 void* userdatap);
	void					(*mLoadFilesCallback)(ELoadFilter type,
												  std::deque<std::string>& files,
												  void* userdatap);
	void					(*mSaveFileCallback)(ESaveFilter type,
												 std::string& filename,
												 void* userdatap);
	void					(*mDirPickCallback)(std::string& dirname,
												void* userdatap);

	void*					mCallbackUserData;

	LLFlyoutButton*			mDirLevelFlyoutBtn;
	LLButton*				mCreateBtn;
	LLButton*				mRefreshBtn;
	LLButton*				mCancelBtn;
	LLButton*				mOKBtn;
	LLCheckBoxCtrl*			mShowHiddenCheck;
	LLCheckBoxCtrl*			mShowAllTypesCheck;
	LLComboBox*				mPathComboBox;
	LLLineEditor*			mInputLine;
	LLScrollListCtrl*		mDirectoriesList;
	LLScrollListCtrl*		mFilesList;
	LLTextBox*				mPromptTextBox;

	ELoadFilter				mLoadFilter;
	ESaveFilter				mSaveFilter;
	EContext				mContext;

	std::string				mCurrentSelection;
	std::string				mCurrentEntry;
	std::deque<std::string>	mFiles;
	std::vector<std::string> mValidExtensions;
	std::string				mFileTypeDescription;
	std::string				mCurrentPath;
	std::string				mOldPath;

	bool					mIsDirty;
	bool					mCallbackDone;
	bool					mMultiple;
	bool					mSavePicker;
	bool					mDirPicker;
	bool					mCreatingDirectory;

	static HBFileSelector*	sInstance;

	typedef fast_hmap<S32, std::string> context_map_t;
	static context_map_t	sContextToPathMap;

	typedef std::set<std::string> used_path_set_t;
	static used_path_set_t	sUsedPaths;

	static std::string		sLastPath;
};
