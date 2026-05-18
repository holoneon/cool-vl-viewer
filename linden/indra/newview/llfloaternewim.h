/** 
 * @file llfloaternewim.h
 * @brief Panel allowing the user to create a new IM session.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#pragma once

#include "llfloater.h"

class LLNameListCtrl;

class LLFloaterNewIM final : public LLFloater
{
public:
	LLFloaterNewIM();

	bool postBuild() override;
	bool handleKeyHere(KEY key, MASK mask) override;
	bool canClose() override;
	void close(bool app_quitting) override;

	void refreshLists();

private:
	void addAgent(const LLUUID& id, bool online);
	void addGroup(const LLUUID& id, const std::string& name);

	static void	onSelectGroup(LLUICtrl*, void* userdata);
	static void	onSelectAgent(LLUICtrl*, void* userdata);
	static void onSearchEdit(const std::string& search_string, void* userdata);
	static void	onStart(void* userdata);
	static void onClickClose(void* userdata);

private:
	LLNameListCtrl*	mAgentList;
	LLNameListCtrl*	mGroupList;
	std::string		mFilterString;
};
