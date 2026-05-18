/**
 * @file lltoolpie.cpp
 * @brief LLToolPie class implementation
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

#include "lltoolpie.h"

#include "lleditmenuhandler.h"
#include "llmediaentry.h"
#include "llmenugl.h"
#include "llparcel.h"
#include "lltrans.h"
#include "llwindow.h"				// For gDebugClicks

#include "llagent.h"
#include "llfirstuse.h"
#include "llfloateravatarinfo.h"
#include "llfloaterland.h"
#include "llfloatertools.h"
#include "llhoverview.h"
#include "llhudeffectspiral.h"
#include "llmutelist.h"
//MK
#include "mkrlinterface.h"
//mk
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolselect.h"
#include "hbviewerautomation.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llworld.h"
#include "llweb.h"

extern void handle_buy(void*);

static void handle_click_action_play();
static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp);
static ECursorType cursor_from_parcel_media(U8 click_action);

LLToolPie gToolPie;

LLToolPie::LLToolPie()
:	LLTool("Pie"),
	mPieMouseButtonDown(false),
	mGrabMouseButtonDown(false),
	mClickAction(0)
{
}

bool LLToolPie::handleMouseDown(S32 x, S32 y, MASK mask)
{
	static LLCachedControl<bool> pick_transparent(gSavedSettings,
												  "AllowPickTransparent");
	gViewerWindowp->pickAsync(x, y, mask, leftMouseCallback,
							  // maybe pick transparent (normally no)
							  pick_transparent,
							  // not rigged, not particles, get surface info
							  false, false, true);
	mGrabMouseButtonDown = true;
	return true;
}

//static
void LLToolPie::leftMouseCallback(const LLPickInfo& pick_info)
{
	gToolPie.mPick = pick_info;
	gToolPie.handleLeftClickPick();
}

bool LLToolPie::handleLeftClickPick()
{
	S32 x = mPick.mMousePt.mX;
	S32 y = mPick.mMousePt.mY;
	MASK mask = mPick.mKeyMask;

	if (handleMediaClick(mPick))
	{
		return true;
	}

	if (mPick.mPickType == LLPickInfo::PICK_PARCEL_WALL)
	{
		LLParcel* parcelp = gViewerParcelMgr.getCollisionParcel();
		if (parcelp)
		{
			gViewerParcelMgr.selectCollisionParcel();
			if (parcelp->getParcelFlag(PF_USE_PASS_LIST) &&
				!gViewerParcelMgr.isCollisionBanned())
			{
				// If selling passes, just buy one
				void* deselect_when_done = (void*)true;
				LLPanelLandGeneral::onClickBuyPass(deselect_when_done);
			}
			else
//MK
			if (!gRLenabled || !gRLInterface.mContainsShowloc)
//mk
			{
				// Not selling passes, get info
				LLFloaterLand::showInstance();
			}
		}

		gFocusMgr.setKeyboardFocus(NULL);
		return LLTool::handleMouseDown(x, y, mask);
	}

	if (mPick.mPickType != LLPickInfo::PICK_LAND)
	{
		gViewerParcelMgr.deselectLand();
	}

	// Did not click in any UI object, so must have clicked in the world
	LLViewerObject* parentp = NULL;
	LLViewerObject* objectp = mPick.getObject();
	if (objectp)
	{
		parentp = objectp->getRootEdit();
	}

	// If we have a special action, do it.
	if (useClickAction(mask, objectp, parentp))
	{
//MK
		if (gRLenabled && !gRLInterface.canTouch(objectp, mPick.mIntersection))
		{
			return true;
		}
//mk

		mClickAction = 0;
		if (objectp && objectp->getClickAction())
		{
			mClickAction = objectp->getClickAction();
		}
		else if (parentp && parentp->getClickAction())
		{
			mClickAction = parentp->getClickAction();
		}

		switch (mClickAction)
		{
			case CLICK_ACTION_SIT:
			{
				if (isAgentAvatarValid() && !gAgentAvatarp->mIsSitting &&
					gSavedSettings.getBool("LeftClickToSit"))
				{
					// Agent is not already sitting
					handle_sit_or_stand();
					// Put focus in world when sitting on an object
					gFocusMgr.setKeyboardFocus(NULL);
					return true;
				}
				break;	// else nothing (fall through to touch)
			}

			case CLICK_ACTION_PAY:
			{
				if (((objectp && objectp->flagTakesMoney()) ||
					 (parentp && parentp->flagTakesMoney())) &&
					gSavedSettings.getBool("LeftClickToPay"))
				{
					// Pay event goes to object actually clicked on
					mClickActionObject = objectp;
					mLeftClickSelection =
						LLToolSelect::handleObjectSelection(mPick, false,
															true);
					if (gSelectMgr.selectGetAllValid())
					{
						// Call this right away, since we have all the info we
						// need to continue the action
						selectionPropertiesReceived();
					}
					return true;
				}
				break;	// Else nothing (fall through to touch)
			}

			case CLICK_ACTION_BUY:
			{
				if (gSavedSettings.getBool("LeftClickToPay"))
				{
					mClickActionObject = parentp;
					mLeftClickSelection =
						LLToolSelect::handleObjectSelection(mPick, false,
															true, true);
					if (gSelectMgr.selectGetAllValid())
					{
						// Call this right away, since we have all the info we
						// need to continue the action
						selectionPropertiesReceived();
					}
					return true;
				}
				break;	// Else nothing (fall through to touch)
			}

			case CLICK_ACTION_OPEN:
			{
				if (parentp && parentp->allowOpen() &&
					gSavedSettings.getBool("LeftClickToOpen"))
				{
					mClickActionObject = parentp;
					mLeftClickSelection =
						LLToolSelect::handleObjectSelection(mPick, false,
															true, true);
					if (gSelectMgr.selectGetAllValid())
					{
						// Call this right away, since we have all the info we
						// need to continue the action
						selectionPropertiesReceived();
					}
					return true;
				}
				break;	// Else nothing (fall through to touch)
			}

			case CLICK_ACTION_PLAY:
			{
				if (gSavedSettings.getBool("LeftClickToPlay"))
				{
					handle_click_action_play();
					return true;
				}
			}

			case CLICK_ACTION_OPEN_MEDIA:
			{
				if (gSavedSettings.getBool("LeftClickToPlay"))
				{
					// mClickActionObject = object;
					handle_click_action_open_media(objectp);
					return true;
				}
				break;	// Else nothing (fall through to touch)
			}

			case CLICK_ACTION_ZOOM:
			{
				if (gSavedSettings.getBool("LeftClickToZoom"))
				{
					constexpr F32 PADDING_FACTOR = 2.f;
					LLViewerObject* objp =
						gObjectList.findObject(mPick.mObjectID);
					if (objp)
					{
						gAgent.setFocusOnAvatar(false);
						LLBBox bbox = objp->getBoundingBoxAgent();
						F32 aspect = gViewerCamera.getAspect();
						F32 view = gViewerCamera.getView();
						F32 angle_of_view = llmax(0.1f,
												  aspect > 1.f ? view * aspect
															   : view);
						F32 distance = bbox.getExtentLocal().length() *
									   PADDING_FACTOR / atanf(angle_of_view);
						LLVector3 obj_to_cam = gViewerCamera.getOrigin() -
											   bbox.getCenterAgent();
						obj_to_cam.normalize();
						LLVector3d center_global =
							gAgent.getPosGlobalFromAgent(bbox.getCenterAgent());
						gAgent.setCameraPosAndFocusGlobal(center_global +
														  LLVector3d(obj_to_cam *
																	 distance),
														  center_global,
														  mPick.mObjectID);
					}
					return true;
				}
				break;	// Else nothing (fall through to touch)
			}

			case CLICK_ACTION_DISABLED:
				return true;

			case CLICK_ACTION_TOUCH:
			default:
				break;	// fall through to touch
		}
	}

	// Put focus back "in world"
	gFocusMgr.setKeyboardFocus(NULL);

	// Switch to grab tool if physical or triggerable
	bool touchable = (objectp && objectp->flagHandleTouch()) ||
					 (parentp && parentp->flagHandleTouch());
	if (objectp && !objectp->isAvatar() &&
		(touchable || objectp->flagUsePhysics() ||
		 (parentp && !parentp->isAvatar() && parentp->flagUsePhysics())))
	{
		gGrabTransientTool = this;
		gToolMgr.getCurrentToolset()->selectTool(&gToolGrab);
		return gToolGrab.handleObjectHit(mPick);
	}

	if (!objectp)
	{
		LLHUDIcon* iconp = mPick.mHUDIcon;
		LLViewerObject* src_obj = iconp ? iconp->getSourceObject() : NULL;
		if (src_obj)
		{
			const LLUUID& object_id = src_obj->getID();
			iconp->fireClickedCallback(object_id);
		}
	}

	if (gSavedSettings.getBool("LeftClickSteersAvatar"))
	{
		// Mouse already released
		if (!mGrabMouseButtonDown)
		{
			return true;
		}

		while (objectp && objectp->isAttachment() &&
			   !objectp->flagHandleTouch())
		{
			// Do not pick avatar through hud attachment
			if (objectp->isHUDAttachment())
			{
				break;
			}
			objectp = (LLViewerObject*)objectp->getParent();
		}
		if (objectp && objectp == gAgentAvatarp)
		{
			// We left clicked on avatar, switch to focus mode
			gToolMgr.setTransientTool(&gToolFocus);
			gViewerWindowp->hideCursor();
			gToolFocus.setMouseCapture(true);
			gToolFocus.pickCallback(mPick);
			gAgent.setFocusOnAvatar();
			return true;
		}
	}

	// Could be first left-click on nothing
	LLFirstUse::useLeftClickNoHit();

	return LLTool::handleMouseDown(x, y, mask);
}

bool LLToolPie::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
//MK
	// HACK : if alt-right-clicking and not in mouselook, HUDs are passed
	// through and we risk right-clicking in-world => discard this click
	if (gRLenabled && (mask & MASK_ALT) &&
		gAgent.getCameraMode() != CAMERA_MODE_MOUSELOOK)
	{
		handleMouseDown(x, y, mask);
		return true;
	}
//mk

	static LLCachedControl<bool> pick_rigged_meshes(gSavedSettings,
													"AllowPickRiggedMeshes");
	static LLCachedControl<bool> pick_particles(gSavedSettings,
												"AllowPickParticles");

	mPieMouseButtonDown = true;
	// Note: we do not pick transparent so users cannot "pay" transparent
	// objects.
	gViewerWindowp->pickAsync(x, y, mask, rightMouseCallback,
							  // do not (always) pick transparent
							  false,
							  // maybe pick rigged meshes or particles
							  pick_rigged_meshes, pick_particles,
							  // get surface info
							  true);

	// Do not steal focus from UI
	return false;
}

bool LLToolPie::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return LLViewerMediaFocus::getInstance()->handleScrollWheel(x, y, clicks);
}

//static
void LLToolPie::rightMouseCallback(const LLPickInfo& pick_info)
{
	gToolPie.mPick = pick_info;
	gToolPie.handleRightClickPick();
}

bool LLToolPie::handleRightClickPick()
{
	S32 x = mPick.mMousePt.mX;
	S32 y = mPick.mMousePt.mY;
	MASK mask = mPick.mKeyMask;

	LLViewerMediaFocus::getInstance()->clearFocus();

	if (mPick.mPickType != LLPickInfo::PICK_LAND)
	{
		gViewerParcelMgr.deselectLand();
	}

	// Put focus back "in world"
	gFocusMgr.setKeyboardFocus(NULL);

	// Cannot ignore children here.
	LLToolSelect::handleObjectSelection(mPick, false, true);

	if (!gMenuHolderp)
	{
		// Either at early initialization or late quitting stage
		return true;
	}

	// Did not click in any UI object, so must have clicked in-world
	LLViewerObject* objectp = mPick.getObject();
	if (objectp && objectp->isAttachment() && !objectp->isHUDAttachment() &&
		!objectp->permYouOwner())
	{
		// Find the avatar corresponding to any attachment object we do not own
		while (objectp->isAttachment())
		{
			objectp = (LLViewerObject*)objectp->getParent();
			if (!objectp) return false;	// Orphaned object ?
		}
	}

	if (mask == MASK_SHIFT && gLuaPiep && gLuaPiep->onPieMenu(mPick, objectp))
	{
		gLuaPiep->show(x, y, mPieMouseButtonDown);
		LLTool::handleRightMouseDown(x, y, mask);
		return true;
	}

	// Spawn the pie menu
	if ((!objectp || !objectp->isHUDAttachment()) && // HUDs got priority !
		gPieParticlep && mPick.mPickParticle &&
		mPick.mParticleOwnerID.notNull())
	{
		gPieParticlep->show(x, y, mPieMouseButtonDown);
		return true;
	}
	else if (mPick.mPickType == LLPickInfo::PICK_LAND)
	{
		LLParcelSelectionHandle selection =
			gViewerParcelMgr.selectParcelAt(mPick.mPosGlobal);
		gMenuHolderp->setParcelSelection(selection);
		gPieLandp->show(x, y, mPieMouseButtonDown);

		// VEFFECT: ShowPie
		LLHUDEffectSpiral::sphereAtPosition(mPick.mPosGlobal);
	}
	else if (mPick.mObjectID == gAgentID)
	{
		LLMenuItemGL* itemp =
			gPieSelfp->getChild<LLMenuItemGL>("Self Sit", true, false);
		if (itemp)
		{
			if (isAgentAvatarValid() && gAgentAvatarp->mIsSitting)
			{
				itemp->setValue(LLTrans::getString("stand_up"));
			}
			else
			{
				itemp->setValue(LLTrans::getString("sit_here"));
			}
		}

		gPieSelfp->show(x, y, mPieMouseButtonDown);
	}
	else if (objectp)
	{
		if (gRLenabled && !objectp->isAvatar() &&
			 LLFloaterTools::isVisible() && !gRLInterface.canEdit(objectp))
		{
			gFloaterToolsp->close();
		}

		gMenuHolderp->setObjectSelection(gSelectMgr.getSelection());

		if (objectp->isAvatar())
		{
			// Object is an avatar, so check for mute by id.
			LLVOAvatar* avatar = (LLVOAvatar*)objectp;
			LLUUID id = avatar->getID();
			std::string name = avatar->getFullname();

			if (gMutesPieMenup)
			{
				bool fully_muted = LLMuteList::isMuted(id, name);
				LLMenuItemGL* itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Mute",
														   true, false);
				if (itemp)
				{
					if (fully_muted)
					{
						itemp->setValue(LLTrans::getString("unmute_all"));
					}
					else
					{
						itemp->setValue(LLTrans::getString("mute_all"));
					}
				}

				itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Mute chat",
														   true, false);
				if (itemp)
				{
					if (LLMuteList::isMuted(id, name, LLMute::flagTextChat))
					{
						itemp->setValue(LLTrans::getString("unmute_chat"));
					}
					else
					{
						itemp->setValue(LLTrans::getString("mute_chat"));
					}
				}

				itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Mute voice",
														   true, false);
				if (itemp)
				{
					if (LLMuteList::isMuted(id, name, LLMute::flagVoiceChat))
					{
						itemp->setValue(LLTrans::getString("unmute_voice"));
					}
					else
					{
						itemp->setValue(LLTrans::getString("mute_voice"));
					}
				}

				itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Mute sounds",
														   true, false);
				if (itemp)
				{
					if (LLMuteList::isMuted(id, name, LLMute::flagObjectSounds))
					{
						itemp->setValue(LLTrans::getString("unmute_sounds"));
					}
					else
					{
						itemp->setValue(LLTrans::getString("mute_sounds"));
					}
				}

				itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Mute particles",
														   true, false);
				if (itemp)
				{
					if (LLMuteList::isMuted(id, name, LLMute::flagParticles))
					{
						itemp->setValue(LLTrans::getString("unmute_particles"));
					}
					else
					{
						itemp->setValue(LLTrans::getString("mute_particles"));
					}
				}

				LLVOAvatar::VisualMuteSettings val =
					avatar->getVisualMuteSettings();
				bool settings_available = LLVOAvatar::sUseImpostors;
//MK
				settings_available = settings_available &&
									 (!gRLenabled || !avatar->isRLVMuted());
//mk

				itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Always Render",
														   true, false);
				if (itemp)
				{
					itemp->setEnabled(!fully_muted && settings_available &&
									  val != LLVOAvatar::AV_ALWAYS_RENDER);
				}

				itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Normal Render",
														   true, false);
				if (itemp)
				{
					itemp->setEnabled(!fully_muted && settings_available &&
									  val != LLVOAvatar::AV_RENDER_NORMALLY);
				}

				itemp =
					gMutesPieMenup->getChild<LLMenuItemGL>("Avatar Never Render",
														   true, false);
				if (itemp)
				{
					itemp->setEnabled(!fully_muted && settings_available &&
									  val != LLVOAvatar::AV_DO_NOT_RENDER);
				}
			}

			gPieAvatarp->show(x, y, mPieMouseButtonDown);
		}
		else if (objectp->isAttachment())
		{
			LLMenuItemGL* itemp =
				gPieAttachmentp->getChild<LLMenuItemGL>("Self Sit Attachment",
														true, false);
			if (itemp)
			{
				if (isAgentAvatarValid() && gAgentAvatarp->mIsSitting)
				{
					itemp->setValue(LLTrans::getString("stand_up"));
				}
				else
				{
					itemp->setValue(LLTrans::getString("sit_here"));
				}
			}

			gPieAttachmentp->show(x, y, mPieMouseButtonDown);
		}
		else
		{
#if 0		// Sadly, the object name is unknown/empty when the pie menu is
			// built...
			std::string name;
			LLSelectNode* node = gSelectMgr.getSelection()->getFirstRootNode();
			if (node)
			{
				name = node->mName;
			}
#endif
			if (gPieObjectMutep)
			{
				LLMenuItemGL* itemp =
					gPieObjectMutep->getChild<LLMenuItemGL>("Mute object",
															true, false);
				if (itemp)
				{
					if (LLMuteList::isMuted(objectp->getID()))
					{
						itemp->setValue(LLTrans::getString("unmute"));
					}
					else
					{
						itemp->setValue(LLTrans::getString("mute"));
					}
				}
#if 0			// Sadly, the object name is unknown/empty when the pie menu is
				// built... So, we cannot determine wether this object is
				// already muted by name of not.
				itemp = gPieObjectMutep->getChild<LLMenuItemGL>("Mute by name",
																true, false);
				if (itemp)
				{
					if (LLMuteList::isMuted(LLUUID::null, name))
					{
						itemp->setValue(LLTrans::getString("unmute_by_name"));
					}
					else
					{
						itemp->setValue(LLTrans::getString("mute_by_name"));
					}
				}
#endif
			}
#if 0		// Avatar puppets "jelly-dollifying" does not work anyway...
			LLVOAvatarPuppet* puppet = objectp->getPuppetAvatar();
			if (puppet && gPieObjectMutep)
			{
				LLVOAvatar::VisualMuteSettings val =
					puppet->getVisualMuteSettings();
				bool settings_available = LLVOAvatar::sUseImpostors;
//MK
				settings_available = settings_available &&
									 (!gRLenabled || !puppet->isRLVMuted());
//mk
				LLMenuItemGL* itemp =
					gPieObjectMutep->getChild<LLMenuItemGL>("Puppet Always Render",
															true, false);
				if (itemp)
				{
					itemp->setEnabled(settings_available &&
									  val != LLVOAvatar::AV_ALWAYS_RENDER);
				}

				itemp =
					gPieObjectMutep->getChild<LLMenuItemGL>("Puppet Normal Render",
															true, false);
				if (itemp)
				{
					itemp->setEnabled(settings_available &&
									  val != LLVOAvatar::AV_RENDER_NORMALLY);
				}

				itemp =
					gPieObjectMutep->getChild<LLMenuItemGL>("Puppet Never Render",
															true, false);
				if (itemp)
				{
					itemp->setEnabled(settings_available &&
									  val != LLVOAvatar::AV_DO_NOT_RENDER);
				}
			}
#endif
			gPieObjectp->show(x, y, mPieMouseButtonDown);

			// VEFFECT: ShowPie object. Do not show when you click on someone
			// else: it could freak them out.
			LLHUDEffectSpiral::sphereAtPosition(mPick.mPosGlobal);
		}
	}

	// Ignore return value
	LLTool::handleRightMouseDown(x, y, mask);

	// We handled the event.
	return true;
}

bool LLToolPie::useClickAction(MASK mask, LLViewerObject* objectp,
							   LLViewerObject* parentp)
{
	if (mask != MASK_NONE || !objectp || objectp->isAttachment() ||
		!LLPrimitive::isPrimitive(objectp->getPCode()))
	{
		return false;
	}

	U8 object_action = objectp->getClickAction();
	U8 parent_action = parentp ? parentp->getClickAction() : 0;
	return (object_action && object_action != CLICK_ACTION_DISABLED) ||
		   (parent_action && parent_action != CLICK_ACTION_DISABLED);
}

U8 final_click_action(LLViewerObject* obj)
{
	if (!obj || obj->isAttachment())
	{
		return CLICK_ACTION_NONE;
	}

	U8 object_action = obj->getClickAction();
	if (object_action)
	{
		return object_action;
	}
	// Note: at this point object_action = 0 = CLICK_ACTION_TOUCH

	LLViewerObject* parentp = obj->getRootEdit();
	U8 parent_action = parentp->getClickAction();
#if 0	// CLICK_ACTION_DISABLED ("None" in UI) is intended for child action to
		// override parents action when assigned to parent or to child.
	if (parent_action && parent_action != CLICK_ACTION_DISABLED)
#else
	if (parent_action != CLICK_ACTION_DISABLED)
#endif
	{
		// Note: no need to test for parent_action != 0 because
		// CLICK_ACTION_TOUCH = 0, which would be returned below anyway.
		return parent_action;
	}

	return CLICK_ACTION_TOUCH;
}

ECursorType cursor_from_object(LLViewerObject* objectp)
{
	LLViewerObject* parentp = NULL;
	if (objectp)
	{
		parentp = objectp->getRootEdit();
	}
	U8 click_action = final_click_action(objectp);
	ECursorType cursor = UI_CURSOR_ARROW;
	switch (click_action)
	{
		case CLICK_ACTION_SIT:
			// Not already sitting ?
			if (isAgentAvatarValid() && !gAgentAvatarp->mIsSitting)
			{
				cursor = UI_CURSOR_TOOLSIT;
			}
			break;

		case CLICK_ACTION_BUY:
			cursor = UI_CURSOR_TOOLBUY;
			break;

		case CLICK_ACTION_OPEN:
			// Open always opens the parent.
			if (parentp && parentp->allowOpen())
			{
				cursor = UI_CURSOR_TOOLOPEN;
			}
			break;

		case CLICK_ACTION_PAY:
			if ((objectp && objectp->flagTakesMoney()) ||
				(parentp && parentp->flagTakesMoney()))
			{
				cursor = UI_CURSOR_TOOLPAY;
			}
			break;

		case CLICK_ACTION_ZOOM:
			cursor = UI_CURSOR_TOOLZOOMIN;
			break;

		case CLICK_ACTION_PLAY:
		case CLICK_ACTION_OPEN_MEDIA:
			cursor = cursor_from_parcel_media(click_action);
			break;

		default:
			break;
	}

	return cursor;
}

void LLToolPie::resetSelection()
{
	mLeftClickSelection = NULL;
	mClickActionObject = NULL;
	mClickAction = 0;
}

//static
void LLToolPie::selectionPropertiesReceived()
{
	// Make sure all data has been received since this function will be called
	// repeatedly as the data comes in.
	if (!gSelectMgr.selectGetAllValid())
	{
		return;
	}

	LLObjectSelection* selection = gToolPie.getLeftClickSelection();
	if (selection)
	{
		LLViewerObject* selected_object = selection->getPrimaryObject();
		// since we don't currently have a way to lock a selection, it could
		// have changed after we initially clicked on the object
		if (selected_object == gToolPie.getClickActionObject())
		{
			U8 click_action = gToolPie.getClickAction();
			switch (click_action)
			{
			case CLICK_ACTION_BUY:
				// When we get object properties after left-clicking on an
				// object with left-click = buy, if it's the same object, do
				// the buy.
				handle_buy(NULL);
				break;

			case CLICK_ACTION_PAY:
				handle_give_money_dialog();
				break;

			case CLICK_ACTION_OPEN:
//MK
				if (gRLenabled &&
					!gRLInterface.canEdit(gSelectMgr.getSelection()->getPrimaryObject()))
				{
					return;
				}

				if (gRLenabled &&
					!gRLInterface.canTouchFar(selected_object,
											  gToolPie.getPick().mIntersection))
				{
					return;
				}
//mk
				handle_object_open();
				break;

			default:
				break;
			}
		}
	}
	gToolPie.resetSelection();
}

bool LLToolPie::handleHover(S32 x, S32 y, MASK mask)
{
	LLPickInfo hover_pick = gViewerWindowp->getHoverPick();
	LLViewerObject* objectp = hover_pick.getObject();
	LLViewerObject* parentp = objectp ? objectp->getRootEdit() : NULL;

	if (handleMediaHover(hover_pick))
	{
		// Cursor set by media object
		// *TODO: implement glow-like highlighting ?
	}
	else if (objectp)
	{
		if (useClickAction(mask, objectp, parentp))
		{
			ECursorType cursor = cursor_from_object(objectp);
			gWindowp->setCursor(cursor);
		}
		else if ((!objectp->isAvatar() && objectp->flagUsePhysics()) ||
				 (parentp && !parentp->isAvatar() &&
				  parentp->flagUsePhysics()))
		{
			gWindowp->setCursor(UI_CURSOR_TOOLGRAB);
		}
		else if ((objectp->getClickAction() != CLICK_ACTION_DISABLED ||
				  !objectp->isAttachment()) &&
				 (objectp->flagHandleTouch() ||
				  (parentp && parentp->flagHandleTouch())))
		{
			gWindowp->setCursor(UI_CURSOR_HAND);
		}

		else
		{
			gWindowp->setCursor(UI_CURSOR_ARROW);
		}
	}
	else
	{
		gWindowp->setCursor(UI_CURSOR_ARROW);
		LLViewerMediaFocus::getInstance()->clearHover();
	}

	return true;
}

bool LLToolPie::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLViewerObject* obj = mPick.getObject();
	U8 click_action = final_click_action(obj);
	if (click_action == CLICK_ACTION_BUY || click_action == CLICK_ACTION_PAY ||
		click_action == CLICK_ACTION_OPEN)
	{
		// Because these actions open UI dialogs, we won't change the cursor
		// again until the next hover and GL pick over the world. Keep the
		// cursor an arrow, assuming that after the user moves off the UI, they
		// won't be on the same object anymore.
		gWindowp->setCursor(UI_CURSOR_ARROW);
		// Make sure the hover-picked object is ignored.
		gHoverViewp->resetLastHoverObject();
	}

	mGrabMouseButtonDown = false;
	gToolMgr.clearTransientTool();

	// Maybe look at object/person clicked on
	gAgent.setLookAt(LOOKAT_TARGET_CONVERSATION, obj);

	return LLTool::handleMouseUp(x, y, mask);
}

bool LLToolPie::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	mPieMouseButtonDown = false;
	gToolMgr.clearTransientTool();
	return LLTool::handleRightMouseUp(x, y, mask);
}

bool LLToolPie::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		llinfos << "LLToolPie handleDoubleClick (becoming mouseDown)"
				<< llendl;
	}

	if (handleMediaDblClick(mPick))
	{
		return true;
	}

	if (mPick.mPosGlobal.isExactlyZero())
	{
		return false;
	}

	LLViewerObject* objp = mPick.getObject();
	LLViewerObject* parentp = objp ? objp->getRootEdit() : NULL;
	bool is_in_world = mPick.mObjectID.notNull() && objp &&
					   !objp->isHUDAttachment();
	bool is_land = mPick.mPickType == LLPickInfo::PICK_LAND;
	bool has_touch_handler = false;
	bool has_click_action = false;
	if (!is_land && is_in_world &&	// Note: if is_in_world then objp != NULL
		!gSavedSettings.getBool("DoubleClickScriptedObject"))
	{
		has_touch_handler = objp->flagHandleTouch() ||
							(parentp && parentp->flagHandleTouch());
		has_click_action = final_click_action(objp);
		if (!has_touch_handler || !has_click_action)
		{
			// Is media playing on this face ?
			const LLTextureEntry* tep = objp->getTE(mPick.mObjectFace);
			viewer_media_t mediap =
				LLViewerMedia::getMediaImplFromTextureEntry(tep);
			if (mediap.notNull() && mediap->hasMedia())
			{
				has_touch_handler = has_click_action = true;
			}
		}
	}

	if (is_land || (is_in_world && !has_touch_handler && !has_click_action))
	{
		U32 action = gSavedSettings.getU32("DoubleClickAction");
		if (action == 1)
		{
			handle_go_to();
			return true;
		}
		else if (action == 2 && isAgentAvatarValid()
//MK
				 && !(gRLenabled && gRLInterface.contains ("tploc")))
//mk
		{
			LLVector3d pos = mPick.mPosGlobal;
			pos.mdV[VZ] += gAgentAvatarp->getPelvisToFoot();
			gAgent.teleportViaLocationLookAt(pos);
			return true;
		}
	}

	return false;
}

void LLToolPie::handleDeselect()
{
	if (hasMouseCapture())
	{
		setMouseCapture(false);  // Calls onMouseCaptureLost() indirectly
	}
	// Remove temporary selection for pie menu
	gSelectMgr.validateSelection();
}

LLTool* LLToolPie::getOverrideTool(MASK mask)
{
	if (mask == MASK_CONTROL || mask == (MASK_CONTROL | MASK_SHIFT))
	{
		return &gToolGrab;
	}
	return LLTool::getOverrideTool(mask);
}

void LLToolPie::stopEditing()
{
	if (hasMouseCapture())
	{
		setMouseCapture(false);  // Calls onMouseCaptureLost() indirectly
	}
}

static void handle_click_action_play()
{
	LLViewerMediaImpl::EMediaStatus status = LLViewerParcelMedia::getStatus();
	switch (status)
	{
		case LLViewerMediaImpl::MEDIA_PLAYING:
			LLViewerParcelMedia::pause();
			break;

		case LLViewerMediaImpl::MEDIA_PAUSED:
			LLViewerParcelMedia::start();
			break;

		default:
			LLViewerParcelMedia::play();
	}
}

bool LLToolPie::handleMediaClick(const LLPickInfo& pick)
{
	// *FIXME: how do we handle object in different parcel than us ?
	LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
	LLPointer<LLViewerObject> objectp = pick.getObject();
	LLViewerMediaFocus* focusp = LLViewerMediaFocus::getInstance();

	if (!LLVOVolume::sObjectMediaClient || !parcelp || objectp.isNull() ||
		pick.mObjectFace < 0 || pick.mObjectFace >= objectp->getNumTEs())
	{
		focusp->clearFocus();
		return false;
	}

	// Does this face have media ?
	const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
	viewer_media_t mediap = LLViewerMedia::getMediaImplFromTextureEntry(tep);
	if (mediap.isNull() || !mediap->hasMedia())
	{
		focusp->clearFocus();
		return false;
	}

	if (!focusp->isFocusedOnFace(pick.getObject(), pick.mObjectFace))
	{
		LL_DEBUGS("Media") << (mediap.isNull() ? "Media impl is NULL"
											   : "New focus detected")
						   << ", focusing on media face." << LL_ENDL;
		focusp->setFocusFace(true, pick.getObject(), pick.mObjectFace,
							 mediap, pick.mNormal);
	}
	else if (gKeyboardp)
	{
		// Make sure keyboard focus is set to the media focus object.
		gFocusMgr.setKeyboardFocus(focusp);
		gEditMenuHandlerp = focusp->getFocusedMediaImpl();

		mediap->mouseDown(pick.mUVCoords, gKeyboardp->currentMask(true));
		// The mouse-up will happen when capture is lost
		mediap->mouseCapture();
		LL_DEBUGS("Media") << "Mouse down event passed to media" << LL_ENDL;
	}

	return true;
}

bool LLToolPie::handleMediaDblClick(const LLPickInfo& pick)
{
	// *FIXME: how do we handle object in different parcel than us ?
	LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
	if (!parcelp)
	{
		return false;
	}

	LLViewerMediaFocus* focusp = LLViewerMediaFocus::getInstance();

	LLPointer<LLViewerObject> objectp = mPick.getObject();
	if (!LLVOVolume::sObjectMediaClient || objectp.isNull() ||
		pick.mObjectFace < 0 || pick.mObjectFace >= objectp->getNumTEs())
	{
		focusp->clearFocus();
		return false;
	}

	const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
	viewer_media_t mediap = LLViewerMedia::getMediaImplFromTextureEntry(tep);
	if (mediap.isNull() || !mediap->hasMedia())
	{
		focusp->clearFocus();
		return false;
	}

	if (!focusp->isFocusedOnFace(pick.getObject(), pick.mObjectFace))
	{
		focusp->setFocusFace(true, pick.getObject(), pick.mObjectFace,
							 mediap, pick.mNormal);
	}
	else if (gKeyboardp)
	{
		// Make sure keyboard focus is set to the media focus object.
		gFocusMgr.setKeyboardFocus(focusp);
		gEditMenuHandlerp = focusp->getFocusedMediaImpl();

		mediap->mouseDoubleClick(pick.mUVCoords,
								 gKeyboardp->currentMask(true));
		// The mouse-up will happen when capture is lost
		mediap->mouseCapture();
		LL_DEBUGS("Media") << "Mouse double-click event passed to media"
						   << LL_ENDL;
	}

	return true;
}

bool LLToolPie::handleMediaHover(const LLPickInfo& pick)
{
	// *FIXME: how do we handle object in different parcel than us ?
	LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
	if (!parcelp)
	{
		return false;
	}

	LLViewerMediaFocus* focusp = LLViewerMediaFocus::getInstance();

	LLPointer<LLViewerObject> objectp = pick.getObject();

	// Early out cases. Must clear mouse over media focus flag did not hit an
	// object or did not hit a valid face
	if (!LLVOVolume::sObjectMediaClient || objectp.isNull() ||
		pick.mObjectFace < 0 || pick.mObjectFace >= objectp->getNumTEs())
	{
		focusp->clearHover();
		return false;
	}

	const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
	viewer_media_t mediap = LLViewerMedia::getMediaImplFromTextureEntry(tep);
	if (mediap.notNull() && gKeyboardp && LLVOVolume::sObjectMediaClient)
	{
		// Update media hover object
		if (!focusp->isHoveringOverFace(objectp, pick.mObjectFace))
		{
			focusp->setHoverFace(objectp, pick.mObjectFace, mediap,
								 pick.mNormal);
			gSelectMgr.setHoverObject(objectp, pick.mObjectFace);
			focusp->setPickInfo(pick);
		}

		// If this is the focused media face, send mouse move events.
		if (focusp->isFocusedOnFace(objectp, pick.mObjectFace))
		{
			mediap->mouseMove(pick.mUVCoords,
							  gKeyboardp->currentMask(true));
			gViewerWindowp->setCursor(mediap->getLastSetCursor());
		}
		else
		{
			// This is not the focused face -- set the default cursor.
			gViewerWindowp->setCursor(UI_CURSOR_ARROW);
		}

		return true;
	}

	// In all other cases, clear media hover.
	focusp->clearHover();

	return false;
}

static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp)
{
	if (!LLVOVolume::sObjectMediaClient)
	{
		return;	// Media disabled.
	}

	// *FIXME: how do we handle object in different parcel than us ?
	LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
	if (!parcelp)
	{
		return;
	}

	// Did we hit an object ?
	if (objectp.isNull()) return;

	// Did we hit a valid face on the object ?
	S32 face = gToolPie.getPick().mObjectFace;
	if (face < 0 || face >= objectp->getNumTEs()) return;

	// Is media playing on this face ?
	LLTextureEntry* tep = objectp->getTE(face);
	if (tep && LLViewerMedia::getMediaImplFromTextureID(tep->getID()))
	{
		handle_click_action_play();
		return;
	}

	std::string media_url = parcelp->getMediaURL();
	std::string media_type = parcelp->getMediaType();
	LLStringUtil::trim(media_url);

	// Get the scheme, see if that is handled as well.
	LLURI uri(media_url);
	std::string media_scheme = uri.scheme() != "" ? uri.scheme() : "http";

	LLWeb::loadURL(media_url);
}

static ECursorType cursor_from_parcel_media(U8 click_action)
{
	if (!LLVOVolume::sObjectMediaClient)
	{
		return UI_CURSOR_ARROW;	// Media disabled.
	}

	// *FIXME: how do we handle object in different parcel than us ?
	LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
	if (!parcelp)
	{
		return UI_CURSOR_ARROW;
	}

	LLViewerMediaImpl::EMediaStatus status = LLViewerParcelMedia::getStatus();
	if (status == LLViewerMediaImpl::MEDIA_PLAYING)
	{
		return click_action == CLICK_ACTION_PLAY ? UI_CURSOR_TOOLPAUSE
												 : UI_CURSOR_TOOLMEDIAOPEN;
	}

	return UI_CURSOR_TOOLPLAY;
}
