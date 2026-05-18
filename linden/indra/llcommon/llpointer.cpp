/** 
 * @file llpointer.h
 * @brief A reference-counted pointer for objects derived from LLRefCount
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "linden_common.h"

#include "llpointer.h"

LL_NO_INLINE void warnUnreferenceDidAssignment()
{
	llwarns << "LLPointer::unref() did assignment to non-NULL because of destructor"
			<< llendl;
}
