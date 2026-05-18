/** 
 * @file llviewermenu.h
 * @brief Builds menus out of objects
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

#include <string>

#include "llassetstorage.h"
#include "llfoldertype.h"
#include "llmenugl.h"
#include "llsafehandle.h"
#include "llvector3.h"

class LLView;
class LLParcelSelection;
class LLObjectSelection;
class LLViewerObject;
class LLVOAvatar;

// Called in llviewerwindow.cpp
void pre_init_menus();
void init_menus();
void cleanup_menus();

// Called in llviewercontrol.cpp
void show_debug_menus();
void clear_assets_cache(void*);
void load_hdri_sky(bool force);

// Called from several places. Returns true when successfully executed or
// false when delayed (when in customize avatar mode and when wearables are
// dirty) or refused (camera constrained by RestrainedLove).
bool handle_reset_view();

// Called in llfloaterpathfindingobjects.
void handle_copy(void*);
void handle_take();
void handle_take_copy();
void handle_object_delete();
void handle_object_return();
bool visible_take_object();
bool enable_object_take_copy();
bool enable_object_return();
bool enable_object_delete();

// Called in lltoolpie.cpp
void handle_buy(void*);
bool handle_sit_or_stand();
bool handle_give_money_dialog();
bool handle_object_open();
bool handle_go_to();

// Called from llstartup.cpp
void set_underclothes_menu_options();

// Called from llstartup.cpp and llviewermessage.cpp
void update_upload_costs_in_menus();

// Called from llappearancemgr.cpp
void confirm_replace_attachment(S32 option, void* user_data);

// Called from llvoavatarself.cpp
bool object_attached(void* user_data);
bool object_selected_and_point_valid(void*);
void handle_detach(void*);
void handle_detach_from_avatar(void* user_data);
void detach_label(std::string& label, void* user_data);
// Called from llinventorybridge.cpp and llvoavatarself.cpp
void attach_label(std::string& label, void* user_data);

// Called from llpuppetmotion.cpp
void handle_reset_avatars_animations(void*);

// Called from llstatusbar.cpp and llfloateravatartextures.cpp
void handle_rebake_textures(void*);

// Formerly declared in the now removed llmenucommands.h
// Called from lltoolbar.cpp and hbviewerautomation.cpp
void handle_chat(void*);
void handle_inventory(void*);

#if 0
// Export to XML or Collada
void handle_export_selected(void*);
#endif

//MK
// Called from mkrlinterface.cpp
void handle_toggle_wireframe(void*);
void handle_toggle_flycam();
//mk

// Called from llfloatertools.cpp
void select_face_or_linked_prim(const std::string& action);

// Called from llinventorybridge.cpp
bool handle_object_edit();
bool handle_object_inspect();

// Called from llfloateravatartextures.cpp
// When refresh_all = false, only refresh the backed textures.
void handle_refresh_avatar(LLVOAvatar* avatarp, bool refresh_all);
bool enable_avatar_textures(void*);

// Called from hbviewerautomation.cpp
bool sit_on_ground();
bool sit_on_object(LLViewerObject* object,
				   const LLVector3& offset = LLVector3::zero);
bool stand_up();
bool derender_object(const LLUUID& object_id);

// *HACK: called from llagent.cpp and llstartup.cpp, each time we need to
// ensure that all objects will properly rez after the agent moved into a new
// region or TPed beyond draw distance in the same region.
enum eRegionChangeType : U32
{
	AFTER_LOGIN = 0,
	AFTER_CROSS_BORDER,
	AFTER_FAR_TP,
};
// 'type' is 1 for refresh on login, 2 for refresh on sim border crossing, and
// 4 for refresh after a TP.
// *TODO: find the race condition causing the failed rezzing; probably in the
// code for objects caching or culling... HB
void schedule_objects_visibility_refresh(U32 type);

// Also called (without delay) from in mkrlinterface.cpp and on some settings
// changes from llviewercontrol.cpp. HB
void handle_objects_visibility(void*);

// Also called on probes settings changes in llpipeline.cpp, llpanelvolume.cpp,
// and llviewercontrol.cpp. HB
void handle_refresh_probes(void*);
// Used in the debug menu (see llviewermenu.cpp)
void handle_compress_image(void*);

// Used in llviewerregion.cpp
void update_upload_costs_in_menus();

// Used in llmediactrl.cpp
void show_viewer_paths_floater(void*);

class LLViewerMenuHolderGL final : public LLMenuHolderGL
{
public:
	LLViewerMenuHolderGL();

	bool hideMenus() override;
	
	void setParcelSelection(LLSafeHandle<LLParcelSelection> selection);
	void setObjectSelection(LLSafeHandle<LLObjectSelection> selection);

	const LLRect getMenuRect() const override;

protected:
	LLSafeHandle<LLParcelSelection> mParcelSelection;
	LLSafeHandle<LLObjectSelection> mObjectSelection;
};

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////

extern LLMenuBarGL*				gMenuBarViewp;
extern LLViewerMenuHolderGL*	gMenuHolderp;
extern LLMenuBarGL*				gLoginMenuBarViewp;

// Pie menus
extern LLPieMenu*				gPieSelfp;
extern LLPieMenu*				gPieAvatarp;
extern LLPieMenu*				gPieObjectp;
extern LLPieMenu*				gPieAttachmentp;
extern LLPieMenu*				gPieLandp;
extern LLPieMenu*				gPieParticlep;

// Sub-menu of gPieAvatarp
extern LLPieMenu*				gMutesPieMenup;
// Sub-menu of gPieObjectp
extern LLPieMenu*				gPieObjectMutep;

// Needed to build menus when attachment site list available
extern LLMenuGL*				gDetachSubMenup;
extern LLPieMenu*				gAttachScreenPieMenup;
extern LLPieMenu*				gDetachScreenPieMenup;
extern LLPieMenu*				gAttachPieMenup;
extern LLPieMenu*				gDetachPieMenup;
