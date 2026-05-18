/**
 * @file llviewerdisplay.h
 * @brief LLViewerDisplay class header file
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llfontgl.h"
#include "llmatrix4a.h"

void display_startup();
void display_cleanup();

void display(bool rebuild = true, F32 zoom_factor = 1.f, S32 subfield = 0,
			 bool for_snapshot = false);

void display_update_camera();	// Also called from LLPipeline

// Uses whole screen to render hud:
bool setup_hud_matrices();

// Specifies portion of screen (in pixels) to render hud attachments from (for
// picking):
bool setup_hud_matrices(const LLRect& screen_region);

bool get_hud_matrices(LLMatrix4a& proj, LLMatrix4a& model);
bool get_hud_matrices(const LLRect& screen_region,
					  LLMatrix4a& proj, LLMatrix4a& model);

// Utility function for rendering HUD elements
void hud_render_text(const LLWString& wstr, const LLVector3& pos_agent,
					 const LLFontGL* fontp, LLFontVertexBuffer* fontvbp,
					 U8 style, F32 x_offset, F32 y_offset, const LLColor4& c,
					 bool orthographic);

// Also used in llviewerwindow.cpp for snapshots
void render_ui(F32 zoom_factor = 1.f);

// Used by LLViewerWindow::cubeSnapshot() for PBR rendering only.
void display_cube_face();

extern bool gDisplaySwapBuffers;
extern bool gDepthDirty;
extern bool	gTeleportDisplay;
extern LLFrameTimer	gTeleportDisplayTimer;
extern bool gForceRenderLandFence;
extern bool gResizeScreenTexture;
extern bool gResizeShadowTexture;
extern F32  gSavedDrawDistance;
extern bool gUpdateDrawDistance;
extern U32	gLastFPSAverage;
extern bool gShaderProfileFrame;
extern bool gScreenIsDirty;
// IMPORTANT: this MUST always be false while in EE rendering mode. HB
extern bool gCubeSnapshot;
