/**
 * @file llnotecard.h
 * @brief LLNotecard class declaration
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llerror.h"
#include "llpointer.h"
#include "llpreprocessor.h"
#include "llinventory.h"

class LLNotecard final
{
protected:
	LOG_CLASS(LLNotecard);

public:
	// Anonymous enumeration to set max size.
	enum
	{
		MAX_SIZE = 65536
	};

	LLNotecard(S32 max_text = LLNotecard::MAX_SIZE);

	bool importStream(std::istream& str);
	bool exportStream(std::ostream& str);

	LL_INLINE const std::vector<LLPointer<LLInventoryItem> >& getItems() const
	{
		return mItems;
	}

	LL_INLINE const std::string& getText() const	{ return mText; }
	LL_INLINE std::string& getText()				{ return mText; }

	LL_INLINE void setItems(const std::vector<LLPointer<LLInventoryItem> >& items)
	{
		mItems = items;
	}

	LL_INLINE void setText(const std::string& text)	{ mText = text; }

	LL_INLINE S32 getVersion()						{ return mVersion; }
	LL_INLINE S32 getEmbeddedVersion()				{ return mEmbeddedVersion; }

private:
	bool importEmbeddedItemsStream(std::istream& str);
	bool exportEmbeddedItemsStream(std::ostream& str);

private:
	S32											mMaxText;
	S32											mVersion;
	S32											mEmbeddedVersion;
	std::vector<LLPointer<LLInventoryItem> >	mItems;
	std::string									mText;
};
