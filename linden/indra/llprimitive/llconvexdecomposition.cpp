/**
 * @file   llconvexdecomposition.cpp
 * @author falcon@lindenlab.com
 * @brief  Inner implementation of LLConvexDecomposition interface
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 *
 * Copyright (c) 2011, Linden Research, Inc.
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

#include "ndconvexdecomphacd.h"
#include "llconvexdecompvhacd.h"

static LLConvexDecomposition* sHACDp = NULL;
static LLConvexDecomposition* sVHACDp = NULL;
// Static member variables
LLConvexDecomposition* LLConvexDecomposition::sConvexDecompositorp = NULL;
bool LLConvexDecomposition::sUseVHACD = false;

//static
void LLConvexDecomposition::setUseVHACD(bool b)
{
	sUseVHACD = b;
	sConvexDecompositorp = b ? sVHACDp : sHACDp;
	llinfos << "Convex decomposition done with " << (b ? "VHACD" : "HACD")
			<< llendl;
}

//static
LLConvexDecomposition* LLConvexDecomposition::getInstance()
{
	return sConvexDecompositorp;
}

//static
LLConvexDecomposition* LLConvexDecomposition::getHACDInstance()
{
	return sHACDp;
}

//static
LLCDResult LLConvexDecomposition::initSystem()
{
	if (!sConvexDecompositorp)
	{
		sHACDp = new NDConvexDecompHACD;
		sVHACDp = new LLConvexDecompVHACD;
		sConvexDecompositorp = sUseVHACD ? sVHACDp : sHACDp;
		llinfos << "Convex decomposition system initialized" << llendl;
	}
	return LLCD_OK;
}

//static
LLCDResult LLConvexDecomposition::quitSystem()
{
	if (sConvexDecompositorp)
	{
		delete sHACDp;
		delete sVHACDp;
		sHACDp = sVHACDp = sConvexDecompositorp = NULL;
		llinfos << "Convex decomposition system shut down." << llendl;
	}
	return LLCD_OK;
}
