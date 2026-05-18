/**
 * @file lltoolplacer.h
 * @brief Tool for placing new objects into the world
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

#include "llpanel.h"
#include "llprimitive.h"

#include "lltool.h"

class LLButton;
class LLViewerRegion;

class LLToolPlacer : public LLTool
{
protected:
	LOG_CLASS(LLToolPlacer);

public:
	LLToolPlacer();

	virtual bool placeObject(S32 x, S32 y, MASK mask);
	virtual bool handleHover(S32 x, S32 y, MASK mask);
	virtual void handleSelect(); 	// do stuff when your tool is selected
	virtual void handleDeselect();	// clean up when your tool is deselected

	static void	setObjectType(LLPCode type)		{ sObjectType = type; }
	static LLPCode getObjectType()				{ return sObjectType; }

protected:
	static LLPCode	sObjectType;

private:
	typedef std::map<std::string, U8, std::less<> > species_list_t;
	U8 getTreeGrassSpecies(species_list_t& table, const char* setting_name,
						   U8 max);

	bool addObject(LLPCode pcode, S32 x, S32 y, U8 use_physics);

	bool raycastForNewObjPos(S32 x, S32 y, LLViewerObject** hit_obj,
							 S32* hit_face, bool* b_hit_land,
							 LLVector3* ray_start_region,
							 LLVector3* ray_end_region,
							 LLViewerRegion** region);

	bool addDuplicate(S32 x, S32 y);
};

////////////////////////////////////////////////////
// LLToolPlacerPanel

constexpr S32 TOOL_PLACER_NUM_BUTTONS = 14;

class LLToolPlacerPanel : public LLPanel
{
public:
	LLToolPlacerPanel(const std::string& name, const LLRect& rect);

	static void	setObjectType(void* data);

	static LLPCode sCube;
	static LLPCode sPrism;
	static LLPCode sPyramid;
	static LLPCode sTetrahedron;
	static LLPCode sCylinder;
	static LLPCode sCylinderHemi;
	static LLPCode sCone;
	static LLPCode sConeHemi;
	static LLPCode sTorus;
	static LLPCode sSquareTorus;
	static LLPCode sTriangleTorus;
	static LLPCode sSphere;
	static LLPCode sSphereHemi;
	static LLPCode sTree;
	static LLPCode sGrass;

private:
	void addButton(const std::string& up_state, const std::string& down_state,
				   LLPCode* pcode);

private:
	static S32			sButtonsAdded;
	static LLButton*	sButtons[TOOL_PLACER_NUM_BUTTONS];
};
