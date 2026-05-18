/**
 * @file hbfloateruploadasset.h
 * @brief HBFloaterUploadAsset class definition
 *        This is a full rewrite of LL's LLFloaterNameDesc class.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 *
 * Copyright (c) 2023, Henri Beauchamp.
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

#include "llfloater.h"

class LLButton;
class LLLineEditor;

class HBFloaterUploadAsset : public LLFloater
{
protected:
	LOG_CLASS(HBFloaterUploadAsset);

public:
	// Note inventory_type is to pick in the LLInventoryType::Etype enum;
	// passed here as a S32 (since this is also what it is), to avoid including
	// llinventorytype.h here... It is currently only used to determine the
	// expected cost of the upload.
	HBFloaterUploadAsset(const std::string& filename, S32 inventory_type);

	bool postBuild() override;

protected:
	// Override for specific (cost-variable) assets. Called in postBuild(), so
	// the overriding method must be capable to return the asset cost at this
	// point. HB
	virtual S32 getExpectedUploadCost() const;

	// This method uploads the file as an inventory asset, which will be
	// charged for mCost. Override if needed, like for image uploads to deal
	// with temporary (free) assets in OpenSim (mTempAsset is set true if
	// needed in the override), and with inventory thumbnails since they are
	// not inventory assets (the upload is then handed over to the thumbnail
	// floater). HB
	virtual void uploadAsset();

private:
	static void	onBtnOK(void*);
	static void	onBtnCancel(void*);

protected:
	LLButton*				mUploadButton;
	LLLineEditor*			mNameEditor;
	LLLineEditor*			mDescEditor;
	std::string				mFilenameAndPath;
	std::string				mFilename;
	S32						mCost;
	S32						mInventoryType;
	bool					mTempAsset;
};

// HBFloaterUploadSound derived class, in which only the constructor differs
// from the base class (it just passes the adequate inventory type and uses the
// floater XML definition for sounds upload).

class HBFloaterUploadSound final : HBFloaterUploadAsset
{
protected:
	LOG_CLASS(HBFloaterUploadSound);

public:
	HBFloaterUploadSound(const std::string& filename);
};
