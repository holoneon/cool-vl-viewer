/**
 * @file llfloaterobjectweights.cpp
 * @brief LLFloaterObjectWeights class implementation
 *
 * $LicenseInfo:firstyear=2012&license=viewergpl$
 *
 * Copyright (c) 2012, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterobjectweights.h"

#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llselectmgr.h"

constexpr F32 UPDATE_INTERVAL = 1.f;

LLFloaterObjectWeights::LLFloaterObjectWeights(const LLSD&)
:	mParentFloater(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_object_weights.xml");
}

//virtual
bool LLFloaterObjectWeights::postBuild()
{
	mObjectCount = getChild<LLTextBox>("selected_objects_count");
	mObjectImpact = getChild<LLTextBox>("objects_impact");
	mObjectPhysics = getChild<LLTextBox>("objects_physics_cost");
	mPrimCount = getChild<LLTextBox>("selected_prims_count");
	mPrimImpact = getChild<LLTextBox>("prims_impact");
	mPrimPhysics = getChild<LLTextBox>("prims_physics_cost");
	mPrimStreaming = getChild<LLTextBox>("streaming_cost");
	mPrimTriangles = getChild<LLTextBox>("triangle_count");
	mRenderingCost = getChild<LLTextBox>("objects_render_cost");

	refresh();

	return true;
}

//virtual
void LLFloaterObjectWeights::refresh()
{
	LLObjectSelectionHandle sel = gSelectMgr.getSelection();
	bool enabled = !sel->isEmpty();
	mObjectCount->setVisible(enabled);
	mObjectImpact->setVisible(enabled);
	mObjectPhysics->setVisible(enabled);
	mPrimCount->setVisible(enabled);
	mPrimImpact->setVisible(enabled);
	mPrimPhysics->setVisible(enabled);
	mPrimStreaming->setVisible(enabled);
	mPrimTriangles->setVisible(enabled);
	mRenderingCost->setVisible(enabled);
	if (!enabled)
	{
		return;
	}
	mObjectCount->setText(llformat("%d", sel->getRootObjectCount()));
	mObjectImpact->setText(llformat("%f", sel->getSelectedLinksetCost()));
	mObjectPhysics->setText(llformat("%f",
									 sel->getSelectedLinksetPhysicsCost()));
	mPrimCount->setText(llformat("%d", sel->getObjectCount()));
	mPrimImpact->setText(llformat("%f", sel->getSelectedObjectCost()));
	mPrimPhysics->setText(llformat("%f", sel->getSelectedPhysicsCost()));
	S32 t, v;
	mPrimStreaming->setText(llformat("%f",
									 sel->getSelectedObjectStreamingCost(&t,
																	     &v)));
	mPrimTriangles->setText(llformat("%d",
									 sel->getSelectedObjectTriangleCount(&t)));
	mRenderingCost->setText(llformat("%d",
									 sel->getSelectedObjectRenderCost()));
}

//virtual
void LLFloaterObjectWeights::draw()
{
	if (mUpdateTimer.hasExpired())
	{
		refresh();
		mUpdateTimer.setTimerExpirySec(UPDATE_INTERVAL);
	}
	LLFloater::draw();
}

//static
void LLFloaterObjectWeights::show(LLFloater* parentp)
{
	LLFloaterObjectWeights* self = findInstance();
	if (self)
	{
		self->open();
		self->refresh();

		if (self->mParentFloater == parentp)
		{
			// Called by the same parent. We are done !
			return;
		}

		// Reparent to the new floater
		if (self->mParentFloater)
		{
			self->mParentFloater->removeDependentFloater(self);
		}
	}
	else
	{
		self = getInstance();	// creates a new instance
	}

	if (parentp && self)
	{
		self->mParentFloater = parentp;
		parentp->addDependentFloater(self);
	}
}
