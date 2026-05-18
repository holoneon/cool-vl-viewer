/** 
 * @file hbprefscool.h
 * @brief  Cool VL Viewer preferences panel
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * 
 * Copyright (c) 2008, Henri Beauchamp.
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

class LLPanel;
class HBPrefsCoolImpl;

class HBPrefsCool
{
public:
	HBPrefsCool();
	~HBPrefsCool();

	void apply();
	void cancel();

	LLPanel* getPanel();

protected:
	HBPrefsCoolImpl& impl;
};
