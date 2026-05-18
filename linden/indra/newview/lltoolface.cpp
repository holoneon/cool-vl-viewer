/**
 * @file lltoolface.cpp
 * @brief A tool to manipulate faces
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

#include "llviewerprecompiledheaders.h"

#include "lltoolface.h"

#include "llfloatertools.h"
//MK
#include "mkrlinterface.h"
//mk
#include "llselectmgr.h"
#include "lltoolview.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"

LLToolFace gToolFace;

LLToolFace::LLToolFace()
:	LLTool("Texture")
{
}

//virtual
bool LLToolFace::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!gSelectMgr.getSelection()->isEmpty())
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		if (gFloaterToolsp)
		{
			gFloaterToolsp->showPanel(LLFloaterTools::PANEL_FACE);
		}
		return true;
	}
	// Nothing selected means the first mouse click was probably bad, so try
	// again.
	return false;
}

//virtual
bool LLToolFace::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gViewerWindowp->pickAsync(x, y, mask, pickCallback);
	return true;
}

//static
void LLToolFace::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj	= pick_info.getObject();
	if (!hit_obj)
	{
		if (pick_info.mKeyMask != MASK_SHIFT)
		{
			gSelectMgr.deselectAll();
		}
		return;
	}

	if (hit_obj->isAvatar())
	{
		// Clicked on an avatar, so do not do anything
		return;
	}

//MK
	if (gRLenabled && !gRLInterface.canTouch(hit_obj) &&
		!hit_obj->isAttachment())
	{
		return;
	}
//mk

	// Clicked on a world object, try to pick the appropriate face

	S32 hit_face = pick_info.mObjectFace;

	if (pick_info.mKeyMask & MASK_SHIFT)
	{
		// If object not selected, need to inform sim
		if ( !hit_obj->isSelected() )
		{
			// Object was not selected so add the object and face
			gSelectMgr.selectObjectOnly(hit_obj, hit_face);
		}
		else if (!gSelectMgr.getSelection()->contains(hit_obj, hit_face))
		{
			// Object is selected, but not this face, so add it.
			gSelectMgr.addAsIndividual(hit_obj, hit_face);
		}
		else
		{
			// Object is selected, as is this face, so remove the face.
			gSelectMgr.remove(hit_obj, hit_face);

			// BUG: If you remove the last face, the simulator won't know
			// about it.
		}
	}
	else
	{
		// Clicked without modifiers, select only this face
		gSelectMgr.deselectAll();
		gSelectMgr.selectObjectOnly(hit_obj, hit_face);
	}
}

//virtual
void LLToolFace::handleSelect()
{
	// From now on, draw faces
	gSelectMgr.setTEMode(true);
}

//virtual
void LLToolFace::handleDeselect()
{
	// Stop drawing faces
	gSelectMgr.setTEMode(false);
}

//virtual
void LLToolFace::render()
{
	// For now, do nothing
}
