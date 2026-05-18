/**
 * @file llvoclouds.cpp
 * @brief Implementation of LLVOClouds class which is a derivation fo LLViewerObject
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

#include "llvoclouds.h"

#include "imageids.h"
#include "llfasttimer.h"
#include "llprimitive.h"

#include "llagent.h"		// to get camera position
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "llenvironment.h"
#include "llface.h"
#include "llpipeline.h"
#include "llsky.h"
#include "llspatialpartition.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llvosky.h"
#include "llworld.h"

LLUUID gCloudTextureID = IMG_CLOUD_POOF;

///////////////////////////////////////////////////////////////////////////////
// LLVOClouds class
///////////////////////////////////////////////////////////////////////////////

LLVOClouds::LLVOClouds(const LLUUID& id, LLViewerRegion* regionp)
:	LLAlphaObject(id, LL_VO_CLOUDS, regionp)
{
	mCloudGroupp = NULL;
	mCanSelect = false;
	setNumTEs(1);

	LLViewerTexture* image;
	if (gCloudTextureID != IMG_CLOUD_POOF ||
		LLViewerFetchedTexture::sDefaultCloudsImagep.isNull())
	{
		image =
			LLViewerTextureManager::getFetchedTexture(gCloudTextureID,
													  FTT_DEFAULT, true,
													  LLGLTexture::BOOST_CLOUDS);
#if !LL_IMPLICIT_SETNODELETE
		image->setNoDelete();
#endif
	}
	else
	{
		image = LLViewerFetchedTexture::sDefaultCloudsImagep.get();
	}
	setTEImage(0, image);
}

void LLVOClouds::idleUpdate(F64 time)
{
	if (mDrawable && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS))
	{
		// Set rebuild flag (so that the renderer will rebuild the primitive)
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME);

		// Compute and cache clouds color for this frame.
		const LLSettingsSky::ptr_t& skyp = gEnvironment.getCurrentSky();
		if (!skyp) return;	// Paranoia
#if 1	// With PBR, LL published totally hacked EE midday settings (and they
		// changed them at least half a dozen of times already, as I am writing
		// this) to try and fix the ugly blue hue on shinies and other lighting
		// issues. One of the hack they use is to set the ambient light to
		// black, meaning both getTotalAmbient() and getLightDiffuse() return a
		// black color, leading to black clouds !... So, here is my workaround
		// (not as good as the original formula for clouds color, thus why I
		// use a threshold for the workaround). HB
		static LLCachedControl<F32> min_ambient(gSavedSettings,
												"CloudsMinAmbientThreshold");
		// Since the diffuse color is less bright than the total ambient color,
		// let's also adjust it to be brighter to try and match what we get
		// with non ruined/hacked EE settings... HB
		static LLCachedControl<F32> adjustment(gSavedSettings,
											   "CloudsDiffuseLightAdjustment");
		LLColor3 total_ambient(skyp->getTotalAmbient());
		if (total_ambient.brightness() < (F32)min_ambient)
		{
			bool sun_up = gPipeline.mIsSunUp;
			total_ambient = LLColor3(sun_up ? gPipeline.mSunLightColor
										    : gPipeline.mMoonLightColor);
			const LLColor3& diffuse = 
				sun_up ? skyp->getSunDiffuse() : skyp->getMoonDiffuse();
			mCloudsColor = diffuse + total_ambient;
			mCloudsColor.adjust(adjustment);
		}
		else	// If the EE settings are not ruined, keep the old formula. HB
#endif
		{
			mCloudsColor = skyp->getLightDiffuse() +
						   LLColor3(skyp->getTotalAmbient());
		}
	}
}

void LLVOClouds::setPixelAreaAndAngle()
{
	mAppAngle = 50;
	mPixelArea = 1500 * 100;
}

void LLVOClouds::updateTextures()
{
	getTEImage(0)->addTextureStats(mPixelArea);
}

LLDrawable* LLVOClouds::createDrawable()
{
	gPipeline.allocDrawable(this);
	mDrawable->setLit(false);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_CLOUDS);
	return mDrawable;
}

bool LLVOClouds::updateGeometry(LLDrawable* drawable)
{
	LL_FAST_TIMER(FTM_UPDATE_CLOUDS);

	S32 num_parts = mCloudGroupp->getNumPuffs();
	LLSpatialGroup* group = drawable->getSpatialGroup();
	if (!group && num_parts)
	{
		drawable->movePartition();
		group = drawable->getSpatialGroup();
	}

	if (group && group->isVisible())
	{
		dirtySpatialGroup();
	}

	if (!num_parts)
	{
		if (group && drawable->getNumFaces())
		{
			group->setState(LLSpatialGroup::GEOM_DIRTY);
		}
		drawable->setNumFaces(0, NULL, getTEImage(0));
		return true;
	}

 	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS))
	{
		return true;
	}

	if (num_parts > drawable->getNumFaces())
	{
		drawable->setNumFacesFast(num_parts + num_parts / 4, NULL,
								  getTEImage(0));
	}

	mDepth = (getPositionAgent() - gViewerCamera.getOrigin()) *
			 gViewerCamera.getAtAxis();

	LLFace* facep;
	S32 face_indx = 0;
	for ( ;	face_indx < num_parts; face_indx++)
	{
		facep = drawable->getFace(face_indx);
		if (!facep)
		{
			continue;
		}

		facep->setTEOffset(face_indx);
		facep->setSize(4, 6);

		facep->setViewerObject(this);

		const LLCloudPuff& puff = mCloudGroupp->getPuff(face_indx);
		facep->mCenterLocal =
			gAgent.getPosAgentFromGlobal(puff.getPositionGlobal());
		facep->setFaceColor(LLColor4(mCloudsColor, puff.getAlpha()));

		facep->setDiffuseMap(getTEImage(0));
	}
	for (S32 count = drawable->getNumFaces(); face_indx < count; ++face_indx)
	{
		facep = drawable->getFace(face_indx);
		if (facep)
		{
			facep->setTEOffset(face_indx);
			facep->setSize(0, 0);
		}
	}

	drawable->movePartition();

	return true;
}

F32 LLVOClouds::getPartSize(S32 idx)
{
	return (CLOUD_PUFF_HEIGHT + CLOUD_PUFF_WIDTH) * 0.5f;
}

void LLVOClouds::getGeometry(S32 idx,
							 LLStrider<LLVector4a>& verticesp,
							 LLStrider<LLVector3>& normalsp,
							 LLStrider<LLVector2>& texcoordsp,
							 LLStrider<LLColor4U>& colorsp,
							 LLStrider<LLColor4U>& emissivep,
							 LLStrider<U16>& indicesp)
{

	if (idx >= mCloudGroupp->getNumPuffs())
	{
		return;
	}

	LLDrawable* drawable = mDrawable;
	LLFace* facep = drawable->getFace(idx);

	if (!facep || !facep->hasGeometry())
	{
		return;
	}

	const LLCloudPuff& puff = mCloudGroupp->getPuff(idx);

	LLColor4 float_color(mCloudsColor, puff.getAlpha());
	facep->setFaceColor(float_color);

	LLVector4a part_pos_agent;
	part_pos_agent.load3(facep->mCenterLocal.mV);
	LLVector4a at;
	at.load3(gViewerCamera.getAtAxis().mV);
	LLVector4a up(0.f, 0.f, 1.f);
	LLVector4a right;

	right.setCross3(at, up);
	right.normalize3fast();
	up.setCross3(right, at);
	up.normalize3fast();
	right.mul(0.5f * CLOUD_PUFF_WIDTH);
	up.mul(0.5f * CLOUD_PUFF_HEIGHT);

	LLVector3 normal(0.f, 0.f, -1.f);

	// *HACK: the verticesp->mV[3] = 0.f here are to set the texture index to 0
	// (particles do not use texture batching, maybe they should) this works
	// because there is actually a 4th float stored after the vertex position
	// which is used as a texture index.

	LLVector4a ppapu;
	LLVector4a ppamu;

	ppapu.setAdd(part_pos_agent, up);
	ppamu.setSub(part_pos_agent, up);

	verticesp->setSub(ppapu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;
	verticesp->setSub(ppamu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;
	verticesp->setAdd(ppapu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;
	verticesp->setAdd(ppamu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;

	LLColor4U color;
	color.set(float_color);
	*colorsp++ = color;
	*colorsp++ = color;
	*colorsp++ = color;
	*colorsp++ = color;

	*normalsp++ = normal;
	*normalsp++ = normal;
	*normalsp++ = normal;
	*normalsp++ = normal;
}

U32 LLVOClouds::getPartitionType() const
{
	return LLViewerRegion::PARTITION_CLOUD;
}

// virtual
void LLVOClouds::updateDrawable(bool force_damped)
{
	// Force an immediate rebuild on any update
	if (mDrawable.notNull())
	{
		mDrawable->updateXform(true);
		gPipeline.markRebuild(mDrawable);
	}
	clearChanged(SHIFTED);
}

///////////////////////////////////////////////////////////////////////////////
// LLCloudPartition class (declared in llspatialpartition.h)
///////////////////////////////////////////////////////////////////////////////

LLCloudPartition::LLCloudPartition(LLViewerRegion* regionp)
:	LLParticlePartition(regionp)
{
	mDrawableType = LLPipeline::RENDER_TYPE_CLOUDS;
	mPartitionType = LLViewerRegion::PARTITION_CLOUD;
}
