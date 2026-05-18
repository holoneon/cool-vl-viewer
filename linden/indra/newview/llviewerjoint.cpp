/**
 * @file llviewerjoint.cpp
 * @brief Implementation of LLViewerJoint class
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

#include "llviewerjoint.h"

#include "llgl.h"
#include "llrender.h"

#include "llpipeline.h"
#include "llvoavatar.h"

constexpr S32 MIN_PIXEL_AREA_3PASS_HAIR = 64 * 64;

LLViewerJoint::LLViewerJoint()
:	LLAvatarJoint()
{
}

LLViewerJoint::LLViewerJoint(const std::string& name, LLJoint* parentp)
:	LLAvatarJoint(name, parentp)
{
}

U32 LLViewerJoint::render(F32 pixel_area, bool first_pass, bool is_dummy)
{
	U32 triangle_count = 0;

	// Ignore invisible objects
	if (mValid)
	{
		// If object is transparent, defer it, otherwise give the joint
		// subclass a chance to draw itself
		if (is_dummy)
		{
			triangle_count += drawShape(first_pass, true);
		}
		else if (LLPipeline::sShadowRender)
		{
			triangle_count += drawShape(first_pass, is_dummy);
		}
		else if (isTransparent() && !LLPipeline::sReflectionRender)
		{
			// Hair and Skirt
			if (pixel_area > MIN_PIXEL_AREA_3PASS_HAIR)
			{
				// Render all three passes
				LLGLDisable cull(GL_CULL_FACE);
				// First pass renders without writing to the z buffer
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape(first_pass, is_dummy);
				}
				// Second pass writes to z buffer only
				gGL.setColorMask(false, false);
				{
					triangle_count += drawShape(false, is_dummy);
				}
				// Third past respects z buffer and writes color
				gGL.setColorMask(true, false);
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape(false, is_dummy);
				}
			}
			else
			{
				// Render Inside (no Z buffer write)
				glCullFace(GL_FRONT);
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape(first_pass, is_dummy);
				}
				// Render Outside (write to the Z buffer)
				glCullFace(GL_BACK);
				{
					triangle_count += drawShape(false, is_dummy);
				}
			}
		}
		else
		{
			// Set up render state
			triangle_count += drawShape(first_pass);
		}
	}

	// Render children
	for (S32 i = 0, count = mChildren.size(); i < count; ++i)
	{
		LLJoint* jointp = mChildren[i];
		if (!jointp) continue;	// Paranoia

		LLAvatarJoint* avjointp = jointp->asAvatarJoint();
		if (!avjointp) continue;

		F32 joint_lod = avjointp->getLOD();
		if (pixel_area >= joint_lod || sDisableLOD)
		{
			triangle_count += avjointp->render(pixel_area, true, is_dummy);
			if (joint_lod != DEFAULT_AVATAR_JOINT_LOD)
			{
				break;
			}
		}
	}

	stop_glerror();

	return triangle_count;
}
