/**
 * @file llviewerfoldertype.h
 * @brief Declaration of LLViewerFolderType.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include <string>

#include "llfoldertype.h"

#include "llui.h"	// For LLUIImagePtr

// This class is similar to LLFolderType, but contains (static) methods only
// used by the viewer.

class LLViewerFolderType final : public LLFolderType
{
public:
	// Name used by the UI
	static const std::string& lookupXUIName(EType type);
	static LLFolderType::EType lookupTypeFromXUIName(const std::string& name);

	// Folder icon name:
	static const std::string& lookupIconName(EType type);
	// Folder icon:
	static const LLUIImagePtr lookupIcon(EType type);
	// Folder does not require UI update when changes have occured:
	static bool lookupIsQuietType(EType type);
	// Folder is not displayed if empty:
	static bool lookupIsHiddenIfEmpty(EType type);
	// Default name when creating new category:
	static const std::string& lookupNewCategoryName(EType type);
#if 0	// Not used
	// Default type when creating new category
	static LLFolderType::EType lookupTypeFromNewCatName(const std::string& n);
#endif

protected:
	LL_INLINE LLViewerFolderType()	{}
	~LLViewerFolderType() override = default;
};
