/** 
 * @file hbpanelgrids.h
 * @author Henri Beauchamp
 * @brief Grid parameters configuration panel
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * 
 * Copyright (c) 2011, Henri Beauchamp.
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

class HBPanelGridsImpl;

class HBPanelGrids final : public LLPanel
{
public:
	HBPanelGrids();
	~HBPanelGrids() override;

	void apply();
	void cancel();

	LLPanel* getPanel();

private:
	HBPanelGridsImpl& impl;
};
