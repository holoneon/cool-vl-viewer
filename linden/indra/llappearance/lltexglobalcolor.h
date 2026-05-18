/** 
 * @file lltexglobalcolor.h
 * @brief This is global texture color info used by llavatarappearance.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llpreprocessor.h"
#include "lltexlayer.h"
#include "lltexlayerparams.h"

class LLAvatarAppearance;
class LLTexGlobalColorInfo;
class LLWearable;

class LLTexGlobalColor
{
public:
	LLTexGlobalColor(LLAvatarAppearance* appearance);
#if 0
	~LLTexGlobalColor();
#endif

	LL_INLINE LLTexGlobalColorInfo* getInfo() const				{ return mInfo; }
	// This sets mInfo and calls initialization functions
	bool setInfo(LLTexGlobalColorInfo* info);
	
	LL_INLINE LLAvatarAppearance* getAvatarAppearance() const	{ return mAvatarAppearance; }
	LLColor4 getColor() const;
	const std::string& getName() const;

private:
	param_color_list_t		mParamGlobalColorList;
	LLAvatarAppearance*		mAvatarAppearance;  // just backlink, don't LLPointer 
	LLTexGlobalColorInfo*	mInfo;
};

// Used by llavatarappearance to determine skin/eye/hair color.
class LLTexGlobalColorInfo
{
	friend class LLTexGlobalColor;

protected:
	LOG_CLASS(LLTexGlobalColorInfo);

public:
	LLTexGlobalColorInfo() = default;
	~LLTexGlobalColorInfo();

	bool parseXml(LLXmlTreeNode* node);

private:
	param_color_info_list_t	mParamColorInfoList;
	std::string				mName;
};

class alignas(16) LLTexParamGlobalColor : public LLTexLayerParamColor
{
public:
	LLTexParamGlobalColor(LLTexGlobalColor* tex_color);

	LLViewerVisualParam* cloneParam(LLWearable* wearable) const override;

protected:
	LLTexParamGlobalColor(const LLTexParamGlobalColor& other);

	void onGlobalColorChanged(bool upload_bake) override;

private:
	LLTexGlobalColor* mTexGlobalColor;
};
