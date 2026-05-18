/**
 * @file llviewervisualparam.cpp
 * @brief Implementation of LLViewerVisualParam class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewervisualparam.h"

#include "llxmltree.h"
#include "llwearable.h"

//-----------------------------------------------------------------------------
// LLViewerVisualParamInfo class
//-----------------------------------------------------------------------------

LLViewerVisualParamInfo::LLViewerVisualParamInfo()
:	mWearableType(LLWearableType::WT_INVALID),
	mCrossWearable(false),
	mCamDist(0.5f),
	mCamAngle(0.f),
	mCamElevation(0.f),
	mEditGroupDisplayOrder(0),
	mSimpleMin(0.f),
	mSimpleMax(100.f)
{
}

bool LLViewerVisualParamInfo::parseXml(LLXmlTreeNode* node)
{
	llassert(node->hasName("param"));

	if (!LLVisualParamInfo::parseXml(node))
	{
		return false;
	}

	// VIEWER SPECIFIC PARAMS

	std::string wearable;
	static LLStdStringHandle wearable_string = LLXmlTree::addAttributeString("wearable");
	if(node->getFastAttributeString(wearable_string, wearable))
	{
		mWearableType = LLWearableType::typeNameToType(wearable);
	}

	static LLStdStringHandle edit_group_string = LLXmlTree::addAttributeString("edit_group");
	if (!node->getFastAttributeString(edit_group_string, mEditGroup))
	{
		mEditGroup = "";
	}

	static LLStdStringHandle cross_wearable_string = LLXmlTree::addAttributeString("cross_wearable");
	if (!node->getFastAttributeBool(cross_wearable_string, mCrossWearable))
	{
		mCrossWearable = false;
	}

	// Optional camera offsets from the current joint center. Used for
	// generating "hints" (thumbnails).
	static LLStdStringHandle camera_distance_string = LLXmlTree::addAttributeString("camera_distance");
	node->getFastAttributeF32(camera_distance_string, mCamDist);
	static LLStdStringHandle camera_angle_string = LLXmlTree::addAttributeString("camera_angle");
	node->getFastAttributeF32(camera_angle_string, mCamAngle);	// in degrees
	static LLStdStringHandle camera_elevation_string = LLXmlTree::addAttributeString("camera_elevation");
	node->getFastAttributeF32(camera_elevation_string, mCamElevation);

	mCamAngle += 180;

	static S32 params_loaded = 0;

	// By default, parameters are displayed in the order in which they appear
	// in the xml file. "edit_group_order" overriddes.
	static LLStdStringHandle edit_group_order_string = LLXmlTree::addAttributeString("edit_group_order");
	if(!node->getFastAttributeF32(edit_group_order_string, mEditGroupDisplayOrder))
	{
		mEditGroupDisplayOrder = (F32)params_loaded;
	}

	++params_loaded;

	return true;
}

//-----------------------------------------------------------------------------
// LLViewerVisualParam class
//-----------------------------------------------------------------------------

LLViewerVisualParam::LLViewerVisualParam()
:	LLVisualParam()
{
}

LLViewerVisualParam::LLViewerVisualParam(const LLViewerVisualParam& other)
:	LLVisualParam(other)
{
}

bool LLViewerVisualParam::setInfo(LLViewerVisualParamInfo* info)
{
	llassert(mInfo == NULL);
	if (info->mID < 0)
	{
		return false;
	}
	mInfo = info;
	mID = info->mID;
	setWeight(getDefaultWeight(), false);
	return true;
}
