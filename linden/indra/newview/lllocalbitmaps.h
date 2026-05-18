/**
 * @file lllocalbitmaps.h
 * @author Vaalith Jinn, code cleanup by Henri Beauchamp
 * @brief Local Bitmaps header
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 *
 * Copyright (c) 2011, Linden Research, Inc.
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

#include "boost/signals2.hpp"

#include "llavatarappearancedefines.h"
#include "lleventtimer.h"
#include "hbfileselector.h"
#include "llimage.h"
#include "llpointer.h"
#include "llwearabletype.h"

class LLGLTFMaterial;
class LLGTTexture;
class LLViewerObject;

class LLLocalBitmapTimer final : public LLEventTimer
{
public:
	LLLocalBitmapTimer();

	bool tick() override;

	void startTimer();
	void stopTimer();
	bool isRunning();
};

class LLLocalBitmap
{
protected:
	LOG_CLASS(LLLocalBitmap);

public:
	LLLocalBitmap(std::string filename);
	~LLLocalBitmap();

	LL_INLINE const std::string& getFilename() const	{ return mFilename; }
	LL_INLINE const std::string& getShortName() const	{ return mShortName; }
	LL_INLINE const LLUUID& getTrackingID()	const		{ return mTrackingID; }
	LL_INLINE const LLUUID& getWorldID() const			{ return mWorldID; }
	LL_INLINE bool getValid() const						{ return mValid; }

	enum EUpdateType
	{
		UT_FIRSTUSE,
		UT_REGUPDATE
	};

	bool updateSelf(EUpdateType = UT_REGUPDATE);

	typedef boost::signals2::signal<void(const LLUUID& tracking_id,
										 const LLUUID& old_id,
										 const LLUUID& new_id)> changed_sig_t;
	typedef changed_sig_t::slot_type changed_cb_t;
	boost::signals2::connection setChangedCallback(const changed_cb_t& cb);

	void addGLTFMaterial(LLGLTFMaterial* matp);

	typedef std::list<LLLocalBitmap*> list_t;
	LL_INLINE static const list_t& getBitmapList()		{ return sBitmapList; }

	LL_INLINE static S32 getBitmapListVersion()			{ return sBitmapsListVersion; }

	static void addUnits();
	static void delUnit(const LLUUID& tracking_id);

	static const LLUUID& getWorldID(const LLUUID& tracking_id);
	static const LLUUID& getTrackingID(const LLUUID& world_id);
	static bool isLocal(const LLUUID& world_id);
	static const std::string& getFilename(const LLUUID& tracking_id);

	static void doUpdates();
	static void setNeedsRebake();
	static void doRebake();

	static boost::signals2::connection setOnChangedCallback(const LLUUID& trac_id,
															const changed_cb_t& cb);
	static void associateGLTFMaterial(const LLUUID& tracking_id,
									  LLGLTFMaterial* matp);

	// To be called on viewer shutdown in LLAppViewer::cleanup(). HB
	static void cleanupClass();

	// Only used for llgltf (new implementation, WIP) for now. HB
	static LLLocalBitmap* addUnit(const std::string& filename);

private:
	bool decodeBitmap(LLPointer<LLImageRaw> raw);
	void replaceIDs(const LLUUID& old_id, LLUUID new_id);
	void prepUpdateObjects(const LLUUID& old_id, U32 channel,
						   std::vector<LLViewerObject*>& obj_list);
	void updateUserPrims(const LLUUID& old_id, const LLUUID& new_id,
						 U32 channel);
	void updateUserVolumes(const LLUUID& old_id, const LLUUID& new_id,
						   U32 channel);
	void updateUserLayers(const LLUUID& old_id, const LLUUID& new_id,
						  LLWearableType::EType type);
	void updateGLTFMaterials(const LLUUID& old_id, const LLUUID& new_id);
	LLAvatarAppearanceDefines::ETextureIndex
		getTexIndex(LLWearableType::EType type,
					LLAvatarAppearanceDefines::EBakedTextureIndex index);

	enum ELinkStatus
	{
		LS_ON,
		LS_BROKEN
	};

	static void addUnitsCallback(HBFileSelector::ELoadFilter type,
								 std::deque<std::string>& files, void*);

private:
	S32							mUpdateRetries;
	ELinkStatus					mLinkStatus;
	LLUUID						mTrackingID;
	LLUUID						mWorldID;
	std::string					mFilename;
	std::string					mShortName;
	changed_sig_t				mChangedSignal;
	typedef std::vector<LLPointer<LLGLTFMaterial> > mat_list_t;
	mat_list_t					mGLTFMaterialWithLocalTextures;
	time_t						mLastModified;
	U8							mCodec;
	bool						mValid;

	static LLLocalBitmapTimer	sTimer;
	static list_t				sBitmapList;
	static S32					sBitmapsListVersion;
	static bool					sNeedsRebake;
};
