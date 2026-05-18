/**
 * @file llinventoryactions.h
 * @brief Definitions of the actions associated with menu items.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "lluuid.h"

class LLFloaterInventory;
class LLInventoryPanel;
class LLInventoryObject;
class LLPanelInventory;
class LLViewerInventoryItem;

void init_object_inventory_panel_actions(LLPanelInventory* panel);
void init_inventory_actions(LLFloaterInventory* floater);
void init_inventory_panel_actions(LLInventoryPanel* panel);

// These functions can open items without the inventory being visible.
// When not LLUUID::null, object_id is the "task inventory" object Id, i.e. the
// in-world object containing the item to open.
void open_notecard(LLViewerInventoryItem* itemp, const std::string& title,
				   bool show_keep_discard = false,
				   const LLUUID& object_id = LLUUID::null,
				   bool take_focus = true);
void open_landmark(LLViewerInventoryItem* itemp, const std::string& title,
				   bool show_keep_discard = false, bool take_focus = true);
void open_texture(const LLUUID& item_id, const std::string& title,
				  bool show_keep_discard = false,
				  const LLUUID& object_id = LLUUID::null,
				  bool take_focus = true);
void open_callingcard(LLViewerInventoryItem* itemp);
void open_sound(const LLUUID& item_id, const std::string& title,
				const LLUUID& object_id = LLUUID::null,
				bool take_focus = true);
void open_animation(const LLUUID& item_id, const std::string& title,
					S32 activate = 0, const LLUUID& object_id = LLUUID::null,
					bool take_focus = true);
void open_script(const LLUUID& item_id, const std::string& title,
				 bool take_focus = true);
void open_gesture(const LLUUID& item_id, const std::string& title,
				  const LLUUID& object_id = LLUUID::null,
				  bool take_focus = true);
void open_material(const LLUUID& item_id, const std::string& title,
				   const LLUUID& object_id = LLUUID::null,
				   bool take_focus = true);
