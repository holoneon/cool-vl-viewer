/**
 * @file lltoolselect.h
 * @brief LLToolSelect class header file
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

#pragma once

#include "llsafehandle.h"
#include "lluuid.h"
#include "llvector3.h"

#include "lltool.h"
#include "llviewerwindow.h" // for LLPickInfo

class LLObjectSelection;

class LLToolSelect : public LLTool
{
public:
	LLToolSelect(LLToolComposite* composite);

	virtual bool handleMouseDown(S32 x, S32 y, MASK mask);
	virtual bool handleMouseUp(S32 x, S32 y, MASK mask);

	virtual void stopEditing();

	static LLSafeHandle<LLObjectSelection> handleObjectSelection(const LLPickInfo& pick,
																 bool ignore_group,
																 bool temp_select,
																 bool select_root = false);

	virtual void onMouseCaptureLost();
	virtual void handleDeselect();

protected:
	LLUUID		mSelectObjectID;
	LLPickInfo	mPick;
	bool		mIgnoreGroup;
};
