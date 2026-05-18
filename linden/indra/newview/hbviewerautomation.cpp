/**
 * @file hbviewerautomation.cpp
 * @brief HBViewerAutomation class implementation
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 *
 * Copyright (c) 2016-2026, Henri Beauchamp.
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

#include <deque>
#include <exception>

#include "boost/algorithm/string.hpp"

#include "lua.hpp"
#include "json.hpp"

#include "hbviewerautomation.h"

#include "imageids.h"
#include "llalertdialog.h"
#include "llatomic.h"
#include "llaudioengine.h"
#include "llbase64.h"
#include "llbutton.h"
#include "llcachename.h"
#include "llcallbacklist.h"
#include "llcheckboxctrl.h"
#include "llclipboard.h"
#include "llcombobox.h"
#include "llcorehttputil.h"
#include "lldir.h"
#include "lleconomy.h"
#include "hbfastmap.h"
#include "hbfileselector.h"
#include "llfasttimer.h"
#include "lllineeditor.h"
#include "llnamelistctrl.h"
#include "llnotifications.h"
#include "llradiogroup.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltexteditor.h"
#include "llthread.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llmessage.h"

#include "llagent.h"
#include "llagentpilot.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llappviewer.h"
#include "llavatartracker.h"
#include "llchatbar.h"
#include "llenvironment.h"
#include "llenvsettings.h"
#include "llfloateractivespeakers.h"
#include "hbfloaterareasearch.h"
#include "llfloateravatarinfo.h"
#include "llfloateravatarpicker.h"
#include "llfloateravatartextures.h"
#include "llfloaterbeacons.h"
#include "hbfloaterbump.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloaterdebugsettings.h"
#include "hbfloaterdebugtags.h"
#include "llfloaterexperiences.h"
#include "llfloaterfriends.h"
#include "llfloatergesture.h"
#include "llfloatergroupinfo.h"
#include "llfloatergroups.h"
#include "llfloaterim.h"
#include "llfloaterinspect.h"
#include "llfloaterinventory.h"
#include "hbfloaterinvitemspicker.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "slfloatermediafilter.h"
#include "llfloaterminimap.h"
#include "llfloatermove.h"
#include "llfloatermute.h"
#include "llfloaternearbymedia.h"
#include "llfloaternotificationsconsole.h"
#include "llfloaterpathfindingcharacters.h"
#include "llfloaterpathfindinglinksets.h"
#include "llfloaterpreference.h"
#include "hbfloaterradar.h"
#include "llfloaterregioninfo.h"
#include "hbfloatersearch.h"
#include "llfloatersnapshot.h"
#include "hbfloatersoundslist.h"
#include "llfloaterstats.h"
#include "hbfloaterteleporthistory.h"
#include "llfloatertools.h"
#include "llfloaterworldmap.h"
#include "llfolderview.h"
#include "llgridmanager.h"
#include "llgroupmgr.h"
#include "llgroupnotify.h"
#include "llimmgr.h"
#include "llinventorymodelfetch.h"
#include "llmutelist.h"
#include "llnotify.h"
#include "lloverlaybar.h"
#include "llpipeline.h"
#include "hbpreprocessor.h"
#include "llpuppetmodule.h"
#include "llpuppetmotion.h"
//MK
#include "mkrlinterface.h"
//mk
#include "llselectmgr.h"
#include "llstartup.h"
#include "llstatusbar.h"
#include "lltooldraganddrop.h"
#include "llurldispatcher.h"
#include "llvieweraudio.h"				// For get_valid_sounds()
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llweb.h"
#include "llwlskyparammgr.h"
#include "llwlwaterparammgr.h"
#include "llworldmap.h"

using namespace std::placeholders;
using namespace LLAvatarAppearanceDefines;
using lljson = nlohmann::json;

HBViewerAutomation* gAutomationp = NULL;

static LLAtomicU32 sCurrentHTTPHandle(0);

// Note: keep in sync with LLSettingsType::EType
static const std::string sEnvSettingsTypes[] = { "sky", "water", "day" };

enum Picked_types
{
	PICKED_LAND = 0,
	PICKED_PARTICLE,
	PICKED_OBJECT,
	PICKED_ATTACHMENT,
	PICKED_AVATAR,
	PICKED_SELF,
	PICKED_INVALID
};

enum Notification_types
{
	NOTIFYTIP,
	NOTIFICATION,
	ALERT,
};

///////////////////////////////////////////////////////////////////////////////
// HBLuaDialog class (generic usage floater for Lua scripts)
///////////////////////////////////////////////////////////////////////////////

class HBLuaDialog final : public LLFloater
{
protected:
	LOG_CLASS(HBLuaDialog);

public:
	static HBLuaDialog* create(const std::string& title,
							   const std::string& text,
							   const std::string& suggestion,
							   const std::string& btn1,
							   const std::string& btn2,
							   const std::string& btn3,
							   const std::string& command1,
							   const std::string& command2,
							   const std::string& command3);

private:
	// Use the create() method to create the floater.
	HBLuaDialog(const LLSD& parameters);
	~HBLuaDialog() override;

	bool postBuild() override;

	// Returns true when the dialog shall be closed
	bool evalLuaCommand(const std::string& command);

	static void onButton(LLUICtrl* ctrl, void* datap);

private:
	LLLineEditor*	mInputLine;
	S32				mPressedButton;
	LLSD			mParameters;
};

//static
HBLuaDialog* HBLuaDialog::create(const std::string& title,
								 const std::string& text,
								 const std::string& suggestion,
								 const std::string& btn1,
								 const std::string& btn2,
								 const std::string& btn3,
								 const std::string& command1,
								 const std::string& command2,
								 const std::string& command3)
{
	LLSD parameters = LLSD::emptyMap();
	parameters["title"] = title;
	parameters["suggestion"] = suggestion;
	if (!text.empty())
	{
		parameters["text"] = text;
	}
	if (!btn1.empty())
	{
		parameters["btn1"] = btn1;
		parameters["command1"] = command1;
	}
	if (!btn2.empty())
	{
		parameters["btn2"] = btn2;
		parameters["command2"] = command2;
	}
	if (!btn3.empty())
	{
		parameters["btn3"] = btn3;
		parameters["command3"] = command3;
	}

	LL_DEBUGS("Lua") << "Creating new Lua dialog with parameters:\n";
	std::stringstream str;
	LLSDSerialize::toPrettyXML(parameters, str);
	LL_CONT << "\n" << str.str() << LL_ENDL;

	return new HBLuaDialog(parameters);
}

HBLuaDialog::HBLuaDialog(const LLSD& parameters)
:	mParameters(parameters),
	mPressedButton(0)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_lua_dialog.xml");

	// If the user did not yet change the floater position, center the latter.
	LLControlVariable* controlp =
		gSavedSettings.getControl(getRectControl().c_str());
	if (!controlp || controlp->isDefault())
	{
		center();
	}
}

//virtual
HBLuaDialog::~HBLuaDialog()
{
	if (gAutomationp)
	{
		gAutomationp->onLuaDialogClose(mParameters["title"].asString(),
									   mPressedButton, mInputLine->getText());
	}
}

//virtual
bool HBLuaDialog::postBuild()
{
	setTitle(mParameters["title"].asString());

	LLButton* button = getChild<LLButton>("btn1");
	if (mParameters.has("btn1"))
	{
		button->setLabel(mParameters["btn1"].asString());
		button->setCommitCallback(onButton);
		button->setCallbackUserData(this);
	}
	else
	{
		button->setEnabled(false);
		button->setVisible(false);
	}

	button = getChild<LLButton>("btn2");
	if (mParameters.has("btn2"))
	{
		button->setLabel(mParameters["btn2"].asString());
		button->setCommitCallback(onButton);
		button->setCallbackUserData(this);
	}
	else
	{
		button->setEnabled(false);
		button->setVisible(false);
	}

	button = getChild<LLButton>("btn3");
	if (mParameters.has("btn3"))
	{
		button->setLabel(mParameters["btn3"].asString());
		button->setCommitCallback(onButton);
		button->setCallbackUserData(this);
	}
	else
	{
		button->setEnabled(false);
		button->setVisible(false);
	}

	LLTextEditor* textedit = getChild<LLTextEditor>("text");
	textedit->setBorderVisible(false);
	if (mParameters.has("text"))
	{
		std::string text = mParameters["text"].asString();
		textedit->setParseHTML(true);
		textedit->appendColoredText(text, false, false,
									gColors.getColor("TextFgReadOnlyColor"));
	}

	mInputLine = getChild<LLLineEditor>("input");
	std::string suggestion = mParameters["suggestion"].asString();
	if (suggestion == " ")
	{
		mInputLine->setEnabled(false);
		mInputLine->setVisible(false);
	}
	else if (suggestion == "*")
	{
		mInputLine->setDrawAsterixes(true);
	}
	else if (!suggestion.empty())
	{
		mInputLine->setText(suggestion);
	}

	return true;
}

bool HBLuaDialog::evalLuaCommand(const std::string& command)
{
	bool close = false;

	// Setup dialog-specific Lua global variables
	std::string functions = "V_DIALOG_CLOSE=false;V_DIALOG_INPUT=\"";
	std::string text = mInputLine->getText();
	LLStringUtil::replaceString(text, "\"", "\\\"");
	functions += text + "\";";

	// Setup dialog-specific Lua functions using the global variables
	functions += "function DialogClose();V_DIALOG_CLOSE=true;end;";
	functions += "function GetDialogInput();return V_DIALOG_INPUT;end;";
	functions += "function SetDialogInput(text);V_DIALOG_INPUT=text;end;";

	HBViewerAutomation lua;
	lua_State* statep = lua.mLuaState;
	if (statep && lua.loadString(functions + command))
	{
		// Retreive and interpret the global variables values
		lua_getglobal(statep, "V_DIALOG_INPUT");
		text = lua_tostring(statep, -1);
		if (mInputLine->getText() != text)
		{
			mInputLine->setText(text);
		}
		lua_getglobal(statep, "V_DIALOG_CLOSE");
		close = lua_toboolean(statep, -1);
	}

	return close;
}

//static
void HBLuaDialog::onButton(LLUICtrl* ctrlp, void* datap)
{
	HBLuaDialog* self = (HBLuaDialog*)datap;
	if (!self || !ctrlp) return;

	std::string command;

	std::string name = ctrlp->getName();
	if (name == "btn1")
	{
		self->mPressedButton = 1;
		command = self->mParameters["command1"].asString();
	}
	else if (name == "btn2")
	{
		self->mPressedButton = 2;
		command = self->mParameters["command2"].asString();
	}
	else if (name == "btn3")
	{
		self->mPressedButton = 3;
		command = self->mParameters["command3"].asString();
	}

	if (!command.empty() && self->evalLuaCommand(command))
	{
		self->close();
	}
	else
	{
		self->mPressedButton = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// HBLuaFloater class (custom floaters support for the Lua scripts)
///////////////////////////////////////////////////////////////////////////////

class HBLuaFloater final : public LLFloater
{
protected:
	LOG_CLASS(HBLuaFloater);

public:
	LL_INLINE HBLuaFloater* asLuaFloater() override		{ return this; }

	static HBLuaFloater* create(const std::string& name,
								const std::string& parameter,
								const std::string& position, bool open,
								const LLSD& ctrl_values);
	static bool setVisible(const std::string& name, bool show);
	static void destroy(const std::string& name, bool excute_callback);

	static LLScrollListCtrl* getList(const std::string& floater_name,
									 const std::string& ctrl_name);

	static bool setControlCallback(const std::string& floater_name,
								   const std::string& ctrl_name,
								   const std::string& lua_command);

	static void registerUICommand(LLUICtrl* ctrlp, LLView* parentp,
								  const std::string& command);

	static bool getControlValue(const std::string& floater_name,
								const std::string& ctrl_name,
								std::string& value);
	static bool getControlValues(const std::string& floater_name,
								 const std::string& ctrl_name,
								 std::vector<std::string>& values);
	static bool setControlValue(const std::string& floater_name,
								const std::string& ctrl_name,
								const std::string& value);
	static bool setInvFilter(const std::string& floater_name,
							 const std::string& ctrl_name,
							 const std::string& value);

	static bool setControlEnabled(const std::string& floater_name,
								  const std::string& ctrl_name, bool enable);
	static bool setControlVisible(const std::string& floater_name,
								  const std::string& ctrl_name, bool visible);

private:
	// Use the create() method to create the floater.
	HBLuaFloater(const std::string& name, const std::string& parameter);
	~HBLuaFloater() override;

	bool postBuild() override;
	void onOpen() override;
	void onClose(bool app_quitting) override;

	bool evalLuaCommand(const std::string& command, std::string value,
						bool with_close = false);


	bool setControlCallback(LLUICtrl* ctrlp, const std::string& lua_command);

	static std::string getCtrlValue(LLUICtrl* ctrlp);
	static void getCtrlValues(LLUICtrl* ctrlp,
							  std::vector<std::string>& values);
	static bool setCtrlValue(LLUICtrl* ctrlp, const std::string& value);
	static void onCommitCallback(LLUICtrl* ctrlp, void* datap);
	static void onSearchEdit(const std::string& search_string, void* datap);
	static void onInventorySelect(LLFolderView* ctrlp, bool user_action,
								  void* datap);

private:
	std::string				mLuaName;
	std::string				mParameter;

	typedef fast_hmap<LLUICtrl*, std::string> commands_map_t;
	commands_map_t			mCommands;

	bool					mInitOK;

	typedef std::map<std::string, HBLuaFloater*, std::less<> > instances_map_t;
	static instances_map_t	sInstances;
};

HBLuaFloater::instances_map_t HBLuaFloater::sInstances;

// Helper function
static bool validate_floater_name(const std::string& name, std::string& fname)
{
	// Only accept a-z, A-Z, 0-9, '_', '-', and at most one '|' (the latter and
	// all following characters not being used to build the XUI file name).
	bool has_pipe = false;
	for (size_t i = 0, count = name.size(); i < count; ++i)
	{
		char c = name[i];
		if (c == '|' && !has_pipe)
		{
			has_pipe = true;
			fname = name.substr(0, i);
			continue;
		}
		if (c == '_' || c == '-' || (c >= '0' && c <= '9') ||
			(c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		{
			continue;
		}
		return false;
	}
	// XUI file name to use for this floater.
	fname = "floater_lua_" + (has_pipe ? fname : name) + ".xml";
	return true;
}

//static
HBLuaFloater* HBLuaFloater::create(const std::string& name,
								   const std::string& parameter,
								   const std::string& position,
								   bool open, const LLSD& ctrl_values)
{
	// Refuse to open two floaters with "dialog" as the name since the
	// corresponding XML file name is already used by our HBLuaDialog class.
	if (name == "dialog" || name == "console")
	{
		llwarns << "The '" << name
				<< "' Lua floater name is reserved. Aborted." << llendl;
		return NULL;
	}

	std::string fname;
	if (!validate_floater_name(name, fname))
	{
		llwarns << "Invalid Lua floater name '" << name << "' . Aborted."
				<< llendl;
		return NULL;
	}

	// Refuse to open two floaters with the same name
	if (sInstances.count(name))
	{
		setVisible(name, true);	// Make sure existing floater is shown.
		llinfos << "Floater '" << name
				<< "' is already opened, not opening a second instance."
				<< llendl;
		return NULL;
	}

	HBLuaFloater* self = new HBLuaFloater(name, parameter);
	if (!LLUICtrlFactory::getInstance()->buildFloater(self, fname, NULL, open))
	{
		self->mInitOK = false;
		delete self;
		return NULL;
	}

	std::string entry, cname, value;
   	for (LLSD::array_const_iterator it = ctrl_values.beginArray(),
									end = ctrl_values.endArray();
		 it != end; ++it)
	{
		entry = it->asString();
		size_t i = entry.find('=');
		if (i == std::string::npos || i == 0 || i + 1 == entry.size())
		{
			llwarns << "Lua floater: " << name
					<< " - Invalid control name=value: " << entry << llendl;
			continue;
		}
		cname = entry.substr(0, i);
		value = entry.substr(i + 1);
		LLStringUtil::trim(cname);
		LLUICtrl* ctrlp = self->getChild<LLUICtrl>(cname.c_str(), true, false);
		if (!ctrlp)
		{
			llwarns << "Lua floater: " << name << " - Invalid control name: "
					<< cname << llendl;
			continue;
		}
		if (!setCtrlValue(ctrlp, value))
		{
			llwarns << "Lua floater: " << name << " - Could not set: " << entry
					<< llendl;
		}
	}

	if (position.empty() || position == "center")
	{
		self->center();
		return self;
	}

	const LLRect& view = gFloaterViewp->getRect();
	const S32 view_width = view.getWidth();
	const S32 view_height = view.getHeight();

	LLRect r = self->getRect();
	const S32 width = r.getWidth();
	const S32 height = r.getHeight();

	// "x,y" position
	size_t i = position.find(',');
	if (i != std::string::npos)
	{
		if (i == 0 || i == position.size() - 1)
		{
			llwarns << "Invalid position parameter '" << position
					<< "' for floater: " << name << llendl;
			self->center();
			return self;
		}
		S32 x0 = atoi(position.substr(0, i).c_str());
		if (x0 < 0)
		{
			x0 += view_width - width;
		}
		S32 y0 = atoi(position.substr(i + 1).c_str());
		if (y0 >= 0)
		{
			y0 = view_height - y0;
		}
		else
		{
			y0 = view.mBottom - y0 + height;
		}
		r.setLeftTopAndSize(x0, y0, width, height);
		self->translateIntoRect(r, false);
		return self;
	}

	// Do not cover the Lua side bar with the Lua floater
	S32 left_margin = 0;
	S32 right_margin = 0;
	if (gLuaSideBarp)
	{
		S32 margin = gLuaSideBarp->getRect().getWidth();
		if (gSavedSettings.getBool("LuaSideBarOnLeft"))
		{
			left_margin = margin;
		}
		else
		{
			right_margin = margin;
		}
	}

	if (position == "top" || position == "top-center")
	{
		r.setLeftTopAndSize((view_width - width) / 2, view_height, width,
							height);
	}
	else if (position == "bottom" || position == "bottom-center")
	{
		r.setOriginAndSize((view_width - width) / 2, view.mBottom, width,
						   height);
	}
	else if (position == "left" || position == "left-center")
	{
		r.setLeftTopAndSize(left_margin,
							view_height - (view_height - height) / 2,
							width, height);
	}
	else if (position == "right" || position == "right-center")
	{
		r.setLeftTopAndSize(view_width - width - right_margin,
							view_height - (view_height - height) / 2,
							width, height);
	}
	else if (position == "top-left")
	{
		r.setLeftTopAndSize(left_margin, view_height, width, height);
	}
	else if (position == "top-right")
	{
		r.setLeftTopAndSize(view_width - width - right_margin, view_height,
							width, height);
	}
	else if (position == "bottom-left")
	{
		r.setOriginAndSize(left_margin, view.mBottom, width, height);
	}
	else if (position == "bottom-right")
	{
		r.setOriginAndSize(view_width - width - right_margin, view.mBottom,
						   width, height);
	}
	else
	{
		// 'position' could be the name of a Rect setting
		LLControlVariable* controlp =
			gSavedSettings.getControl(position.c_str());
		if (controlp)
		{
			if (controlp->type() == TYPE_RECT)
			{
				// Check that the variable has been initialized at least once.
				LLRect r0;
				r0.setValue(controlp->getValue());
				if (r0.getWidth() <= 0 || r0.getHeight() <= 0)
				{
					controlp->setValue(r.getValue());
				}
				// Use this variable as the Rect control
				self->setRectControl(position);
				self->applyRectControl();
			}
			else
			{
				llwarns << "Invalid control setting (not a Rect): '"
						<< position << "' for floater: " << name << llendl;
				self->center();
			}
			return self;
		}
		else
		{
			llwarns << "Unrecognized position parameter '" << position
					<< "' for floater: " << name << llendl;
			self->center();
			return self;
		}
	}
	self->translateIntoRect(r, false);

	return self;
}

//static
bool HBLuaFloater::setVisible(const std::string& name, bool show)
{
	instances_map_t::iterator it = sInstances.find(name);
	if (it == sInstances.end())
	{
		return false;
	}

	LLFloater* self = (LLFloater*)it->second;
	bool visible = self->getVisible();
	if (show && !visible)
	{
		self->open();
	}
	else if (!show && visible)
	{
		self->setVisible(false);
	}

	return true;
}

//static
void HBLuaFloater::destroy(const std::string& name, bool excute_callback)
{
	instances_map_t::iterator it = sInstances.find(name);
	if (it != sInstances.end())
	{
		HBLuaFloater* self = it->second;
		if (!excute_callback)
		{
			// Do not call the OnLuaFloaterClose() callback:
			self->mInitOK = false;
		}
		self->close();
	}
}

//static
LLScrollListCtrl* HBLuaFloater::getList(const std::string& floater_name,
										const std::string& ctrl_name)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return NULL;
	}

	HBLuaFloater* self = it->second;

	LLUICtrl* ctrlp = self->getChild<LLUICtrl>(ctrl_name.c_str(), true, false);
	if (!ctrlp)
	{
		return NULL;
	}
	return dynamic_cast<LLScrollListCtrl*>(ctrlp);
}

bool HBLuaFloater::setControlCallback(LLUICtrl* ctrlp,
									  const std::string& lua_command)
{
	mCommands[ctrlp] = lua_command;

	// For inventory panels, we use a special commit on selection callback
	LLInventoryPanel* invp = dynamic_cast<LLInventoryPanel*>(ctrlp);
	if (invp)
	{
		invp->setSelectCallback(onInventorySelect, this);
		return true;
	}

	// For search editors, use a special commit callback too.
	LLSearchEditor* searcheditp = dynamic_cast<LLSearchEditor*>(ctrlp);
	if (searcheditp)
	{
		searcheditp->setSearchCallback(onSearchEdit, searcheditp);
		return true;
	}

	// For all other control types, use the LLUICtrl commit callback
	ctrlp->setCommitCallback(onCommitCallback);
	ctrlp->setCallbackUserData(this);

	// For line and text editors controls, we commit on lost focus

	LLLineEditor* lineeditp = dynamic_cast<LLLineEditor*>(ctrlp);
	if (lineeditp)
	{
		lineeditp->setCommitOnFocusLost(true);
		return true;
	}

	LLTextEditor* texteditp = dynamic_cast<LLTextEditor*>(ctrlp);
	if (texteditp)
	{
		texteditp->setCommitOnFocusLost(true);
		return true;
	}

	// For scroll list controls (and derived classes such as name list), we
	// commit on selection change
	LLScrollListCtrl* listp = dynamic_cast<LLScrollListCtrl*>(ctrlp);
	if (listp)
	{
		listp->setCommitOnSelectionChange(true);
	}

	return true;
}

//static
bool HBLuaFloater::setControlCallback(const std::string& floater_name,
									  const std::string& ctrl_name,
									  const std::string& lua_command)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return false;
	}
	HBLuaFloater* self = it->second;

	LLUICtrl* ctrlp = self->getChild<LLUICtrl>(ctrl_name.c_str(), true, false);
	if (!ctrlp)
	{
		return false;
	}

	return self->setControlCallback(ctrlp, lua_command);
}

//static
void HBLuaFloater::registerUICommand(LLUICtrl* ctrlp, LLView* parentp,
									 const std::string& lua_command)
{
	if (!ctrlp || !parentp || lua_command.empty())
	{
		return;
	}
	// Search for the Lua floater this UI control pertains to, if any.
	HBLuaFloater* self = NULL;
	while (!self && parentp)
	{
		self = parentp->asLuaFloater();
		parentp = parentp->getParent();
	}
	if (!self)
	{
		llwarns << "Control " << ctrlp->getName()
				<< " is not part of a Lua floater: 'lua_command' attribute ignored."
				<< llendl;
		return;
	}
	if (!self->setControlCallback(ctrlp, lua_command))
	{
		llwarns << "Failed to set Lua command for control: "
				<< ctrlp->getName() << llendl;
	}
}

// A mere wrapper function to avoid declaring HBLuaFloater in the
// hbviewerautomation.h header.
void register_ui_lua_command(LLUICtrl* ctrlp, LLView* parentp,
							 const std::string& command)
{
	HBLuaFloater::registerUICommand(ctrlp, parentp, command);
}

//static
bool HBLuaFloater::getControlValue(const std::string& floater_name,
								   const std::string& ctrl_name,
								   std::string& value)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return false;
	}

	HBLuaFloater* self = it->second;

	LLUICtrl* ctrlp = self->getChild<LLUICtrl>(ctrl_name.c_str(), true, false);
	if (!ctrlp)
	{
		return false;
	}

	value.assign(getCtrlValue(ctrlp));

	return true;
}

//static
bool HBLuaFloater::getControlValues(const std::string& floater_name,
									const std::string& ctrl_name,
									std::vector<std::string>& values)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return false;
	}

	HBLuaFloater* self = it->second;

	LLUICtrl* ctrlp = self->getChild<LLUICtrl>(ctrl_name.c_str(), true, false);
	if (!ctrlp)
	{
		return false;
	}

	getCtrlValues(ctrlp, values);

	return true;
}

//static
bool HBLuaFloater::setControlValue(const std::string& floater_name,
								   const std::string& ctrl_name,
								   const std::string& value)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return false;
	}

	HBLuaFloater* self = it->second;

	LLUICtrl* ctrlp = self->getChild<LLUICtrl>(ctrl_name.c_str(), true, false);
	if (!ctrlp)
	{
		return false;
	}

	return setCtrlValue(ctrlp, value);
}

//static
bool HBLuaFloater::setInvFilter(const std::string& floater_name,
								const std::string& ctrl_name,
								const std::string& value)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return false;
	}

	HBLuaFloater* self = it->second;

	LLInventoryPanel* invp =
		self->getChild<LLInventoryPanel>(ctrl_name.c_str(), true, false);
	if (!invp)
	{
		return false;
	}

	invp->setFilterSubString(value);
	return true;
}

//static
bool HBLuaFloater::setControlEnabled(const std::string& floater_name,
									 const std::string& ctrl_name, bool enable)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return false;
	}

	HBLuaFloater* self = it->second;

	LLUICtrl* ctrlp = self->getChild<LLUICtrl>(ctrl_name.c_str(), true, false);
	if (!ctrlp)
	{
		return false;
	}

	ctrlp->setEnabled(enable);

	return true;
}

//static
bool HBLuaFloater::setControlVisible(const std::string& floater_name,
									 const std::string& ctrl_name,
									 bool visible)
{
	instances_map_t::iterator it = sInstances.find(floater_name);
	if (it == sInstances.end())
	{
		return false;
	}

	HBLuaFloater* self = it->second;

	LLUICtrl* ctrlp = self->getChild<LLUICtrl>(ctrl_name.c_str(), true, false);
	if (!ctrlp)
	{
		return false;
	}

	ctrlp->setVisible(visible);

	return true;
}

HBLuaFloater::HBLuaFloater(const std::string& name,
						   const std::string& parameter)
:	mLuaName(name),
	mParameter(parameter),
	mInitOK(false)
{
	sInstances[mLuaName] = this;
}

//virtual
HBLuaFloater::~HBLuaFloater()
{
	sInstances.erase(mLuaName);
}

//virtual
bool HBLuaFloater::postBuild()
{
	std::string name = getTitle();
	LLStringUtil::trimHead(name);
	LLStringUtil::toLower(name);
	if (name.compare(0, 3, "lua") != 0)
	{
		setTitle("Lua: " + getTitle());
	}

	U32 i = 0;
	LLButton* buttonp;
	while ((buttonp = getChild<LLButton>(llformat("button%d", ++i).c_str(),
										 true, false)))
	{
		buttonp->setCommitCallback(onCommitCallback);
		buttonp->setCallbackUserData(this);
	}

	i = 0;
	LLCheckBoxCtrl* checkp;
	while ((checkp = getChild<LLCheckBoxCtrl>(llformat("check%d", ++i).c_str(),
											  true, false)))
	{
		checkp->setCommitCallback(onCommitCallback);
		checkp->setCallbackUserData(this);
	}

	i = 0;
	LLRadioGroup* radiop;
	while ((radiop = getChild<LLRadioGroup>(llformat("radio%d", ++i).c_str(),
											true, false)))
	{
		radiop->setCommitCallback(onCommitCallback);
		radiop->setCallbackUserData(this);
	}

	i = 0;
	LLComboBox* combop;
	while ((combop = getChild<LLComboBox>(llformat("combo%d", ++i).c_str(),
										  true, false)))
	{
		combop->setCommitCallback(onCommitCallback);
		combop->setCallbackUserData(this);
	}

	i = 0;
	LLFlyoutButton* flyp;
	while ((flyp = getChild<LLFlyoutButton>(llformat("flyout%d", ++i).c_str(),
											true, false)))
	{
		flyp->setCommitCallback(onCommitCallback);
		flyp->setCallbackUserData(this);
	}

	i = 0;
	LLSliderCtrl* sliderp;
	while ((sliderp = getChild<LLSliderCtrl>(llformat("slider%d", ++i).c_str(),
											 true, false)))
	{
		sliderp->setCommitCallback(onCommitCallback);
		sliderp->setCallbackUserData(this);
	}

	i = 0;
	LLSpinCtrl* spinp;
	while ((spinp = getChild<LLSpinCtrl>(llformat("spin%d", ++i).c_str(), true,
										 false)))
	{
		spinp->setCommitCallback(onCommitCallback);
		spinp->setCallbackUserData(this);
	}

	i = 0;
	LLSearchEditor* searchp;
	while ((searchp = getChild<LLSearchEditor>(llformat("searchedit%d",
														++i).c_str(),
											   true, false)))
	{
		searchp->setSearchCallback(onSearchEdit, searchp);
		name = mLuaName + " " + searchp->getName();
		searchp->setCustomMenuType(name.c_str());
	}

	i = 0;
	LLLineEditor* linep;
	while ((linep = getChild<LLLineEditor>(llformat("lineedit%d", ++i).c_str(),
										   true, false)))
	{
		linep->setCommitCallback(onCommitCallback);
		linep->setCallbackUserData(this);
		linep->setCommitOnFocusLost(true);
		name = mLuaName + " " + linep->getName();
		linep->setCustomMenuType(name.c_str());
	}

	i = 0;
	LLTextEditor* textp;
	while ((textp = getChild<LLTextEditor>(llformat("textedit%d", ++i).c_str(),
										   true, false)))
	{
		textp->setCommitCallback(onCommitCallback);
		textp->setCallbackUserData(this);
		textp->setCommitOnFocusLost(true);
		name = mLuaName + " " + textp->getName();
		textp->setCustomMenuType(name.c_str());
	}

	i = 0;
	LLScrollListCtrl* listp;
	while ((listp = getChild<LLScrollListCtrl>(llformat("list%d", ++i).c_str(),
											   true, false)))
	{
		listp->setCommitCallback(onCommitCallback);
		listp->setCallbackUserData(this);
		listp->setCommitOnSelectionChange(true);
	}

	i = 0;
	LLScrollListCtrl* namep;
	while ((namep = getChild<LLNameListCtrl>(llformat("namelist%d",
													  ++i).c_str(),
											 true, false)))
	{
		namep->setCommitCallback(onCommitCallback);
		namep->setCallbackUserData(this);
		namep->setCommitOnSelectionChange(true);
	}

	i = 0;
	LLInventoryPanel* invp;
	while ((invp = getChild<LLInventoryPanel>(llformat("inventory%d",
													   ++i).c_str(),
											  true, false)))
	{
		invp->setSelectCallback(onInventorySelect, this);
	}

	mInitOK = true;

	return true;
}

//virtual
void HBLuaFloater::onOpen()
{
	if (mInitOK && gAutomationp)
	{
		gAutomationp->onLuaFloaterOpen(mLuaName, mParameter);
	}
}

//virtual
void HBLuaFloater::onClose(bool app_quitting)
{
	if (mInitOK && gAutomationp)
	{
		gAutomationp->onLuaFloaterClose(mLuaName, mParameter);
	}
	LLFloater::onClose(app_quitting);	// Calls LLFloater::destroy()
}

bool HBLuaFloater::evalLuaCommand(const std::string& command,
								  std::string value, bool with_close)
{
	bool close = false;

	// Setup floater-specific Lua global variables and functions
	std::string functions = "V_UICTRL_VALUE=\"";
	LLStringUtil::replaceString(value, "\"", "\\\"");
	functions += value + "\";";
	functions += "V_FLOATER_NAME=\"" + mLuaName + "\";";
	value = mParameter;
	LLStringUtil::replaceString(value, "\"", "\\\"");
	functions += "V_FLOATER_PARAM=\"" + value + "\";";
	functions += "function GetValue();return V_UICTRL_VALUE;end;";
	functions += "function GetFloaterName();return V_FLOATER_NAME;end;";
	functions += "function GetFloaterParam();return V_FLOATER_PARAM;end;";
	if (with_close)
	{
		functions += "V_FLOATER_CLOSE=false;";
		functions += "function FloaterClose();V_FLOATER_CLOSE=true;end;";
	}

	HBViewerAutomation lua;
	bool success = lua.loadString(functions + command);
	if (success && with_close)
	{
		lua_State* statep = lua.mLuaState;
		if (statep)
		{
			// Retreive and interpret the global variable value
			lua_getglobal(statep, "V_FLOATER_CLOSE");
			close = lua_toboolean(statep, -1);
		}
	}

	return close;
}

//static
std::string HBLuaFloater::getCtrlValue(LLUICtrl* ctrlp)
{
	LLInventoryPanel* panelp = dynamic_cast<LLInventoryPanel*>(ctrlp);
	if (panelp)
	{
		ctrlp = panelp->getRootFolder();
		if (!ctrlp) return "";
	}
	LLFolderView* invp = dynamic_cast<LLFolderView*>(ctrlp);
	if (invp)
	{
		std::string result;
		const LLFolderView::selected_items_t& items = invp->getSelectedItems();
		LLFolderView::selected_items_t::const_iterator it = items.begin();
		if (it != items.end())
		{
			const LLFolderViewEventListener* listenerp = (*it)->getListener();
			if (listenerp)
			{
				result = listenerp->getUUID().asString();
			}
		}
		return result;
	}

	LLCheckBoxCtrl* checkp = dynamic_cast<LLCheckBoxCtrl*>(ctrlp);
	if (checkp)
	{
		return checkp->get() ? "true": "false";
	}

	return ctrlp->getValue().asString();
}

//static
void HBLuaFloater::getCtrlValues(LLUICtrl* ctrlp,
								 std::vector<std::string>& values)
{
	LLInventoryPanel* panelp = dynamic_cast<LLInventoryPanel*>(ctrlp);
	if (panelp)
	{
		ctrlp = panelp->getRootFolder();
		if (!ctrlp) return;
	}
	LLFolderView* invp = dynamic_cast<LLFolderView*>(ctrlp);
	if (invp)
	{
		const LLFolderView::selected_items_t& items = invp->getSelectedItems();
		for (LLFolderView::selected_items_t::const_iterator it = items.begin(),
															end = items.end();
			 it != end; ++it)
		{
			const LLFolderViewEventListener* listenerp = (*it)->getListener();
			if (listenerp)
			{
				values.emplace_back(listenerp->getUUID().asString());
			}
		}
		return;
	}

	// Note: name list controls share this code, LLNameListCtrl being a derived
	// class of LLScrollListCtrl.
	LLScrollListCtrl* listp = dynamic_cast<LLScrollListCtrl*>(ctrlp);
	if (listp)
	{
		LLScrollListCtrl::items_vec_t items = listp->getAllSelected();
		for (S32 i = 0, count = items.size(); i < count; ++i)
		{
			values.emplace_back(items[i]->getValue().asString());
		}
		return;
	}

	LLCheckBoxCtrl* checkp = dynamic_cast<LLCheckBoxCtrl*>(ctrlp);
	if (checkp)
	{
		values.emplace_back(checkp->get() ? "true": "false");
		return;
	}

	// Valid for other UI element types.
	values.emplace_back(ctrlp->getValue().asString());
}

//static
bool HBLuaFloater::setCtrlValue(LLUICtrl* ctrlp, const std::string& value)
{
	// For line, text and search editors controls, we set their text

	LLLineEditor* lineeditp = dynamic_cast<LLLineEditor*>(ctrlp);
	if (lineeditp)
	{
		lineeditp->setText(value);
		return true;
	}

	LLTextEditor* texteditp = dynamic_cast<LLTextEditor*>(ctrlp);
	if (texteditp)
	{
		texteditp->setText(value);
		return true;
	}

	LLSearchEditor* searcheditp = dynamic_cast<LLSearchEditor*>(ctrlp);
	if (searcheditp)
	{
		searcheditp->setText(value);
		return true;
	}

	// For inventory panels, we set the filter to open a corresponding folder
	// and its descendents only.
	LLInventoryPanel* panelp = dynamic_cast<LLInventoryPanel*>(ctrlp);
	if (panelp)
	{
		LLFolderView* invp = panelp->getRootFolder();
		if (!invp) return false;

		bool is_category = false;
		const LLUUID& cat_id =
			HBViewerAutomation::getInventoryObjectId(value, is_category);
		if (!is_category) return false;

		LLInventoryFilter* filterp = panelp->getFilter();
		if (!filterp) return false;

		if (filterp->isActive())
		{
			// If our filter is active we may be the first thing requiring a
			// fetch in this folder, so we better start it here.
			LLInventoryModelFetch::getInstance()->start(cat_id);
		}

		// Do not open recursively all sub-folders in the target folder.
		invp->setCanAutoSelect(false);
		// But open all folders on the path from root to the target folder.
		LLFolderViewFolder* folderp =
			dynamic_cast<LLFolderViewFolder*>(invp->getItemByID(cat_id));
		LLFolderViewFolder* rootp = dynamic_cast<LLFolderViewFolder*>(invp);
		if (rootp)	// Paranoia
		{
			while (folderp && folderp != rootp)
			{
				invp->setSelection(folderp, false, false);
				folderp->setOpen(true);
				folderp = folderp->getParentFolder();
			}
		}

		panelp->setLastOpenLocked(true);
		panelp->setFilterLastOpen(true);
		panelp->setFilterShowLinks(true);

		filterp->markDefault();
		filterp->setLastOpenID(cat_id);
		filterp->setModified(LLInventoryFilter::FILTER_RESTART);

		return true;
	}

	// For name lists, we support only simple ones (with just one column), and
	// set the new value by UUID, with <GROUP> as a group tag marker.
	LLNameListCtrl* namelistp = dynamic_cast<LLNameListCtrl*>(ctrlp);
	if (namelistp)
	{
		if (value.empty())
		{
			namelistp->clearRows();
			return true;
		}

		bool is_group = false;
		std::string uuid_str = value;
		if (uuid_str.compare(0, 7, "<GROUP>") == 0)
		{
			uuid_str = uuid_str.substr(7);
			is_group = true;
		}
		if (!LLUUID::validate(uuid_str))
		{
			return false;
		}
		if (is_group)
		{
			namelistp->addGroupNameItem(LLUUID(uuid_str));
		}
		else
		{
			namelistp->addNameItem(LLUUID(uuid_str));
		}
		return true;
	}

	// For scroll lists, the case is more complex... We split the string using
	// the pipe character as a separator, to get the various columns from the
	// 'value' string, and set them as a new line for the list.
	LLScrollListCtrl* listp = dynamic_cast<LLScrollListCtrl*>(ctrlp);
	if (listp)
	{
		if (value.empty())
		{
			listp->clearRows();
			return true;
		}

		LLSD element;
		std::vector<std::string> cols;
		boost::split(cols, value, boost::is_any_of("|"));
		std::string col, style;
		for (S32 i = 0, count = cols.size(); i < count; ++i)
		{
			element["columns"][i]["column"] = llformat("col%d", i);
			col = cols[i];

			// Check for font style "<BOLD>" and/or "<ITALIC>" markers
			style.clear();
			if (col.compare(0, 6, "<BOLD>") == 0)
			{
				col = col.substr(6);
				style = "BOLD";
			}
			if (col.compare(0, 8, "<ITALIC>") == 0)
			{
				col = col.substr(8);
				if (!style.empty())
				{
					style += '|';
				}
				style += "ITALIC";
			}
			if (!style.empty())
			{
				element["columns"][i]["font-style"] = style;
			}

			// Check for color <name> or <r,g,b> marker
			if (col.compare(0, 1, "<") == 0)
			{
				size_t j = col.find('>');
				if (j != std::string::npos)
				{
					LLColor4 color;
					if (LLColor4::parseColor(col.substr(1, j - 1), &color))
					{
						col = col.substr(j + 1);
						element["columns"][i]["color"] = color.getValue();
					}
				}
			}

			element["columns"][i]["value"] = col;
		}
		element["id"] = listp->getItemCount();
		listp->addElement(element, ADD_BOTTOM);
		return true;
	}

	// The other control types set with a LLSD-converted value (which may not
	// have any effect with some controls, but we report a success anyway).
	ctrlp->setValue(LLSD(value));

	return true;
}

//static
void HBLuaFloater::onCommitCallback(LLUICtrl* ctrlp, void* datap)
{
	HBLuaFloater* self = (HBLuaFloater*)datap;
	if (!self || !ctrlp || !self->mInitOK) return;

	bool close = false;

	std::string value = getCtrlValue(ctrlp);
	commands_map_t::iterator it = self->mCommands.find(ctrlp);
	if (it != self->mCommands.end())
	{
		bool with_close = dynamic_cast<LLButton*>(ctrlp) != NULL;
		close = self->evalLuaCommand(it->second, value, with_close);
	}
	if (gAutomationp)
	{
		// We want the name of the parent inventory panel, not the name
		// of the folder view (selected) item...
		std::string ctrl_name;
		LLFolderView* folderp = dynamic_cast<LLFolderView*>(ctrlp);
		if (folderp)
		{
			LLPanel* panelp = folderp->getParentPanel();
			if (panelp)
			{
				ctrl_name = panelp->getName();
			}
		}
		else
		{
			ctrl_name = ctrlp->getName();
		}
		gAutomationp->onLuaFloaterAction(self->mLuaName, ctrl_name, value);
	}

	if (close)
	{
		self->close();
	}
}

//static
void HBLuaFloater::onSearchEdit(const std::string& search_string,
								void* datap)
{
	LLSearchEditor* editp = (LLSearchEditor*)datap;
	if (!editp) return;

	// Search the Lua floater this editor pertains to.
	HBLuaFloater* self = NULL;
	LLView* parentp = editp->getParent();
	while (!self && parentp)
	{
		self = parentp->asLuaFloater();
		parentp = parentp->getParent();
	}
	if (!self || !self->mInitOK)
	{
		return;
	}

	commands_map_t::iterator it = self->mCommands.find(editp);
	if (it != self->mCommands.end())
	{
		self->evalLuaCommand(it->second, search_string);
	}
	if (gAutomationp)
	{
		gAutomationp->onLuaFloaterAction(self->mLuaName, editp->getName(),
										 search_string);
	}
}

//static
void HBLuaFloater::onInventorySelect(LLFolderView* ctrlp,
									 bool user_action, void* datap)
{
	// Do the Lua callback only if the commit is the result of a user's action
	// (e.g. a selection with the mouse); avoids callback storms when filtering
	// the inventory, for example...
	if (user_action)
	{
		onCommitCallback(ctrlp, datap);
	}
}

///////////////////////////////////////////////////////////////////////////////
// HBAutomationThread class
///////////////////////////////////////////////////////////////////////////////

// For now, a maximum of eight concurrent threads are permitted.
constexpr S32 MAX_LUA_THREADS = 8;

class HBAutomationThread final : public HBViewerAutomation, public LLThread
{
protected:
	LOG_CLASS(HBAutomationThread);

public:
	LL_INLINE HBAutomationThread()
	:	HBViewerAutomation(true),
		LLThread("Lua thread"),
		mLuaThreadID(++sThreadID),
		mHasSignal(false),
		mRunning(true)
	{
		// Update the LLThread name with our Lua thread Id
		mName = llformat("Lua thread %d", mLuaThreadID);
	}

	// HBViewerAutomation override
	LL_INLINE bool isThreaded() const override		{ return true; }
	LL_INLINE U32 getLuaThreadID() const override	{ return mLuaThreadID; }

	// LLThread overrides
	void run() override;
	LL_INLINE bool runCondition() override			{ return mRunning; }

	LL_INLINE bool isRunning()						{ return mRunning; }

	LL_INLINE void setRunning()
	{
		mRunning = true;
		wake();
	}

	LL_INLINE void setSignal()
	{
		mHasSignal = true;
		wake();
	}

	LL_INLINE void threadStart()
	{
		LLThread::start();
	}

	LL_INLINE void threadStop()
	{
		mWatchdogTimer.start();
		mWatchdogTimer.setTimerExpirySec(0.01f);
		mRunning = true;
		setQuitting();
	}

	LL_INLINE const std::string& getName() const	{ return mName; }

	LL_INLINE bool hasFuncCall() const
	{
		return !mMainFuncCall.empty();
	}

	LL_INLINE const std::string& getFuncCall() const
	{
		return mMainFuncCall;
	}

	LL_INLINE void setFuncCallError(const std::string& err)
	{
		mFuncCallError = err;
	}

	LL_INLINE void appendSignal(const std::string& sig_str)
	{
		mSignals.emplace_back(sig_str);
	}

	int callMainFunction(const std::string& func_name);

	static int getThreadID(lua_State* statep);
	static int sleep(lua_State* statep);

private:
	// This method is to be called to process pending signals. Returns true
	// (and an unchanged Lua stack) on success or false on failure (with the
	// error message on the Lua stack).
	bool processSignals();

private:
	// Set to the name of the function to call by the thread (*before* setting
	// mRunning to false) to signal it is waiting for that call to a function
	// of the automation script. Reset by the thread (after mRunning is reset
	// to true by the automation script) after the call is completed with the
	// result pushed on the thread stack.
	std::string			mMainFuncCall;
	std::string			mFuncCallError;

	// Filled up by the automation idle loop (while mRunning is false), and
	// consummed in run() (while mRunning is true).
	strings_vec_t		mSignals;

	U32					mLuaThreadID;

	// NOTE: we cannot use the mPaused variable of LLThread, because is is not
	// protected by a mutex and is not atomic either, while we need to pause
	// (mRunning = false) from inside the thread and un-pause (mRunning = true)
	// from the main loop...
	LLAtomicBool		mRunning;

	// Set by the automation script idle loop when the thread is caught running
	// (mRunning = true) while we have signals for it. Reset in run() by the
	// thread, after it acknowledged it and put itself in pause mode
	// (mRunning = false).
	LLAtomicBool		mHasSignal;

	static U32			sThreadID;
};

//static
U32 HBAutomationThread::sThreadID = 0;

//virtual
void HBAutomationThread::run()
{
	bool loop;
	do
	{
		// At each loop, sleep 1ms and yield to the OS for threads rescheduling
		ms_sleep(1);
		// The thread could be awaiting for HTTP replies, so yield to its
		// coroutines when any.
		if (mHasOnHTTPReply)
		{
			llcoro::suspend();
		}

		// This will block until runCondition() returns true or the thread
		// leaves the RUNNING state.
		checkPause();
		if (isQuitting() || !mLuaState)
		{
			break;
		}

		if (mHasSignal)
		{
			// Pause and wait for the automation idle loop to send us the
			// pending signal(s)
			mRunning = false;
			mHasSignal = false;  // Acknowledged !
			continue;
		}

		if (!mPrintBuffer.empty())
		{
			// Pause and wait for the automation idle loop to print our stuff
			mRunning = false;
			continue;
		}

		// Process our signals, now...
		if (!processSignals())
		{
			reportError();
			break;
		}

		{
			LL_TRACY_TIMER(TRC_LUA_THREAD_LOOP);

			// Run our main Lua function at each loop
			lua_getglobal(mLuaState, "ThreadRun");
			resetTimer();
			if (lua_pcall(mLuaState, 0, LUA_MULTRET, 0) != LUA_OK)
			{
				reportError();
				break;
			}
			if (lua_gettop(mLuaState) != 1 ||
				lua_type(mLuaState, 1) != LUA_TBOOLEAN)
			{
				lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
				lua_pushliteral(mLuaState,
								"ThreadRun() did not return an unique boolean");
				reportError();
				break;
			}
			loop = lua_toboolean(mLuaState, 1);
			lua_pop(mLuaState, 1);
		}
	}
	while (loop);

	llinfos << "Exiting " << mName << llendl;
}

// This method is used to call viewer-specific Lua functions that are not
// thread-safe and must therefore be executed from the main thread (in an idle
// loop callback) on our thread's behalf.
int HBAutomationThread::callMainFunction(const std::string& func_name)
{
	LL_TRACY_TIMER(TRC_LUA_THREAD_CALL_MAIN_FN);

	// Clear any previous error message (since mFuncCallError is also used as a
	// flag do denote an error during the function call by the automation
	// script).
	mFuncCallError.clear();

	// Signal to the automation idle callback that we have work for it by
	// filling up mMainFuncCall with the name of the function to call.
	mMainFuncCall = func_name;

	// Set the thread as "not running" (Lua processing paused), which will
	// allow the automation script idle callback to process our request.
	mRunning = false;

	// Sleep until allowed to resume running by the idle callback, or quitting
	while (!mRunning && !isQuitting())
	{
		// Since the idle callback is called once per frame, there is no use in
		// sleeping less than 5ms (= 1/2 frame at 100fps) at each loop...
		ms_sleep(5);
	}

	// Reset this now that we are done
	mMainFuncCall.clear();

	if (isQuitting())
	{
		std::string err = getName() + " aborted.";
		luaL_error(mLuaState, err.c_str());
	}

	if (!mFuncCallError.empty())
	{
		luaL_error(mLuaState, mFuncCallError.c_str());
	}

	// Return whatever the idle callback left onto our stack.
	return lua_gettop(mLuaState);
}

bool HBAutomationThread::processSignals()
{
	LL_TRACY_TIMER(TRC_LUA_THREAD_PROCESS_SIG);

	if (mSignals.empty())
	{
		// Nothing to do, report a success.
		return true;
	}
	if (!mHasOnSignal)
	{
		// No OnSignal() callback, so no need to bother and report a success.
		mSignals.clear();
		return true;
	}

	// Copy mSignals on stack (and clear it), because OnSignal() could call
	// custom Lua functions that would make us enter the mRunning = false
	// state, which could, in turn, allow the modification of mSignals by the
	// automation script idle loop...
	strings_vec_t signals_copy;
	signals_copy.swap(mSignals);	// This also empties mSignals
	for (U32 s = 0, count = signals_copy.size(); s < count; ++s)
	{
		std::string& signal_str = signals_copy[s];
		LL_DEBUGS("Lua") << "Processing signal: " << signal_str << LL_ENDL;
		// A signal string is always in the following form:
		// from_lua_thread_id;time_stamp_seconds|serialized_Lua_table
		size_t i = signal_str.find("|");
		std::string temp = signal_str.substr(0, i);
		signal_str = "_V_SIGNAL_TABLE=" + signal_str.substr(i + 1);

		lua_getglobal(mLuaState, "OnSignal");

		i = temp.find(";");
		lua_pushnumber(mLuaState, atoi(temp.substr(0, i).c_str()));
		lua_pushnumber(mLuaState, atof(temp.substr(i + 1).c_str()));

		if (luaL_dostring(mLuaState, signal_str.c_str()) == LUA_OK)
		{
			lua_getglobal(mLuaState, "_V_SIGNAL_TABLE");
			lua_pushnil(mLuaState);
			lua_setglobal(mLuaState, "_V_SIGNAL_TABLE");
		}
		else
		{
			lua_pushliteral(mLuaState, "Could not evaluate the signal table");
			return false;
		}

		resetTimer();
		if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
		{
			return false;
		}

		// Check that we did not get killed during the signal processing...
		if (isQuitting())
		{
			lua_pushliteral(mLuaState, "Thread aborted");
			return false;
		}
	}

	if (lua_gettop(mLuaState))
	{
		lua_pushliteral(mLuaState,
						"OnSignal() returned something when it should not !");
		return false;
	}

	return true;
}

//static
int HBAutomationThread::getThreadID(lua_State* statep)
{
	HBAutomationThread* self = (HBAutomationThread*)findInstance(statep);
	if (!self)
	{
		return 0;
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_pushinteger(statep, self->mLuaThreadID);
	return 1;
}

//static
int HBAutomationThread::sleep(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_THREAD_SLEEP);

	HBAutomationThread* self = (HBAutomationThread*)findInstance(statep);
	if (!self)
	{
		return 0;
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	S32 sleep_time = lua_tointeger(statep, 1);
	lua_pop(statep, 1);
	if (sleep_time < 0)
	{
		luaL_error(statep, "Invalid (negative) sleep time");
	}

	// Always set ourselves as "not running" to let the automation idle loop
	// send us any pending signal and/or print whatever is in our print buffer.
	self->mRunning = false;
	if (self->mHasSignal)
	{
		self->mHasSignal = false;  // Acknowledged !
	}

	// Now wait for at least our sleep time and until permitted to run again.
	do
	{
		// Sleep by 10ms maximum slices (so that we check often enough for any
		// thread abortion), until we exhaust our sleep time
		if (sleep_time > 0)
		{
			if (sleep_time > 10)
			{
				sleep_time -= 10;
				ms_sleep(10);
			}
			else
			{
				ms_sleep(sleep_time);
				sleep_time = 0;
			}
		}
		else if (!self->mRunning)
		{
			// Sleep some more if not yet allowed to run by the automation
			// idle loop...
			ms_sleep(10);
		}

		// Check for any thread abortion request
		if (self->isQuitting())
		{
			std::string err = self->getName() + " aborted.";
			luaL_error(statep, err.c_str());
		}
		// Extend our grace period since we just checked for exit conditions
		self->resetTimer();
	}
	while (sleep_time > 0 || !self->mRunning);

	// Process pending signals, if any.
	if (!self->processSignals())
	{
		// Let's use luaL_error() so to abort the Lua script.
		std::string message(lua_tostring(statep, -1));
		luaL_error(statep, message.c_str());
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// HBFriendsStatusObserver helper class
// Used for the automation script to observe friends-related events and call
// the OnFriendsStatus() Lua callback in consequence.
///////////////////////////////////////////////////////////////////////////////
class HBFriendsStatusObserver final : public LLFriendObserver
{
protected:
	LOG_CLASS(HBFriendsStatusObserver);

public:
	HBFriendsStatusObserver()
	:	mMask(0)
	{
		gAvatarTracker.addObserver(this);
	}

	~HBFriendsStatusObserver() override
	{
		gAvatarTracker.removeObserver(this);
	}

	LL_INLINE void changed(U32 mask) override
	{
		mMask = mask;
	}

	void changedBuddies(const uuid_list_t& buddies) override
	{
		if (!gAutomationp)
		{
			return;
		}
		for (uuid_list_t::const_iterator it = buddies.begin(),
										 end = buddies.end();
			 it != end; ++it)
		{
			const LLUUID& id = *it;
			bool online = gAvatarTracker.isBuddyOnline(id);
			gAutomationp->onFriendStatusChange(id, mMask, online);
		}
	}

private:
	U32 mMask;
};

///////////////////////////////////////////////////////////////////////////////
// HBGroupTitlesObserver helper class
// Used by setAgentGroup() to set asynchronously the group title after
// receiving the appropriate data.
///////////////////////////////////////////////////////////////////////////////
class HBGroupTitlesObserver final : public LLGroupMgrObserver
{
protected:
	LOG_CLASS(HBGroupTitlesObserver);

public:
	~HBGroupTitlesObserver() override
	{
		gGroupMgr.removeObserver(this);
		sObservers.erase(mGroupId);
	}

	void changed(LLGroupChange gc) override
	{
		bool success = false;

		LLGroupMgrGroupData* gdatap = gGroupMgr.getGroupData(mGroupId);
		if (gdatap)	// Still a member of this group ?
		{
			if (gc != GC_TITLES)
			{
				return;	// Not interested in this type of changes...
			}

			for (U32 i = 0, count = gdatap->mTitles.size(); i < count; ++i)
			{
				const LLGroupTitle& title = gdatap->mTitles[i];
				if (title.mTitle == mTitleName)
				{
					if (gAgent.setGroup(mGroupId))
					{
						LL_DEBUGS("Lua") << "Setting requested agent group and role ("
										 << mTitleName << ")" << LL_ENDL;
						gGroupMgr.sendGroupTitleUpdate(mGroupId,
													   title.mRoleID);
						success = true;
					}
					break;
				}
			}
		}

		if (!success)
		{
			LL_DEBUGS("Lua") << "Failed to set agent group and role ("
							 << mTitleName << ")" << LL_ENDL;
		}

		// Commit suicide once we are no more needed.
		delete this;
	}

	static HBGroupTitlesObserver* addObserver(const LLUUID& group_id,
											  const std::string& title)
	{
		group_obs_map_t::iterator it = sObservers.find(group_id);

		if (it != sObservers.end())
		{
			// If we already got an observer, just update the desired group
			// title for it...
			it->second->mTitleName = title;
			return it->second;
		}

		// Create a new observer
		return new HBGroupTitlesObserver(group_id, title);
	}

	static void deleteObservers()
	{
		if (!sObservers.empty())
		{
			for (group_obs_map_t::iterator it = sObservers.begin(),
										   end = sObservers.end();
				 it != end; ++it)
			{
				delete it->second;
			}
			sObservers.clear();
		}
	}

private:
	// Use addObserver() to construct instances of this class
	HBGroupTitlesObserver(const LLUUID& group_id, const std::string& title)
	:	LLGroupMgrObserver(group_id),
		mGroupId(group_id),
		mTitleName(title)
	{
		sObservers.emplace(group_id, this);
		gGroupMgr.addObserver(this);
	}

private:
	LLUUID					mGroupId;
	std::string				mTitleName;

	typedef fast_hmap<LLUUID, HBGroupTitlesObserver*> group_obs_map_t;
	static group_obs_map_t	sObservers;
};

HBGroupTitlesObserver::group_obs_map_t HBGroupTitlesObserver::sObservers;

///////////////////////////////////////////////////////////////////////////////
// HBIgnoreCallback helper class to prevent infinite loops in Lua callbacks
///////////////////////////////////////////////////////////////////////////////

class HBIgnoreCallback
{
protected:
	LOG_CLASS(HBIgnoreCallback);

public:
	HBIgnoreCallback(S32 callback_code)
	:	mCallbackCode(callback_code)
	{
		++HBViewerAutomation::sIgnoredCallbacks[callback_code];
	}

	~HBIgnoreCallback()
	{
		if (--HBViewerAutomation::sIgnoredCallbacks[mCallbackCode] < 0)
		{
			llwarns << "Invocations count mismatch for callback: "
					<< mCallbackCode << llendl;
			llassert(false);
			HBViewerAutomation::sIgnoredCallbacks[mCallbackCode] = 0;
		}
	}

private:
	S32 mCallbackCode;
};

///////////////////////////////////////////////////////////////////////////////
// HBViewerAutomation class
///////////////////////////////////////////////////////////////////////////////

//static
HBViewerAutomation::instances_map_t HBViewerAutomation::sInstances;
LLMutex HBViewerAutomation::sThreadsMutex;
HBViewerAutomation::threads_list_t HBViewerAutomation::sThreadsInstances;
HBViewerAutomation::threads_list_t HBViewerAutomation::sDeadThreadsInstances;
HBViewerAutomation::signals_map_t HBViewerAutomation::sThreadsSignals;
std::string HBViewerAutomation::sLastAutomationScriptFile;
uuid_list_t HBViewerAutomation::sMuteObjectRequests;
uuid_list_t HBViewerAutomation::sUnmuteObjectRequests;
S32 HBViewerAutomation::sIgnoredCallbacks[HBViewerAutomation::E_IGN_CB_COUNT];
LLFriendObserver* HBViewerAutomation::sFriendsObserver = NULL;
HBViewerAutomation::pos_history_t HBViewerAutomation::sPositionsHistory;
#if LL_LINUX
LLUUID HBViewerAutomation::sLuaDBusFakeObjectId;
#endif

//static
void HBViewerAutomation::start(std::string file_name)
{
	if (file_name.empty())
	{
		file_name = sLastAutomationScriptFile;
	}
	if (file_name.empty())
	{
		llwarns << "No file name given for automation script. Aborted."
				<< llendl;
		return;
	}
	sLastAutomationScriptFile = file_name;

	if (gAutomationp)
	{
		cleanup();
		llinfos << "Restarting Lua automation..." << llendl;
	}
	else
	{
		llinfos << "Initializing Lua automation..." << llendl;
	}

	gAutomationp = new HBViewerAutomation();
	if (gAutomationp->load(file_name))
	{
		if (gAutomationp->mHasCallbacks)
		{
			for (S32 i = 0; i < E_IGN_CB_COUNT; ++i)
			{
				sIgnoredCallbacks[i] = 0;
			}
			llinfos << "Initialisation successful." << llendl;
			if (LLStartUp::isLoggedIn())
			{
				gAutomationp->onLogin();
			}
			return;
		}
		else
		{
			llinfos << "Lua script executed successfully, no callback found. Closing."
					<< llendl;
		}
	}
	else
	{
		llwarns << "Initialisation failed !" << llendl;
	}
	LLEditMenuHandler::setCustomCallback(NULL);
	delete gAutomationp;
	gAutomationp = NULL;
}

//static
void HBViewerAutomation::cleanup()
{
	if (gAutomationp)
	{
		llinfos << "Stopping Lua automation." << llendl;
		LLEditMenuHandler::setCustomCallback(NULL);
		delete gAutomationp;
		gAutomationp = NULL;
	}

	if (!sDeadThreadsInstances.empty())
	{
		llinfos << "Trying to clean-up dead thread instances..." << llendl;
		for (threads_list_t::iterator it = sDeadThreadsInstances.begin(),
									  end = sDeadThreadsInstances.end();
			 it != end; )
		{
			threads_list_t::iterator curit = it++;
			HBAutomationThread* threadp = curit->second;
			bool stopped = threadp->isStopped();
			for (U32 i = 0; !stopped && i < 10; ++i)
			{
				// It was already set quitting, but this will also wake it up
				threadp->threadStop();
				// Give it some more time...
				ms_sleep(10);
				stopped = threadp->isStopped();
			}
			if (stopped)
			{
				LL_DEBUGS("Lua") << "Deleting stopped thread: "
								 << threadp->getName() << LL_ENDL;
				delete threadp;
				sDeadThreadsInstances.erase(curit);
			}
			else
			{
				llwarns << "Timed out waiting for '" << threadp->getName()
						<< "' to stop" << llendl;
			}
		}
		if (sDeadThreadsInstances.empty())
		{
			llinfos << "All dead threads successfully removed." << llendl;
		}
	}
}

//static
std::string HBViewerAutomation::eval(const std::string& chunk,
									 bool use_print_buffer,
									 const LLUUID& id, const std::string& name)
{
	LL_TRACY_TIMER(TRC_LUA_EVAL);

	if (chunk.empty())
	{
		return "";
	}
	LL_DEBUGS("Lua") << "Executing Lua command line: " << chunk << LL_ENDL;
	HBViewerAutomation self(use_print_buffer);
	if (id.notNull())
	{
		LL_DEBUGS("Lua") << "Originator object: " << name << " (" << id
						 << ")" << LL_ENDL;
		self.mFromObjectId = id;
		self.mFromObjectName = name;
	}
	self.loadString(chunk);
	print(self.mLuaState);
	return self.mPrintBuffer;
}

//static
bool HBViewerAutomation::checkLuaCommand(const std::string& message,
										 const LLUUID& from_object_id,
										 const std::string& from_object_name)
{
	static LLCachedControl<bool> scripts_cmd(gSavedSettings,
											 "LuaAcceptScriptCommands");
	if (!scripts_cmd)
	{
		return false;
	}

	static LLCachedControl<std::string> prefix(gSavedSettings,
											   "LuaScriptCommandPrefix");
	size_t len = std::string(prefix).size();
	if (!len)
	{
		return false;
	}
	if (message.compare(0, len, prefix) != 0)
	{
		return false;
	}

	eval(message.substr(len), false, from_object_id, from_object_name);
	return true;
}

//static
void HBViewerAutomation::execute(const std::string& file_name)
{
	HBViewerAutomation self;

	// Allow a relaxed watchdog timeout for one-shot scripts loaded from files.
	static LLCachedControl<F32> timeout(gSavedSettings,
										"LuaTimeoutForScriptFile");
	self.mWatchdogTimeout = llclamp((F32)timeout, 0.01f, 30.f);

	if (self.load(file_name))
	{
		llinfos << "Lua script '" << file_name << "' executed successfully."
				<< llendl;
	}
	else
	{
		llwarns << "Lua script '" << file_name << "' failed !" << llendl;
	}
}

//static
HBViewerAutomation* HBViewerAutomation::findInstance(lua_State* statep)
{
	if (statep)
	{
		instances_map_t::iterator it = sInstances.find(statep);
		if (it != sInstances.end())
		{
			return it->second;
		}
	}
	return NULL;
}

HBViewerAutomation::HBViewerAutomation(bool use_print_buffer, bool for_console)
:	mUsePrintBuffer(use_print_buffer),
	mUseConsoleOutput(for_console),
	mPausedWarnings(false),
	mForceWarningsToChat(false),
	mFromObjectName("Lua script"),
	mFromObjectId(gAgentID)
{
	if (isThreaded())
	{
		mUsePrintBuffer = true;
		mWatchdogTimeout = 0.5f;
	}
	else
	{
		static LLCachedControl<F32> lua_timeout(gSavedSettings, "LuaTimeout");
		mWatchdogTimeout = llclamp((F32)lua_timeout, 0.01f, 2.f);
	}
	resetCallbackFlags();

	mLuaState = luaL_newstate();
	if (mLuaState)
	{
		luaL_requiref(mLuaState, "_G", luaopen_base, 1);
		luaL_requiref(mLuaState, LUA_TABLIBNAME, luaopen_table, 1);
		luaL_requiref(mLuaState, LUA_STRLIBNAME, luaopen_string, 1);
		luaL_requiref(mLuaState, LUA_MATHLIBNAME, luaopen_math, 1);
		luaL_requiref(mLuaState, LUA_UTF8LIBNAME, luaopen_utf8, 1);
		lua_settop(mLuaState, 0);
		sInstances[mLuaState] = this;
		LL_DEBUGS("Lua") << "Created new Lua state: " << std::hex << mLuaState
				<< std::dec << LL_ENDL;
	}
	else
	{
		llwarns << "Failure to allocate a new Lua state !" << llendl;
		llassert(false);
	}
}

HBViewerAutomation::~HBViewerAutomation()
{
	if (mHasOnAlertDialog)
	{
		// Un-register the Lua callback for alert dialogs. HB
		LLAlertDialog::setLuaCallback(NULL);
	}
	if (mHasOnFailedTPSimChange)
	{
		gIdleCallbacks.deleteFunction(onIdleSimChange, this);
	}
	if (mRegionChangedConnection.connected())
	{
		mRegionChangedConnection.disconnect();
	}
	if (mParcelChangedConnection.connected())
	{
		mParcelChangedConnection.disconnect();
	}
	if (mPositionChangedConnection.connected())
	{
		mPositionChangedConnection.disconnect();
	}
	if (mLuaState)
	{
		sInstances.erase(mLuaState);
		lua_settop(mLuaState, 0);
		lua_close(mLuaState);
		mLuaState = NULL;
	}

	if (this != gAutomationp)
	{
		// If we are not the automation script instance, then we are done !
		return;
	}

	if (sFriendsObserver)
	{
		delete sFriendsObserver;
		sFriendsObserver = NULL;
	}

	HBGroupTitlesObserver::deleteObservers();

	// Do not close the opened UI elements if the automation script does not
	// have any callback (it was likely just used to setup those UI elements).
	if (mHasCallbacks)
	{
		if (gLuaSideBarp)
		{
			gLuaSideBarp->removeAllButtons();
		}
		if (gLuaPiep)
		{
			gLuaPiep->removeAllSlices();
		}
		if (gOverlayBarp)
		{
			gOverlayBarp->setLuaFunctionButton("", "", "");
		}
		if (gStatusBarp)
		{
			gStatusBarp->setLuaFunctionButton("", "");
		}
	}

	sThreadsMutex.lock();
	if (!sThreadsInstances.empty())
	{
		gIdleCallbacks.deleteFunction(onIdleThread, this);
		sThreadsSignals.clear();	// Abort any pending signal
		for (threads_list_t::iterator it = sThreadsInstances.begin(),
									  end = sThreadsInstances.end();
			 it != end; ++it)
		{
			HBAutomationThread* threadp = it->second;
			if (!threadp->isStopped())
			{
				threadp->threadStop();
			}
		}
		mWatchdogTimer.start();
		mWatchdogTimer.setTimerExpirySec(0.1f);
		while (!sThreadsInstances.empty() && !mWatchdogTimer.hasExpired())
		{
			for (threads_list_t::iterator it = sThreadsInstances.begin(),
										  end = sThreadsInstances.end();
				it != end; )
			{
				threads_list_t::iterator curit = it++;
				HBAutomationThread* threadp = curit->second;
				if (threadp->isStopped())
				{
					LL_DEBUGS("Lua") << "Deleting stopped thread: "
									 << threadp->getName() << LL_ENDL;
					delete threadp;
					sThreadsInstances.erase(curit);
				}
			}
			ms_sleep(1);
		}
		if (!sThreadsInstances.empty())
		{
			llwarns << "Could not stop all running threads before timeout..."
					<< llendl;
			sDeadThreadsInstances.clear();
			sDeadThreadsInstances.swap(sThreadsInstances);
		}
	}
	sThreadsMutex.unlock();
}


void HBViewerAutomation::resetCallbackFlags()
{
	mHasCallbacks = mHasOnSignal = mHasOnLogin = mHasOnRegionChange =
					mHasOnParcelChange = mHasOnPositionChange =
					mHasOnAveragedFPS = mHasOnAgentOccupationChange =
					mHasOnAgentPush = mHasOnSendChat = mHasOnReceivedChat =
					mHasOnChatTextColoring = mHasOnInstantMsg =
					mHasOnAlertDialog = mHasOnScriptDialog =
					mHasOnNotification = mHasOnGroupNotification =
					mHasOnSLURLDispatch = mHasOnURLDispatch =
					mHasOnFriendStatusChange = mHasOnAvatarRezzing =
					mHasOnAgentBaked = mHasOnObjectAttached = mHasOnRadar =
					mHasOnRadarSelection = mHasOnRadarMark = mHasOnRadarTrack =
					mHasOnLuaDialogClose = mHasOnSideBarVisibilityChange =
					mHasOnLuaFloaterAction = mHasOnLuaFloaterOpen =
					mHasOnLuaFloaterClose = mHasOnAutomationMessage =
					mHasOnAutomationRequest = mHasOnAutoPilotFinished =
					mHasOnTPStateChange = mHasOnFailedTPSimChange =
					mHasOnWindlightChange = mHasOnCameraModeChange =
					mHasOnJoystickButtons = mHasOnLuaPieMenu =
					mHasOnContextMenu = mHasOnRLVHandleCommand =
					mHasOnRLVAnswerOnChat = mHasOnObjectInfoReply =
					mHasOnPickInventoryItem = mHasOnPickAvatar =
					mHasOnHTTPReply = false;
}

void HBViewerAutomation::reportError()
{
	if (!mLuaState)	// Paranoia
	{
		return;
	}

	std::string message(lua_tostring(mLuaState, -1));
	lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
	llwarns << "Lua error: " << message << llendl;

	// NOTE: we need verify we have logged in before printing in chat, since
	// we otherwise could crash due to LLFloaterChat not yet being constructed.
	if (mUsePrintBuffer || !LLStartUp::isLoggedIn())
	{
		// Overwrite any existing contents with the error message
		mPrintBuffer = message;
		return;
	}
	if (!mUseConsoleOutput || !HBLuaConsole::print(message))
	{
		LLChat chat;
		chat.mFromName = "Lua";
		chat.mText = "Lua: " + message;
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		LLFloaterChat::addChat(chat, false, false);
	}
}

//static
void HBViewerAutomation::reportWarning(void* data, const char* msg,
									   int to_continue)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	lua_State* statep = (lua_State*)data;
	HBViewerAutomation* self = findInstance(statep);
	if (!self || !msg || !*msg)
	{
		return;
	}

	std::string message(msg);

	// Check for Lua warning system control messages: only "@on" and "@off" are
	// standard messages.
	if (message[0] == '@')
	{
		if (message == "@on")
		{
			self->mPausedWarnings = false;
		}
		else if (message == "@off")
		{
			self->mPausedWarnings = true;
		}
		else if (message == "@prefix")
		{
			self->mWarningPrefix.clear();
		}
		else if (message.compare(0, 8, "@prefix:") == 0)
		{
			if (message.size() > 8)
			{
				self->mWarningPrefix = message.substr(8);
				LLStringUtil::trim(self->mWarningPrefix);
			}
			else
			{
				self->mWarningPrefix.clear();
			}
		}
		else if (message == "@tochat")
		{
			self->mForceWarningsToChat = !self->isThreaded();
		}
		else if (message.compare(0, 8, "@tochat:") == 0)
		{
			self->mForceWarningsToChat = !self->isThreaded() &&
										 message != "@tochat:0" &&
										 message != "@tochat:false" &&
										 message != "@tochat:off";
		}
		if (to_continue || self->mPausedWarnings ||
			self->mPendingWarningText.empty())
		{
			return;	// Nothing to print right now.
		}
		message.clear();	// Do not print the control message itself !
	}

	if (to_continue || self->mPausedWarnings)
	{
		self->mPendingWarningText += message;
		return;
	}

	if (!self->mPendingWarningText.empty())
	{
		message = self->mPendingWarningText + message;
		self->mPendingWarningText.clear();
	}
	LL_DEBUGS("Lua") << "Lua warning: " << message << LL_ENDL;

	if (self->mWarningPrefix.empty())
	{
		message = "WARNING: " + message;
	}
	else
	{
		message = self->mWarningPrefix + ": " + message;
	}

	if ((self->mUsePrintBuffer && !self->mForceWarningsToChat) ||
		!LLStartUp::isLoggedIn())
	{
		if (!self->mPrintBuffer.empty())
		{
#if LL_WINDOWS
			self->mPrintBuffer += "\r\n";
#else
			self->mPrintBuffer += '\n';
#endif
		}
		self->mPrintBuffer += message;
		return;
	}

	if (!self->mUseConsoleOutput || !HBLuaConsole::print(message))
	{
		LLChat chat;
		chat.mFromName = "Lua";
		chat.mText = "Lua: " + message;
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		LLFloaterChat::addChat(chat, false, false);
	}
}

bool HBViewerAutomation::registerCFunctions()
{
	static const struct luaL_Reg printlib[] = {
  		{ "print", HBViewerAutomation::print },
		{ NULL, NULL }
	};

	if (!mLuaState)	// Paranoia
	{
		return false;
	}

	// Register a warning callback
	lua_setwarnf(mLuaState, reportWarning, mLuaState);

	// This registers our custom print(), overriding Lua's, and disables
	// load(), loadfile() and dofile().
	lua_getglobal(mLuaState, "_G");
	luaL_setfuncs(mLuaState, printlib, 0);
	lua_pushnil(mLuaState);
	lua_setfield(mLuaState, -2, "load");
	lua_pushnil(mLuaState);
	lua_setfield(mLuaState, -2, "loadfile");
	lua_pushnil(mLuaState);
	lua_setfield(mLuaState, -2, "dofile");
	lua_setglobal(mLuaState, "_G");

	// Set some useful global variables so that Lua scripts know what viewer
	// they are running within.
	lua_pushstring(mLuaState, gSecondLife.c_str());
	lua_setglobal(mLuaState, "VIEWER_NAME");
	lua_pushstring(mLuaState, gViewerVersionString.c_str());
	lua_setglobal(mLuaState, "VIEWER_VERSION");
	lua_pushinteger(mLuaState, gViewerVersionNumber);
	lua_setglobal(mLuaState, "VIEWER_VERNUM");

	// We setup Lua so that it calls our watchdog every 500 operations (which
	// should be small enough a number, even on slow computers).
	lua_sethook(mLuaState, watchdog, LUA_MASKCOUNT, 500);

	// Register our custom Lua functions
	lua_register(mLuaState, "GetSourceFileName", getSourceFileName);
	lua_register(mLuaState, "GetWatchdogState", getWatchdogState);
	lua_register(mLuaState, "IsUUID", isUUID);
	lua_register(mLuaState, "IsAvatar", isAvatar);
	lua_register(mLuaState, "IsObject", isObject);
	lua_register(mLuaState, "IsAgentFriend", isAgentFriend);
	lua_register(mLuaState, "IsAgentGroup", isAgentGroup);
	lua_register(mLuaState, "GetAvatarName", getAvatarName);
	lua_register(mLuaState, "GetGroupName", getGroupName);
	lua_register(mLuaState, "IsAdmin", isAdmin);
	lua_register(mLuaState, "GetRadarList", getRadarList);
	lua_register(mLuaState, "GetRadarData", getRadarData);
	lua_register(mLuaState, "SetRadarTracking", setRadarTracking);
	lua_register(mLuaState, "SetRadarToolTip", setRadarToolTip);
	lua_register(mLuaState, "SetRadarMarkChar", setRadarMarkChar);
	lua_register(mLuaState, "SetRadarMarkColor", setRadarMarkColor);
	lua_register(mLuaState, "SetRadarNameColor", setRadarNameColor);
	lua_register(mLuaState, "SetAvatarMinimapColor", setAvatarMinimapColor);
	lua_register(mLuaState, "SetAvatarNameTagColor", setAvatarNameTagColor);
	lua_register(mLuaState, "GetAgentPosHistory", getAgentPosHistory);
	lua_register(mLuaState, "GetAgentInfo", getAgentInfo);
	lua_register(mLuaState, "SetAgentOccupation", setAgentOccupation);
	lua_register(mLuaState, "GetAgentGroupData", getAgentGroupData);
	lua_register(mLuaState, "SetAgentGroup", setAgentGroup);
	lua_register(mLuaState, "AgentGroupInvite", agentGroupInvite);
	lua_register(mLuaState, "AgentSit", agentSit);
	lua_register(mLuaState, "AgentStand", agentStand);
	lua_register(mLuaState, "SetAgentTyping", setAgentTyping);
	lua_register(mLuaState, "SendChat", sendChat);
	lua_register(mLuaState, "GetIMSession", getIMSession);
	lua_register(mLuaState, "SendIM", sendIM);
	lua_register(mLuaState, "AlertDialogResponse", alertDialogResponse);
	lua_register(mLuaState, "ScriptDialogResponse", scriptDialogResponse);
	lua_register(mLuaState, "NotificationResponse", scriptDialogResponse);
	lua_register(mLuaState, "GroupNotificationAcceptOffer",
							 groupNotificationAcceptOffer);
	lua_register(mLuaState, "CancelNotification", cancelNotification);
	lua_register(mLuaState, "BrowseToURL", browseToURL);
	lua_register(mLuaState, "DispatchSLURL", dispatchSLURL);
	lua_register(mLuaState, "ExecuteRLV", executeRLV);
	lua_register(mLuaState, "OpenNotification", openNotification);
	lua_register(mLuaState, "OpenFloater", openFloater);
	lua_register(mLuaState, "CloseFloater", closeFloater);
	lua_register(mLuaState, "MakeDialog", makeDialog);
	lua_register(mLuaState, "OpenLuaFloater", openLuaFloater);
	lua_register(mLuaState, "ShowLuaFloater", showLuaFloater);
	lua_register(mLuaState, "SetLuaFloaterCommand", setLuaFloaterCommand);
	lua_register(mLuaState, "GetLuaFloaterListLine", getLuaFloaterListLine);
	lua_register(mLuaState, "GetLuaFloaterValue", getLuaFloaterValue);
	lua_register(mLuaState, "GetLuaFloaterValues", getLuaFloaterValues);
	lua_register(mLuaState, "SetLuaFloaterValue", setLuaFloaterValue);
	lua_register(mLuaState, "SetLuaFloaterInvFilter", setLuaFloaterInvFilter);
	lua_register(mLuaState, "SetLuaFloaterEnabled", setLuaFloaterEnabled);
	lua_register(mLuaState, "SetLuaFloaterVisible", setLuaFloaterVisible);
	lua_register(mLuaState, "CloseLuaFloater", closeLuaFloater);
	lua_register(mLuaState, "OverlayBarLuaButton", overlayBarLuaButton);
	lua_register(mLuaState, "StatusBarLuaIcon", statusBarLuaIcon);
	lua_register(mLuaState, "SideBarButton", sideBarButton);
	lua_register(mLuaState, "SideBarButtonToggle", sideBarButtonToggle);
	lua_register(mLuaState, "SideBarHide", sideBarHide);
	lua_register(mLuaState, "SideBarHideOnRightClick",
				 sideBarHideOnRightClick);
	lua_register(mLuaState, "SideBarButtonHide", sideBarButtonHide);
	lua_register(mLuaState, "SideBarButtonDisable", sideBarButtonDisable);
	lua_register(mLuaState, "LuaPieMenuSlice", luaPieMenuSlice);
	lua_register(mLuaState, "LuaContextMenu", luaContextMenu);
	lua_register(mLuaState, "PasteToContextHandler", pasteToContextHandler);
	lua_register(mLuaState, "PlayUISound", playUISound);
	lua_register(mLuaState, "RenderDebugInfo", renderDebugInfo);
	lua_register(mLuaState, "GetDebugSetting", getDebugSetting);
	lua_register(mLuaState, "SetDebugSetting", setDebugSetting);
	lua_register(mLuaState, "GetFrameTimeSeconds", getFrameTimeSeconds);
	lua_register(mLuaState, "GetTimeSinceEpoch", getTimeSinceEpoch);
	lua_register(mLuaState, "GetTimeStamp", getTimeStamp);
	lua_register(mLuaState, "GetClipBoardString", getClipBoardString);
	lua_register(mLuaState, "SetClipBoardString", setClipBoardString);
	lua_register(mLuaState, "FindInventoryObject", findInventoryObject);
	lua_register(mLuaState, "GiveInventory", giveInventory);
	lua_register(mLuaState, "MakeInventoryLink", makeInventoryLink);
	lua_register(mLuaState, "DeleteInventoryLink", deleteInventoryLink);
	lua_register(mLuaState, "NewInventoryFolder", newInventoryFolder);
	lua_register(mLuaState, "ListInventoryFolder", listInventoryFolder);
	lua_register(mLuaState, "GetAgentAttachments", getAgentAttachments);
	lua_register(mLuaState, "RemoveAgentAttachment", removeAgentAttachment);
	lua_register(mLuaState, "GetAgentWearables", getAgentWearables);
	lua_register(mLuaState, "AgentAutoPilotToPos", agentAutoPilotToPos);
	lua_register(mLuaState, "AgentAutoPilotFollow", agentAutoPilotFollow);
	lua_register(mLuaState, "AgentAutoPilotStop", agentAutoPilotStop);
	lua_register(mLuaState, "AgentAutoPilotLoad", agentAutoPilotLoad);
	lua_register(mLuaState, "AgentAutoPilotSave", agentAutoPilotSave);
	lua_register(mLuaState, "AgentAutoPilotRemove", agentAutoPilotRemove);
	lua_register(mLuaState, "AgentAutoPilotRecord", agentAutoPilotRecord);
	lua_register(mLuaState, "AgentAutoPilotReplay", agentAutoPilotReplay);
	lua_register(mLuaState, "AgentRotate", agentRotate);
	lua_register(mLuaState, "GetAgentRotation", getAgentRotation);
	lua_register(mLuaState, "TeleportAgentHome", teleportAgentHome);
	lua_register(mLuaState, "TeleportAgentToPos", teleportAgentToPos);
	lua_register(mLuaState, "GetGridSimAndPos", getGridSimAndPos);
	lua_register(mLuaState, "GetParcelInfo", getParcelInfo);
	lua_register(mLuaState, "GetCameraMode", getCameraMode);
	lua_register(mLuaState, "SetCameraMode", setCameraMode);
	lua_register(mLuaState, "SetCameraFocus", setCameraFocus);
	lua_register(mLuaState, "SetVisualMute", setVisualMute);
	lua_register(mLuaState, "AddMute", addMute);
	lua_register(mLuaState, "RemoveMute", removeMute);
	lua_register(mLuaState, "IsMuted", isMuted);
	lua_register(mLuaState, "BlockSound", blockSound);
	lua_register(mLuaState, "IsBlockedSound", isBlockedSound);
	lua_register(mLuaState, "GetBlockedSounds", getBlockedSounds);
	lua_register(mLuaState, "DerenderObject", derenderObject);
	lua_register(mLuaState, "GetDerenderedObjects", getDerenderedObjects);
	lua_register(mLuaState, "GetAgentPushes", getAgentPushes);
	lua_register(mLuaState, "ApplyDaySettings", applyDaySettings);
	lua_register(mLuaState, "ApplySkySettings", applySkySettings);
	lua_register(mLuaState, "ApplyWaterSettings", applyWaterSettings);
	lua_register(mLuaState, "SetDayTime", setDayTime);
	lua_register(mLuaState, "GetEESettingsList", getEESettingsList);
	lua_register(mLuaState, "GetWLSettingsList", getWLSettingsList);
	lua_register(mLuaState, "GetEnvironmentStatus", getEnvironmentStatus);
	lua_register(mLuaState, "EncodeJSON", encodeJSON);
	lua_register(mLuaState, "DecodeJSON", decodeJSON);
	lua_register(mLuaState, "EncodeBase64", encodeBase64);
	lua_register(mLuaState, "DecodeBase64", decodeBase64);
	if (isThreaded())
	{
		lua_register(mLuaState, "GetThreadID",
					 HBAutomationThread::getThreadID);
		lua_register(mLuaState, "Sleep", HBAutomationThread::sleep);
		lua_register(mLuaState, "HasThread", hasThread);
		lua_register(mLuaState, "SendSignal", sendSignal);
		// Also allow CloseIMSession() and GetObjectInfo() in threads
		lua_register(mLuaState, "CloseIMSession", closeIMSession);
		lua_register(mLuaState, "GetAgentFriends", getAgentFriends);
		lua_register(mLuaState, "GetAgentGroups", getAgentGroups);
		lua_register(mLuaState, "GetObjectInfo", getObjectInfo);
		lua_register(mLuaState, "GetFloaterInstances", getFloaterInstances);
		lua_register(mLuaState, "GetFloaterControls", getFloaterControls);
		lua_register(mLuaState, "GetFloaterCtrlState", getFloaterCtrlState);
		lua_register(mLuaState, "GetFloaterList", getFloaterList);
		lua_register(mLuaState, "ShowFloater", showFloater);
		lua_register(mLuaState, "ReadTextFromFile", readTextFromFile);
		lua_register(mLuaState, "WriteTextToFile", writeTextToFile);
		lua_register(mLuaState, "GetFileSize", getFileSize);
		lua_register(mLuaState, "RemoveFile", removeFile);
		lua_register(mLuaState, "CreateDirectory", createDirectory);
		lua_register(mLuaState, "RemoveDirectory", removeDirectory);
	}
	else if (this == gAutomationp || mFromObjectId == gAgentID)
	{
#if LL_PUPPETRY
		lua_register(mLuaState, "AgentPuppetryStart", agentPuppetryStart);
		lua_register(mLuaState, "AgentPuppetryStop", agentPuppetryStop);
#endif
		lua_register(mLuaState, "CloseIMSession", closeIMSession);
		lua_register(mLuaState, "GetAgentFriends", getAgentFriends);
		lua_register(mLuaState, "GetAgentGroups", getAgentGroups);
		lua_register(mLuaState, "GetObjectInfo", getObjectInfo);
		lua_register(mLuaState, "GetGlobalData", getGlobalData);
		lua_register(mLuaState, "SetGlobalData", setGlobalData);
		lua_register(mLuaState, "GetPerAccountData", getPerAccountData);
		lua_register(mLuaState, "SetPerAccountData", setPerAccountData);
		lua_register(mLuaState, "PickAvatar", pickAvatar);
		lua_register(mLuaState, "MoveToInventoryFolder",
					 moveToInventoryFolder);
		lua_register(mLuaState, "PickInventoryItem", pickInventoryItem);
		lua_register(mLuaState, "GetFloaterInstances", getFloaterInstances);
		lua_register(mLuaState, "GetFloaterControls", getFloaterControls);
		lua_register(mLuaState, "GetFloaterCtrlState", getFloaterCtrlState);
		lua_register(mLuaState, "GetFloaterList", getFloaterList);
		lua_register(mLuaState, "ShowFloater", showFloater);
		if (this == gAutomationp)
		{
			lua_register(mLuaState, "CallbackAfter", callbackAfter);
			lua_register(mLuaState, "ForceQuit", forceQuit);
			lua_register(mLuaState, "MinimizeWindow", minimizeWindow);
		}
		else
		{
			lua_register(mLuaState, "AutomationMessage", automationMessage);
			lua_register(mLuaState, "AutomationRequest", automationRequest);
		}
		if (this == gAutomationp || mUseConsoleOutput)
		{
			lua_register(mLuaState, "HasThread", hasThread);
			lua_register(mLuaState, "StartThread", startThread);
			lua_register(mLuaState, "StopThread", stopThread);
			lua_register(mLuaState, "SendSignal", sendSignal);
			lua_register(mLuaState, "ReadTextFromFile", readTextFromFile);
			lua_register(mLuaState, "WriteTextToFile", writeTextToFile);
			lua_register(mLuaState, "GetFileSize", getFileSize);
			lua_register(mLuaState, "RemoveFile", removeFile);
			lua_register(mLuaState, "CreateDirectory", createDirectory);
			lua_register(mLuaState, "RemoveDirectory", removeDirectory);
		}
	}
	else
	{
		lua_register(mLuaState, "AutomationMessage", automationMessage);
		lua_register(mLuaState, "AutomationRequest", automationRequest);
	}
	// HTTP communications related functions: only available from automation
	// script and threads.
	if (this == gAutomationp || isThreaded())
	{
		lua_register(mLuaState, "GetHTTP", getHTTP);
		lua_register(mLuaState, "PostHTTP", postHTTP);
	}

	return true;
}

//static
void HBViewerAutomation::preprocessorMessageCB(const std::string& message,
											   bool is_warning, void*)
{
	LLChat chat;
	chat.mFromName = "Lua";
	chat.mText = "Lua preprocessor ";
	chat.mText += is_warning ? "warning: " : "error: ";
	chat.mText += message;
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	// NOTE: we need verify we have logged in before printing in chat, since
	// we otherwise could crash due to LLFloaterChat not yet being constructed.
	if (LLStartUp::isLoggedIn())
	{
		LLFloaterChat::addChat(chat, false, false);
	}
	else	// Just warn/report error in the log
	{
		llwarns << chat.mText << llendl;
	}
}

//static
S32 HBViewerAutomation::loadInclude(std::string& include_name,
									const std::string& default_path,
									std::string& buffer, void*)
{
	if (include_name.empty())
	{
		return HBPreprocessor::FAILURE;
	}

	std::string file;
	if (default_path.compare(0, 2, "~/") == 0)
	{
		// Search in user "home" directory, without fallback sub-directory
		file = gDirUtil.getUserFilename(default_path, "", include_name);
	}
	else
	{
		file = gDirUtil.getUserFilename(default_path, "include", include_name);
	}
	if (file.empty())
	{
		return HBPreprocessor::FAILURE;
	}

	llifstream include_file(file.c_str());
	if (!include_file.is_open())
	{
		llwarns << "Failure to open file: " << file << llendl;
		return HBPreprocessor::FAILURE;
	}

	// Return the full path of the include file we opened successfully
	include_name = file;

	while (!include_file.eof())
	{
		getline(include_file, file);
		buffer += file + "\n";
	}
	include_file.close();

	return HBPreprocessor::SUCCESS;
}

std::string HBViewerAutomation::preprocess(const std::string& file_name)
{
	llifstream source_file(file_name.c_str());
	if (!source_file.is_open())
	{
		return "";
	}

	bool first_line = true;
	std::string sources, line;
	while (!source_file.eof())
	{
		getline(source_file, line);
		if (first_line && line.compare(0, 4, "\x1bLua") == 0)
		{
			// This is a Lua compiled file: cannot pre-process it !
			source_file.close();
			return "";
		}
		first_line = false;
		sources += line + "\n";
	}

	source_file.close();

	if (!HBPreprocessor::needsPreprocessing(sources))
	{
		// No known preprocesor directive in the file, so nothing to do here !
		return "";
	}

	HBPreprocessor pp(file_name, loadInclude);
	pp.setMessageCallback(preprocessorMessageCB);
	pp.addForbiddenToken("_G");	// This shall not be overridden !
	if (pp.preprocess(sources) != HBPreprocessor::SUCCESS)
	{
		// In case of error return an empty sources string.
		return "";
	}

	return pp.getResult();
}

bool HBViewerAutomation::load(const std::string& file_name)
{
	LL_TRACY_TIMER(TRC_LUA_LOAD);

	resetCallbackFlags();

	if (!mLuaState)
	{
		llwarns << "No Lua state defined. Aborted." << llendl;
		llassert(false);
		return false;
	}

	mSourceFileName = file_name;

	llinfos << "Loading Lua script file: " << file_name << llendl;
	S32 ret = luaL_loadfile(mLuaState, file_name.c_str());
	if (ret == LUA_ERRSYNTAX)
	{
		std::string err(lua_tostring(mLuaState, -1));
		llinfos << "Loading failure, attempting to pre-process the file..."
				<< llendl;
		std::string preprocessed = preprocess(file_name);
		if (preprocessed.empty())
		{
			// Any pre-processing error already got reported via the
			// preprocessorMessageCB() callback. Report the initial Lua error
			// (which is still on stack), in case the file did not need
			// pre-processing anyway and the error was a Lua one.
			reportError();
			return false;
		}
		lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
		llinfos << "Loading pre-processed Lua script..." << llendl;
		if (luaL_loadstring(mLuaState, preprocessed.c_str()) != LUA_OK)
		{
			// Report the two errors we encountered: before and after
			// pre-processing
			err = "Before preprocesing: " + err + "\nAfter preprocessing: ";
			err += lua_tostring(mLuaState, -1);
			lua_pushstring(mLuaState, err.c_str());
			reportError();
			return false;
		}
	}
	else if (ret != LUA_OK)
	{
		reportError();
		return false;
	}

	if (!registerCFunctions())
	{
		return false;
	}

	resetTimer();
	if (lua_pcall(mLuaState, 0, LUA_MULTRET, 0) != LUA_OK)
	{
		reportError();
		return false;
	}

	// Register callbacks that can be used by the automation script and threads
	// alike.
	if (this == gAutomationp || isThreaded())
	{
		mHasOnSignal = lua_getglobal(mLuaState, "OnSignal");
		lua_pop(mLuaState, 1);
		if (mHasOnSignal)
		{
			mHasCallbacks = true;
			llinfos << "OnSignal Lua callback found" << llendl;
		}

		mHasOnHTTPReply = getGlobal("OnHTTPReply") == LUA_TFUNCTION;
		lua_pop(mLuaState, 1);
		if (mHasOnHTTPReply)
		{
			mHasCallbacks = true;
			llinfos << "OnHTTPReply Lua callback found" << llendl;
		}
	}

	if (isThreaded())
	{
		if (getGlobal("ThreadRun") != LUA_TFUNCTION)
		{
			lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
			lua_pushliteral(mLuaState,
							"Missing ThreadRun() function in thread code");
			reportError();
			return false;
		}
		lua_settop(mLuaState, 0);

		if (gAutomationp)
		{
			// Register the idle callback for our thead
			LL_DEBUGS("Lua") << "Registering thread idle callback." << LL_ENDL;
			gIdleCallbacks.addFunction(onIdleThread, gAutomationp);
		}

		// No other callback for threads...
		return true;
	}

	// All what follows is exclusive to the automation script.
	if (this != gAutomationp)
	{
		return true;
	}

	mHasOnLogin = getGlobal("OnLogin") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnLogin)
	{
		mHasCallbacks = true;
		llinfos << "OnLogin Lua callback found" << llendl;
	}

	mHasOnAveragedFPS = getGlobal("OnAveragedFPS") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAveragedFPS)
	{
		mHasCallbacks = true;
		llinfos << "OnAveragedFPS Lua callback found" << llendl;
	}

	mHasOnAgentOccupationChange =
		getGlobal("OnAgentOccupationChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAgentOccupationChange)
	{
		mHasCallbacks = true;
		llinfos << "OnAgentOccupationChange Lua callback found" << llendl;
	}

	mHasOnAgentPush = getGlobal("OnAgentPush") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAgentPush)
	{
		mHasCallbacks = true;
		llinfos << "OnAgentPush Lua callback found" << llendl;
	}

	mHasOnSendChat = getGlobal("OnSendChat") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnSendChat)
	{
		mHasCallbacks = true;
		llinfos << "OnSendChat Lua callback found" << llendl;
	}

	mHasOnReceivedChat = getGlobal("OnReceivedChat") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnReceivedChat)
	{
		mHasCallbacks = true;
		llinfos << "OnReceivedChat Lua callback found" << llendl;
	}

	mHasOnChatTextColoring = getGlobal("OnChatTextColoring") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnChatTextColoring)
	{
		mHasCallbacks = true;
		llinfos << "OnChatTextColoring Lua callback found" << llendl;
	}

	mHasOnInstantMsg = getGlobal("OnInstantMsg") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnInstantMsg)
	{
		mHasCallbacks = true;
		llinfos << "OnInstantMsg Lua callback found" << llendl;
	}

	mHasOnAlertDialog = getGlobal("OnAlertDialog") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAlertDialog)
	{
		// Register the Lua callback for alert dialogs. HB
		LLAlertDialog::setLuaCallback(lua_alert_callback);
		mHasCallbacks = true;
		llinfos << "OnAlertDialog Lua callback found" << llendl;
	}

	mHasOnScriptDialog = getGlobal("OnScriptDialog") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnScriptDialog)
	{
		mHasCallbacks = true;
		llinfos << "OnScriptDialog Lua callback found" << llendl;
	}

	mHasOnNotification = getGlobal("OnNotification") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnNotification)
	{
		mHasCallbacks = true;
		llinfos << "OnNotification Lua callback found" << llendl;
	}

	mHasOnGroupNotification =
		getGlobal("OnGroupNotification") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnGroupNotification)
	{
		mHasCallbacks = true;
		llinfos << "OnGroupNotification Lua callback found" << llendl;
	}

	mHasOnSLURLDispatch = getGlobal("OnSLURLDispatch") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnSLURLDispatch)
	{
		mHasCallbacks = true;
		llinfos << "OnSLURLDispatch Lua callback found" << llendl;
	}

	mHasOnURLDispatch = getGlobal("OnURLDispatch") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnURLDispatch)
	{
		mHasCallbacks = true;
		llinfos << "OnURLDispatch Lua callback found" << llendl;
	}

	mHasOnFriendStatusChange =
		getGlobal("OnFriendStatusChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnFriendStatusChange)
	{
		mHasCallbacks = true;
		llinfos << "OnFriendStatusChange Lua callback found" << llendl;
		if (!sFriendsObserver)
		{
			sFriendsObserver = new HBFriendsStatusObserver;
		}
	}

	mHasOnAvatarRezzing = getGlobal("OnAvatarRezzing") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAvatarRezzing)
	{
		mHasCallbacks = true;
		llinfos << "OnAvatarRezzing Lua callback found" << llendl;
	}

	mHasOnAgentBaked = getGlobal("OnAgentBaked") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAgentBaked)
	{
		mHasCallbacks = true;
		llinfos << "OnAgentBaked Lua callback found" << llendl;
	}

	mHasOnObjectAttached = getGlobal("OnObjectAttached") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnObjectAttached)
	{
		mHasCallbacks = true;
		llinfos << "OnObjectAttached Lua callback found" << llendl;
	}

	mHasOnRadar = getGlobal("OnRadar") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnRadar)
	{
		mHasCallbacks = true;
		llinfos << "OnRadar Lua callback found" << llendl;
	}

	mHasOnRadarSelection = getGlobal("OnRadarSelection") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnRadarSelection)
	{
		mHasCallbacks = true;
		llinfos << "OnRadarSelection Lua callback found" << llendl;
	}

	mHasOnRadarMark = getGlobal("OnRadarMark") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnRadarMark)
	{
		mHasCallbacks = true;
		llinfos << "OnRadarMark Lua callback found" << llendl;
	}

	mHasOnRadarTrack = getGlobal("OnRadarTrack") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnRadarTrack)
	{
		mHasCallbacks = true;
		llinfos << "OnRadarTrack Lua callback found" << llendl;
	}

	mHasOnLuaDialogClose = getGlobal("OnLuaDialogClose") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnLuaDialogClose)
	{
		mHasCallbacks = true;
		llinfos << "OnLuaDialogClose Lua callback found" << llendl;
	}

	mHasOnLuaFloaterAction = getGlobal("OnLuaFloaterAction") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnLuaFloaterAction)
	{
		mHasCallbacks = true;
		llinfos << "OnLuaFloaterAction Lua callback found" << llendl;
	}

	mHasOnLuaFloaterOpen = getGlobal("OnLuaFloaterOpen") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnLuaFloaterOpen)
	{
		mHasCallbacks = true;
		llinfos << "OnLuaFloaterOpen Lua callback found" << llendl;
	}

	mHasOnLuaFloaterClose = getGlobal("OnLuaFloaterClose") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnLuaFloaterClose)
	{
		mHasCallbacks = true;
		llinfos << "OnLuaFloaterClose Lua callback found" << llendl;
	}

	mHasOnSideBarVisibilityChange =
		getGlobal("OnSideBarVisibilityChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnSideBarVisibilityChange)
	{
		mHasCallbacks = true;
		llinfos << "OnSideBarVisibilityChange Lua callback found" << llendl;
	}

	mHasOnAutomationMessage =
		getGlobal("OnAutomationMessage") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAutomationMessage)
	{
		mHasCallbacks = true;
		llinfos << "OnAutomationMessage Lua callback found" << llendl;
	}

	mHasOnAutomationRequest =
		getGlobal("OnAutomationRequest") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAutomationRequest)
	{
		mHasCallbacks = true;
		llinfos << "OnAutomationRequest Lua callback found" << llendl;
	}

	mHasOnAutoPilotFinished =
		getGlobal("OnAutoPilotFinished") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnAutoPilotFinished)
	{
		mHasCallbacks = true;
		llinfos << "OnAutoPilotFinished Lua callback found" << llendl;
	}

	mHasOnTPStateChange = getGlobal("OnTPStateChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnTPStateChange)
	{
		mHasCallbacks = true;
		llinfos << "OnTPStateChange Lua callback found" << llendl;
	}

	mHasOnFailedTPSimChange =
		getGlobal("OnFailedTPSimChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnFailedTPSimChange)
	{
		mHasCallbacks = true;
		gIdleCallbacks.addFunction(onIdleSimChange, this);
		llinfos << "OnFailedTPSimChange Lua callback found" << llendl;
	}

	mHasOnRegionChange = getGlobal("OnRegionChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnRegionChange)
	{
		mHasCallbacks = true;
		mRegionChangedConnection =
			gAgent.addRegionChangedCB(std::bind(&HBViewerAutomation::onRegionChange,
												this));
		llinfos << "OnRegionChange Lua callback found" << llendl;
	}

	mHasOnParcelChange = getGlobal("OnParcelChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnParcelChange)
	{
		mHasCallbacks = true;
		mParcelChangedConnection =
			gViewerParcelMgr.addAgentParcelChangedCB(std::bind(&HBViewerAutomation::onParcelChange,
															   this));
		llinfos << "OnParcelChange Lua callback found" << llendl;
	}

	mHasOnPositionChange = getGlobal("OnPositionChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnPositionChange)
	{
		mHasCallbacks = true;
		mPositionChangedConnection =
			gAgent.setPosChangeCallback(std::bind(&HBViewerAutomation::onPositionChange,
												  this, _1, _2));
		llinfos << "OnPositionChange Lua callback found" << llendl;
	}

	mHasOnWindlightChange = getGlobal("OnWindlightChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnWindlightChange)
	{
		mHasCallbacks = true;
		llinfos << "OnWindlightChange Lua callback found" << llendl;
	}

	mHasOnCameraModeChange = getGlobal("OnCameraModeChange") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnCameraModeChange)
	{
		mHasCallbacks = true;
		llinfos << "OnCameraModeChange Lua callback found" << llendl;
	}

	mHasOnJoystickButtons = getGlobal("OnJoystickButtons") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnJoystickButtons)
	{
		mHasCallbacks = true;
		llinfos << "OnJoystickButtons Lua callback found" << llendl;
	}

	mHasOnLuaPieMenu = getGlobal("OnLuaPieMenu") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnLuaPieMenu)
	{
		mHasCallbacks = true;
		llinfos << "OnLuaPieMenu Lua callback found" << llendl;
	}

	mHasOnContextMenu = getGlobal("OnContextMenu") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnContextMenu)
	{
		mHasCallbacks = true;
		llinfos << "OnContextMenu Lua callback found" << llendl;
		LLEditMenuHandler::setCustomCallback(contextMenuCallback);
	}
	else
	{
		LLEditMenuHandler::setCustomCallback(NULL);
	}

	mHasOnRLVHandleCommand = getGlobal("OnRLVHandleCommand") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnRLVHandleCommand)
	{
		mHasCallbacks = true;
		llinfos << "OnRLVHandleCommand Lua callback found" << llendl;
	}

	mHasOnRLVAnswerOnChat = getGlobal("OnRLVAnswerOnChat") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnRLVAnswerOnChat)
	{
		mHasCallbacks = true;
		llinfos << "OnRLVAnswerOnChat Lua callback found" << llendl;
	}

	mHasOnObjectInfoReply = getGlobal("OnObjectInfoReply") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnObjectInfoReply)
	{
		mHasCallbacks = true;
		llinfos << "OnObjectInfoReply Lua callback found" << llendl;
	}

	mHasOnPickInventoryItem = getGlobal("OnPickInventoryItem") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnPickInventoryItem)
	{
		mHasCallbacks = true;
		llinfos << "OnPickInventoryItem Lua callback found" << llendl;
	}

	mHasOnPickAvatar = getGlobal("OnPickAvatar") == LUA_TFUNCTION;
	lua_pop(mLuaState, 1);
	if (mHasOnPickAvatar)
	{
		mHasCallbacks = true;
		llinfos << "OnPickAvatar Lua callback found" << llendl;
	}

	return true;
}

bool HBViewerAutomation::loadString(const std::string& chunk)
{
	LL_TRACY_TIMER(TRC_LUA_LOAD_STRING);

	resetCallbackFlags();

	if (!mLuaState)
	{
		llwarns << "No Lua state defined. Aborted." << llendl;
		llassert(false);
		return false;
	}

	if (luaL_loadstring(mLuaState, chunk.c_str()) != LUA_OK)
	{
		reportError();
		return false;
	}

	if (!registerCFunctions())
	{
		return false;
	}

	resetTimer();
	if (lua_pcall(mLuaState, 0, LUA_MULTRET, 0) != LUA_OK)
	{
		reportError();
		return false;
	}

	return true;
}

S32 HBViewerAutomation::getGlobal(const std::string& global)
{
	if (!mLuaState)
	{
		llwarns << "No valid Lua state loaded. Aborted." << llendl;
		llassert(false);
		return LUA_TNONE;
	}

	if (global.empty())
	{
		return LUA_TNONE;
	}

	return lua_getglobal(mLuaState, global.c_str());
}

//static
int HBViewerAutomation::hasThread(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!findInstance(statep))
	{
		return 0;
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	S32 thread_id = lua_tointeger(statep, 1);
	lua_pop(statep, 1);
	if (thread_id < 0)
	{
		luaL_error(statep, "Not a valid thread Id: ", thread_id);
	}

	bool has_thread = false;

	if (thread_id)	// 0 = automation script, which is not a thread...
	{
		sThreadsMutex.lock();
		threads_list_t::iterator it = sThreadsInstances.find(thread_id);
		if (it != sThreadsInstances.end())
		{
			has_thread = !it->second->isStopped();
		}
		sThreadsMutex.unlock();
	}

	lua_pushboolean(statep, has_thread);

	return 1;
}

//static
int HBViewerAutomation::startThread(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || self != gAutomationp)
	{
		return 0;
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	// The first argument is the file name for the thread code.
	std::string fname(luaL_checkstring(statep, 1));
	if (fname.empty())
	{
		luaL_error(statep, "Empty thread code file name");
	}
	lua_remove(statep, 1);

	// When it exists, the second argument must be a "simple" table that we
	// will use for "argv".
	std::string argv;
	if (n > 1)
	{
		if (!serializeTable(statep, 1, &argv))
		{
			luaL_error(statep, "Unsupported thread argument format");
		}
		argv = "argv=" + argv;
	}

	std::string fpath;
	if (fname.compare(0, 2, "~/") == 0)
	{
		// Search in user "home" directory, without fallback sub-directory
		fpath = gDirUtil.getUserFilename("~/", "",
										 LLDir::getBaseFileName(fname));
	}
	else
	{
		// Search in the user_settings application directory, with an "include"
		// fallback sub-directory.
		fpath = gDirUtil.getUserFilename(gDirUtil.getOSUserAppDir(), "include",
										 fname);
	}
	if (fpath.empty())
	{
		luaL_error(statep, "Invalid path for file: %s", fname.c_str());
	}

	sThreadsMutex.lock();
	if (sThreadsInstances.size() >= MAX_LUA_THREADS)
	{
		sThreadsMutex.unlock();
		LL_DEBUGS("Lua") << "Too many running threads to start a new one."
						 << LL_ENDL;
		lua_pushboolean(statep, false);
		return 1;
	}
	sThreadsMutex.unlock();

	HBAutomationThread* threadp = new HBAutomationThread;
	bool success = threadp->load(fpath);
	if (success && threadp->mLuaState)
	{
		if (!argv.empty())
		{
			if (luaL_dostring(threadp->mLuaState, argv.c_str()))
			{
				success = false;
				llwarns << "Failed to set the thread argv table for thread: "
						<< threadp->getName() << llendl;
			}
		}
	}
	else
	{
		success = false;	// Paranoia, in case threadp->mLuaState == NULL...
		llwarns << "Failed to load the Lua code for thread: "
				<< threadp->getName() << llendl;
	}
	if (success)
	{
		U32 thread_id = threadp->getLuaThreadID();
		sThreadsMutex.lock();
		sThreadsInstances[thread_id] = threadp;
		sThreadsMutex.unlock();
		threadp->threadStart();
		lua_pushnumber(statep, thread_id);
	}
	else
	{
		delete threadp;
		lua_pushnil(statep);
	}

	return 1;
}

//static
int HBViewerAutomation::stopThread(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || self != gAutomationp)
	{
		return 0;
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	S32 thread_id = lua_tointeger(statep, 1);
	lua_pop(statep, 1);
	if (thread_id <= 0)
	{
		luaL_error(statep, "Not a valid thread Id: ", thread_id);
	}

	sThreadsMutex.lock();
	threads_list_t::iterator it = sThreadsInstances.find(thread_id);
	if (it == sThreadsInstances.end())
	{
		sThreadsMutex.unlock();
		lua_pushboolean(statep, false);
		return 1;
	}
	HBAutomationThread* threadp = it->second;
	LL_DEBUGS("Lua") << "Stopping the running thread: " << threadp->getName()
					 << LL_ENDL;
	threadp->threadStop();
	sThreadsMutex.unlock();

	lua_pushboolean(statep, true);
	return 1;
}

//static
int HBViewerAutomation::sendSignal(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self)
	{
		return 0;
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	S32 thread_id = lua_tointeger(statep, 1);
	if (thread_id < 0)
	{
		luaL_error(statep, "Not a valid thread Id: ", thread_id);
	}
	if ((U32)thread_id == self->getLuaThreadID())
	{
		luaL_error(statep, "Cannot send a signal to self !");
	}
	lua_remove(statep, 1);

	if (lua_type(statep, 1) != LUA_TTABLE)
	{
		luaL_error(statep,
				   "Invalid type pased as second argument: table expected");
	}

	// Particular case for sending a signal from a thread to the automation
	// script itself.
	if (thread_id == 0 && self->isThreaded())
	{
		if (!gAutomationp || !gAutomationp->mHasOnSignal)
		{
			lua_pop(statep, 1);
			lua_pushboolean(statep, false);
			return 1;
		}
		// Push our thread Id on the stack...
		lua_pushnumber(statep, self->getLuaThreadID());
		// ... and move it above the table in the stack.
		lua_insert(statep, 1);
		// Push the time stamp on stack...
		lua_pushnumber(statep, gFrameTimeSeconds);
		// ... and move it above the table in the stack.
		lua_insert(statep, 2);
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		// Send the signal to the automation script via callAutomationFunc() by
		// calling its OnSignal() callback instead of reentering this method.
		threadp->callMainFunction("OnSignal");
		lua_pushboolean(statep, true);
		return 1;
	}

	std::string signal_str;
	if (!serializeTable(statep, 1, &signal_str))
	{
		luaL_error(statep, "Unsupported thread signal format");
	}
	signal_str = llformat("%d;%f|", self->getLuaThreadID(),
						  gFrameTimeSeconds) + signal_str;
	LL_DEBUGS("Lua") << "Serialized signal string: " << signal_str << LL_ENDL;

	sThreadsMutex.lock();

	threads_list_t::iterator tit = sThreadsInstances.find(thread_id);
	if (tit == sThreadsInstances.end())
	{
		sThreadsMutex.unlock();
		lua_pushboolean(statep, false);
		return 1;
	}
	HBAutomationThread* threadp = tit->second;
	if (!threadp->mHasOnSignal)
	{
		sThreadsMutex.unlock();
		lua_pushboolean(statep, false);
		return 1;
	}

	HBThreadSignals* signals;
	signals_map_t::iterator sit = sThreadsSignals.find(threadp);
	if (sit == sThreadsSignals.end())
	{
		LL_DEBUGS("Lua") << "Creating new signal queue for thread: "
						 << thread_id << LL_ENDL;
		signals = new HBThreadSignals;
		signals->mThreadID = thread_id;
		sThreadsSignals[threadp] = signals;
	}
	else
	{
		LL_DEBUGS("Lua") << "Existing signal queue found for thread: "
						 << thread_id << LL_ENDL;
		signals = sit->second;
		if (signals->mThreadID != (U32)thread_id)
		{
			llwarns << "Dead thread signals found, removing them." << llendl;
			signals->mThreadID = thread_id;
			signals->mSignals.clear();
		}
	}
	signals->mSignals.emplace_back(signal_str);

	sThreadsMutex.unlock();

	lua_pushboolean(statep, true);
	return 1;
}

//static
bool HBViewerAutomation::callAutomationFunc(HBAutomationThread* threadp)
{
	lua_State* astatep = gAutomationp->mLuaState;
	lua_State* tstatep = threadp->mLuaState;

	// Get the function name and the corresponding global in the automation
	// script.
	const std::string& function = threadp->getFuncCall();
	lua_getglobal(astatep, function.c_str());
	if (lua_type(astatep, -1) != LUA_TFUNCTION)
	{
		lua_settop(astatep, 0);	// Clear the automation script stack
		threadp->setFuncCallError("No function named '" + function +
								  "' in automation script");
		return false;
	}

	// Process the paramaters present on the thread state stack, copying them
	// onto the automation script state stack...
	S32 n = lua_gettop(tstatep);
	for (S32 i = 1; i <= n; ++i)
	{
		int type = lua_type(tstatep, i);
		switch (type)
		{
			case LUA_TBOOLEAN:
				lua_pushboolean(astatep, lua_toboolean(tstatep, i));
				break;

			case LUA_TNUMBER:
				lua_pushnumber(astatep, lua_tonumber(tstatep, i));
				break;

			case LUA_TSTRING:
				lua_pushstring(astatep, lua_tostring(tstatep, i));
				break;

			case LUA_TNIL:
				lua_pushnil(astatep);
				break;

			case LUA_TTABLE:
			{
				std::string table;
				if (serializeTable(tstatep, i, &table))
				{
					table = "_V_TABLE_PARAM=" + table;
					if (luaL_dostring(astatep, table.c_str()) == LUA_OK)
					{
						lua_getglobal(astatep, "_V_TABLE_PARAM");
						lua_pushnil(astatep);
						lua_setglobal(astatep, "_V_TABLE_PARAM");
						break;
					}
				}
				lua_settop(tstatep, 0);	// Clear the thread script stack
				lua_settop(astatep, 0);	// Clear the automation script stack
				threadp->setFuncCallError("Failed to copy a table parameter");
				return false;
			}

			default:
			{
				lua_settop(tstatep, 0);	// Clear the thread script stack
				lua_settop(astatep, 0);	// Clear the automation script stack
				std::string err = "Unsupported parameter type: ";
				err += lua_typename(tstatep, type);
				threadp->setFuncCallError(err);
				return false;
			}
		}
	}
	lua_settop(tstatep, 0);	// Clear the thread script stack

	gAutomationp->resetTimer();
	if (lua_pcall(astatep, n, LUA_MULTRET, 0) != LUA_OK)
	{
		threadp->setFuncCallError(lua_tostring(astatep, -1));
		lua_settop(astatep, 0);	// Clear the automation script stack
		return false;
	}

	n = lua_gettop(astatep);	// Number or returned results
	if (!n)
	{
		return true;	// We are done !
	}

	for (S32 i = 1; i <= n; ++i)
	{
		int type = lua_type(astatep, i);
		switch (type)
		{
			case LUA_TBOOLEAN:
				lua_pushboolean(tstatep, lua_toboolean(astatep, i));
				break;

			case LUA_TNUMBER:
				lua_pushnumber(tstatep, lua_tonumber(astatep, i));
				break;

			case LUA_TSTRING:
				lua_pushstring(tstatep, lua_tostring(astatep, i));
				break;

			case LUA_TNIL:
				lua_pushnil(tstatep);
				break;

			case LUA_TTABLE:
			{
				std::string table;
				if (serializeTable(astatep, i, &table))
				{
					table = "_V_RET_TABLE=" + table;
					if (luaL_dostring(tstatep, table.c_str()) == LUA_OK)
					{
						lua_getglobal(tstatep, "_V_RET_TABLE");
						lua_pushnil(tstatep);
						lua_setglobal(tstatep, "_V_RET_TABLE");
						break;
					}
				}
				lua_settop(tstatep, 0);	// Clear the thread script stack
				lua_settop(astatep, 0);	// Clear the automation script stack
				threadp->setFuncCallError("Failed to copy a returned table");
				return false;
			}

			default:
			{
				lua_settop(tstatep, 0);	// Clear the thread script stack
				lua_settop(astatep, 0);	// Clear the automation script stack
				std::string err = "Unsupported return type: ";
				err += lua_typename(astatep, type);
				threadp->setFuncCallError(err);
				return false;
			}
		}
	}
	lua_settop(astatep, 0);	// Clear the automation script stack

	return true;
}

//static
void HBViewerAutomation::onIdleThread(void* datap)
{
	LL_FAST_TIMER(FTM_IDLE_LUA_THREAD);

	HBViewerAutomation* self = (HBViewerAutomation*)datap;
	if (!self || self != gAutomationp)	// Paranoia
	{
		return;
	}

	// Note: no need to lock sThreadsMutex at this point, since only the
	// automation thread can change sThreadsInstances, either in startThread()
	// or here.
	if (sThreadsInstances.empty())
	{
		LL_DEBUGS("Lua") << "No thread left, unregistering idle callback."
						 << LL_ENDL;
		gIdleCallbacks.deleteFunction(onIdleThread, self);
		sThreadsSignals.clear();	// Clear any signal leftover
		return;
	}

	// This will be used to store pointers to threads waiting for a custom Lua
	// function call.
	std::vector<HBAutomationThread*> waiting_threads;

	for (threads_list_t::iterator it = sThreadsInstances.begin(),
								  end = sThreadsInstances.end();
		 it != end; )
	{
		threads_list_t::iterator curit = it++;
		HBAutomationThread* threadp = curit->second;

		// Only intervene after the thread sets itself to "not running" (i.e.
		// got locked on its run condition, or is executing a sleeping loop) or
		// exited... When it is, it is also safe to use/change its member
		// variables and Lua state.
		// Note: isRunning() usually returns true after the thread is actually
		// stopped (i.e. running loop exited after receiving a threadStop()
		// request)...
		if (threadp->isRunning() && !threadp->isStopped())
		{
			// Check for any pending signals to send to this thread.
			sThreadsMutex.lock();
			if (sThreadsSignals.count(threadp))
			{
				// Let it know that it has got signals and should pause so that
				// we can send them to it !
				threadp->setSignal();
			}
			sThreadsMutex.unlock();
			continue;
		}

		// If the thread print buffer contains something, print it now.
		if (!threadp->mPrintBuffer.empty() && LLStartUp::isLoggedIn())
		{
			LLChat chat;
			chat.mFromName = threadp->getName();
			chat.mText = chat.mFromName + ": " + threadp->mPrintBuffer;
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChat(chat, false, false);
			threadp->mPrintBuffer.clear();
		}

		// If the thread is stopped, remove it.
		if (threadp->isStopped())
		{
			LL_DEBUGS("Lua") << "Thread '" << threadp->getName()
							 << "' stopped, deleting it." << LL_ENDL;
			// Protect sThreadsSignals and sThreadsInstances, in case some
			// other running thread would try and access them (via hasThread()
			// or sendSignal()) while we are deleting this thread.
			sThreadsMutex.lock();
			sThreadsInstances.erase(curit);
			signals_map_t::iterator sit = sThreadsSignals.find(threadp);
			if (sit != sThreadsSignals.end())
			{
				delete sit->second;
				sThreadsSignals.erase(sit);
			}
			sThreadsMutex.unlock();
			delete threadp;
			continue;
		}

		// Check for any pending signals to send to this thread.
		sThreadsMutex.lock();
		signals_map_t::iterator sit = sThreadsSignals.find(threadp);
		if (sit != sThreadsSignals.end())
		{
			HBThreadSignals* signals = sit->second;
			// Current thread Id and Id stored in signals table should match !
			if (signals->mThreadID == threadp->getLuaThreadID())
			{
				// Copy the signal strings in the proper (chronological) order
				// into the thread's own signals vector.
				std::vector<std::string>& sigs_vec = signals->mSignals;
				for (U32 i = 0, count = sigs_vec.size(); i < count; ++i)
				{
					const std::string& sig_str = sigs_vec[i];
					LL_DEBUGS("Lua") << "Copying signal string: " << sig_str
									 << LL_ENDL;
					threadp->appendSignal(sig_str);
				}
			}
			// Stale signals from a dead (crashed ?) thread which old address
			// got reaffected to a new thread (unlikely but possible)...
			else
			{
				llwarns << "Non-matching thread Id " << signals->mThreadID
						<< " found for signals queue associated with thread "
						<< threadp->getLuaThreadID()
						<< ": deleting stale queue." << llendl;
			}
			delete signals;
			sThreadsSignals.erase(sit);
		}
		sThreadsMutex.unlock();

		// If the thread is waiting for an automation script function call, we
		// must perform it on its behalf... But later (see below).
		if (threadp->hasFuncCall())
		{
			waiting_threads.push_back(threadp);
		}
		else
		{
			// Let the thread run again
			threadp->setRunning();
		}
	}

	// Now that we cleaned-up sThreadsInstances and sThreadsSignals, we can
	// proceed with performing our custom Lua function calls on behalf of the
	// waiting threads (since these calls could result in changes to either of
	// sThreadsInstances or sThreadsSignals via callbacks they would trigger in
	// the automation script)...
	for (U32 i = 0, count = waiting_threads.size(); i < count; ++i)
	{
		HBAutomationThread* threadp = waiting_threads[i];
		callAutomationFunc(threadp);
		// We can let this thread run again now
		threadp->setRunning();
	}
}

void HBViewerAutomation::pushGridSimAndPos()
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		lua_newtable(mLuaState);

		lua_pushliteral(mLuaState, "grid");
		lua_pushstring(mLuaState,
					   LLGridManager::getInstance()->getGridLabel().c_str());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "region");
		lua_pushstring(mLuaState, regionp->getName().c_str());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "version");
		lua_pushstring(mLuaState, gLastVersionChannel.c_str());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "width");
		lua_pushnumber(mLuaState, regionp->getWidth());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "water_height");
		lua_pushnumber(mLuaState, regionp->getWaterHeight());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "flags");
		lua_pushinteger(mLuaState, regionp->getRegionFlags());
		lua_rawset(mLuaState, -3);

		std::vector<S32> neighbors;
		regionp->getNeighboringRegionsStatus(neighbors);
		lua_pushliteral(mLuaState, "neighbors");
		lua_pushinteger(mLuaState, neighbors.size());
		lua_rawset(mLuaState, -3);

		const LLVector3d& pos_global = gAgent.getPositionGlobal();
		lua_pushliteral(mLuaState, "global_x");
		lua_pushnumber(mLuaState, pos_global.mdV[VX]);
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "global_y");
		lua_pushnumber(mLuaState, pos_global.mdV[VY]);
		lua_rawset(mLuaState, -3);

		const LLVector3& pos_local = gAgent.getPositionAgent();
		lua_pushliteral(mLuaState, "local_x");
		lua_pushnumber(mLuaState, pos_local.mV[VX]);
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "local_y");
		lua_pushnumber(mLuaState, pos_local.mV[VY]);
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "altitude");
		lua_pushnumber(mLuaState, pos_local.mV[VZ]);
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "navmesh");
		const char* navmesh;
		if (!regionp->hasDynamicPathfinding())
		{
			navmesh = "none";
		}
		else if (gOverlayBarp && gOverlayBarp->isNavmeshDirty())
		{
			navmesh = "dirty";
		}
		else if (gOverlayBarp && gOverlayBarp->isNavmeshRebaking())
		{
			navmesh = "rebaking";
		}
		else if (regionp->dynamicPathfindingEnabled())
		{
			navmesh = "enabled";
		}
		else
		{
			navmesh = "disabled";
		}
		lua_pushstring(mLuaState, navmesh);
		lua_rawset(mLuaState, -3);
	}
	else
	{
		lua_pushnil(mLuaState);
	}
}

void HBViewerAutomation::onLogin()
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (this != gAutomationp)	// Paranoia
	{
		return;
	}

	// Ensure mFromObjectId is properly initialized for gAutomationp which is
	// created on viewer launch while gAgentID was still a null UUID...
	mFromObjectId = gAgentID;

	// Print anything that got printed from the automation script before login.
	if (!mPrintBuffer.empty())
	{
		LLChat chat;
		chat.mFromName = "Lua";
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		chat.mText = "Lua: " + mPrintBuffer;
		mPrintBuffer.clear();
		LLFloaterChat::addChat(chat, false, false);
	}

	if (!mHasOnLogin || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnLogin Lua callback." << LL_ENDL;

	lua_getglobal(mLuaState, "OnLogin");
	pushGridSimAndPos();
	lua_pushboolean(mLuaState, gAvatarMovedOnLogin);
	lua_pushboolean(mLuaState, gSavedSettings.getBool("AutoLogin"));
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onRegionChange()
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnRegionChange || !mLuaState)
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnRegionChange Lua callback." << LL_ENDL;

	lua_getglobal(mLuaState, "OnRegionChange");
	pushGridSimAndPos();
	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::pushParcelInfo()
{
	LLViewerRegion* region = gAgent.getRegion();
	LLParcel* parcel = gViewerParcelMgr.getAgentParcel();
	if (region && parcel)
	{
		lua_newtable(mLuaState);

		lua_pushliteral(mLuaState, "name");
		lua_pushstring(mLuaState, parcel->getName().c_str());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "description");
		lua_pushstring(mLuaState, parcel->getDesc().c_str());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "flags");
		lua_pushinteger(mLuaState, parcel->getParcelFlags());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "build");
		lua_pushboolean(mLuaState, gViewerParcelMgr.allowAgentBuild());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "damage");
		lua_pushboolean(mLuaState,
						gViewerParcelMgr.allowAgentDamage(region, parcel));
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "fly");
		lua_pushboolean(mLuaState,
						gViewerParcelMgr.allowAgentFly(region, parcel));
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "push");
		lua_pushboolean(mLuaState,
						gViewerParcelMgr.allowAgentPush(region, parcel));
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "scripts");
		lua_pushboolean(mLuaState,
						gViewerParcelMgr.allowAgentScripts(region, parcel));
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "see");
		lua_pushboolean(mLuaState,
						!parcel->getHaveNewParcelLimitData() ||
						parcel->getSeeAVs());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "voice");
		lua_pushboolean(mLuaState,
						gIsInSecondLife ? gViewerParcelMgr.allowAgentVoice()
										: parcel->getParcelFlagAllowVoice());
		lua_rawset(mLuaState, -3);
	}
	else
	{
		lua_pushnil(mLuaState);
	}
}

void HBViewerAutomation::onParcelChange()
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnParcelChange || !mLuaState)
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnParcelChange Lua callback." << LL_ENDL;

	lua_getglobal(mLuaState, "OnParcelChange");
	pushParcelInfo();
	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onPositionChange(const LLVector3& pos_local,
										  const LLVector3d& pos_global)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnPositionChange || !mLuaState)
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnPositionChange Lua callback." << LL_ENDL;

	lua_getglobal(mLuaState, "OnPositionChange");

	lua_newtable(mLuaState);

	lua_pushliteral(mLuaState, "global_x");
	lua_pushnumber(mLuaState, pos_global.mdV[VX]);
	lua_rawset(mLuaState, -3);

	lua_pushliteral(mLuaState, "global_y");
	lua_pushnumber(mLuaState, pos_global.mdV[VY]);
	lua_rawset(mLuaState, -3);

	lua_pushliteral(mLuaState, "local_x");
	lua_pushnumber(mLuaState, pos_local.mV[VX]);
	lua_rawset(mLuaState, -3);

	lua_pushliteral(mLuaState, "local_y");
	lua_pushnumber(mLuaState, pos_local.mV[VY]);
	lua_rawset(mLuaState, -3);

	lua_pushliteral(mLuaState, "altitude");
	lua_pushnumber(mLuaState, pos_local.mV[VZ]);
	lua_rawset(mLuaState, -3);

	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onAveragedFPS(F32 fps, bool limited, F32 frame_time)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnAveragedFPS || !mLuaState)
	{
		return;
	}

	// Average the frame rates before actually invoking the Lua callback. Note:
	// onAveragedFPS() is called every 200ms or so by the status bar refresh()
	// method, sometimes at a shorter interval whenever the status bar needs an
	// immediate refresh.
	static F32 next_report = 0.f;
	static U32 cumulated_count = 0;
	static F32 cumulative_fps = 0.f;
	static bool has_been_limited = false;
	cumulative_fps += fps;
	++cumulated_count;
	has_been_limited |= limited;
	if (gFrameTimeSeconds < next_report || cumulated_count < 5)
	{
		return;
	}
	fps = cumulative_fps / F32(cumulated_count);
	limited = has_been_limited;
	cumulative_fps = 0.f;
	cumulated_count = 0;
	has_been_limited = false;
	static LLCachedControl<F32> cb_interval(gSavedSettings,
											"LuaOnAveragedFPSInterval");
	next_report = gFrameTimeSeconds + llmax(1.f, F32(cb_interval));

	LL_DEBUGS("Lua") << "Invoking OnAveragedFPS Lua callback. fps="
					 << fps << " - limited=" << (limited ? "true" : "false")
					 << " - frame_render_time= " << frame_time << LL_ENDL;

	lua_getglobal(mLuaState, "OnAveragedFPS");
	lua_pushnumber(mLuaState, fps);
	lua_pushboolean(mLuaState, limited);
	lua_pushnumber(mLuaState, frame_time);
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onAgentOccupationChange(S32 type)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnAgentOccupationChange || !mLuaState ||
		sIgnoredCallbacks[E_ONAGENTOCCUPATIONCHANGE])
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnAgentOccupationChange Lua callback. type="
					 << type << LL_ENDL;

	lua_getglobal(mLuaState, "OnAgentOccupationChange");
	lua_pushinteger(mLuaState, type);
	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onAgentPush(const LLUUID& id, S32 type, F32 mag)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnAgentPush || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnAgentPush Lua callback. id=" << id
					 << " - type=" << type << " - mag=" << mag << LL_ENDL;

	lua_getglobal(mLuaState, "OnAgentPush");
	lua_pushstring(mLuaState, id.asString().c_str());
	lua_pushinteger(mLuaState, type);
	lua_pushnumber(mLuaState, mag);
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

bool HBViewerAutomation::onSendChat(std::string& text)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnSendChat || !mLuaState || sIgnoredCallbacks[E_ONSENDCHAT])
	{
		return false;
	}

	LL_DEBUGS("Lua") << "Invoking onSendChat Lua callback." << LL_ENDL;

	HBIgnoreCallback lock_on_chat(E_ONSENDCHAT);

	lua_getglobal(mLuaState, "OnSendChat");
	lua_pushstring(mLuaState, text.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 1, 1, 0) != LUA_OK)
	{
		reportError();
		return false;
	}

	if (lua_gettop(mLuaState) == 0 || lua_type(mLuaState, -1) != LUA_TSTRING)
	{
		lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
		lua_pushliteral(mLuaState,
						"OnSendChat() Lua callback did not return a string");
		reportError();
		return false;
	}

	std::string new_text(lua_tolstring(mLuaState, -1, NULL));
	lua_pop(mLuaState, 1);
	if (new_text != text)
	{
		text = new_text;
		return true;
	}

	return false;
}

void HBViewerAutomation::onReceivedChat(U8 chat_type, const LLUUID& from_id,
										const std::string& name,
										const std::string& text)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnReceivedChat || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnReceivedChat Lua callback. chat_type="
					 << chat_type << " - from_id=" << from_id << " - name="
					 << name << LL_ENDL;

	lua_getglobal(mLuaState, "OnReceivedChat");
	lua_pushinteger(mLuaState, chat_type);
	lua_pushstring(mLuaState, from_id.asString().c_str());
	lua_pushboolean(mLuaState, gObjectList.findAvatar(from_id) != NULL);
	lua_pushstring(mLuaState, name.c_str());
	lua_pushstring(mLuaState, text.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 5, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

bool HBViewerAutomation::onChatTextColoring(const LLUUID& from_id,
											const std::string& name,
											const std::string& text,
											LLColor4& color)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnChatTextColoring || !mLuaState) return false;

	LL_DEBUGS("Lua") << "Invoking OnChatTextColoring Lua callback. name="
					 << name << LL_ENDL;

	lua_getglobal(mLuaState, "OnChatTextColoring");
	lua_pushstring(mLuaState, from_id.asString().c_str());
	lua_pushstring(mLuaState, name.c_str());
	lua_pushstring(mLuaState, text.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 3, 1, 0) != LUA_OK)
	{
		reportError();
	}

	if (lua_gettop(mLuaState) == 0 || lua_type(mLuaState, -1) != LUA_TSTRING)
	{
		lua_pushliteral(mLuaState,
						"OnChatTextColoring() Lua callback did not return a string");
		reportError();
		return false;
	}

	std::string color_str(lua_tolstring(mLuaState, -1, NULL));
	lua_pop(mLuaState, 1);
	if (color_str.empty())
	{
		return false;
	}

	if (!LLColor4::parseColor(color_str, &color))
	{
		lua_pushliteral(mLuaState,
						"OnChatTextColoring() Lua returned an invalid color");
		reportError();
		return false;
	}

	return true;
}

void HBViewerAutomation::onInstantMsg(const LLUUID& session_id,
									  const LLUUID& origin_id,
									  const std::string& name,
									  const std::string& text)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnInstantMsg || !mLuaState || sIgnoredCallbacks[E_ONINSTANTMSG])
	{
		return;
	}

	// See LLIMMgr::computeSessionID() for the session Id computation rules
	S32 type;
	LLUUID other_participant_id = session_id;
	if (session_id == gAgentID || (session_id ^ origin_id) == gAgentID)
	{
		LL_DEBUGS("Lua") << "Peer to peer session detected." << LL_ENDL;
		other_participant_id = origin_id;
		type = 0;
	}
	else if (gAgent.isInGroup(session_id, true))
	{
		LL_DEBUGS("Lua") << "Group session detected." << LL_ENDL;
		type = 1;
	}
	else
	{
		LL_DEBUGS("Lua") << "Conference session assumed." << LL_ENDL;
		type = 2;
	}

	LL_DEBUGS("Lua") << "Invoking OnInstantMsg Lua callback. session_id="
					 << session_id << " - other_participant_id="
					 << other_participant_id << " - type=" << type
					 << " - name=" << name << LL_ENDL;
	HBIgnoreCallback lock_on_im(E_ONINSTANTMSG);
	lua_getglobal(mLuaState, "OnInstantMsg");
	lua_pushstring(mLuaState, session_id.asString().c_str());
	lua_pushstring(mLuaState, other_participant_id.asString().c_str());
	lua_pushinteger(mLuaState, type);
	lua_pushstring(mLuaState, name.c_str());
	lua_pushstring(mLuaState, text.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 5, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

// A mere wrapper for onAlertDialog()
void lua_alert_callback(const std::string& dialog_name, const LLUUID& alert_id,
						const std::string& message,
						const std::vector<std::string>& buttons)
{
	if (gAutomationp)
	{
		gAutomationp->onAlertDialog(dialog_name, alert_id, message, buttons);
	}
}

void HBViewerAutomation::onAlertDialog(const std::string& dialog_name,
									   const LLUUID& alert_id,
									   const std::string& message,
									   const std::vector<std::string>& buttons)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnAlertDialog || !mLuaState || dialog_name == "LuaAlert")
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnAlertDialog Lua callback. dialog_name="
					 << dialog_name << " - alert_id=" << alert_id << LL_ENDL;

	lua_getglobal(mLuaState, "OnAlertDialog");

	lua_pushstring(mLuaState, dialog_name.c_str());
	lua_pushstring(mLuaState, alert_id.asString().c_str());
	lua_pushstring(mLuaState, message.c_str());

	lua_newtable(mLuaState);
	for (S32 i = 0, count = buttons.size(); i < count; ++i)
	{
		lua_pushnumber(mLuaState, i + 1);
		lua_pushstring(mLuaState, buttons[i].c_str());
		lua_rawset(mLuaState, -3);
	}

	resetTimer();
	if (lua_pcall(mLuaState, 4, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onScriptDialog(const LLUUID& notif_id,
										const std::string& message,
										const std::vector<std::string>& buttons)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnScriptDialog || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnScriptDialog Lua callback. notif_id="
					 << notif_id << LL_ENDL;

	lua_getglobal(mLuaState, "OnScriptDialog");

	lua_pushstring(mLuaState, notif_id.asString().c_str());
	lua_pushstring(mLuaState, message.c_str());

	lua_newtable(mLuaState);
	for (S32 i = 0, count = buttons.size(); i < count; ++i)
	{
		lua_pushnumber(mLuaState, i + 1);
		lua_pushstring(mLuaState, buttons[i].c_str());
		lua_rawset(mLuaState, -3);
	}

	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onNotification(const std::string& dialog_name,
										const LLUUID& notif_id,
										const std::string& message)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnNotification || !mLuaState ||
		dialog_name == "LuaNotifyTip" || dialog_name == "LuaNotification")
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnNotification Lua callback. dialog_name="
					 << dialog_name << " - notif_id=" << notif_id << LL_ENDL;

	lua_getglobal(mLuaState, "OnNotification");

	lua_pushstring(mLuaState, dialog_name.c_str());
	lua_pushstring(mLuaState, notif_id.asString().c_str());
	lua_pushstring(mLuaState, message.c_str());

	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onGroupNotification(const std::string& group_name,
											 const LLUUID& group_id,
											 const LLUUID& notif_id,
											 F64 timestamp,
											 const std::string& sender,
											 const std::string& subject,
											 const std::string& message,
											 const std::string& inventory)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnGroupNotification || !mLuaState)
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnGroupNotification Lua callback. group_name="
					 << group_name << " - notif_id=" << notif_id << LL_ENDL;

	lua_getglobal(mLuaState, "OnGroupNotification");

	lua_pushstring(mLuaState, group_name.c_str());
	lua_pushstring(mLuaState, group_id.asString().c_str());
	lua_pushstring(mLuaState, notif_id.asString().c_str());
	lua_pushnumber(mLuaState, timestamp);
	lua_pushstring(mLuaState, sender.c_str());
	lua_pushstring(mLuaState, subject.c_str());
	lua_pushstring(mLuaState, message.c_str());
	lua_pushstring(mLuaState, inventory.c_str());

	resetTimer();
	if (lua_pcall(mLuaState, 8, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

bool HBViewerAutomation::onSLURLDispatch(const std::string& slurl,
										 const std::string& nav_type,
										 bool trusted)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnSLURLDispatch || !mLuaState ||
		sIgnoredCallbacks[E_ONDISPATCHSLURL])
	{
		return true;	// 'true' means "allow dispatching".
	}

	LL_DEBUGS("Lua") << "Invoking OnSLURLDispatch Lua callback. slurl="
					 << slurl << " - nav_type=" << nav_type << " - trusted="
					 << (trusted ? " true" : "false") << LL_ENDL;

	lua_getglobal(mLuaState, "OnSLURLDispatch");

	lua_pushstring(mLuaState, slurl.c_str());
	lua_pushstring(mLuaState, nav_type.c_str());
	lua_pushboolean(mLuaState, trusted);

	resetTimer();
	if (lua_pcall(mLuaState, 3, 1, 0) != LUA_OK)
	{
		reportError();
	}

	if (lua_gettop(mLuaState) == 0 || lua_type(mLuaState, -1) != LUA_TBOOLEAN)
	{
		lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
		lua_pushliteral(mLuaState,
						"OnSLURLDispatch() Lua callback did not return a boolean");
		reportError();
		return true;	// 'true' means "allow dispatching".
	}

	bool result = lua_toboolean(mLuaState, -1);
	lua_pop(mLuaState, 1);
	return result;
}

U32 HBViewerAutomation::onURLDispatch(const std::string& url,
									  const std::string& target,
									  bool external_browser)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnURLDispatch || !mLuaState || sIgnoredCallbacks[E_ONDISPATCHURL])
	{
		// If no callback, do not modify the configured dispatching.
		return external_browser ? 2 : 1;
	}

	LL_DEBUGS("Lua") << "Invoking OnURLDispatch Lua callback. url="
					 << url << " - target=" << target << LL_ENDL;

	lua_getglobal(mLuaState, "OnURLDispatch");

	lua_pushstring(mLuaState, url.c_str());
	lua_pushstring(mLuaState, target.c_str());
	lua_pushboolean(mLuaState, external_browser);

	resetTimer();
	if (lua_pcall(mLuaState, 3, 1, 0) != LUA_OK)
	{
		reportError();
	}

	if (lua_gettop(mLuaState) == 0 || lua_type(mLuaState, -1) != LUA_TNUMBER)
	{
		lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
		lua_pushliteral(mLuaState,
						"OnURLDispatch() Lua callback did not return a number");
		reportError();
		return external_browser ? 2 : 1;
	}

	S32 result = llclamp(lua_tonumber(mLuaState, -1), -1, 2);
	lua_pop(mLuaState, 1);
	if (result < 0)
	{
		result = external_browser ? 2 : 1;
	}
	return (U32)result;
}

void HBViewerAutomation::onFriendStatusChange(const LLUUID& id, U32 mask,
											  bool is_online)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnFriendStatusChange || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnFriendStatusChange Lua callback. id="
					 << id << " - mask=" << mask << " - is_online="
					 << (is_online ? " true" : "false") << LL_ENDL;

	lua_getglobal(mLuaState, "OnFriendStatusChange");
	lua_pushstring(mLuaState, id.asString().c_str());
	lua_pushinteger(mLuaState, mask);
	lua_pushboolean(mLuaState, is_online);
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onAvatarRezzing(const LLUUID& id)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnAvatarRezzing || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnAvatarRezzing Lua callback. id="
					 << id << LL_ENDL;

	lua_getglobal(mLuaState, "OnAvatarRezzing");
	lua_pushstring(mLuaState, id.asString().c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onAgentBaked()
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnAgentBaked || !mLuaState || !isAgentAvatarValid()) return;

	if (!gAgent.isGodlikeWithoutAdminMenuFakery() &&
		!enable_avatar_textures(NULL))
	{
		return;
	}

	LL_DEBUGS("Lua") << "Queuing OnAgentBaked Lua callback." << LL_ENDL;

	// We use a callback with a 2 seconds delay, because we may otherwise
	// encounter race conditions between baking, messaging (in OpenSIM, with
	// legacy UDP messages), and the actual availability of the baked textures.
	doAfterInterval(std::bind(&doCallOnAgentBaked, mLuaState), 2.f);
}

void HBViewerAutomation::onObjectAttached(const LLUUID& obj_id,
										  const LLUUID& inv_id)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnObjectAttached || !mLuaState || !isAgentAvatarValid())
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnObjectAttached Lua callback. obj_id="
					 << obj_id << " - inv_id=" << inv_id << LL_ENDL;

	std::string item_name;
	if (inv_id.isNull())
	{
		item_name = "|Temp attachment|";
	}
	else
	{
		LLViewerInventoryItem* itemp = gInventory.getItem(inv_id);
		item_name = itemp ? itemp->getName() : "|Not found|";
	}

	lua_getglobal(mLuaState, "OnObjectAttached");
	lua_pushstring(mLuaState,
				   gAgentAvatarp->getAttachedPointName(inv_id).c_str());
	lua_pushstring(mLuaState, item_name.c_str());
	lua_pushstring(mLuaState, inv_id.asString().c_str());
	lua_pushstring(mLuaState, obj_id.asString().c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 4, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

//static
void HBViewerAutomation::doCallOnAgentBaked(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || !self->mHasOnAgentBaked || !isAgentAvatarValid()) return;

	// Double check...
	if (!gAgent.isGodlikeWithoutAdminMenuFakery() &&
		 !enable_avatar_textures(NULL))
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnAgentBaked Lua callback." << LL_ENDL;

	lua_getglobal(statep, "OnAgentBaked");

	lua_newtable(statep);
	std::string te_name;
	uuid_vec_t ids;
	for (S32 i = 0, count = gAgentAvatarp->getNumTEs(); i < count; ++i)
	{
		LLFloaterAvatarTextures::getTextureIds(gAgentAvatarp, ETextureIndex(i),
											   te_name, ids);
		const LLUUID& id = ids[0];
		if (id != IMG_DEFAULT_AVATAR &&
			te_name.rfind("-baked") != std::string::npos)
		{
			lua_pushstring(statep, te_name.c_str());
			lua_pushstring(statep, id.asString().c_str());
			lua_rawset(statep, -3);
		}
	}

	self->resetTimer();
	if (lua_pcall(statep, 1, 0, 0) != LUA_OK)
	{
		self->reportError();
	}
}

void HBViewerAutomation::onRadar(const LLUUID& id, const std::string& name,
								 S32 range, bool marked)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnRadar || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnRadar Lua callback. id=" << id
					 << " - name=" << name << " - range=" << range
					  << " - marked=" << marked << LL_ENDL;

	lua_getglobal(mLuaState, "OnRadar");
	lua_pushstring(mLuaState, id.asString().c_str());
	lua_pushstring(mLuaState, name.c_str());
	lua_pushinteger(mLuaState, range);
	lua_pushboolean(mLuaState, marked);
	resetTimer();
	if (lua_pcall(mLuaState, 4, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onRadarSelection(const uuid_vec_t& ids)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnRadarSelection || !mLuaState || ids.empty()) return;

	S32 count = ids.size();
	LL_DEBUGS("Lua") << "Invoking OnRadarSelection Lua callback with " << count
					 << " selected radar entries." << LL_ENDL;

	lua_getglobal(mLuaState, "OnRadarSelection");

	lua_newtable(mLuaState);
	for (S32 i = 0; i < count; ++i)
	{
		lua_pushstring(mLuaState, ids[i].asString().c_str());
		lua_rawseti(mLuaState, -2, i + 1);
	}

	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onRadarMark(const LLUUID& id, const std::string& name,
									 bool marked)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnRadarMark || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnRadarMark Lua callback. avid=" << id
					 << " - name=" << name << " - marked="
					 << (marked ? "true" : "false") << LL_ENDL;

	lua_getglobal(mLuaState, "OnRadarMark");
	lua_pushstring(mLuaState, id.asString().c_str());
	lua_pushstring(mLuaState, name.c_str());
	lua_pushboolean(mLuaState, marked);
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onRadarTrack(const LLUUID& id,
									  const std::string& name, bool tracked)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnRadarTrack || !mLuaState || sIgnoredCallbacks[E_ONRADARTRACK])
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnRadarTrack Lua callback. avid=" << id
					 << " - name=" << name << " - tracking="
					 << (tracked ? "true" : "false") << LL_ENDL;

	lua_getglobal(mLuaState, "OnRadarTrack");
	lua_pushstring(mLuaState, id.asString().c_str());
	lua_pushstring(mLuaState, name.c_str());
	lua_pushboolean(mLuaState, tracked);
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onLuaDialogClose(const std::string& title, S32 button,
										  const std::string& text)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnLuaDialogClose || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnLuaDialogClose Lua callback. button="
					 << button << " - text=" << text << LL_ENDL;

	lua_getglobal(mLuaState, "OnLuaDialogClose");
	lua_pushstring(mLuaState, title.c_str());
	lua_pushinteger(mLuaState, button);
	lua_pushstring(mLuaState, text.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onLuaFloaterAction(const std::string& floater_name,
											const std::string& ctrl_name,
											const std::string& value)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnLuaFloaterAction || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnLuaFloaterAction Lua callback. Floater: "
					 << floater_name << " - Control: " << ctrl_name
					 << " - Value: " << value << LL_ENDL;

	lua_getglobal(mLuaState, "OnLuaFloaterAction");
	lua_pushstring(mLuaState, floater_name.c_str());
	lua_pushstring(mLuaState, ctrl_name.c_str());
	lua_pushstring(mLuaState, value.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onLuaFloaterOpen(const std::string& floater_name,
										  const std::string& parameter)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnLuaFloaterOpen || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnLuaFloaterOpen Lua callback. Floater: "
					 << floater_name << LL_ENDL;

	lua_getglobal(mLuaState, "OnLuaFloaterOpen");
	lua_pushstring(mLuaState, floater_name.c_str());
	lua_pushstring(mLuaState, parameter.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 2, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onLuaFloaterClose(const std::string& floater_name,
										   const std::string& parameter)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnLuaFloaterClose || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnLuaFloaterClose Lua callback. Floater: "
					 << floater_name << LL_ENDL;

	lua_getglobal(mLuaState, "OnLuaFloaterClose");
	lua_pushstring(mLuaState, floater_name.c_str());
	lua_pushstring(mLuaState, parameter.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 2, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onSideBarVisibilityChange(bool visible)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnSideBarVisibilityChange || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnSideBarVisibilityChange Lua callback. visible="
					 << (visible ? "true" : "false") << LL_ENDL;

	lua_getglobal(mLuaState, "OnSideBarVisibilityChange");
	lua_pushboolean(mLuaState, visible);
	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onTPStateChange(S32 state, const std::string& reason)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnTPStateChange || !mLuaState)
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnTPStateChange Lua callback. state="
					 << state << " - Reason: " << reason << LL_ENDL;

	lua_getglobal(mLuaState, "OnTPStateChange");
	lua_pushinteger(mLuaState, state);
	lua_pushstring(mLuaState, reason.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 2, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onFailedTPSimChange(S32 agents_count)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnFailedTPSimChange || !mLuaState ||
		// Is a teleport in progress ?
		gAgent.teleportInProgress() ||
		// Are there valid global TP coordinates available ?
		gAgent.getTeleportedPosGlobal().isExactlyZero())
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnFailedTPSimChange Lua callback. agents_count="
					 << agents_count << LL_ENDL;

	lua_getglobal(mLuaState, "OnFailedTPSimChange");
	lua_pushinteger(mLuaState, agents_count);
	lua_pushinteger(mLuaState, gAgent.getTeleportedPosGlobal().mdV[VX]);
	lua_pushinteger(mLuaState, gAgent.getTeleportedPosGlobal().mdV[VY]);
	lua_pushinteger(mLuaState, gAgent.getTeleportedPosGlobal().mdV[VZ]);
	resetTimer();
	if (lua_pcall(mLuaState, 4, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onWindlightChange(const std::string& sky_settings,
										   const std::string& water_settings,
										   const std::string& day_settings)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnWindlightChange || !mLuaState ||
		sIgnoredCallbacks[E_ONWINDLIGHTCHANGE])
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnWindlightChange Lua callback. sky_settings_name="
					 << sky_settings << " - water_settings_name="
					 << water_settings << " - day_settings_name="
					 << day_settings << LL_ENDL;

	lua_getglobal(mLuaState, "OnWindlightChange");
	lua_pushstring(mLuaState, sky_settings.c_str());
	lua_pushstring(mLuaState, water_settings.c_str());
	lua_pushstring(mLuaState, day_settings.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onCameraModeChange(S32 mode)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnCameraModeChange || !mLuaState ||
		sIgnoredCallbacks[E_ONCAMERAMODECHANGE])
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnCameraModeChange Lua callback. mode="
					 << mode << LL_ENDL;

	lua_getglobal(mLuaState, "OnCameraModeChange");
	lua_pushinteger(mLuaState, mode);
	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onJoystickButtons(S32 old_state, S32 new_state)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnJoystickButtons || !mLuaState)
	{
		return;
	}
	LL_DEBUGS("Lua") << "Invoking OnJoystickButtons Lua callback. old_state="
					 << old_state << " - new_state=" << new_state << LL_ENDL;
	lua_getglobal(mLuaState, "OnJoystickButtons");
	lua_pushinteger(mLuaState, old_state);
	lua_pushinteger(mLuaState, new_state);
	resetTimer();
	if (lua_pcall(mLuaState, 2, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onLuaPieMenu(U32 slice, S32 type, const LLPickInfo& pick)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnLuaPieMenu || !mLuaState)
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnLuaPieMenu Lua callback." << LL_ENDL;

	lua_getglobal(mLuaState, "OnLuaPieMenu");

	lua_newtable(mLuaState);

	lua_pushliteral(mLuaState, "type");
	lua_pushinteger(mLuaState, type);
	lua_rawset(mLuaState, -3);

	lua_pushliteral(mLuaState, "slice");
	lua_pushinteger(mLuaState, slice);
	lua_rawset(mLuaState, -3);

	const LLVector3d& pos_global = pick.mPosGlobal;
	lua_pushliteral(mLuaState, "global_x");
	lua_pushnumber(mLuaState, pos_global.mdV[VX]);
	lua_rawset(mLuaState, -3);

	lua_pushliteral(mLuaState, "global_y");
	lua_pushnumber(mLuaState, pos_global.mdV[VY]);
	lua_rawset(mLuaState, -3);

	lua_pushliteral(mLuaState, "altitude");
	lua_pushnumber(mLuaState, pos_global.mdV[VZ]);
	lua_rawset(mLuaState, -3);

	const LLUUID& object_id = pick.mObjectID;
	lua_pushliteral(mLuaState, "object_id");
	lua_pushstring(mLuaState, object_id.asString().c_str());
	lua_rawset(mLuaState, -3);

	if (object_id.notNull())
	{
		lua_pushliteral(mLuaState, "object_face");
		lua_pushinteger(mLuaState, pick.mObjectFace);
		lua_rawset(mLuaState, -3);
	}

	if (type == PICKED_PARTICLE)
	{
		lua_pushliteral(mLuaState, "particle_owner_id");
		lua_pushstring(mLuaState, pick.mParticleOwnerID.asString().c_str());
		lua_rawset(mLuaState, -3);

		lua_pushliteral(mLuaState, "particle_source_id");
		lua_pushstring(mLuaState, pick.mParticleSourceID.asString().c_str());
		lua_rawset(mLuaState, -3);
	}

	resetTimer();
	if (lua_pcall(mLuaState, 1, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

//static
void HBViewerAutomation::contextMenuCallback(HBContextMenuData* datap)
{
	if (gAutomationp && datap)
	{
		bool ret = gAutomationp->onContextMenu(datap->mHandlerID,
											   datap->mOperation,
											   datap->mMenuType);
		if (ret)
		{
			// When the OnContextMenu Lua callback returns true, perform the
			// default operation, where appropriate.
			switch (datap->mOperation)
			{
				case HBContextMenuData::SET:
					ret = LLEditMenuHandler::setCustomMenu(datap->mHandlerID,
														   "Cut to Lua",
														   "Copy to Lua",
														   "Paste from Lua");
					LL_DEBUGS("Lua") << "Default Lua context entries creation "
									 << (ret ? "succeeded" : "failed")
									 << " for handler_id=" << datap->mHandlerID
									 << LL_ENDL;
					
					break;

				case HBContextMenuData::PASTE:
					ret = LLEditMenuHandler::pasteTo(datap->mHandlerID);
					LL_DEBUGS("Lua") << "Pasting "
									 << (ret ? "succeeded" : "failed")
									 << " to handler_id=" << datap->mHandlerID
									 << LL_ENDL;
					break;

				default:
					LL_DEBUGS("Lua") << "handler_id=" << " - operation="
									 << datap->mOperation << LL_ENDL;
			}
		}
		else
		{
			LL_DEBUGS("Lua") << "No default action taken for handler_id="
							 << datap->mHandlerID << " - operation="
							 << datap->mOperation << LL_ENDL;
		}
	}
	delete datap;
}

bool HBViewerAutomation::onContextMenu(U32 handler_id, S32 operation,
									   const std::string& type)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnContextMenu || !mLuaState)
	{
		return false;
	}
	LL_DEBUGS("Lua") << "Invoking OnContextMenu Lua callback. handler_id="
					 << handler_id << " - operation=" << operation
					 << " - type=" << type << LL_ENDL;
	lua_getglobal(mLuaState, "OnContextMenu");
	lua_pushstring(mLuaState, type.c_str());
	lua_pushinteger(mLuaState, handler_id);
	lua_pushinteger(mLuaState, operation);
	lua_pushstring(mLuaState,
				   wstring_to_utf8str(gClipboard.getClipBoardString()).c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 4, 1, 0) != LUA_OK)
	{
		reportError();
		return false;
	}
	if (lua_gettop(mLuaState) == 0 || lua_type(mLuaState, -1) != LUA_TBOOLEAN)
	{
		lua_settop(mLuaState, 0);	// Sanitize stack by emptying it
		lua_pushliteral(mLuaState,
						"OnContextMenu() Lua callback did not return a boolean");
		reportError();
		return false;
	}

	bool result = lua_toboolean(mLuaState, -1);
	lua_pop(mLuaState, 1);
	return result;
}

void HBViewerAutomation::onRLVHandleCommand(const LLUUID& object_id,
											const std::string& behav,
											const std::string& option,
											const std::string& param)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnRLVHandleCommand || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnRLVHandleCommand Lua callback. Object Id: "
					 << object_id << " - behav=" << behav << " - option="
					 << option << " - param=" << param << LL_ENDL;

	lua_getglobal(mLuaState, "OnRLVHandleCommand");
	lua_pushstring(mLuaState, object_id.asString().c_str());
	lua_pushstring(mLuaState, behav.c_str());
	lua_pushstring(mLuaState, option.c_str());
	lua_pushstring(mLuaState, param.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 4, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onRLVAnswerOnChat(const LLUUID& obj_id, S32 channel,
										   const std::string& text)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnRLVAnswerOnChat || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnRLVAnswerOnChat Lua callback for object Id: "
					 << obj_id << " - channel: " << channel << LL_ENDL;

	lua_getglobal(mLuaState, "OnRLVAnswerOnChat");
	lua_pushstring(mLuaState, obj_id.asString().c_str());
	lua_pushinteger(mLuaState, channel);
	lua_pushstring(mLuaState, text.c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::onObjectInfoReply(const LLUUID& object_id,
										   const std::string& name,
										   const std::string& desc,
										   const LLUUID& owner_id,
										   const LLUUID& group_id)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnObjectInfoReply || !mLuaState) return;

	LL_DEBUGS("Lua") << "Invoking OnObjectInfoReply Lua callback. Object: "
					 << name << " (" << object_id << ")" << LL_ENDL;

	lua_getglobal(mLuaState, "OnObjectInfoReply");
	lua_pushstring(mLuaState, object_id.asString().c_str());
	lua_pushstring(mLuaState, name.c_str());
	lua_pushstring(mLuaState, desc.c_str());
	lua_pushstring(mLuaState, owner_id.asString().c_str());
	lua_pushstring(mLuaState, group_id.asString().c_str());
	resetTimer();
	if (lua_pcall(mLuaState, 5, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

void HBViewerAutomation::resetTimer()
{
	mWatchdogTimer.start();
	mWatchdogTimer.setTimerExpirySec(mWatchdogTimeout);
}

//static
void HBViewerAutomation::watchdog(lua_State* statep, lua_Debug*)
{
	HBViewerAutomation* self = findInstance(statep);
	if (self)
	{
		if (self->mWatchdogTimer.hasExpired())
		{
			lua_pushliteral(statep, "Lua watchdog timeout reached !");
			lua_error(statep);
		}
	}
	else
	{
		llwarns << "Lua instance gone !" << llendl;
	}
}

//static
bool HBViewerAutomation::requestObjectPropertiesFamily(const LLUUID& object_id,
													   U32 reason)
{
	LLMessageSystem* msg = gMessageSystemp;
	if (!msg || object_id.isNull())
	{
		return false;
	}
	// We need for the object to be around...
	LLViewerObject* objectp = gObjectList.findObject(object_id);
	if (!objectp)
	{
		return false;
	}
	// We need for the object to have a region (which should always be the
	// case)...
	LLViewerRegion* regionp = objectp->getRegion();
	if (!regionp)
	{
		return false;
	}

	bool in_mute = sMuteObjectRequests.count(object_id) != 0;
	bool in_unmute = sUnmuteObjectRequests.count(object_id) != 0;
	bool in_object_info = false;
	if (gAutomationp)
	{
		in_object_info =
			gAutomationp->mObjectInfoRequests.count(object_id) != 0;
	}

	switch (reason)
	{
		case 0:		// For mute
			if (in_mute)
			{
				return true;	// No need to re-request
			}
			sMuteObjectRequests.emplace(object_id);
			if (in_unmute || in_object_info)
			{
				return true;	// No need to re-request
			}
			break;

		case 1:		// For un-mute
			if (in_unmute)
			{
				return true;	// No need to re-request
			}
			sUnmuteObjectRequests.emplace(object_id);
			if (in_mute || in_object_info)
			{
				return true;	// No need to re-request
			}
			break;

		default:	// For object info request
			if (!gAutomationp)
			{
				return false;	// Not requesting if no automation script
			}
			if (in_object_info)
			{
				return true;	// No need to re-request
			}
			gAutomationp->mObjectInfoRequests.emplace(object_id);
			if (in_mute || in_unmute)
			{
				return true;	// No need to re-request
			}
	}

	msg->newMessageFast(_PREHASH_RequestObjectPropertiesFamily);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_RequestFlags, 0);
	msg->addUUIDFast(_PREHASH_ObjectID, object_id);
	msg->sendReliable(regionp->getHost());
	LL_DEBUGS("Lua") << "Sent data request for object " << object_id
					 << LL_ENDL;

	return true;
}

//static
void HBViewerAutomation::processObjectPropertiesFamily(LLMessageSystem* msg)
{
	LL_TRACY_TIMER(TRC_LUA_PROCESS_OBJ_PROP);

	if (!msg) return;

	LLUUID object_id;
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, object_id);

	uuid_list_t::iterator it = sMuteObjectRequests.find(object_id);
	bool for_mute = it != sMuteObjectRequests.end();
	if (for_mute)
	{
		sMuteObjectRequests.erase(it);
	}

	it = sUnmuteObjectRequests.find(object_id);
	bool for_unmute = it != sUnmuteObjectRequests.end();
	if (for_unmute)
	{
		sUnmuteObjectRequests.erase(it);
	}

	bool for_object_info = false;
	if (gAutomationp)
	{
		it = gAutomationp->mObjectInfoRequests.find(object_id);
		if (it != gAutomationp->mObjectInfoRequests.end())
		{
			for_object_info = true;
			gAutomationp->mObjectInfoRequests.erase(it);
		}
	}

	if (!for_mute && !for_unmute && !for_object_info)
	{
		// Object data not requested by us.
		return;
	}

	LLUUID owner_id, group_id;
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, owner_id);
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID, group_id);
	std::string name, desc;
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, name);
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, desc);

	// Process (un)mute first, in case we requested both one and object info
	if (for_mute || for_unmute)
	{
		LLMute mute(object_id, name, LLMute::OBJECT);
		if (for_mute)
		{
			LLMuteList::add(mute);
		}
		else
		{
			LLMuteList::remove(mute);
		}
	}

	if (for_object_info)
	{
		gAutomationp->onObjectInfoReply(object_id, name, desc, owner_id,
										group_id);
	}
}

//static
int HBViewerAutomation::print(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	S32 n = lua_gettop(statep);
	if (!n) return 0;

	std::string value;
	for (S32 i = 1; i <= n; ++i)
	{
		int type = lua_type(statep, i);
		switch (type)
		{
			case LUA_TNIL:
				value = "nil";
				break;

			case LUA_TBOOLEAN:
				value = lua_toboolean(statep, i) ? "true" : "false";
				break;

			case LUA_TNUMBER:
				value = llformat(LUA_NUMBER_FMT, lua_tonumber(statep, i));
				break;

			case LUA_TSTRING:
				value = lua_tostring(statep, i);
				break;

			case LUA_TTABLE:
				if (serializeTable(statep, i, &value))
				{
					break;
				}
				// Else, fall through.

			default:
				value = lua_typename(statep, type);
		}
		// NOTE: we need to delay chat printing until after login, since we
		// otherwise could crash due to LLFloaterChat not yet being
		// constructed.
		if (self->mUsePrintBuffer || !LLStartUp::isLoggedIn())
		{
			if (!self->mPrintBuffer.empty())
			{
#if LL_WINDOWS
				self->mPrintBuffer += "\r\n";
#else
				self->mPrintBuffer += '\n';
#endif
			}
			self->mPrintBuffer += value;
		}
		else if (!self->mUseConsoleOutput || !HBLuaConsole::print(value))
		{
			LLChat chat;
			chat.mFromName = "Lua";
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			chat.mText = "Lua: " + value;
			LLFloaterChat::addChat(chat, false, false);
		}
	}

	lua_pop(statep, n);

	return 0;
}

//static
int HBViewerAutomation::isUUID(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	bool valid = false;

	if (lua_type(statep, 1) == LUA_TSTRING)
	{
		std::string param(luaL_checkstring(statep, 1));
		valid = LLUUID::validate(param);
		// For a Null UUID, return 0 instead of true (0 is true as well for Lua
		// and it allows to easily test for a Null UUID).
		if (valid && LLUUID(param).isNull())
		{
			lua_pop(statep, 1);
			lua_pushinteger(statep, 0);
			return 1;
		}
	}
	lua_pop(statep, 1);

	lua_pushboolean(statep, valid);

	return 1;
}

//static
int HBViewerAutomation::isAvatar(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("IsAvatar");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	bool is_avatar = false;
	if (id.notNull())
	{
		is_avatar = gObjectList.findAvatar(id) != NULL;
	}
	lua_pushboolean(statep, is_avatar);

	return 1;
}

//static
int HBViewerAutomation::isObject(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("IsObject");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	bool is_object = false;
	if (id.notNull())
	{
		LLViewerObject* objectp = gObjectList.findObject(id);
		is_object = objectp && !objectp->isAvatar();
	}

	lua_pushboolean(statep, is_object);

	return 1;
}

//static
int HBViewerAutomation::isAgentFriend(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("IsAgentFriend");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string param(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	LLUUID id;
	if (LLUUID::validate(param))
	{
		id.set(param);
	}

	bool is_friend = false;
	bool is_online = false;

	if (id.notNull())
	{
		is_friend = LLAvatarTracker::isAgentFriend(id);
		is_online = is_friend && gAvatarTracker.isBuddyOnline(id);
	}
	else if (!param.empty())
	{
		// 'param' should contain the legacy name of the putative friend, with
		// the "Display Name [Legacy Name]" format accepted as well.
		size_t i = param.rfind(']');
		if (i == param.size() - 1)
		{
			size_t j = param.rfind('[');
			if (j != std::string::npos)
			{
				// This is indeed the "Display Name [Legacy Name]" format
				param = param.substr(j + 1, i - j - 1);
			}
		}
		// Eliminate the " Resident" last name if any.
		i = param.find(" Resident");
		if (i != std::string::npos)
		{
			param = param.substr(0, i);
		}

		// Collect all our friends in a map
		LLCollectAllBuddies friends;
		gAvatarTracker.applyFunctor(friends);
		// Try and find a matching friend name (case-sensitive)
		std::string name;
		typedef LLCollectAllBuddies::buddy_map_t::const_iterator buddies_it;
		for (buddies_it it = friends.mOnline.begin(),
						end = friends.mOnline.end();
			 it != end; ++it)
		{
			name = it->first;
			i = name.find(" Resident");
			if (i != std::string::npos)
			{
				name = name.substr(0, i);
			}
			if (name == param)
			{
				is_friend = is_online = true;
				break;
			}
		}
		if (!is_friend)
		{
			for (buddies_it it = friends.mOffline.begin(),
							end = friends.mOffline.end();
				 it != end; ++it)
			{
				name = it->first;
				i = name.find(" Resident");
				if (i != std::string::npos)
				{
					name = name.substr(0, i);
				}
				if (name == param)
				{
					is_friend = true;
					break;
				}
			}
		}
	}

	lua_pushboolean(statep, is_friend);
	lua_pushboolean(statep, is_online);

	return 2;
}

//static
int HBViewerAutomation::isAgentGroup(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("IsAgentGroup");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	bool is_in_group = false;
	if (id.notNull())
	{
		is_in_group = gAgent.isInGroup(id, true);
	}

	lua_pushboolean(statep, is_in_group);
	lua_pushboolean(statep, is_in_group && gAgent.getGroupID() == id);

	return 2;
}

//static
int HBViewerAutomation::getAvatarName(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAvatarName");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);

	S32 type = 0;
	if (n > 1)
	{
		type = luaL_checknumber(statep, 2);
	}

	lua_pop(statep, n);

	std::string name;
	if (id.notNull() && gCacheNamep && !gCacheNamep->getFullName(id, name))
	{
		name.clear();	// Prevents "loading..."
	}
	if (type != 0 && !name.empty())
	{
		LLAvatarName avatar_name;
		if (LLAvatarNameCache::get(id, &avatar_name))
		{
			if (type == 1)
			{
				name = avatar_name.mDisplayName;
			}
			else
			{
				name = avatar_name.getNames();
			}
		}
	}

	lua_pushstring(statep, name.c_str());

	return 1;
}

//static
int HBViewerAutomation::getGroupName(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetGroupName");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	std::string name;
	if (id.notNull() && gCacheNamep && !gCacheNamep->getGroupName(id, name))
	{
		name.clear();	// Prevents "loading..."
	}

	lua_pushstring(statep, name.c_str());

	return 1;
}

//static
int HBViewerAutomation::getAgentFriends(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentFriends");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	S32 type = 0;	// Legacy names by default
	if (n)
	{
		type = llclamp(luaL_checknumber(statep, 1), -1, 2);
		lua_pop(statep, 1);
	}
	if (type > 0 && !LLAvatarNameCache::useDisplayNames())
	{
		llwarns_once << "Display names not available or disabled; falling back to legacy names."
					 << llendl;
		type = 0;
	}
	if (type == -1)	// Names as listed in the Friends floater
	{
		if (LLAvatarName::sLegacyNamesForFriends)
		{
			type = 0;
		}
		else
		{
			type = LLAvatarNameCache::useDisplayNames();
		}
	}

	lua_newtable(statep);

	// Get all buddies we know about
	std::string fullname;
	LLAvatarName avatar_name;
	LLAvatarTracker::buddy_map_t all_buddies;
	gAvatarTracker.copyBuddyList(all_buddies);
	for (LLAvatarTracker::buddy_map_t::const_iterator it = all_buddies.begin(),
													  end = all_buddies.end();
		 it != end; ++it)
	{
		const LLUUID& id = it->first;

		fullname.clear();
		lua_pushstring(statep, id.asString().c_str());
		bool has_name = gCacheNamep &&
						gCacheNamep->getFullName(id, fullname);
		if (has_name && type)
		{
			if (LLAvatarNameCache::get(id, &avatar_name))
			{
				if (type == 2)
				{
					fullname = avatar_name.mDisplayName;
				}
				else
				{
					fullname = avatar_name.getNames();
				}
			}
		}
		lua_pushstring(statep, fullname.c_str());
		lua_rawset(statep, -3);
	}

	return 1;
}

//static
int HBViewerAutomation::getAgentGroups(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentGroups");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_newtable(statep);

	for (U32 i = 0, count = gAgent.mGroups.size(); i < count; ++i)
	{
		const LLGroupData& group_data = gAgent.mGroups[i];
		lua_pushstring(statep, group_data.mID.asString().c_str());
		lua_pushstring(statep, group_data.mName.c_str());
		lua_rawset(statep, -3);
	}

	return 1;
}

//static
int HBViewerAutomation::isAdmin(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("IsAdmin");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string param(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	std::string name;
	if (LLUUID::validate(param))
	{
		LLUUID av_id(param);
		if (av_id.notNull() && gCacheNamep &&
			gCacheNamep->getName(av_id, name, param))
		{
			name += " " + param;
		}
		else
		{
			name.clear();
		}
	}
	else
	{
		name = param;
	}

	if (name.empty())
	{
		lua_pushnil(statep);
	}
	else
	{
		lua_pushboolean(statep, LLMuteList::isLinden(name));
	}

	return 1;
}

//static
int HBViewerAutomation::getRadarList(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetRadarList");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	S32 type = 0;	// Legacy names by default
	if (n)
	{
		type = llclamp(luaL_checknumber(statep, 1), -1, 2);
		lua_pop(statep, 1);
	}
	if (type > 0 && !LLAvatarNameCache::useDisplayNames())
	{
		llwarns_once << "Display names not available or disabled; falling back to legacy names."
					 << llendl;
		type = 0;
	}

	HBFloaterRadar* avlistp = HBFloaterRadar::findInstance();
	if (!avlistp)
	{
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);

	std::string fullname;
	LLAvatarName avatar_name;
	const HBFloaterRadar::avatar_list_t& list = avlistp->getAvatarList();
	for (HBFloaterRadar::avatar_list_t::const_iterator it = list.begin(),
													   end = list.end();
		 it != end; ++it)
	{
		const HBRadarListEntry& entry = it->second;
		if (!entry.isDead())
		{
			const LLUUID& id = it->first;
			lua_pushstring(statep, id.asString().c_str());
			if (type == -1)
			{
				fullname = entry.getListedName();
			}
			else if (type && LLAvatarNameCache::get(id, &avatar_name))
			{
				if (type == 2)
				{
					fullname = avatar_name.mDisplayName;
				}
				else
				{
					fullname = avatar_name.getNames();
				}
			}
			else	// type == 0 and fallback path
			{
				fullname = entry.getLegacyName();
			}
			lua_pushstring(statep, fullname.c_str());
			lua_rawset(statep, -3);
		}
	}

	return 1;
}

//static
int HBViewerAutomation::getRadarData(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetRadarData");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	HBRadarListEntry* entryp = NULL;
	if (id.notNull())
	{
		HBFloaterRadar* avlistp = HBFloaterRadar::findInstance();
		if (avlistp)
		{
			entryp = avlistp->getAvatarEntry(id);
		}
	}
	if (!entryp || entryp->isDead())
	{
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);

	lua_pushliteral(statep, "id");
	lua_pushstring(statep, id.asString().c_str());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "name");
	lua_pushstring(statep, entryp->getLegacyName().c_str());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "display_name");
	lua_pushstring(statep, entryp->getListedName().c_str());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "name_color");
	const LLColor4& name_color = entryp->getColor();
	lua_pushstring(statep, llformat("%f, %f, %f",
									name_color.mV[0],
									name_color.mV[1],
									name_color.mV[2]).c_str());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "tooltip");
	lua_pushstring(statep, entryp->getToolTip().c_str());
	lua_rawset(statep, -3);

	const LLVector3d& global_pos = entryp->getPosition();
	lua_pushliteral(statep, "global_x");
	lua_pushnumber(statep, global_pos.mdV[VX]);
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "global_y");
	lua_pushnumber(statep, global_pos.mdV[VY]);
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "altitude");
	lua_pushnumber(statep, global_pos.mdV[VZ]);
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "friend");
	lua_pushboolean(statep, entryp->isFriend());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "muted");
	lua_pushboolean(statep, entryp->isMuted());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "derendered");
	lua_pushboolean(statep, entryp->isDerendered());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "marked");
	lua_pushboolean(statep, entryp->isMarked());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "mark_char");
	lua_pushstring(statep, entryp->getMarkChar().c_str());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "mark_color");
	const LLColor4& mark_color = entryp->getMarkColor();
	lua_pushstring(statep, llformat("%f, %f, %f",
									mark_color.mV[0],
									mark_color.mV[1],
									mark_color.mV[2]).c_str());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "focused");
	lua_pushboolean(statep, entryp->isFocused());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "drawn");
	lua_pushboolean(statep, entryp->isDrawn());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "in_sim");
	lua_pushboolean(statep, entryp->isInSim());
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "entry_age");
	lua_pushnumber(statep, entryp->getEntryAgeSeconds());
	lua_rawset(statep, -3);

	return 1;
}

//static
int HBViewerAutomation::setRadarTracking(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetRadarTracking");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}
	LLUUID id(luaL_checkstring(statep, 1), false);
	bool force = n >= 2 && lua_toboolean(statep, 2);
	lua_pop(statep, n);

	HBIgnoreCallback lock_on_radar_track(E_ONRADARTRACK);

	bool success = false;

	HBFloaterRadar* avlistp = HBFloaterRadar::findInstance();
	if (id.isNull())
	{
		if (avlistp)
		{
			avlistp->stopTracker();
		}
		success = true;
	}
	else if (avlistp)
	{
		success = avlistp->startTracker(id);
	}
	else if (force)
	{
		if (!gSavedSettings.getBool("RadarKeepOpen"))
		{
			llinfos << "Enabling Radar background tracking" << llendl;
			gSavedSettings.setBool("RadarKeepOpen", true);
		}
		avlistp = HBFloaterRadar::getInstance();
		if (avlistp)
		{
			success = avlistp->startTracker(id);
			HBFloaterRadar::hideInstance();
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setRadarToolTip(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetRadarToolTip");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}
	LLUUID id(luaL_checkstring(statep, 1), false);
	std::string tooltip;
	if (n > 1)
	{
		tooltip = luaL_checkstring(statep, 2);
	}
	lua_pop(statep, n);

	HBRadarListEntry* entryp = NULL;
	if (id.notNull())
	{
		HBFloaterRadar* avlistp = HBFloaterRadar::findInstance();
		if (avlistp)
		{
			entryp = avlistp->getAvatarEntry(id);
		}
	}

	bool success = entryp && !entryp->isDead();
	if (success)
	{
		entryp->setToolTip(tooltip);
	}
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setRadarMarkChar(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetRadarMarkChar");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}
	LLUUID id(luaL_checkstring(statep, 1), false);
	std::string chr;
	if (n > 1)
	{
		chr = luaL_checkstring(statep, 2);
	}
	lua_pop(statep, n);

	HBRadarListEntry* entryp = NULL;
	if (id.notNull())
	{
		HBFloaterRadar* avlistp = HBFloaterRadar::findInstance();
		if (avlistp)
		{
			entryp = avlistp->getAvatarEntry(id);
		}
	}

	bool success = entryp && !entryp->isDead();
	if (success)
	{
		entryp->setMarkChar(chr);
	}
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setRadarMarkColor(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetRadarMarkColor");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);
	std::string color_str;
	if (n > 1)
	{
		color_str = luaL_checkstring(statep, 2);
	}
	lua_pop(statep, n);

	LLColor4 color;
	if (color_str.empty())
	{
		color = gColors.getColor("RadarMarkColor");
	}
	else if (!LLColor4::parseColor(color_str, &color))
	{
		luaL_error(statep, "invalid color: %s", color_str.c_str());
	}
	else
	{
		color.mV[3] = 1.f;	// Make sure we use an opaque color...
	}

	HBRadarListEntry* entryp = NULL;
	if (id.notNull())
	{
		HBFloaterRadar* avlistp = HBFloaterRadar::findInstance();
		if (avlistp)
		{
			entryp = avlistp->getAvatarEntry(id);
		}
	}

	bool success = entryp && !entryp->isDead();
	if (success)
	{
		entryp->setMarkColor(color);
	}
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setRadarNameColor(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetRadarNameColor");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);

	std::string color_str;
	if (n > 1)
	{
		color_str = luaL_checkstring(statep, 2);
	}

	lua_pop(statep, n);

	bool success = false;

	LLColor4 color;
	if (color_str.empty())
	{
		color = LLColor4::black;
	}
	else if (!LLColor4::parseColor(color_str, &color))
	{
		luaL_error(statep, "invalid color: %s", color_str.c_str());
	}
	else
	{
		color.mV[3] = 1.f;	// Make sure we use an opaque color...
	}

	if (id != gAgentID && id.notNull())
	{
		LLVOAvatar* avatarp = gObjectList.findAvatar(id);
		success = avatarp != NULL;
		if (success)
		{
			avatarp->setRadarColor(color);
			success = HBFloaterRadar::setAvatarNameColor(id, color);
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setAvatarMinimapColor(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetAvatarMinimapColor");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);

	std::string color_str;
	if (n > 1)
	{
		color_str = luaL_checkstring(statep, 2);
	}

	lua_pop(statep, n);

	bool success = true;

	LLColor4 color;
	if (color_str.empty())
	{
		static LLCachedControl<LLColor4U> map_avatar(gColors, "MapAvatar");
		static LLCachedControl<LLColor4U> map_friend(gColors, "MapFriend");
		bool is_friend = LLAvatarTracker::isAgentFriend(id);
		color = LLColor4(is_friend ? map_friend : map_avatar);
	}
	else if (!LLColor4::parseColor(color_str, &color))
	{
		luaL_error(statep, "invalid color: %s", color_str.c_str());
	}

	if (id == gAgentID)
	{
		success = false;
	}
	else
	{
		LLVOAvatar* avatarp = gObjectList.findAvatar(id);
		success = avatarp != NULL;
		if (success)
		{
			avatarp->setMinimapColor(color);
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setAvatarNameTagColor(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetAvatarNameTagColor");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);

	std::string color_str;
	if (n > 1)
	{
		color_str = luaL_checkstring(statep, 2);
	}

	lua_pop(statep, n);

	bool success = true;

	LLColor4 color;
	if (color_str.empty())
	{
		static LLCachedControl<LLColor4U> tag_color(gColors,
													"AvatarNameColor");
		color = LLColor4(tag_color);
	}
	else if (!LLColor4::parseColor(color_str, &color))
	{
		luaL_error(statep, "invalid color: %s", color_str.c_str());
	}

	if (id == gAgentID)
	{
		success = isAgentAvatarValid();
		if (success)
		{
			gAgentAvatarp->setNameTagColor(color);
		}
	}
	else
	{
		LLVOAvatar* avatarp = gObjectList.findAvatar(id);
		success = avatarp != NULL;
		if (success)
		{
			avatarp->setNameTagColor(color);
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
void HBViewerAutomation::addToAgentPosHistory(const LLVector3d& global_pos)
{
	static LLCachedControl<U32> max_history(gSavedSettings,
											"LuaMaxAgentPosHistorySize");
	while (sPositionsHistory.size() >= (size_t)max_history)
	{
		sPositionsHistory.pop_front();
	}
	if (max_history > 0)
	{
		sPositionsHistory.emplace_back(global_pos);
	}
}

//static
int HBViewerAutomation::getAgentPosHistory(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentPosHistory");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	if (sPositionsHistory.empty())
	{
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);
	S32 i = 1;
	std::string vecstr;
	// Place the positions in reverse order in the Lua table (i.e. last known
	// position will be first in the table).
	for (pos_history_t::reverse_iterator it = sPositionsHistory.rbegin(),
										 end = sPositionsHistory.rend();
		 it != end; ++it)
	{
		vecstr = llformat("%lf %lf %lf", it->mdV[0], it->mdV[1], it->mdV[2]);
		lua_pushnumber(statep, i++);
		lua_pushstring(statep, vecstr.c_str());
		lua_rawset(statep, -3);
	}
	return 1;
}

//static
int HBViewerAutomation::getAgentInfo(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentInfo");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_newtable(statep);

	std::string temp;
	gAgent.getName(temp);
	lua_pushliteral(statep, "name");
	lua_pushstring(statep, temp.c_str());
	lua_rawset(statep, -3);

	if (LLStartUp::isLoggedIn() && isAgentAvatarValid())
	{
		lua_pushliteral(statep, "id");
		lua_pushstring(statep, gAgentID.asString().c_str());
		lua_rawset(statep, -3);

		LLAvatarName avatar_name;
		if (LLAvatarNameCache::get(gAgentID, &avatar_name))
		{
			lua_pushliteral(statep, "display_name");
			lua_pushstring(statep, avatar_name.mDisplayName.c_str());
			lua_rawset(statep, -3);
		}

		temp = "unknown";
		if (gAgent.isTeen())
		{
			temp = "teen";
		}
		else if (gAgent.isAdult())
		{
			temp = "adult";
		}
		else if (gAgent.isMature())
		{
			temp = "mature";
		}
		lua_pushliteral(statep, "maturity");
		lua_pushstring(statep, temp.c_str());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "active_group_id");
		lua_pushstring(statep, gAgent.getGroupID().asString().c_str());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "camera_mode");
		lua_pushinteger(statep, gAgent.getCameraMode());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "control_flags");
		lua_pushinteger(statep, gAgent.getControlFlags());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "occupation");
		S32 occupation = 0;
		if (gAgent.getAFK())
		{
			occupation = 1;
		}
		else if (gAgent.getBusy())
		{
			occupation = 2;
		}
		else if (gAgent.getAutoReply())
		{
			occupation = 3;
		}
		lua_pushinteger(statep, occupation);
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "flying");
		lua_pushboolean(statep, gAgent.getFlying());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "sitting");
		lua_pushboolean(statep, gAgentAvatarp->mIsSitting);
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "sitting_on_ground");
		lua_pushboolean(statep, gAgent.sittingOnGround());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "baked");
		lua_pushboolean(statep, gAppearanceMgr.isAvatarFullyBaked());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "can_rebake_region");
		lua_pushboolean(statep,
						gOverlayBarp && gOverlayBarp->canRebakeRegion());
		lua_rawset(statep, -3);
//MK
		lua_pushliteral(statep, "rlv");
		lua_pushboolean(statep, gRLenabled);
		lua_rawset(statep, -3);

		if (gRLenabled)
		{
			std::string restrictions = ",";
			rl_map_it_t it = gRLInterface.mSpecialObjectBehaviours.begin();
			while (it != gRLInterface.mSpecialObjectBehaviours.end())
			{
				temp = it++->second + ",";
				if (restrictions.find("," + temp) == std::string::npos)
				{
					restrictions += temp;
				}
			}
			if (restrictions != ",")
			{
				restrictions = restrictions.substr(1, restrictions.size() - 2);
			}
			else
			{
				restrictions.clear();
			}
			lua_pushliteral(statep, "restrictions");
			lua_pushstring(statep, restrictions.c_str());
			lua_rawset(statep, -3);
		}
//mk
		LLEconomy* economyp = LLEconomy::getInstance();

		lua_pushliteral(statep, "max_upload_cost");
		lua_pushinteger(statep, economyp->getPriceUpload());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "animation_upload_cost");
		lua_pushinteger(statep, economyp->getAnimationUploadCost());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "sound_upload_cost");
		lua_pushinteger(statep, economyp->getSoundUploadCost());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "texture_upload_cost");
		lua_pushinteger(statep, economyp->getTextureUploadCost());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "create_group_cost");
		lua_pushinteger(statep, economyp->getCreateGroupCost());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "picks_limit");
		lua_pushinteger(statep, economyp->getPicksLimit());
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "group_membership_limit");
		lua_pushinteger(statep, gMaxAgentGroups);
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "attachment_limit");
		lua_pushinteger(statep, gMaxSelfAttachments);
		lua_rawset(statep, -3);

		lua_pushliteral(statep, "animated_object_limit");
		lua_pushinteger(statep,
						gAgentAvatarp->getMaxAnimatedObjectAttachments());
		lua_rawset(statep, -3);
	}

	return 1;
}

//static
int HBViewerAutomation::setAgentOccupation(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetAgentOccupation");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	S32 type = luaL_checknumber(statep, 1);
	lua_pop(statep, 1);

	HBIgnoreCallback lock_on_occupation(E_ONAGENTOCCUPATIONCHANGE);

	bool success = true;
	switch (type)
	{
		case 0:
			gAgent.clearAutoReply();
			gAgent.clearBusy();
			gAgent.clearAFK();
			break;

		case 1:
			gAgent.setAFK();
			break;

		case 2:
			gAgent.setBusy();
			break;

		case 3:
			gAgent.setAutoReply();
			break;

		default:
			success = false;
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::getAgentGroupData(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentGroupData");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	const LLUUID& current_group_id = gAgent.getGroupID();
	std::string group_name;
	if (n > 0)
	{
		group_name.assign(luaL_checkstring(statep, 1));
		lua_pop(statep, 1);
	}
	else
	{
		LLGroupData grp_data;
		if (gAgent.getGroupData(current_group_id, grp_data))
		{
			group_name = grp_data.mName;
		}
	}
	if (group_name.empty())
	{
		group_name = "none";
	}

	LL_DEBUGS("Lua") << "Searching group data for group: " << group_name
					 << LL_ENDL;

	lua_newtable(statep);

	// The first time this method is called, we scan the whole agent groups
	// list, even after we found the right group, so to ensure that data for
	// all groups will have been loaded via gGroupMgr.fetchGroupMissingData().
	static bool scanned_once = false;

	U64 powers = 0;
	bool active_group = current_group_id.isNull() && group_name == "none";
	bool success = false;
	for (S32 i = 0, count = gAgent.mGroups.size(); i < count; ++i)
	{
		if (success && scanned_once)
		{
			// Stop scanning if we found the right group and already did a full
			// scan beforehand.
			break;
		}

		LLGroupData* gdatap = &gAgent.mGroups[i];
		if (!gdatap) continue;	// Paranoia

		const LLUUID& group_id = gdatap->mID;

		LLGroupMgrGroupData* mgrdatap = gGroupMgr.getGroupData(group_id);
		if (!mgrdatap)
		{
			gGroupMgr.fetchGroupMissingData(group_id);
			LL_DEBUGS("Lua") << "Group data not yet received for group Id: "
							 << group_id << LL_ENDL;
			continue;
		}

		if (!success && group_id.asString() == group_name)
		{
			group_name = gdatap->mName;
			// Make sure we get all the data for this group
			gGroupMgr.fetchGroupMissingData(group_id);
		}

		if (!success && gdatap->mName == group_name)
		{
			LL_DEBUGS("Lua") << "Found matching group name: " << group_name
							 << " - Group Id: " << group_id << LL_ENDL;
			lua_pushliteral(statep, "group_id");
			lua_pushstring(statep, group_id.asString().c_str());
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "insignia_id");
			lua_pushstring(statep, gdatap->mInsigniaID.asString().c_str());
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "contribution");
			lua_pushinteger(statep, gdatap->mContribution);
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "in_profile");
			lua_pushboolean(statep, gdatap->mListInProfile);
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "accept_notices");
			lua_pushboolean(statep, gdatap->mAcceptNotices);
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "chat_muted");
			lua_pushboolean(statep,
							LLMuteList::isMuted(group_id,
												LLMute::flagTextChat));
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "founder_id");
			lua_pushstring(statep, mgrdatap->mFounderID.asString().c_str());
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "charter");
			lua_pushstring(statep, mgrdatap->mCharter.c_str());
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "fee");
			lua_pushinteger(statep, mgrdatap->mMembershipFee);
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "member_count");
			lua_pushinteger(statep, mgrdatap->mMemberCount);
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "open_enrollment");
			lua_pushboolean(statep, mgrdatap->mOpenEnrollment);
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "mature");
			lua_pushboolean(statep, mgrdatap->mMaturePublish);
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "members_list_ok");
			lua_pushboolean(statep, mgrdatap->isMemberDataComplete());
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "roles_list_ok");
			lua_pushboolean(statep, mgrdatap->isRoleDataComplete() &&
								   mgrdatap->isRoleMemberDataComplete());
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "properties_ok");
			lua_pushboolean(statep, mgrdatap->isGroupPropertiesDataComplete());
			lua_rawset(statep, -3);

			lua_pushliteral(statep, "group_titles_ok");
			lua_pushboolean(statep, mgrdatap->hasGroupTitles());
			lua_rawset(statep, -3);

			powers = gdatap->mPowers;

			if (group_id == current_group_id)
			{
				LL_DEBUGS("Lua") << "Group is active" << LL_ENDL;
				active_group = true;
			}

			for (U32 j = 0, count2 = mgrdatap->mTitles.size(); j < count2; ++j)
			{
				const LLGroupTitle& title = mgrdatap->mTitles[j];
				const LLUUID& title_id = title.mRoleID;
				const std::string& title_name = title.mTitle;
				lua_pushstring(statep, title_id.asString().c_str());
				lua_pushstring(statep, title_name.c_str());
				lua_rawset(statep, -3);
				LL_DEBUGS("Lua") << "Found group title: " << title_name
							 << " - Group title id: " << title_id << LL_ENDL;
				if (active_group &&	title.mSelected)
				{
					LL_DEBUGS("Lua") << "Group title is selected" << LL_ENDL;
					lua_pushliteral(statep, "current_title_id");
					lua_pushstring(statep, title_id.asString().c_str());
					lua_rawset(statep, -3);
					lua_pushliteral(statep, "current_title_name");
					lua_pushstring(statep, title_name.c_str());
					lua_rawset(statep, -3);
				}
			}

			success = true;
		}
	}

	scanned_once = true;

	lua_pushliteral(statep, "name");
	lua_pushstring(statep, group_name.c_str());
	lua_rawset(statep, -3);

	if (!success)
	{
		LL_DEBUGS("Lua") << "Group not found" << LL_ENDL;
		lua_pushliteral(statep, "group_id");
		lua_pushstring(statep, LLUUID::null.asString().c_str());
		lua_rawset(statep, -3);
	}

	lua_pushliteral(statep, "powers");
	lua_pushinteger(statep, powers);
	lua_rawset(statep, -3);

	lua_pushliteral(statep, "active");
	lua_pushboolean(statep, active_group);
	lua_rawset(statep, -3);

	return 1;
}

//static
int HBViewerAutomation::setAgentGroup(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetAgentGroup");
	}

	S32 n = lua_gettop(statep);
	if (n > 2)
	{
		luaL_error(statep, "%d arguments passed; expected 0 to 2.", n);
	}

	bool success = true;

	std::string param;
	LLUUID group_id;
	if (n > 0)
	{
		param = luaL_checkstring(statep, 1);
		if (param != "none" && param != LLUUID::null.asString())
		{
			LL_DEBUGS("Lua") << "Searching a match in agent's groups for: "
							 << param << LL_ENDL;

			for (S32 i = 0, count = gAgent.mGroups.size(); i < count; ++i)
			{
				LLGroupData* gdatap = &gAgent.mGroups[i];
				if (!gdatap) continue;	// Paranoia

				const LLUUID& id = gdatap->mID;
				const std::string& name = gdatap->mName;
				if (name == param || id.asString() == param)
				{
					group_id = id;
					LL_DEBUGS("Lua") << "Found group Id: " << group_id
									 << LL_ENDL;
					break;
				}
			}
			success = group_id.notNull();
		}
	}

	LLUUID role_id;
	if (success && n > 1 && group_id.notNull())
	{
		param = luaL_checkstring(statep, 2);

		LLGroupMgrGroupData* mgrdatap = gGroupMgr.getGroupData(group_id);
		if (mgrdatap)
		{
			LL_DEBUGS("Lua") << "Searching a match in roles for: " << param
							 << LL_ENDL;

			for (U32 i = 0, count = mgrdatap->mTitles.size(); i < count; ++i)
			{
				const LLGroupTitle& title = mgrdatap->mTitles[i];
				const LLUUID& title_id = title.mRoleID;
				const std::string& title_name = title.mTitle;
				if (title_name == param || title_id.asString() == param)
				{
					role_id = title_id;
					success = true;
					LL_DEBUGS("Lua") << "Found role Id: " << role_id
									 << LL_ENDL;
					break;
				}
			}
		}

		success = role_id.notNull();
		if (!success)
		{
			// Still try and set the group for now, at least...
			success = gAgent.setGroup(group_id);
			if (success)
			{
				LL_DEBUGS("Lua") << "Role/title not found; sending data requests for group Id "
								 << group_id
								 << ", with asynchronous title setting to: "
								 << param << LL_ENDL;
				gGroupMgr.fetchGroupMissingData(group_id);
				HBGroupTitlesObserver::addObserver(group_id, param);

				lua_pop(statep, n);
				// Return a special value which is not 'true' since we could
				// not set the title for now, but not 'false' either, since it
				// may finally get set, asynchronously... The 'nil' value is
				// also compatible with the older versions of SetAgentGroup()
				// which used to give up and return 'false' in this case.
				lua_pushnil(statep);
				return 1;
			}
		}
	}

	if (n)
	{
		lua_pop(statep, n);
	}

	// Set the group if needed.
	if (success)
	{
		success = gAgent.setGroup(group_id);
		LL_DEBUGS("Lua") << "Setting agent group "
						 << (success ? "succeeded" : " failed") << LL_ENDL;
	}

	if (success && group_id.notNull() && role_id.notNull())
	{
		// Set the title for this group
		LL_DEBUGS("Lua") << "Setting agent group title" << LL_ENDL;
		gGroupMgr.sendGroupTitleUpdate(group_id, role_id);
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::agentGroupInvite(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentGroupInvite");
	}

	S32 n = lua_gettop(statep);
	if (n != 2 && n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 2 or 3.", n);
	}

	U32 invited = 0;
	uuid_vec_t avids;
	if (lua_type(statep, 1) == LUA_TTABLE)
	{
		for (U32 i = 1, count = lua_rawlen(statep, 1); i <= count; ++i)
		{
			if (lua_rawgeti(statep, 1, i) == LUA_TSTRING)
			{
				LLUUID id(luaL_checkstring(statep, -1), false);
				if (id.notNull())
				{
					avids.emplace_back(id);
					++invited;
				}
			}
			lua_pop(statep, 1);
		}
	}
	else
	{
		LLUUID id(luaL_checkstring(statep, 1), false);
		if (id.notNull())
		{
			avids.emplace_back(id);
			invited = 1;
		}
	}

	LLUUID group_id(luaL_checkstring(statep, 2), false);
	if (group_id.isNull())
	{
		llwarns << "Invalid (null) group Id passed." << llendl;
		invited = 0;
	}

	LLUUID role_id;
	if (n > 2)
	{
		role_id.set(luaL_checkstring(statep, 2), false);
	}

	lua_pop(statep, n);

	if (invited)
	{
		if (!gGroupMgr.agentCanAddToRole(group_id, role_id))
		{
			LL_DEBUGS("Lua") << "Cannot invite to group Id " << group_id
							 << " with role Id " << role_id << LL_ENDL;
			invited = 0;
		}
		else if (invited > MAX_GROUP_INVITES)
		{
			llwarns << "Too many simultaneous group invitations requested ("
					<< invited << ") to group Id: " << group_id
					<< ". Only the first " << MAX_GROUP_INVITES
					<< " invitations will be sent." << llendl;
			invited = MAX_GROUP_INVITES;
		}
	}

	if (invited)
	{
		LLGroupMgrGroupData* gdatap = gGroupMgr.getGroupData(group_id);
		LLGroupMgrGroupData::member_list_t::iterator mit;
		LLGroupMgrGroupData::member_list_t::iterator mend =
			gdatap->mMembers.end();
		LLGroupMgr::role_member_pairs_t invites;
		for (U32 i = 0; i < invited; ++i)
		{
			const LLUUID& id = avids[i];
	 		mit = gdatap->mMembers.find(id);
			// Do not re-invite a member in the role they already got...
			if (mit == mend || !mit->second->isInRole(role_id))
			{
				invites[id] = role_id;
			}
		}
		gGroupMgr.sendGroupMemberInvites(group_id, invites);
	}

	lua_pushinteger(statep, invited);

	return 1;
}

//static
int HBViewerAutomation::agentSit(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentSit");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	if (n)
	{
		LLUUID object_id(luaL_checkstring(statep, 1), false);
		if (object_id.isNull())
		{
			luaL_error(statep, "Invalid object UUID passed as argument");
		}
		lua_pop(statep, 1);

		LLViewerObject* objectp = gObjectList.findObject(object_id);
		lua_pushboolean(statep, objectp && sit_on_object(objectp));
	}
	else
	{
		lua_pushboolean(statep, sit_on_ground());
	}

	return 1;
}

//static
int HBViewerAutomation::agentStand(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentStand");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_pushboolean(statep, stand_up());

	return 1;
}

//static
int HBViewerAutomation::setAgentTyping(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetAgentTyping");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	bool start = true;
	if (n == 1)
	{
		start = lua_toboolean(statep, 1);
		lua_pop(statep, 1);
	}

	LL_DEBUGS("Lua") << "start=" << (start ? "true" : "false") << LL_ENDL;

	if (start)
	{
		gAgent.startTyping();
	}
	else
	{
		gAgent.stopTyping();
	}

	return 0;
}

//static
int HBViewerAutomation::sendChat(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SendChat");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	std::string message(luaL_checkstring(statep, 1));

	std::string type;
	if (n > 1)
	{
		type.assign(luaL_checkstring(statep, 2));
	}

	lua_pop(statep, n);

	LL_DEBUGS("Lua") << "type=" << type << LL_ENDL;

	EChatType chat_type = CHAT_TYPE_NORMAL;
	if (type.find("whisper") != std::string::npos)
	{
		chat_type = CHAT_TYPE_WHISPER;
	}
	else if (type.find("shout") != std::string::npos)
	{
		chat_type = CHAT_TYPE_SHOUT;
	}

	bool animate = type.find("animate") != std::string::npos;

	if (gChatBarp)
	{
		HBIgnoreCallback lock_on_chat(E_ONSENDCHAT);
		gChatBarp->sendChatFromViewer(message, chat_type, animate, false);
	}

	return 0;
}

//static
int HBViewerAutomation::getIMSession(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || !gIMMgrp) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetIMSession");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	LLUUID target_id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	EInstantMessage dialog =
		gAgent.isInGroup(target_id, true) ? IM_SESSION_GROUP_START
										  : IM_NOTHING_SPECIAL;

	std::string name;
	if (gCacheNamep)
	{
		if (dialog == IM_SESSION_GROUP_START)
		{
			gCacheNamep->getGroupName(target_id, name);
		}
		else
		{
			gCacheNamep->getFullName(target_id, name);
		}
	}
	if (name.empty())
	{
		name = target_id.asString();
	}

	LLUUID session_id = gIMMgrp->addSession(name, dialog, target_id);

	LL_DEBUGS("Lua") << "target_id=" << target_id << " - target type: "
					 << (dialog == IM_SESSION_GROUP_START ? "group" : "agent")
					 << " - session_id=" << session_id << LL_ENDL;

	lua_pushstring(statep, session_id.asString().c_str());

	return 1;
}

//static
int HBViewerAutomation::closeIMSession(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("CloseIMSession");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	LLUUID session_id(luaL_checkstring(statep, 1), false);
	U32 duration = 0;
	if (n > 1)
	{
		duration = llmax((S32)lua_tointeger(statep, 2), 0);
	}
	lua_pop(statep, n);

	if (session_id.notNull())
	{
		LLFloaterIMSession* im_floater =
			LLFloaterIMSession::findInstance(session_id);
		if (im_floater)
		{
			if (duration)
			{
				bool success = im_floater->setSnoozeDuration(duration);
				if (success)
				{
					LL_DEBUGS("Lua") << "Snoozing group IM session for: "
									 << duration << " minutes." << LL_ENDL;
				}
				else
				{
					llwarns << "Cannot snooze IM session: " << session_id
							<< ". Only group IM sessions may be snoozed. Leaving session instead."
							<< llendl;
				}
			}
			else
			{
				LL_DEBUGS("Lua") << "Closing IM session: " << session_id
								 << LL_ENDL;
			}
			im_floater->close();
		}
	}

	return 0;
}

//static
int HBViewerAutomation::sendIM(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SendIM");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	LLUUID session_id(luaL_checkstring(statep, 1), false);
	std::string message(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	if (session_id.notNull())
	{
		LLFloaterIMSession* im_floater =
			LLFloaterIMSession::findInstance(session_id);
		if (im_floater)
		{
			LL_DEBUGS("Lua") << "other_participant_id=" << session_id
							 << LL_ENDL;
			HBIgnoreCallback lock_on_im(E_ONINSTANTMSG);
			im_floater->sendText(utf8str_to_wstring(message));
		}
	}

	return 0;
}

//static
int HBViewerAutomation::alertDialogResponse(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AlertDialogResponse");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	LLUUID alert_id(luaL_checkstring(statep, 1), false);
	S32 button_idx = luaL_checknumber(statep, 2) - 1;	// Index, not number
	lua_pop(statep, 2);

	bool result = false;
	if (button_idx >= 0 && alert_id.notNull())
	{
		LLAlertDialog* alertp =
			LLAlertDialog::getNamedInstance(alert_id).get();
		if (alertp && !alertp->isDead())
		{
			result = alertp->simulateClickButton(button_idx);
		}
	}

	lua_pushboolean(statep, result);

	return 1;
}

//static
int HBViewerAutomation::scriptDialogResponse(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ScriptDialogResponse");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	LLUUID notif_id(luaL_checkstring(statep, 1), false);
	std::string button(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	if (notif_id.notNull() && !button.empty())
	{
		LLNotifyBox* boxp = LLNotifyBox::getNamedInstance(notif_id).get();
		if (boxp && !boxp->isDead())
		{
			const LLNotifyBox::cb_data_vec_t& data = boxp->getCallbackData();
			for (S32 i = 0, count = data.size(); i < count; ++i)
			{
				if (data[i]->mButtonName == button)
				{
					LLSD response =
						boxp->getNotification()->getResponseTemplate();
					if (!boxp->isDefaultBtnAdded())
					{
						response[button] = true;
					}
					boxp->getNotification()->respond(response);
					lua_pushboolean(statep, true);
					return 1;
				}
			}
		}
	}

	lua_pushboolean(statep, false);

	return 1;
}

//static
int HBViewerAutomation::groupNotificationAcceptOffer(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GroupNotificationAcceptOffer");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID notif_id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	if (notif_id.notNull())
	{
		LLGroupNotifyBox* boxp =
			LLGroupNotifyBox::getNamedInstance(notif_id).get();
		if (boxp && !boxp->isDead() && boxp->hasInventory())
		{
			LLGroupNotifyBox::onClickSaveInventory(boxp);
			lua_pushboolean(statep, true);
			return 1;
		}
	}

	lua_pushboolean(statep, false);

	return 1;
}

//static
int HBViewerAutomation::cancelNotification(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("CancelNotification");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID notif_id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	if (notif_id.notNull())
	{
		LLNotificationPtr n = gNotifications.find(notif_id);
		if (n)
		{
			gNotifications.cancel(n);
			lua_pushboolean(statep, true);
			return 1;
		}
	}

	lua_pushboolean(statep, false);

	return 1;
}

//static
int HBViewerAutomation::getObjectInfo(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetObjectInfo");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID object_id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	lua_pushboolean(statep, requestObjectPropertiesFamily(object_id, 2));

	return 1;
}

//static
int HBViewerAutomation::browseToURL(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("BrowseToURL");
	}

	HBIgnoreCallback lock_on_dispatch_url(E_ONDISPATCHURL);

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	std::string url(luaL_checkstring(statep, 1));

	S32 browser = 0;
	if (n > 1)
	{
		browser = luaL_checknumber(statep, 2);
	}

	lua_pop(statep, n);

	LL_DEBUGS("Lua") << "Browsing with "
					 << (browser ? (browser == 1 ? "built-in" : "external")
								 : "preferred")
					 << " browser to URL: " << url << LL_ENDL;
	switch (browser)
	{
		case 1:
			LLWeb::loadURLInternal(url);
			break;

		case 2:
			LLWeb::loadURLExternal(url);
			break;

		default:
			LLWeb::loadURL(url);
	}

	return 0;
}

//static
int HBViewerAutomation::dispatchSLURL(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("DispatchSLURL");
	}

	HBIgnoreCallback lock_on_dispatch_slurl(E_ONDISPATCHSLURL);

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}
	std::string slurl(luaL_checkstring(statep, 1));
	bool trusted = n >= 2 && lua_toboolean(statep, 2);
	lua_pop(statep, n);

	LL_DEBUGS("Lua") << "Dispatching ("
					 << (trusted ? "trusted" : "untrusted") << "): " << slurl
					 << LL_ENDL;

	LLURLDispatcher::dispatch(slurl, "clicked", NULL, trusted);

	return 0;
}

//static
int HBViewerAutomation::executeRLV(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ExecuteRLV");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string rlvcmd(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	if (gRLenabled && rlvcmd.length())
	{
		LLStringUtil::toLower(rlvcmd);

		LL_DEBUGS("Lua") << "Executing RLV command: \"" << rlvcmd
						 << "\" on behalf of: " << self->mFromObjectName
						 << LL_ENDL;

		gRLInterface.queueCommands(self->mFromObjectId, self->mFromObjectName,
								   rlvcmd);
	}

	return 0;
}

//static
int HBViewerAutomation::openNotification(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("OpenNotification");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	S32 type = luaL_checknumber(statep, 1);
	std::string message(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	std::string name;
	switch (type)
	{
		case ALERT:
			name = "LuaAlert";
			break;

		case NOTIFICATION:
			name = "LuaNotification";
			break;

		case NOTIFYTIP:
			name = "LuaNotifyTip";
			break;

		default:
			luaL_error(statep, "Unknown notification type !");
	}

	LL_DEBUGS("Lua") << "Notification type: " << name << LL_ENDL;

	LLSD args;
	args["MESSAGE"] = message;
	gNotifications.add(name, args);

	return 0;
}

//static
int HBViewerAutomation::openFloater(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("OpenFloater");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	std::string name(luaL_checkstring(statep, 1));

	std::string param;
	LLUUID target_id;
	if (n == 2)
	{
		param.assign(luaL_checkstring(statep, 2));
		if (LLUUID::validate(param))
		{
			target_id.set(param);
		}
	}

	lua_pop(statep, n);

	LL_DEBUGS("Lua") << "Floater: " << name << " - parameter: " << param
					 << LL_ENDL;

	if (name == "active speakers")
	{
		LLFloaterActiveSpeakers::showInstance();
	}
	else if (name == "area search")
	{
		HBFloaterAreaSearch::showInstance();
	}
	else if (name == "beacons")
	{
		LLFloaterBeacons::showInstance();
	}
	else if (name == "avatar info")
	{
		if (self && gObjectList.findAvatar(target_id))
		{
			LLFloaterAvatarInfo::show(target_id);
		}
	}
	else if (name == "camera controls")
	{
		LLFloaterCamera::showInstance();
	}
	else if (name == "chat")
	{
		LLFloaterChat::showInstance();
	}
	else if (name == "debug settings")
	{
		LLFloaterDebugSettings::showInstance();
	}
	else if (name == "debug tags")
	{
		HBFloaterDebugTags::showInstance();
	}
	else if (name == "experiences")
	{
		LLFloaterExperiences::showInstance();
	}
	else if (name == "friends")
	{
		LLFloaterFriends::showInstance();
	}
	else if (name == "gestures")
	{
		LLFloaterGesture::showInstance();
	}
	else if (name == "group info")
	{
		if (self && gAgent.isInGroup(target_id))
		{
			LLFloaterGroupInfo::showFromUUID(target_id);
		}
	}
	else if (name == "groups")
	{
		LLFloaterGroups::showInstance();
	}
	else if (name == "inspect")
	{
		LLViewerObject* objp = gObjectList.findObject(target_id);
		if (objp)
		{
			if (objp->isAvatar())
			{
				HBFloaterInspectAvatar::show(target_id);
			}
			else
			{
				LLFloaterInspect::show(objp);
			}
		}
	}
	else if (name == "instant messages")
	{
		LLFloaterChatterBox::showInstance();
	}
	else if (name == "inventory")
	{
		if (!gSavedSettings.getBool("ShowInventory"))
		{
			LLFloaterInventory::toggleVisibility();
		}
	}
	else if (name == "land")
	{
		if (!gRLenabled || !gRLInterface.mContainsShowloc)
		{
			if (gViewerParcelMgr.selectionEmpty())
			{
				gViewerParcelMgr.selectParcelAt(gAgent.getPositionGlobal());
			}
			LLFloaterLand::showInstance();
		}
	}
	else if (name == "land holdings")
	{
		LLFloaterLandHoldings::showInstance();
	}
	else if (name == "lua console")
	{
		HBLuaConsole::showInstance();
	}
	else if (name == "map")
	{
		if (!gSavedSettings.getBool("ShowWorldMap"))
		{
			LLFloaterWorldMap::toggle(NULL);
		}
	}
	else if (name == "media filter")
	{
		SLFloaterMediaFilter::showInstance();
	}
	else if (name == "mini map")
	{
		LLFloaterMiniMap::showInstance();
	}
	else if (name == "movement controls")
	{
		LLFloaterMove::showInstance();
	}
	else if (name == "mute list")
	{
		if (target_id.notNull())
		{
			LLFloaterMute::selectMute(target_id);
		}
		else
		{
			LLFloaterMute::selectMute(param);
		}
	}
	else if (name == "nearby media")
	{
		LLFloaterNearByMedia::showInstance();
	}
	else if (name == "notifications")
	{
		LLFloaterNotificationConsole::showInstance();
	}
	else if (name == "characters")
	{
		LLFloaterPathfindingCharacters::openCharactersWithSelectedObjects();
	}
	else if (name == "linksets")
	{
		LLFloaterPathfindingLinksets::openLinksetsWithSelectedObjects();
	}
	else if (name == "preferences")
	{
		S32 tab = param.empty() ? -1 : atoi(param.c_str());
		if (tab >= 0 && tab < LLFloaterPreference::NUMBER_OF_TABS)
		{
			LLFloaterPreference::openInTab(tab);
		}
		else
		{
			LLFloaterPreference::showInstance();
		}
	}
	else if (name == "pushes")
	{
		HBFloaterBump::showInstance();
	}
	else if (name == "radar")
	{
		HBFloaterRadar::showInstance();
	}
	else if (name == "region")
	{
		if (!gRLenabled || !gRLInterface.mContainsShowloc)
		{
			LLFloaterRegionInfo::showInstance();
		}
	}
	else if (name == "search")
	{
		if (!gSavedSettings.getBool("ShowSearch"))
		{
			HBFloaterSearch::toggle();
		}
	}
	else if (name == "snapshot")
	{
		LLFloaterSnapshot::show(NULL);
	}
	else if (name == "sounds list")
	{
		HBFloaterSoundsList::showInstance();
	}
	else if (name == "stats")
	{
		LLFloaterStats::showInstance();
	}
	else if (name == "teleport history")
	{
		if (gFloaterTeleportHistoryp &&
			!gFloaterTeleportHistoryp->getVisible())
		{
			gFloaterTeleportHistoryp->toggle();
		}
	}

	return 0;
}

//static
int HBViewerAutomation::closeFloater(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("CloseFloater");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string name(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	LL_DEBUGS("Lua") << "Floater: " << name << LL_ENDL;

	if (name == "active speakers")
	{
		LLFloaterActiveSpeakers::hideInstance();
	}
	else if (name == "area search")
	{
		HBFloaterAreaSearch::hideInstance();
	}
	else if (name == "beacons")
	{
		LLFloaterBeacons::hideInstance();
	}
	else if (name == "camera controls")
	{
		LLFloaterCamera::hideInstance();
	}
	else if (name == "chat")
	{
		LLFloaterChat::hideInstance();
	}
	else if (name == "debug settings")
	{
		LLFloaterDebugSettings::hideInstance();
	}
	else if (name == "debug tags")
	{
		HBFloaterDebugTags::hideInstance();
	}
	else if (name == "experiences")
	{
		LLFloaterExperiences::hideInstance();
	}
	else if (name == "friends")
	{
		LLFloaterFriends::hideInstance();
	}
	else if (name == "gestures")
	{
		LLFloaterGesture::hideInstance();
	}
	else if (name == "groups")
	{
		LLFloaterGroups::hideInstance();
	}
	else if (name == "inspect object")
	{
		LLFloaterInspect::hideInstance();
	}
	else if (name == "inspect avatar")
	{
		HBFloaterInspectAvatar::hideInstance();
	}
	else if (name == "instant messages")
	{
		LLFloaterChatterBox::hideInstance();
	}
	else if (name == "inventory")
	{
		if (gSavedSettings.getBool("ShowInventory"))
		{
			LLFloaterInventory::toggleVisibility();
		}
	}
	else if (name == "land")
	{
		LLFloaterLand::hideInstance();
	}
	else if (name == "land holdings")
	{
		LLFloaterLandHoldings::hideInstance();
	}
	else if (name == "map")
	{
		if (gSavedSettings.getBool("ShowWorldMap"))
		{
			LLFloaterWorldMap::toggle(NULL);
		}
	}
	else if (name == "media filter")
	{
		SLFloaterMediaFilter::hideInstance();
	}
	else if (name == "mini map")
	{
		LLFloaterMiniMap::hideInstance();
	}
	else if (name == "movement controls")
	{
		LLFloaterMove::hideInstance();
	}
	else if (name == "mute list")
	{
		LLFloaterMute::hideInstance();
	}
	else if (name == "nearby media")
	{
		LLFloaterNearByMedia::hideInstance();
	}
	else if (name == "notifications")
	{
		LLFloaterNotificationConsole::hideInstance();
	}
	else if (name == "characters")
	{
		LLFloaterPathfindingCharacters::hideInstance();
	}
	else if (name == "linksets")
	{
		LLFloaterPathfindingLinksets::hideInstance();
	}
	else if (name == "preferences")
	{
		LLFloaterPreference::hideInstance();
	}
	else if (name == "pushes")
	{
		HBFloaterBump::hideInstance();
	}
	else if (name == "radar")
	{
		HBFloaterRadar::hideInstance();
	}
	else if (name == "region")
	{
		LLFloaterRegionInfo::hideInstance();
	}
	else if (name == "search")
	{
		if (gSavedSettings.getBool("ShowSearch"))
		{
			HBFloaterSearch::toggle();
		}
	}
	else if (name == "snapshot")
	{
		LLFloaterSnapshot::hide(NULL);
	}
	else if (name == "sounds list")
	{
		HBFloaterSoundsList::hideInstance();
	}
	else if (name == "stats")
	{
		LLFloaterStats::hideInstance();
	}
	else if (name == "teleport history")
	{
		if (gFloaterTeleportHistoryp &&
			gFloaterTeleportHistoryp->getVisible())
		{
			gFloaterTeleportHistoryp->toggle();
		}
	}

	return 0;
}

//static
int HBViewerAutomation::getFloaterInstances(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetFloaterInstances");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	std::string match;
	if (n)
	{
		match = luaL_checkstring(statep, 1);
		lua_pop(statep, 1);
	}

	lua_newtable(statep);
	std::string name, title;
	bool do_match = !match.empty();
	using iter_t = LLView::child_list_const_iter_t;
	for (iter_t it = gFloaterViewp->getChildList()->begin(),
				end = gFloaterViewp->getChildList()->end();
		 it != end; ++it)
	{
		LLFloater* floaterp = (*it)->asFloater();
		if (floaterp)
		{
			name = floaterp->getName();
			if (do_match && name != match)
			{
				continue;
			}
			if (!floaterp->isTitlePristine())
			{
				title = floaterp->getTitle();
				if (!title.empty() && stricmp(name.c_str(), title.c_str()))
				{
					name += "=" + title;
				}
			}
			lua_pushstring(statep, name.c_str());
			lua_rawseti(statep, -2, floaterp->getId());
		}
	}
	return 1;
}

static LLFloater* get_floater_by_id(S32 id)
{
	if (id > 0)
	{
		using iter_t = LLView::child_list_const_iter_t;
		for (iter_t it = gFloaterViewp->getChildList()->begin(),
					end = gFloaterViewp->getChildList()->end();
			 it != end; ++it)
		{
			LLFloater* floaterp = (*it)->asFloater();
			if (floaterp && floaterp->getId() == (U32)id)
			{
				return floaterp;
			}
		}
	}
	return NULL;
}

//static
int HBViewerAutomation::showFloater(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ShowFloater");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	S32 id = luaL_checknumber(statep, 1);
	lua_pop(statep, 1);

	LLFloater* floaterp = get_floater_by_id(id);
	if (floaterp)
	{
		floaterp->open();
	}
	lua_pushboolean(statep, floaterp && floaterp->getVisible());
	return 1;
}

static void get_all_children(LLView* parentp, std::vector<LLView*>& children)
{
	size_t count = parentp->getChildCount();
	if (!count)
	{
		return;
	}
	children.reserve(children.size() + count);

	using iter_t = LLView::child_list_const_iter_t;
	for (iter_t it = parentp->getChildList()->begin(),
				end = parentp->getChildList()->end();
		 it != end; ++it)
	{
		LLView* viewp = *it;
		if (viewp)	// Paranoia
		{
			children.push_back(viewp);
			get_all_children(viewp, children);
		}
	}
}

//static
int HBViewerAutomation::getFloaterControls(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetFloaterControls");
	}

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 5)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 5.", n);
	}
	S32 id = luaL_checknumber(statep, 1);
	bool list_disabled = n >= 2 && lua_toboolean(statep, 2);
	bool list_hidden = n >= 3 && lua_toboolean(statep, 3);
	bool filter_tag = false;
	std::string tag_match;
	if (n >= 4)
	{
		tag_match = luaL_checkstring(statep, 4);
		filter_tag = !tag_match.empty();
	}
	bool filter_name = false;
	std::string name_match;
	if (n >= 5)
	{
		name_match = luaL_checkstring(statep, 5);
		filter_name = !name_match.empty();
	}
	lua_pop(statep, n);

	LLFloater* floaterp = get_floater_by_id(id);
	if (!floaterp)
	{
		// No such floater: return nil.
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);
	if (!list_hidden && !floaterp->getVisible())
	{
		// Return an empty table since all elements are hidden.
		return 1;
	}

	// Get all children first.
	std::vector<LLView*> children;
	get_all_children(floaterp, children);

	std::string name;
	for (U32 i = 0, count = children.size(); i < count; ++i)
	{
		LLView* viewp = children[i];
		if (viewp && viewp->isCtrl() &&
			(list_hidden || viewp->getVisible()) &&
			(list_disabled || viewp->getEnabled()))
		{
			const std::string& tag = viewp->getTag();
			if (filter_tag && tag != tag_match)
			{
				continue;
			}
			std::string name = viewp->getName();
			if (filter_name && name.find(name_match) == std::string::npos)
			{
				continue;
			}
			lua_pushstring(statep, name.c_str());
			lua_pushstring(statep, tag.c_str());
			lua_rawset(statep, -3);
		}
	}

	return 1;
}

//static
int HBViewerAutomation::getFloaterCtrlState(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetFloaterCtrlState");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}
	S32 id = luaL_checknumber(statep, 1);
	std::string name = luaL_checkstring(statep, 2);
	lua_pop(statep, 2);

	LLFloater* floaterp = get_floater_by_id(id);
	if (!floaterp || name.empty())
	{
		// No such floater, or no UI control name: return nil.
		lua_pushnil(statep);
		return 1;
	}

	LLView* viewp = floaterp->getChildView(name.c_str(), true, false);
	if (!viewp || !viewp->isCtrl())
	{
		// No such *control*: return nil.
		lua_pushnil(statep);
		return 1;
	}
	LLUICtrl* ctrlp = (LLUICtrl*)viewp;

	lua_newtable(statep);

	lua_pushliteral(statep, "type");
	lua_pushstring(statep, ctrlp->getTag().c_str());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "children");
	lua_pushinteger(statep, ctrlp->getChildCount());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "visible");
	lua_pushboolean(statep, ctrlp->getVisible());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "enabled");
	lua_pushboolean(statep, ctrlp->getEnabled());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "chrome");
	lua_pushboolean(statep, ctrlp->getIsChrome());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "control");
	lua_pushstring(statep, ctrlp->getControlName().c_str());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "value");
	lua_pushstring(statep, ctrlp->getValue().asString().c_str());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "dirty");
	lua_pushboolean(statep, ctrlp->isDirty());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "focusable");
	lua_pushboolean(statep, ctrlp->isFocusableCtrl());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "focused");
	lua_pushboolean(statep, ctrlp->hasFocus());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "mouse_capture");
	lua_pushboolean(statep, ctrlp->hasMouseCapture());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "text_input");
	lua_pushboolean(statep, ctrlp->acceptsTextInput());
	lua_rawset(statep, -3);

	return 1;
}

//static
int HBViewerAutomation::getFloaterList(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetFloaterList");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}
	S32 id = luaL_checknumber(statep, 1);
	std::string name = luaL_checkstring(statep, 2);
	lua_pop(statep, 2);

	LLFloater* floaterp = get_floater_by_id(id);
	if (!floaterp || name.empty())
	{
		// No such floater, or no UI control name: return nil.
		lua_pushnil(statep);
		return 1;
	}

	LLScrollListCtrl* listp =
		floaterp->getChild<LLScrollListCtrl>(name.c_str(), true, false);
	if (!listp)
	{
		// No such list: return nil.
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);

	// Number of columns in the list.
	S32 columns = listp->getNumColumns();
	lua_pushliteral(statep, "columns");
	lua_pushinteger(statep, columns);
	lua_rawset(statep, -3);

	// Give the header for each column as a string in the form:
	// "col1_name=col1_label|...|colN_name=colN_label"
	std::string temp;
	for (S32 i = 0; i < columns; ++i)
	{
		LLScrollListColumn* colp = listp->getColumn(i);
		if (i)
		{
			temp += "|";
		}
		temp += colp->mName + "=" + colp->mLabel;
	}
	lua_pushliteral(statep, "headers");
	lua_pushstring(statep, temp.c_str());
	lua_rawset(statep, -3);

	// Give one table entry per list line (numbered from 1 to n, with n the
	// number of lines in the list); each column cell value in the line is
	// converted into a string and separated from the next with a pipe
	// character.
	LLScrollListCtrl::items_vec_t items = listp->getAllData();
	for (S32 i = 0, lines = items.size(); i < lines; ++i)
	{
		LLScrollListItem* itemp = items[i];
		if (!itemp) continue;	// Paranoia

		temp.clear();
		for (S32 j = 0; j < columns; ++j)
		{
			if (j)
			{
				temp += "|";
			}
			LLScrollListCell* cellp = itemp->getColumn(j);
			if (cellp)	// Paranoia ?
			{
				temp += cellp->getValue().asString();
			}
		}
		lua_pushinteger(statep, i + 1);
		lua_pushstring(statep, temp.c_str());
		lua_rawset(statep, -3);
	}

	return 1;
}

//static
int HBViewerAutomation::makeDialog(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("MakeDialog");
	}

	S32 n = lua_gettop(statep);
	if (n != 9)
	{
		luaL_error(statep, "%d arguments passed; expected 9.", n);
	}

	std::string params[9];
	for (S32 i = 0; i < 9; ++i)
	{
		params[i] = luaL_checkstring(statep, i + 1);
	}

	lua_pop(statep, 9);

	HBLuaDialog::create(params[0], params[1], params[2], params[3], params[4],
						params[5], params[6], params[7], params[8]);

	return 0;
}

//static
int HBViewerAutomation::openLuaFloater(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("OpenLuaFloater");
	}

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 5)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 5.", n);
	}

	std::string name(luaL_checkstring(statep, 1));
	std::string param, pos;
	if (n >= 2)
	{
		param.assign(luaL_checkstring(statep, 2));
	}
	if (n >= 3)
	{
		pos.assign(luaL_checkstring(statep, 3));
		LLStringUtil::trim(pos);
	}
	bool open = n < 4 || lua_toboolean(statep, 4);
	LLSD ctrl_values = LLSD::emptyArray();
	if (n >= 5)
	{
		LLSD value;
		if (lua_type(statep, 5) == LUA_TTABLE)
		{
			for (U32 i = 1, count = lua_rawlen(statep, 5); i <= count; ++i)
			{	
				if (lua_rawgeti(statep, 5, i) == LUA_TSTRING)
				{
					value = std::string(luaL_checkstring(statep, -1));
					ctrl_values.append(value);
				}
				lua_pop(statep, 1);
			}
		}
		else
		{
			value = std::string(luaL_checkstring(statep, 5));
			ctrl_values.append(value);
		}
	}
	lua_pop(statep, n);

	lua_pushboolean(statep,
					HBLuaFloater::create(name, param, pos, open,
										 ctrl_values) != NULL);

	return 1;
}

//static
int HBViewerAutomation::showLuaFloater(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ShowLuaFloater");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	std::string name(luaL_checkstring(statep, 1));
	bool show = n < 2 || lua_toboolean(statep, 2);
	lua_pop(statep, n);

	lua_pushboolean(statep, HBLuaFloater::setVisible(name, show));

	return 1;
}

//static
int HBViewerAutomation::closeLuaFloater(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("CloseLuaFloater");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string name(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	// This will call the OnLuaFloaterClose() callback if
	// CloseLuaFloater() was not invoked from the automation script.
	HBLuaFloater::destroy(name, self != gAutomationp);

	return 0;
}

//static
int HBViewerAutomation::setLuaFloaterCommand(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetLuaFloaterCommand");
	}

	S32 n = lua_gettop(statep);
	if (n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 3.", n);
	}

	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	std::string command(luaL_checkstring(statep, 3));
	lua_pop(statep, 3);

	lua_pushboolean(statep,
					HBLuaFloater::setControlCallback(floater_name,
													 ctrl_name, command));
	return 1;
}

//static
int HBViewerAutomation::getLuaFloaterValue(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetLuaFloaterValue");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	std::string value;
	if (HBLuaFloater::getControlValue(floater_name, ctrl_name, value))
	{
		lua_pushstring(statep, value.c_str());
	}
	else
	{
		lua_pushnil(statep);
	}

	return 1;
}

//static
int HBViewerAutomation::getLuaFloaterValues(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetLuaFloaterValues");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	std::vector<std::string> values;
	if (HBLuaFloater::getControlValues(floater_name, ctrl_name, values))
	{
		lua_newtable(statep);
		for (S32 i = 0, count = values.size(); i < count; ++i)
		{
			lua_pushstring(statep, values[i].c_str());
			lua_rawseti(statep, -2, i + 1);
		}
	}
	else
	{
		lua_pushnil(statep);
	}

	return 1;
}

//static
int HBViewerAutomation::getLuaFloaterListLine(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetLuaFloaterListLine");
	}

	S32 n = lua_gettop(statep);
	if (n != 2 && n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 2 or 3.", n);
	}
	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	S32 line = -1;
	if (n > 2)
	{
		line = lua_tointeger(statep, 3);
	}
	lua_pop(statep, n);

	LLScrollListCtrl* listp = HBLuaFloater::getList(floater_name, ctrl_name);
	if (!listp)
	{
		// No such list: return nil.
		lua_pushnil(statep);
		return 1;
	}

	// When line number is negative, use the first selected line number.
	if (line < 0)
	{
		std::string value;
		if (!HBLuaFloater::getControlValue(floater_name, ctrl_name, value))
		{
			lua_pushnil(statep);
			return 1;
		}
		line = atoi(value.c_str());
	}

	LLScrollListItem* itemp = listp->getItemByIndex(line);
	if (!itemp)
	{
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);
	for (S32 i = 0, count = listp->getNumColumns(); i < count; ++i)
	{
		LLScrollListColumn* colp = listp->getColumn(i);
		LLScrollListCell* cellp = itemp->getColumn(i);
		if (colp && cellp)	// Paranoia
		{
			lua_pushstring(statep, colp->mName.c_str());
			lua_pushstring(statep, cellp->getValue().asString().c_str());
			lua_rawset(statep, -3);
		}
	}

	return 1;
}

//static
int HBViewerAutomation::setLuaFloaterValue(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetLuaFloaterValue");
	}

	S32 n = lua_gettop(statep);
	if (n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 3.", n);
	}

	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	std::string value(luaL_checkstring(statep, 3));
	lua_pop(statep, n);

	lua_pushboolean(statep,
					HBLuaFloater::setControlValue(floater_name, ctrl_name,
												  value));

	return 1;
}

//static
int HBViewerAutomation::setLuaFloaterInvFilter(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetLuaFloaterInvFilter");
	}

	S32 n = lua_gettop(statep);
	if (n != 2 && n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 2 or 3.", n);
	}

	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	std::string value;
	if (n > 2)
	{
		value.assign(luaL_checkstring(statep, 3));
	}
	lua_pop(statep, n);

	lua_pushboolean(statep,
					HBLuaFloater::setInvFilter(floater_name, ctrl_name,
											   value));
	return 1;
}

//static
int HBViewerAutomation::setLuaFloaterEnabled(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetLuaFloaterEnabled");
	}

	S32 n = lua_gettop(statep);
	if (n != 2 && n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 2 or 3.", n);
	}

	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	bool enable = n < 3 || lua_toboolean(statep, 3);
	lua_pop(statep, n);

	lua_pushboolean(statep,
					HBLuaFloater::setControlEnabled(floater_name, ctrl_name,
													enable));

	return 1;
}

//static
int HBViewerAutomation::setLuaFloaterVisible(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetLuaFloaterVisible");
	}

	S32 n = lua_gettop(statep);
	if (n != 2 && n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 2 or 3.", n);
	}

	std::string floater_name(luaL_checkstring(statep, 1));
	std::string ctrl_name(luaL_checkstring(statep, 2));
	bool visible = n < 3 || lua_toboolean(statep, 3);
	lua_pop(statep, n);

	lua_pushboolean(statep,
					HBLuaFloater::setControlVisible(floater_name, ctrl_name,
													visible));

	return 1;
}

//static
int HBViewerAutomation::overlayBarLuaButton(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("OverlayBarLuaButton");
	}

	S32 n = lua_gettop(statep);
	if (n != 2 && n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 2 or 3.", n);
	}

	std::string label(luaL_checkstring(statep, 1));
	std::string command(luaL_checkstring(statep, 2));
	std::string tooltip;
	if (n == 3)
	{
		tooltip.assign(luaL_checkstring(statep, 3));
	}
	lua_pop(statep, n);

	if (gOverlayBarp)
	{
		gOverlayBarp->setLuaFunctionButton(label, command, tooltip);
	}

	return 0;
}

//static
int HBViewerAutomation::statusBarLuaIcon(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("StatusBarLuaIcon");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	std::string command(luaL_checkstring(statep, 1));
	std::string tooltip;
	if (n == 2)
	{
		tooltip.assign(luaL_checkstring(statep, 2));
	}
	lua_pop(statep, n);

	if (gStatusBarp)
	{
		gStatusBarp->setLuaFunctionButton(command, tooltip);
	}

	return 0;
}

//static
int HBViewerAutomation::sideBarButton(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SideBarButton");
	}

	S32 n = lua_gettop(statep);
	if (n == 0 || n > 4)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 4.", n);
	}

	U32 number = luaL_checknumber(statep, 1);
	std::string icon, tooltip, command;
	if (n > 1)
	{
		icon.assign(luaL_checkstring(statep, 2));
		if (!icon.empty() && n > 2)
		{
			command.assign(luaL_checkstring(statep, 3));
			if (n > 3)
			{
				tooltip.assign(luaL_checkstring(statep, 4));
			}
		}
	}
	lua_pop(statep, n);

	U32 result = 0;
	if (gLuaSideBarp)
	{
		result = gLuaSideBarp->setButton(number, icon, command, tooltip);
	}
	lua_pushinteger(statep, result);

	return 1;
}

//static
int HBViewerAutomation::sideBarButtonToggle(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SideBarButtonToggle");
	}

	S32 n = lua_gettop(statep);
	if (n == 0 || n > 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	U32 number = luaL_checknumber(statep, 1);

	S32 toggle = -1;
	if (n == 2)
	{
		int type = lua_type(statep, 2);
		if (type == LUA_TNIL || type == LUA_TSTRING)
		{
			LLControlVariable* controlp = NULL;
			std::string cname;
			if (type == LUA_TSTRING)
			{
				cname.assign(luaL_checkstring(statep, 2));
			}
			if (!cname.empty())
			{
				controlp = gSavedSettings.getControl(cname.c_str());
				if (!controlp)
				{
					controlp =
						gSavedPerAccountSettings.getControl(cname.c_str());
				}
				if (!controlp)
				{
					luaL_error(statep, "No setting named: %s", cname.c_str());
				}
				if (controlp->type() != TYPE_BOOLEAN)
				{
					luaL_error(statep, "Setting '%s' is not of boolean type",
							   cname.c_str());
				}
			}
			if (gLuaSideBarp)
			{
				gLuaSideBarp->buttonSetControl(number, controlp);
			}
		}
		else
		{
			toggle = lua_toboolean(statep, 2) ? 1 : 0;
		}
	}
	lua_pop(statep, n);

	if (gLuaSideBarp)
	{
		toggle = gLuaSideBarp->buttonToggle(number, toggle);
	}

	if (toggle == -1)
	{
		lua_pushnil(statep);
	}
	else
	{
		lua_pushboolean(statep, toggle);
	}

	return 1;
}

//static
int HBViewerAutomation::sideBarHide(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SideBarHide");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	bool hide = true;
	if (n == 1)
	{
		hide = lua_toboolean(statep, 1);
		lua_pop(statep, 1);
	}

	if (gLuaSideBarp)
	{
		gLuaSideBarp->setHidden(hide);
	}

	return 0;
}

//static
int HBViewerAutomation::sideBarHideOnRightClick(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SideBarHideOnRightClick");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	bool hide = true;
	if (n == 1)
	{
		hide = lua_toboolean(statep, 1);
		lua_pop(statep, 1);
	}

	if (gLuaSideBarp)
	{
		gLuaSideBarp->hideOnRightClick(hide);
	}

	return 0;
}

//static
int HBViewerAutomation::sideBarButtonHide(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SideBarButtonHide");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	U32 number = luaL_checknumber(statep, 1);
	bool hide = n < 2 || lua_toboolean(statep, 2);
	lua_pop(statep, n);

	if (gLuaSideBarp)
	{
		gLuaSideBarp->setButtonVisible(number, !hide);
	}

	return 0;
}

//static
int HBViewerAutomation::sideBarButtonDisable(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SideBarButtonDisable");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	U32 number = luaL_checknumber(statep, 1);
	bool disable = n < 2 || lua_toboolean(statep, 2);
	lua_pop(statep, n);

	if (gLuaSideBarp)
	{
		gLuaSideBarp->setButtonEnabled(number, !disable);
	}

	return 0;
}

//static
int HBViewerAutomation::luaPieMenuSlice(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("LuaPieMenuSlice");
	}

	S32 n = lua_gettop(statep);
	if (n == 0 || n > 4)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 4.", n);
	}

	S32 type = luaL_checknumber(statep, 1);
	U32 slice = 0;
	std::string label, command;
	if (n > 1)
	{
		slice = luaL_checknumber(statep, 2);
		if (slice && n > 2)
		{
			label.assign(luaL_checkstring(statep, 3));
			if (n > 3)
			{
				command.assign(luaL_checkstring(statep, 4));
			}
		}
	}
	lua_pop(statep, n);

	if (gLuaPiep)
	{
		gLuaPiep->setSlice(type, slice, label, command);
	}

	return 0;
}

//static
int HBViewerAutomation::luaContextMenu(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("LuaContextMenu");
	}

	S32 n = lua_gettop(statep);
	if (n == 0 || n > 4)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 4.", n);
	}

	S32 id = luaL_checknumber(statep, 1);
	std::string cut_label, copy_label, paste_label;
	if (n > 1)
	{
		cut_label = luaL_checkstring(statep, 2);
		if (n > 2)
		{
			copy_label.assign(luaL_checkstring(statep, 3));
			if (n > 3)
			{
				paste_label.assign(luaL_checkstring(statep, 4));
			}
		}
	}
	lua_pop(statep, n);

	lua_pushboolean(statep,
					LLEditMenuHandler::setCustomMenu(id, cut_label, copy_label,
													 paste_label));
	return 1;
}

//static
int HBViewerAutomation::pasteToContextHandler(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("PasteToContextHandler");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	S32 id = luaL_checknumber(statep, 1);
	if (n > 1)
	{
		std::string text(luaL_checkstring(statep, 2));
		gClipboard.copyFromSubstring(utf8str_to_wstring(text), 0,
									 text.length());
	}
	lua_pop(statep, n);

	lua_pushboolean(statep, LLEditMenuHandler::pasteTo(id));
	return 1;
}

//static
int HBViewerAutomation::automationMessage(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	std::string text(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	if (gAutomationp && gAutomationp->mHasOnAutomationMessage)
	{
		LL_DEBUGS("Lua") << "Invoking OnAutomationMessage Lua callback. text="
						 << text << LL_ENDL;

		lua_getglobal(gAutomationp->mLuaState, "OnAutomationMessage");
		lua_pushstring(gAutomationp->mLuaState, text.c_str());
		gAutomationp->resetTimer();
		if (lua_pcall(gAutomationp->mLuaState, 1, 0, 0) != LUA_OK)
		{
			gAutomationp->reportError();
		}
	}

	return 0;
}

//static
int HBViewerAutomation::automationRequest(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	std::string request(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	if (!gAutomationp || !gAutomationp->mHasOnAutomationRequest)
	{
		LL_DEBUGS("Lua") << "No OnAutomationRequest Lua callback. request="
						 << request << ". Returning an empty result string."
						 << LL_ENDL;
		lua_pushliteral(statep, "");
		return 1;
	}

	LL_DEBUGS("Lua") << "Invoking OnAutomationRequest Lua callback. request="
					 << request << LL_ENDL;

	lua_getglobal(gAutomationp->mLuaState, "OnAutomationRequest");
	lua_pushstring(gAutomationp->mLuaState, request.c_str());
	gAutomationp->resetTimer();
	if (lua_pcall(gAutomationp->mLuaState, 1, 1, 0) != LUA_OK)
	{
		gAutomationp->reportError();
		return 0;
	}

	if (lua_gettop(gAutomationp->mLuaState) == 0 ||
		lua_type(gAutomationp->mLuaState, -1) != LUA_TSTRING)
	{
		lua_settop(gAutomationp->mLuaState, 0);	// Sanitize stack by emptying it
		lua_pushliteral(gAutomationp->mLuaState,
						"OnAutomationRequest() Lua callback did not return a string");
		gAutomationp->reportError();
		return 0;
	}

	// Recover the result from the automation script stack...
	std::string result(lua_tolstring(gAutomationp->mLuaState, -1, NULL));
	lua_pop(gAutomationp->mLuaState, 1);

	// ... and push it on our stack.
	lua_pushstring(statep, result.c_str());

	return 1;
}

//static
int HBViewerAutomation::playUISound(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	static const std::string valid_sounds = get_valid_sounds();

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("PlayUISound");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	std::string name(luaL_checkstring(statep, 1));
	name = "UISnd" + name;
	if (valid_sounds.find(";" + name + ";") == std::string::npos)
	{
		llwarns << "No such UI sound name: " << name << llendl;
		lua_pop(statep, n);
		return 0;
	}

	bool force = n >= 2 && lua_toboolean(statep, 2);

	lua_pop(statep, n);

	make_ui_sound(name.c_str(), force);

	return 0;
}

//static
int HBViewerAutomation::renderDebugInfo(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("RenderDebugInfo");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	S32 feature = luaL_checknumber(statep, 1);
	lua_pop(statep, 1);

	if (feature < 0 || feature > 32)
	{
		luaL_error(statep,
				   "Invalid render debug feature index (valid range is 0 to 32");
	}

	if (feature)
	{
		gPipeline.setRenderDebugMask(1U << (feature - 1));
	}
	else
	{
		gPipeline.setRenderDebugMask(0);
	}

	return 0;
}

//static
int HBViewerAutomation::getDebugSetting(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetDebugSetting");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string name(luaL_checkstring(statep, 1));
	if (name.empty())
	{
		luaL_error(statep, "Empty setting name");
	}
	lua_pop(statep, 1);

	// Note: as of Cool VL Viewer v1.28.2.72 & v1.30.0.0, commands sent via
	// scripted objects (or via D-Bus, under Linux) are now forbidden access
	// to debug settings, but the settings white lists have been removed (i.e.
	// non-external scripts are granted full access to any valid debug
	// setting).
	if (self->mFromObjectId != gAgentID)
	{
		lua_pushnil(statep);
		return 1;
	}

	LLControlVariable* controlp = gSavedSettings.getControl(name.c_str());
	if (!controlp)
	{
		controlp = gSavedPerAccountSettings.getControl(name.c_str());
	}
	if (!controlp)
	{
		controlp = gColors.getControl(name.c_str());
	}
	if (!controlp)
	{
		luaL_error(statep, "No setting named: %s", name.c_str());
	}

	eControlType type = controlp->type();
	switch (type)
	{
		case TYPE_U32:
		case TYPE_S32:
			lua_pushinteger(statep, controlp->getValue().asInteger());
			break;

		case TYPE_F32:
			lua_pushnumber(statep, controlp->getValue().asReal());
			break;

		case TYPE_BOOLEAN:
			lua_pushboolean(statep, controlp->getValue().asBoolean());
			break;

		case TYPE_STRING:
			lua_pushstring(statep, controlp->getValue().asString().c_str());
			break;

		case TYPE_VEC3:
		{
			LLVector3 vec;
			vec.setValue(controlp->getValue());
			lua_pushnumber(statep, vec.mV[0]);
			lua_pushnumber(statep, vec.mV[1]);
			lua_pushnumber(statep, vec.mV[2]);
			n = 3;
			break;
		}

		case TYPE_RECT:
		{
			LLRect r;
			r.setValue(controlp->getValue());
			lua_pushinteger(statep, r.mLeft);
			lua_pushinteger(statep, r.mTop);
			lua_pushinteger(statep, r.mRight);
			lua_pushinteger(statep, r.mBottom);
			n = 4;
			break;
		}

		case TYPE_COL4:
		{
			LLColor4 color;
			color.setValue(controlp->getValue());
			lua_pushnumber(statep, color.mV[0]);
			lua_pushnumber(statep, color.mV[1]);
			lua_pushnumber(statep, color.mV[2]);
			lua_pushnumber(statep, color.mV[3]);
			n = 4;
			break;
		}

		case TYPE_COL3:
		{
			LLColor3 color;
			color.setValue(controlp->getValue());
			lua_pushnumber(statep, color.mV[0]);
			lua_pushnumber(statep, color.mV[1]);
			lua_pushnumber(statep, color.mV[2]);
			n = 3;
			break;
		}

		case TYPE_COL4U:
		{
			LLColor4U color;
			color.setValue(controlp->getValue());
			lua_pushinteger(statep, color.mV[0]);
			lua_pushinteger(statep, color.mV[1]);
			lua_pushinteger(statep, color.mV[2]);
			lua_pushinteger(statep, color.mV[3]);
			n = 4;
			break;
		}

		default:
			// Other setting types (TYPE_LLSD which is only used in a couple
			// hidden settings, and TYPE_VEC3D which is not used at all) are
			// unsupported for now.
			lua_pushnil(statep);
	}

	return n;
}

//static
int HBViewerAutomation::setDebugSetting(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetDebugSetting");
	}

	S32 n = lua_gettop(statep);
	if (!n)
	{
		luaL_error(statep, "Missing arguments.", n);
	}

	std::string name(luaL_checkstring(statep, 1));
	if (name.empty())
	{
		luaL_error(statep, "Empty setting name");
	}

	// Note: as of Cool VL Viewer v1.28.2.72 & v1.30.0.0, commands sent via
	// scripted objects (or via D-Bus, under Linux) are now forbidden access
	// to debug settings, but the settings white lists have been removed (i.e.
	// non-external scripts are granted full access to any valid debug
	// setting).
	if (self->mFromObjectId != gAgentID)
	{
		lua_pop(statep, n);
		lua_pushnil(statep);
		return 1;
	}

	LLControlVariable* controlp = gSavedSettings.getControl(name.c_str());
	if (!controlp)
	{
		controlp = gSavedPerAccountSettings.getControl(name.c_str());
	}
	if (!controlp)
	{
		controlp = gColors.getControl(name.c_str());
	}
	if (!controlp)
	{
		luaL_error(statep, "No setting named: %s", name.c_str());
	}
	if (controlp->isHiddenFromUser())
	{
		luaL_error(statep,
				   "Cannot set '%s' which is reserved for internal viewer code use only",
				   name.c_str());
	}

	bool success;
	eControlType type = controlp->type();

	if (n == 1)
	{
		success = type != TYPE_LLSD && type != TYPE_VEC3D;
		if (success)
		{
			controlp->resetToDefault();
		}
	}
	else
	{
		LLSD value;
		switch (type)
		{
			case TYPE_U32:
			case TYPE_S32:
				success = n == 2;
				if (success)
				{
					value = LLSD::Integer(luaL_checknumber(statep, 2));
				}
				break;

			case TYPE_F32:
				success = n == 2;
				if (success)
				{
					value = LLSD::Real(luaL_checknumber(statep, 2));
				}
				break;

			case TYPE_BOOLEAN:
				success = n == 2;
				if (success)
				{
					value = LLSD::Boolean(lua_toboolean(statep, 2));
				}
				break;

			case TYPE_STRING:
				success = n == 2;
				if (success)
				{
					value = LLSD::String(luaL_checkstring(statep, 2));
				}
				break;

			case TYPE_VEC3:
				success = n == 4;
				if (success)
				{
					value = LLVector3(luaL_checknumber(statep, 2),
									  luaL_checknumber(statep, 3),
									  luaL_checknumber(statep, 4)).getValue();
				}
				break;

			case TYPE_RECT:
				success = n == 5;
				if (success)
				{
					value = LLRect(S32(luaL_checknumber(statep, 2)),
								   S32(luaL_checknumber(statep, 3)),
								   S32(luaL_checknumber(statep, 4)),
								   S32(luaL_checknumber(statep, 5))).getValue();
				}
				break;

			case TYPE_COL4:
				success = n == 4 || n == 5;
				if (success)
				{
					F32 r = luaL_checknumber(statep, 2);
					F32 g = luaL_checknumber(statep, 3);
					F32 b = luaL_checknumber(statep, 4);
					F32 a = 1.f;
					if (n == 5)
					{
						a = luaL_checknumber(statep, 5);
					}
					if (r >= 0.f && g >= 0.f && b >= 0.f && a >= 0.f &&
						r <= 1.f && g <= 1.f && b <= 1.f && a <= 1.f)
					{
						value = LLColor4(r, g, b, a).getValue();
					}
					else
					{
						success = false;
					}
				}
				break;

			case TYPE_COL3:
				success = n == 4;
				if (success)
				{
					F32 r = luaL_checknumber(statep, 2);
					F32 g = luaL_checknumber(statep, 3);
					F32 b = luaL_checknumber(statep, 4);
					if (r >= 0.f && g >= 0.f && b >= 0.f &&
						r <= 1.f && g <= 1.f && b <= 1.f)
					{
						value = LLColor3(r, g, b).getValue();
					}
					else
					{
						success = false;
					}
				}
				break;

			case TYPE_COL4U:
				success = n == 4 || n == 5;
				if (success)
				{
					F32 r = luaL_checknumber(statep, 2);
					F32 g = luaL_checknumber(statep, 3);
					F32 b = luaL_checknumber(statep, 4);
					F32 a = 255.f;
					if (n == 5)
					{
						a = luaL_checknumber(statep, 5);
					}
					if (r >= 0.f && g >= 0.f && b >= 0.f && a >= 0.f &&
						r <= 255.f && g <= 255.f && b <= 255.f && a <= 255.f)
					{
						value = LLColor4U((U8)r, (U8)g, (U8)b,
										  (U8)a).getValue();
					}
				}
				break;

			default:
				// Other setting types (TYPE_LLSD which is only used in a
				// couple hidden settings, and TYPE_VEC3D which is not used at
				// all) are unsupported for now.
				success = false;
		}
		if (success)
		{
			controlp->setValue(value);
		}
	}

	lua_pop(statep, n);
	lua_pushboolean(statep, success);

	return 1;
}

//static
bool HBViewerAutomation::serializeTable(lua_State* statep, S32 stack_level,
										std::string* output)
{
	if (!statep || lua_type(statep, stack_level) != LUA_TTABLE)
	{
		return false;
	}

	std::string data, value;
	lua_pushnil(statep);
	while (lua_next(statep, stack_level))
	{
		if (data.empty())
		{
			data = "{[";
		}
		else
		{
			data += ";[";
		}

		int key_type = lua_type(statep, -2);
		switch (key_type)
		{
			case LUA_TNUMBER:
				value = llformat(LUA_NUMBER_FMT, lua_tonumber(statep, -2));
				break;

			case LUA_TSTRING:
				value = lua_tostring(statep, -2);
				LLStringUtil::replaceString(value, "\"", "\\\"");
				value = "\"" + value + "\"";
				break;

			case LUA_TBOOLEAN:
			case LUA_TNIL:
			default:
				lua_pop(statep, 2);
				return false;
		}
		data += value + "]=";

		int value_type = lua_type(statep, -1);
		switch (value_type)
		{
			case LUA_TNIL:
				value = "nil";
				break;

			case LUA_TBOOLEAN:
				value = lua_toboolean(statep, -1) ? "true" : "false";
				break;

			case LUA_TNUMBER:
				value = llformat(LUA_NUMBER_FMT, lua_tonumber(statep, -1));
				break;

			case LUA_TSTRING:
				value = lua_tostring(statep, -1);
				LLStringUtil::replaceString(value, "\"", "\\\"");
				value = "\"" + value + "\"";
				break;

			default:
				lua_pop(statep, 2);
				return false;
		}
		data += value;
		lua_pop(statep, 1);
	}
	lua_pop(statep, 1);

	if (data.empty())
	{
		data = "{}";
	}
	else
	{
		data += "}";
	}
	LL_DEBUGS("Lua") << "Resulting Lua code (table): " << data << LL_ENDL;

	if (output)
	{
		*output = data;
	}
	else
	{
		data = "base64:" + LLBase64::encode(data);
		lua_pushstring(statep, data.c_str());
	}

	return true;
}

//static
bool HBViewerAutomation::deserializeTable(lua_State* statep, std::string data)
{
	if (!statep || data.compare(0, 7, "base64:") != 0)
	{
		return false;
	}

	data = LLBase64::decode(data.substr(7).c_str());
	LL_DEBUGS("Lua") << "Decoded Base64 data: " << data << LL_ENDL;
	data = "_V_SETTINGS=" + data;
	if (luaL_dostring(statep, data.c_str()))
	{
		return false;
	}
	lua_getglobal(statep, "_V_SETTINGS");
	lua_pushnil(statep);
	lua_setglobal(statep, "_V_SETTINGS");

	return true;
}

//static
int HBViewerAutomation::getGlobalData(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	std::string data = gSavedSettings.getString("LuaSessionData");
	if (!deserializeTable(statep, data))
	{
		lua_pushstring(statep, data.c_str());
	}

	return 1;
}

//static
int HBViewerAutomation::setGlobalData(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	if (lua_type(statep, 1) == LUA_TTABLE && !serializeTable(statep))
	{
		luaL_error(statep, "Unsupported table format");
	}

	std::string data(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	gSavedSettings.setString("LuaSessionData", data);

	return 0;
}

//static
int HBViewerAutomation::getPerAccountData(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	std::string data = gSavedPerAccountSettings.getString("LuaUserData");
	if (!deserializeTable(statep, data))
	{
		lua_pushstring(statep, data.c_str());
	}

	return 1;
}

//static
int HBViewerAutomation::setPerAccountData(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	if (lua_type(statep, 1) == LUA_TTABLE && !serializeTable(statep))
	{
		luaL_error(statep, "Unsupported table format");
	}

	std::string data(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	gSavedPerAccountSettings.setString("LuaUserData", data);

	return 0;
}

//static
int HBViewerAutomation::getSourceFileName(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	if (self->mSourceFileName.empty())
	{
		lua_pushnil(statep);
		return 1;
	}

	lua_pushstring(statep, self->mSourceFileName.c_str());
	return 1;
}

//static
int HBViewerAutomation::getWatchdogState(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_pushnumber(statep, self->mWatchdogTimer.getRemainingTimeF64());
	lua_pushnumber(statep, self->mWatchdogTimer.getElapsedTimeF64());
	return 2;
}

//static
int HBViewerAutomation::getFrameTimeSeconds(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_pushnumber(statep, gFrameTimeSeconds);

	return 1;
}

//static
int HBViewerAutomation::getTimeSinceEpoch(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_pushnumber(statep, LLTimer::getEpochSeconds());

	return 1;
}

//static
int HBViewerAutomation::getTimeStamp(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetTimeStamp");
	}

	S32 n = lua_gettop(statep);
	if (n > 2)
	{
		luaL_error(statep, "%d arguments passed; expected 0 to 2.", n);
	}

	S32 time_zone = 0;	// Default to UTC
	if (n)
	{
		time_zone = luaL_checknumber(statep, 1);
	}

	std::string time_format;
	if (n > 1)
	{
		time_format.assign(luaL_checkstring(statep, 2));
	}
	else
	{
		time_format = gSavedSettings.getString("ShortDateFormat") + " ";
		time_format += gSavedSettings.getString("ShortTimeFormat");
	}

	if (n)
	{
		lua_pop(statep, n);
	}

	// Correct the UTC time, adding the time zone offset
	time_t tz_time = time_corrected() + time_zone * 3600;
	struct tm* internal_time = utc_time_to_tm(tz_time);

	std::string timestamp;
	timeStructToFormattedString(internal_time, time_format, timestamp);
	timestamp += " UTC";
	if (time_zone != 0)
	{
		timestamp += llformat("%+d", time_zone);
	}

	lua_pushstring(statep, timestamp.c_str());

	return 1;
}

//static
int HBViewerAutomation::getClipBoardString(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetClipBoardString");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	S32 clipboard = 0;	// Default to viewer clipboard
	if (n)
	{
		clipboard = luaL_checknumber(statep, 1);
		lua_pop(statep, 1);
	}

	LLWString wtext;
	switch (clipboard)
	{
		case 0:
			wtext = gClipboard.getClipBoardString();
			break;

		case 1:
			if (gWindowp)
			{
				gWindowp->pasteTextFromClipboard(wtext);
			}
			break;

		case 2:
			if (gWindowp)
			{
				gWindowp->pasteTextFromPrimary(wtext);
			}
			break;

		default:
			luaL_error(statep,
					   "Invalid clipboard type %d (valid types are 0 to 2).",
					   n);
	}
	lua_pushstring(statep, wstring_to_utf8str(wtext).c_str());

	return 1;
}

//static
int HBViewerAutomation::setClipBoardString(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetClipBoardString");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	std::string text;
	if (n)
	{
		text = luaL_checkstring(statep, 1);
		lua_pop(statep, 1);
	}

	gClipboard.copyFromSubstring(utf8str_to_wstring(text), 0, text.length());

	return 0;
}

//static
const LLUUID& HBViewerAutomation::getInventoryObjectId(const std::string& name,
													   bool& is_category)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (name.empty() || name == "|")
	{
		is_category = true;
		return gInventory.getRootFolderID();
	}

	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;

	// First check if the passed string is a valid object inventory Id
	if (LLUUID::validate(name))
	{
		LLUUID inv_obj_id(name);
		cat = gInventory.getCategory(inv_obj_id);
		if (cat)
		{
			LL_DEBUGS("Lua") << "Found an inventory category for Id: "
							 << inv_obj_id << " - Name: " << cat->getName()
							 << LL_ENDL;
			is_category = true;
			return cat->getUUID();
		}
		item = gInventory.getItem(inv_obj_id);
		if (item)
		{
			LL_DEBUGS("Lua") << "Found an inventory item for Id: "
							 << inv_obj_id << " - Name: " << item->getName()
							 << LL_ENDL;
			is_category = false;
			return item->getUUID();
		}
	}

	// Not an UUID, so split the string into path elements
	std::string item_name = name;
	std::deque<std::string> path;
	size_t i;
	std::string temp;
	while ((i = item_name.find('|')) != std::string::npos)
	{
		temp = item_name.substr(0, i);
		item_name = item_name.substr(i + 1);
		// temp is empty when 2+ successive '|' exist in path, or when one is
		// leading the full path. In both cases, skip the empty element.
		if (!temp.empty())
		{
			LL_DEBUGS("Lua") << "Adding name to path: " << temp << LL_ENDL;
			path.emplace_back(temp);
		}
	}
	// item_name is empty when a '|' is trailing in path (in which case the
	// empty string shall not be added to the path elements queue).
	if (!item_name.empty())
	{
		LL_DEBUGS("Lua") << "Adding name to path: " << item_name << LL_ENDL;
		path.emplace_back(item_name);
	}

	// Search for a matching inventory object
	const LLUUID* cat_id = &gInventory.getRootFolderID();
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	bool last_name = false;
	while (!last_name)
	{
		item_name = path.front();
		path.pop_front();
		last_name = path.empty();

		gInventory.getDirectDescendentsOf(*cat_id, cats, items);

		item = NULL;
		cat = NULL;
		if (last_name)
		{
			for (S32 i = 0, count = items->size(); i < count; ++i)
			{
				item = (*items)[i];
				if (!item) continue;	// Paranoia

				if (item->getName() == item_name)
				{
					LL_DEBUGS("Lua") << "Found matching item name: "
									 << item_name << " - Returning item Id: "
									 << item->getUUID() << LL_ENDL;
					is_category = false;
					return item->getUUID();
				}
			}
		}
		for (S32 i = 0, count = cats->size(); i < count; ++i)
		{
			cat = (*cats)[i];
			if (!cat) continue;	// Paranoia

			if (cat->getName() == item_name)
			{
				LL_DEBUGS("Lua") << "Found matching category name: "
								 << item_name << LL_ENDL;
				if (last_name)
				{
					LL_DEBUGS("Lua") << "Returning category Id: "
									 << cat->getUUID() << LL_ENDL;
					is_category = true;
					return cat->getUUID();
				}
				break;
			}

			cat = NULL;
		}
		if (!cat)
		{
			break;
		}
		cat_id = &cat->getUUID();
	}

	is_category = false;
	return LLUUID::null;
}

//static
int HBViewerAutomation::findInventoryObject(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("FindInventoryObject");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	std::string obj_name(luaL_checkstring(statep, 1));
	if (obj_name.empty())
	{
		luaL_error(statep, "Empty inventory object path name");
	}
	lua_pop(statep, 1);

	bool is_category = false;
	const LLUUID& obj_id = getInventoryObjectId(obj_name, is_category);

	bool export_support = gAgent.regionHasExportPermSupport();
	bool copy_ok = false;
	bool mod_ok = false;
	bool xfer_ok = false;
	bool export_ok = false;
	S32 type = LLAssetType::AT_NONE;
	if (is_category)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(obj_id);
		if (cat)
		{
			obj_name = cat->getName();
		}
		type = LLAssetType::AT_CATEGORY;
	}
	else if (obj_id.notNull())
	{
		LLViewerInventoryItem* itemp = gInventory.getItem(obj_id);
		if (itemp)
		{
			type = itemp->getType();
			obj_name = itemp->getName();
			const LLPermissions& perms = itemp->getPermissions();
			copy_ok = perms.allowCopyBy(gAgentID);
			mod_ok = perms.allowModifyBy(gAgentID);
			xfer_ok = perms.allowTransferBy(gAgentID);
			export_ok = export_support &&
						perms.allowExportBy(gAgentID, ep_export_bit);
		}
	}

	lua_newtable(statep);
	lua_pushliteral(statep, "id");
	lua_pushstring(statep, obj_id.asString().c_str());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "name");
	lua_pushstring(statep, obj_name.c_str());
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "type");
	lua_pushinteger(statep, type);
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "copy_ok");
	lua_pushboolean(statep, copy_ok);
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "mod_ok");
	lua_pushboolean(statep, mod_ok);
	lua_rawset(statep, -3);
	lua_pushliteral(statep, "xfer_ok");
	lua_pushboolean(statep, xfer_ok);
	lua_rawset(statep, -3);
	if (!is_category && export_support)
	{
		lua_pushliteral(statep, "export_ok");
		lua_pushboolean(statep, export_ok);
		lua_rawset(statep, -3);
	}

	return 1;
}

//static
int HBViewerAutomation::giveInventory(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GiveInventory");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}
	LLUUID avatar_id(luaL_checkstring(statep, 1), false);
	std::string item_name(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	bool success = false;

	if (avatar_id.notNull())
	{
		bool is_category = false;
		const LLUUID& inv_obj_id = getInventoryObjectId(item_name,
													    is_category);
		if (inv_obj_id.notNull())
		{
			if (is_category)
			{
				LLViewerInventoryCategory* cat =
					gInventory.getCategory(inv_obj_id);
				if (cat)
				{
					LL_DEBUGS("Lua") << "avatar_id=" << avatar_id
									 << " - cat_id=" << cat->getUUID()
									 << LL_ENDL;
					LLToolDragAndDrop::giveInventoryCategory(avatar_id, cat);
					success = true;
				}
			}
			else
			{
				LLViewerInventoryItem* item = gInventory.getItem(inv_obj_id);
				if (item)
				{
					LL_DEBUGS("Lua") << "avatar_id=" << avatar_id
									 << " - item_id=" << item->getUUID()
									 << LL_ENDL;
					LLToolDragAndDrop::giveInventory(avatar_id, item);
					success = true;
				}
			}
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::makeInventoryLink(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("MakeInventoryLink");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}
	std::string item_path(luaL_checkstring(statep, 1));
	if (item_path.empty())
	{
		luaL_error(statep, "Empty item name");
	}
	std::string link_cat_path(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	bool success = false;
	bool is_category = false;
	const LLUUID& item_id = getInventoryObjectId(item_path, is_category);
	if (!is_category && item_id.notNull())
	{
		LLUUID cat_id;
		if (link_cat_path.empty())
		{
			cat_id = gInventory.getRootFolderID();
		}
		else
		{
			cat_id = getInventoryObjectId(link_cat_path, is_category);
			if (!is_category || gInventory.isInTrash(cat_id) ||
				gInventory.isInMarketPlace(cat_id))
			{
				cat_id.setNull();
			}
		}
		if (cat_id.notNull())
		{
			link_inventory_object(cat_id, item_id);
			success = true;
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::deleteInventoryLink(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("DeleteInventoryLink");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	std::string link_path(luaL_checkstring(statep, 1));
	if (link_path.empty())
	{
		luaL_error(statep, "Empty link name");
	}
	lua_pop(statep, 1);

	bool success = false;
	bool is_category = false;
	const LLUUID& item_id = getInventoryObjectId(link_path, is_category);
	if (!is_category && item_id.notNull())
	{
		LLViewerInventoryItem* item = gInventory.getItem(item_id);
		if (item && item->getIsLinkType() && !gInventory.isInTrash(item_id) &&
			!gInventory.isInMarketPlace(item_id))
		{
			remove_inventory_item(item_id);
			success = true;
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::newInventoryFolder(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("NewInventoryFolder");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}
	std::string folder_path(luaL_checkstring(statep, 1));
	std::string folder_name(luaL_checkstring(statep, 2));
	lua_pop(statep, 2);

	LLUUID cat_id;
	if (folder_path.empty())
	{
		cat_id = gInventory.getRootFolderID();
	}
	else
	{
		bool is_category = false;
		cat_id = getInventoryObjectId(folder_path, is_category);
		if (!is_category ||
			// Forbid to make a folder in trash or market place.
			gInventory.isInTrash(cat_id) || gInventory.isInMarketPlace(cat_id))
		{
			cat_id.setNull();
		}
	}

	// Verify that the folder name is valid. Skip folder creation if not.
	std::string tmp = folder_name;
	LLStringFn::replace_nonprintable_and_pipe_in_ascii(tmp, LL_UNKNOWN_CHAR);
	if (tmp != folder_name)
	{
		cat_id.setNull();
	}

	if (cat_id.notNull())
	{
		cat_id = gInventory.createCategoryUDP(cat_id, LLFolderType::FT_NONE,
											  folder_name);
	}

	lua_pushstring(statep, cat_id.asString().c_str());

	return 1;
}

//static
int HBViewerAutomation::listInventoryFolder(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ListInventoryFolder");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	std::string folder_path(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	LLUUID cat_id;
	if (folder_path.empty())
	{
		cat_id = gInventory.getRootFolderID();
	}
	else
	{
		bool is_category = false;
		cat_id = getInventoryObjectId(folder_path, is_category);
		if (!is_category)
		{
			cat_id.setNull();
		}
	}
	if (cat_id.isNull())
	{
		lua_pushnil(statep);
		return 1;
	}

	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(cat_id, cats, items);

	lua_newtable(statep);

	for (S32 i = 0, count = cats->size(); i < count; ++i)
	{
		LLViewerInventoryCategory* cat = (*cats)[i];
		if (cat)	// Paranoia
		{
			folder_path = cat->getName() + "|";
			lua_pushstring(statep, cat->getUUID().asString().c_str());
			lua_pushstring(statep, folder_path.c_str());
			lua_rawset(statep, -3);
		}
	}
	for (S32 i = 0, count = items->size(); i < count; ++i)
	{
		LLViewerInventoryItem* item = (*items)[i];
		if (item)	// Paranoia
		{
			lua_pushstring(statep, item->getUUID().asString().c_str());
			lua_pushstring(statep, item->getName().c_str());
			lua_rawset(statep, -3);
		}
	}

	return 1;
}

//static
int HBViewerAutomation::moveToInventoryFolder(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("MoveToInventoryFolder");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}
	std::string folder_path(luaL_checkstring(statep, 1));
	LLUUID cat_id;
	if (folder_path.empty())
	{
		cat_id = gInventory.getRootFolderID();
	}
	else
	{
		bool is_category = false;
		cat_id = getInventoryObjectId(folder_path, is_category);
		if (!is_category)
		{
			cat_id.setNull();
		}
	}
	if (cat_id.isNull())
	{
		llwarns << "Could not find destination folder: " << folder_path
				<< llendl;
	}
	// Forbid to move to trash, COF or market place folders.
	else if (gInventory.isInTrash(cat_id) || gInventory.isInCOF(cat_id) ||
			 gInventory.isInMarketPlace(cat_id))
	{
		llwarns << "Invalid destination folder." << llendl;
		cat_id.setNull();
	}
	if (cat_id.isNull())
	{
		lua_pop(statep, n);
		lua_pushboolean(statep, false);
		return 1;
	}
	LL_DEBUGS("Lua") << "Destination folder found. Id = " << cat_id << LL_ENDL;

	bool success = true;

	uuid_vec_t inv_objects;
	std::string invobj_path;
	LLUUID obj_id;
	bool is_category = false;

	int type = lua_type(statep, 2);
	if (type == LUA_TTABLE)
	{
		// We accept either a list of string values (i.e. with numbers as keys)
		// representing inventory items paths or UUIDs, or a table with UUIDs
		// as keys and paths as values (as returned by ListInventoryFolder()).
		lua_pushnil(statep);
		while (lua_next(statep, 2))
		{
			int key_type = lua_type(statep, -2);
			if (key_type == LUA_TNUMBER)
			{
				// It could be an element of a list of strings.
				if (lua_type(statep, -1) != LUA_TSTRING)
				{
					llwarns << "Table element key is a number but value is not a string."
							<< llendl;
					success = false;
					break;
				}
				// Use the string value to find the inventory object
				invobj_path = lua_tostring(statep, -1);
			}
			else if (key_type == LUA_TSTRING)
			{
				// It is a pair of key,value, and we expect the key to be the
				// UUID or the full path name for an inventory object.
				invobj_path = lua_tostring(statep, -2);				
			}
			else
			{
				llwarns << "Table element key is not a number or string."
						<< llendl;
				success = false;
				break;
			}
			if (invobj_path.empty())
			{
				llwarns << "Inventory object path/UUID empty." << llendl;
				success = false;
				break;
			}
			obj_id = getInventoryObjectId(invobj_path, is_category);
			if (obj_id.isNull())
			{
				llwarns << "Could not find inventory object: " << invobj_path
						<< llendl;
				success = false;
				break;
			}
			LL_DEBUGS("Lua") << "Inventory object found. Id = " << obj_id
							 << LL_ENDL;
			inv_objects.emplace_back(obj_id);
			lua_pop(statep, 1);
		}
	}
	else
	{
		// We accept a single inventory object path or UUID too, passed as a
		// string.
		invobj_path = luaL_checkstring(statep, 2);
		if (!invobj_path.empty())
		{
			obj_id = getInventoryObjectId(invobj_path, is_category);
		}
		success = obj_id.notNull();
		if (success)
		{
			inv_objects.emplace_back(obj_id);
			LL_DEBUGS("Lua") << "Inventory object found. Id = " << obj_id
							 << LL_ENDL;
		}
		else
		{
			llwarns << "Could not find inventory object: " << invobj_path
					<< llendl;
		}
	}
	lua_pop(statep, lua_gettop(statep));

	if (success)
	{
		success = !inv_objects.empty() &&
				  reparent_to_folder(cat_id, inv_objects);
	}

	lua_pushboolean(statep, success);
	return 1;
}

//static
void HBViewerAutomation::onPickInventoryItem(const std::vector<std::string>& names,
											 const uuid_vec_t& ids,
											 void* datap, bool on_close)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	lua_State* statep = (lua_State*)datap;
	HBViewerAutomation* self = findInstance(statep);
	if (!self || !self->mHasOnPickInventoryItem) return;

	S32 count = ids.size();
	LL_DEBUGS("Lua") << "Invoking OnPickInventoryItem Lua callback with "
					 << count << " selected inventory item"
					 << (count > 1 ? "s." : ".") << LL_ENDL;

	lua_getglobal(statep, "OnPickInventoryItem");
	if (count)
	{
		lua_newtable(statep);
		for (S32 i = 0; i < count; ++i)
		{
			lua_pushstring(statep, ids[i].asString().c_str());
			lua_pushstring(statep, names[i].c_str());
			lua_rawset(statep, -3);
		}
	}
	else
	{
		lua_pushnil(statep);
	}
	lua_pushboolean(statep, on_close);

	self->resetTimer();
	if (lua_pcall(statep, 2, 0, 0) != LUA_OK)
	{
		self->reportError();
	}
}

//static
int HBViewerAutomation::pickInventoryItem(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 7)
	{
		luaL_error(statep, "%d arguments passed; expected 2 to 7.", n);
	}
	S32 type = lua_tointeger(statep, 1);
	S32 subtype = -1;
	if (n > 1)
	{
		subtype = lua_tointeger(statep, 2);
	}
	bool allow_multiple = n >= 3 && lua_toboolean(statep, 3);
	bool exclude_library = n < 4 || lua_toboolean(statep, 4);
	bool can_apply_immediately = false;
	bool apply_immediately = false;
	if (n >= 5)
	{
		can_apply_immediately = true;
		apply_immediately = lua_toboolean(statep, 5);
	}
	PermissionMask mask = PERM_NONE;	// No restriction on permissions
	if (n >= 6)
	{
		mask = lua_tointeger(statep, 6);
	}
	bool callback_on_close = n >= 7 && lua_toboolean(statep, 7);
	lua_pop(statep, n);

	// NOTE: the inventory item picker will auto-close on selection or cancel
	// action. We therefore do not need to track its pointer...
	HBFloaterInvItemsPicker* pickerp =
		new HBFloaterInvItemsPicker(NULL, &onPickInventoryItem, statep);
	pickerp->setAssetType((LLAssetType::EType)type, subtype);
	pickerp->setAllowMultiple(allow_multiple);
	pickerp->setExcludeLibrary(exclude_library);
	pickerp->setFilterPermMask(mask);
	if (can_apply_immediately)
	{
		pickerp->allowApplyImmediately();
		pickerp->setApplyImmediately(apply_immediately);
	}
	if (callback_on_close)
	{
		pickerp->callBackOnClose();
	}

	return 0;
}

//static
void HBViewerAutomation::onPickAvatar(const std::vector<std::string>& names,
									  const std::vector<LLUUID>& ids,
									  void* datap)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	lua_State* statep = (lua_State*)datap;
	HBViewerAutomation* self = findInstance(statep);
	if (!self || !self->mHasOnPickAvatar) return;

	S32 count = ids.size();
	LL_DEBUGS("Lua") << "Invoking OnPickAvatar Lua callback with "
					 << count << " picked avatars" << (count > 1 ? "s." : ".")
					 << LL_ENDL;

	lua_getglobal(statep, "OnPickAvatar");
	if (count)
	{
		lua_newtable(statep);
		for (S32 i = 0; i < count; ++i)
		{
			lua_pushstring(statep, ids[i].asString().c_str());
			lua_pushstring(statep, names[i].c_str());
			lua_rawset(statep, -3);
		}
	}
	else
	{
		lua_pushnil(statep);
	}
	self->resetTimer();
	if (lua_pcall(statep, 1, 0, 0) != LUA_OK)
	{
		self->reportError();
	}
}

//static
int HBViewerAutomation::pickAvatar(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n > 2)
	{
		luaL_error(statep, "%d arguments passed; expected 0 to 2.", n);
	}
	bool allow_multiple = n >= 1 && lua_toboolean(statep, 1);
	std::string search_name;
	if (n > 1)
	{
		search_name.assign(luaL_checkstring(statep, 2));
	}
	lua_pop(statep, n);

	// NOTE: the avatar picker will auto-close on selection or cancel action.
	// We therefore do not need to track its pointer...
	LLFloaterAvatarPicker::show(&onPickAvatar, statep, allow_multiple, true,
								search_name);

	return 0;
}

//static
int HBViewerAutomation::getAgentAttachments(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || !isAgentAvatarValid()) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentAttachments");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	std::string search_string;
	if (n)
	{
		search_string.assign(luaL_checkstring(statep, 1));
		lua_pop(statep, 1);
		LLStringUtil::toLower(search_string);
	}
	bool has_search_string = !search_string.empty();

	lua_newtable(statep);

	std::string inv_item_uuid, item_name, lc_name, joint_name;
	for (S32 i = 0, count = gAgentAvatarp->mAttachedObjectsVector.size();
		 i < count; ++i)
	{
		LLViewerJointAttachment* vatt =
			gAgentAvatarp->mAttachedObjectsVector[i].second;
		if (!vatt) continue;	// Paranoia

		joint_name = LLTrans::getString(vatt->getName());
		LLStringUtil::toLower(joint_name);

		LLViewerObject* vobj = gAgentAvatarp->mAttachedObjectsVector[i].first;
		if (!vobj) continue;	// Paranoia

		const LLUUID& item_id = vobj->getAttachmentItemID();
		if (item_id.isNull()) continue;

		LLViewerInventoryItem* inv_item = gInventory.getItem(item_id);
		if (inv_item)
		{
			inv_item_uuid = inv_item->getLinkedUUID().asString();
			item_name = inv_item->getName();
		}
		else if (vobj->isTempAttachment())
		{
			inv_item_uuid = item_id.asString();
			item_name = "temp_attachment:" + inv_item_uuid;
		}
		else
		{
			llwarns << "Could not find any valid object for attachment Id: "
					<< item_id << llendl;
			continue;
		}

		if (has_search_string)
		{
			lc_name = item_name;
			LLStringUtil::toLower(lc_name);
		}

		if (!has_search_string || joint_name == search_string ||
			inv_item_uuid == search_string ||
			lc_name.find(search_string) != std::string::npos)
		{
			item_name += "|" + joint_name;
			lua_pushstring(statep, inv_item_uuid.c_str());
			lua_pushstring(statep, item_name.c_str());
			lua_rawset(statep, -3);
		}
	}

	return 1;
}

//static
int HBViewerAutomation::removeAgentAttachment(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || !isAgentAvatarValid()) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("RemoveAgentAttachment");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	LLUUID id = LLUUID(luaL_checkstring(statep, 1), false);
	if (id.isNull())
	{
		luaL_error(statep, "Invalid attachment Id");
	}
	lua_pop(statep, 1);

	LLViewerObject* objp = NULL;
	for (S32 i = 0, count = gAgentAvatarp->mAttachedObjectsVector.size();
		 i < count; ++i)
	{
		objp = gAgentAvatarp->mAttachedObjectsVector[i].first;
		if (!objp) continue;	// Paranoia

		const LLUUID& item_id = objp->getAttachmentItemID();
		if (item_id == id || objp->getID() == id)
		{
			if (gRLenabled && !objp->isTempAttachment())
			{
				LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
				if (itemp && !gRLInterface.canDetach(itemp))
				{
					lua_pushboolean(statep, false);
					return 1;
				}
			}
			break;
		}
		objp = NULL;
	}

	if (objp)
	{
		// Deselect if needed.
		gSelectMgr.remove(objp);

		LLMessageSystem* msg = gMessageSystemp;
		msg->newMessage(_PREHASH_ObjectDetach);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_ObjectLocalID, objp->getLocalID());
		msg->sendReliable(gAgent.getRegionHost());

		lua_pushboolean(statep, true);
	}
	else
	{
		lua_pushnil(statep);
	}

	return 1;
}

//static
int HBViewerAutomation::getAgentWearables(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || !isAgentAvatarValid()) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentWearables");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	std::string search_string;
	if (n)
	{
		search_string.assign(luaL_checkstring(statep, 1));
		lua_pop(statep, 1);
		LLStringUtil::toLower(search_string);
	}
	bool has_search_string = !search_string.empty();

	lua_newtable(statep);

	std::string inv_item_uuid, item_name, lc_name, type_name;
	for (U32 i = 0; i < (U32)LLWearableType::WT_COUNT; ++i)
	{
		LLWearableType::EType type = (LLWearableType::EType)i;

		type_name = LLTrans::getString(LLWearableType::getTypeLabel(type));
		LLStringUtil::toLower(type_name);

		for (U32 j = 0, count = gAgentWearables.getWearableCount(type);
			 j < count; ++j)
		{
			LLViewerWearable* wearable =
				gAgentWearables.getViewerWearable(type, j);
			if (!wearable) continue;

			LLViewerInventoryItem* inv_item =
				gInventory.getItem(wearable->getItemID());
			if (!inv_item) continue;

			inv_item_uuid = inv_item->getLinkedUUID().asString();
			item_name = inv_item->getName();
			if (has_search_string)
			{
				lc_name = item_name;
				LLStringUtil::toLower(lc_name);
			}

			if (!has_search_string || type_name == search_string ||
				inv_item_uuid == search_string ||
				lc_name.find(search_string) != std::string::npos)
			{
				item_name += "|" + type_name;
				lua_pushstring(statep, inv_item_uuid.c_str());
				lua_pushstring(statep, item_name.c_str());
				lua_rawset(statep, -3);
			}
		}
	}

	return 1;
}

//static
int HBViewerAutomation::getGridSimAndPos(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetGridSimAndPos");
	}

	S32 n = lua_gettop(statep);
	if (n != 0)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	self->pushGridSimAndPos();

	return 1;
}

//static
int HBViewerAutomation::getParcelInfo(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetParcelInfo");
	}

	S32 n = lua_gettop(statep);
	if (n != 0)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	self->pushParcelInfo();

	return 1;
}

//static
int HBViewerAutomation::getCameraMode(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetCameraMode");
	}

	S32 n = lua_gettop(statep);
	if (n != 0)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_pushnumber(statep, gAgent.getCameraMode());

	return 1;
}

//static
int HBViewerAutomation::setCameraMode(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetCameraMode");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	S32 mode = lua_tointeger(statep, 1);
	bool animate = n < 2 || lua_toboolean(statep, 2);
	lua_pop(statep, n);

	HBIgnoreCallback lock_on_camera_change(E_ONCAMERAMODECHANGE);

	bool success = false;
	if (mode == -2)
	{
		success = handle_reset_view();
	}
	else if (mode == -1)
	{
		success = gAgent.changeCameraToDefault(animate);
	}
	else if (mode == (S32)CAMERA_MODE_THIRD_PERSON)
	{
		success = gAgent.changeCameraToThirdPerson(animate);
	}
	else if (mode == (S32)CAMERA_MODE_MOUSELOOK)
	{
		success = gAgent.changeCameraToMouselook(animate);
	}
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setCameraFocus(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetCameraFocus");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	std::string id_str;
	if (n)
	{
		id_str = luaL_checkstring(statep, 1);
		if (!id_str.empty() && !LLUUID::validate(id_str))
		{
			luaL_error(statep, "Invalid UUID: %s", id_str.c_str());
		}
		lua_pop(statep, 1);
	}

	HBIgnoreCallback lock_on_camera_change(E_ONCAMERAMODECHANGE);

	if (id_str.empty())
	{
		gAgent.setFocusOnAvatar(true);
	}
	else
	{
		gAgent.lookAtObject(LLUUID(id_str), CAMERA_POSITION_OBJECT);
	}

	return 0;
}

//static
int HBViewerAutomation::setVisualMute(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetVisualMute");
	}

	S32 n = lua_gettop(statep);
	if (n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 2.", n);
	}

	LLUUID avid = LLUUID(luaL_checkstring(statep, 1), false);
	if (avid.isNull())
	{
		luaL_error(statep, "Invalid avatar Id");
	}
	if (avid == gAgentID)
	{
		luaL_error(statep, "Cannot visually mute your own avatar");
	}

	S32 val = luaL_checknumber(statep, 2);
	if (val < 0 || val > 2)
	{
		luaL_error(statep, "Invalid visual mute value passed: %d", val);
	}

	lua_pop(statep, 2);

	bool success = false;

	LLVOAvatar* avatarp = gObjectList.findAvatar(avid);
	if (avatarp)
	{
		avatarp->setVisualMuteSettings((LLVOAvatar::VisualMuteSettings)val);
		success = true;
	}

	lua_pushboolean(statep, success);

	return 1;
}

static void on_name_cache_mute(const LLUUID& id, const std::string& name,
							   bool is_group, S32 flags, bool mute_it)
{
	LLMute mute(id, name, is_group ? LLMute::GROUP : LLMute::AGENT);
	if (mute_it)
	{
		LLMuteList::add(mute, flags);
	}
	else
	{
		LLMuteList::remove(mute, flags);
	}
}

//static
int HBViewerAutomation::addMute(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AddMute");
	}

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 3)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 3.", n);
	}

	std::string name = luaL_checkstring(statep, 1);
	LLUUID id;
	if (LLUUID::validate(name))
	{
		id.set(name);
		name.clear();
	}

	S32 type = LLMute::BY_NAME;
	if (n > 1)
	{
		type = luaL_checknumber(statep, 2);
		if (type < 0 || type >= (S32)LLMute::COUNT)
		{
			luaL_error(statep, "Invalid mute type passed: %d", type);
		}
	}
	if (type == (S32)LLMute::BY_NAME && id.notNull())
	{
		luaL_error(statep, "Cannot mute by name with an UUID");
	}

	S32 flags = 0;
	if (n > 2)
	{
		flags = luaL_checknumber(statep, 3);
		if (flags < 0)
		{
			luaL_error(statep, "Invalid mute flag(s) passed: %d", flags);
		}
	}

	lua_pop(statep, n);

	bool success = false;
	switch (type)
	{
		case (S32)LLMute::AGENT:
		case (S32)LLMute::GROUP:
		{
			if (gCacheNamep)
			{
				gCacheNamep->get(id, type == (S32)LLMute::GROUP,
								 std::bind(&on_name_cache_mute, _1, _2, _3,
										   flags, true));
				success = true;
			}
			break;
		}

		case (S32)LLMute::OBJECT:
		{
			success = requestObjectPropertiesFamily(id, 0);
			break;
		}

		case (S32)LLMute::BY_NAME:
		{
			LLMute mute(LLUUID::null, name, LLMute::BY_NAME);
			success = LLMuteList::add(mute, flags);
			break;
		}

		default:	// Never happens, unless the LLMute::EType enum got changed
			llerrs << "Invalid mute type: " << type << llendl;
	}
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::removeMute(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("RemoveMute");
	}

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 3)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 3.", n);
	}

	std::string name = luaL_checkstring(statep, 1);
	LLUUID id;
	if (LLUUID::validate(name))
	{
		id.set(name);
		name.clear();
	}

	S32 type = LLMute::BY_NAME;
	if (n > 1)
	{
		type = luaL_checknumber(statep, 2);
		if (type < 0 || type >= (S32)LLMute::COUNT)
		{
			luaL_error(statep, "Invalid mute type passed: %d", type);
		}
	}
	if (type == (S32)LLMute::BY_NAME && id.notNull())
	{
		luaL_error(statep, "Cannot unmute by name with an UUID");
	}

	S32 flags = 0;
	if (n > 2)
	{
		flags = luaL_checknumber(statep, 3);
		if (flags < 0)
		{
			luaL_error(statep, "Negative mute flag passed: %d", flags);
		}
	}

	lua_pop(statep, n);

	bool success = false;
	switch (type)
	{
		case (S32)LLMute::AGENT:
		case (S32)LLMute::GROUP:
		{
			if (gCacheNamep)
			{
				gCacheNamep->get(id, type == (S32)LLMute::GROUP,
								 std::bind(&on_name_cache_mute, _1, _2, _3,
										   flags, false));
				success = true;
			}
			break;
		}

		case (S32)LLMute::OBJECT:
		{
			success = requestObjectPropertiesFamily(id, 1);
			break;
		}

		case (S32)LLMute::BY_NAME:
		{
			LLMute mute(LLUUID::null, name, LLMute::BY_NAME);
			success = LLMuteList::remove(mute);
			break;
		}

		default:	// Never happens, unless the LLMute::EType enum got changed
			llerrs << "Invalid mute type: " << type << llendl;
	}
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::isMuted(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("IsMuted");
	}

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 3)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 3.", n);
	}

	std::string name = luaL_checkstring(statep, 1);
	LLUUID object_id;
	if (LLUUID::validate(name))
	{
		object_id.set(name);
		name.clear();
	}

	S32 type = LLMute::COUNT;
	if (n > 1)
	{
		type = luaL_checknumber(statep, 2);
		if (type < 0 || type > (S32)LLMute::COUNT)
		{
			luaL_error(statep, "Invalid mute type passed: %d", type);
		}
	}

	S32 flags = 0;
	if (n > 2)
	{
		flags = luaL_checknumber(statep, 3);
		if (flags < 0)
		{
			luaL_error(statep, "Negative mute flag passed: %d", flags);
		}
	}

	lua_pop(statep, n);

	lua_pushboolean(statep, LLMuteList::isMuted(object_id, name, flags,
												(LLMute::EType)type));

	return 1;
}

//static
int HBViewerAutomation::blockSound(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("BlockSound");
	}

	S32 n = lua_gettop(statep);
	if (n != 1 && n != 2)
	{
		luaL_error(statep, "%d arguments passed; expected 1 or 2.", n);
	}

	std::string id_str = luaL_checkstring(statep, 1);
	if (!LLUUID::validate(id_str))
	{
		luaL_error(statep, "Invalid UUID: %s", id_str.c_str());
	}

	bool block = n < 2 || lua_toboolean(statep, 2);

	lua_pop(statep, n);

	LLAudioData::blockSound(LLUUID(id_str), block);

	// Inform the sounds list floater (if opened) that blocked sounds changed.
	HBFloaterSoundsList::setDirty();

	return 0;
}

//static
int HBViewerAutomation::isBlockedSound(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("IsBlockedSound");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string id_str = luaL_checkstring(statep, 1);
	if (!LLUUID::validate(id_str))
	{
		luaL_error(statep, "Invalid UUID: %s", id_str.c_str());
	}
	lua_pop(statep, 1);

	lua_pushboolean(statep,  LLAudioData::isBlockedSound(LLUUID(id_str)));

	return 1;
}

//static
int HBViewerAutomation::getBlockedSounds(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetBlockedSounds");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	const uuid_list_t& sounds = LLAudioData::getBlockedSounds();
	int i = 0;
	lua_newtable(statep);
	for (uuid_list_t::const_iterator it = sounds.begin(), end = sounds.end();
 		 it != end; ++it)
	{
		lua_pushstring(statep, it->asString().c_str());
		lua_rawseti(statep, -2, ++i);
	}

	return 1;
}

//static
int HBViewerAutomation::derenderObject(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("DerenderObject");
	}

	S32 n = lua_gettop(statep);
	if (n > 2)
	{
		luaL_error(statep, "%d arguments passed; expected 0 to 2.", n);
	}

	LLUUID object_id;
	if (n > 0)
	{
		object_id.set(luaL_checkstring(statep, 1), false);
	}
	bool derender = n < 2 || lua_toboolean(statep, 2);
	lua_pop(statep, n);

	bool success = true;
	if (n == 0)
	{
		gObjectList.mBlackListedObjects.clear();
		HBFloaterRadar::setRenderStatusDirty();
	}
	else if (derender)
	{
		// Note: HBFloaterRadar::setRenderStatusDirty() will be called if
		// needed by derender_object().
		success = derender_object(object_id);
	}
	else if (gObjectList.mBlackListedObjects.count(object_id))
	{
		gObjectList.mBlackListedObjects.erase(object_id);
		// Call unconditionnaly (even for non-avatar objects): it really does
		// not matter, and searching for the object in the avatars list to
		// check whether it is an avatar or not would take more time.
		HBFloaterRadar::setRenderStatusDirty(object_id);
	}
	else
	{
		success = false;
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::getDerenderedObjects(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetDerenderedObjects");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	int i = 0;
	lua_newtable(statep);
	for (uuid_list_t::iterator it = gObjectList.mBlackListedObjects.begin(),
							   end = gObjectList.mBlackListedObjects.end();
		 it != end ; ++it)
	{
		lua_pushstring(statep, (*it).asString().c_str());
		lua_rawseti(statep, -2, ++i);
	}

	return 1;
}

//static
int HBViewerAutomation::getAgentPushes(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetAgentPushes");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	LLUUID perpetrator_id(luaL_checkstring(statep, 1), false);
	lua_pop(statep, 1);

	std::string desc = HBFloaterBump::getMeanCollisionsStats(perpetrator_id);

	lua_pushstring(statep, desc.c_str());

	return 1;
}

//static
int HBViewerAutomation::applyDaySettings(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ApplyDaySettings");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string preset(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	bool success = LLStartUp::isLoggedIn() &&
//MK
				   (!gRLenabled || !gRLInterface.mContainsSetenv);
//mk
	if (!success)
	{
		lua_pushboolean(statep, success);
		return 1;
	}

	HBIgnoreCallback lock_on_wl_change(E_ONWINDLIGHTCHANGE);

	// Check special "settings" that trigger specific environments
	if (preset == "animate")
	{
		LLEnvironment::setRegion();
	}
	else if (preset == "region")
	{
		success = false;
	}
	else if (preset == "sunrise")
	{
		LLEnvironment::setSunrise();
	}
	else if (preset == "midday" || preset == "noon")
	{
		LLEnvironment::setMidday();
	}
	else if (preset == "noon_pbr")
	{
		LLEnvironment::setMiddayPBR();
	}
	else if (preset == "sunset")
	{
		LLEnvironment::setSunset();
	}
	else if (preset == "midnight")
	{
		LLEnvironment::setMidnight();
	}
	else if (preset == "parcel")
	{
		gSavedSettings.setBool("UseParcelEnvironment", true);
	}
	else if (preset == "local")
	{
		gSavedSettings.setBool("UseLocalEnvironment", true);
	}
	else if (preset == "windlight")
	{
		gSavedSettings.setBool("UseParcelEnvironment", false);
		gSavedSettings.setBool("UseLocalEnvironment", false);
	}
	else
	{
		success = false;
	}

	// Then try actual settings (inventory assets or Windlight)
	if (!success)
	{
		success = LLEnvSettingsDay::applyPresetByName(preset);
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::applySkySettings(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ApplySkySettings");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string preset(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	bool success = LLStartUp::isLoggedIn() &&
//MK
				   (!gRLenabled || !gRLInterface.mContainsSetenv);
//mk
	if (success)
	{
		HBIgnoreCallback lock_on_wl_change(E_ONWINDLIGHTCHANGE);
		success = LLEnvSettingsSky::applyPresetByName(preset);
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::applyWaterSettings(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("ApplyWaterSettings");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string preset(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	bool success = LLStartUp::isLoggedIn() &&
//MK
				   (!gRLenabled || !gRLInterface.mContainsSetenv);
//mk

	if (success)
	{
		HBIgnoreCallback lock_on_wl_change(E_ONWINDLIGHTCHANGE);
		success = LLEnvSettingsWater::applyPresetByName(preset);
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::setDayTime(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("SetDayTime");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	F32 time = luaL_checknumber(statep, 1);
	lua_pop(statep, 1);

	if (!LLStartUp::isLoggedIn() || time > 1.f ||
//MK
		(gRLenabled && gRLInterface.mContainsSetenv))
//mk
	{
		return 0;
	}

	HBIgnoreCallback lock_on_wl_change(E_ONWINDLIGHTCHANGE);

	if (time < 0.f)
	{
		// Revert to parcel environment...
		gSavedSettings.setBool("UseParcelEnvironment", true);
		return 0;
	}

	// Extended environment time of day, using a fixed sky setting...
	if (gEnvironment.hasEnvironment(LLEnvironment::ENV_LOCAL))
	{
		if (gEnvironment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
		{
			// We have a full day cycle in the local environment: freeze the
			// sky.
			LLSettingsSky::ptr_t skyp =
				gEnvironment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL)->buildClone();
			gEnvironment.setEnvironment(LLEnvironment::ENV_LOCAL, skyp, 0);
		}
	}
	else
	{
		// Use a copy of the parcel environment sky instead.
		LLSettingsSky::ptr_t skyp =
			gEnvironment.getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL,
												true)->buildClone();
		gEnvironment.setEnvironment(LLEnvironment::ENV_LOCAL, skyp, 0);
	}
	gEnvironment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL,
										LLEnvironment::TRANSITION_INSTANT);

	// Set the time now...
	gEnvironment.setFixedTimeOfDay(time);

	return 0;
}

//static
int HBViewerAutomation::getEESettingsList(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetEESettingsList");
	}

	S32 n = lua_gettop(statep);
	if (n != 0 && n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	S32 wanted_type = -1;
	if (n)
	{
		wanted_type = luaL_checknumber(statep, 1);
		lua_pop(statep, n);
		if (wanted_type < 0 || wanted_type > 2)
		{
			wanted_type = -1;
		}
	}

	if (!gAgent.hasInventorySettings())
	{
		lua_pushnil(statep);
		return 1;
	}

	const LLUUID& folder_id =
		gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS, false);
	if (folder_id.isNull())
	{
		lua_pushnil(statep);
		return 1;
	}

	strings_map_t settings;

	LLEnvSettingsCollector collector;
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendentsIf(folder_id, cats, items, false, collector);
	std::string name, type_str;
	for (LLInventoryModel::item_array_t::iterator iter = items.begin(),
												  end = items.end();
		 iter != end; ++iter)
	{
		LLViewerInventoryItem* itemp = *iter;
		S32 type = (S32)itemp->getSettingsType();
		if (type < 0 || type > 2 || (wanted_type != -1 && type != wanted_type))
		{
			continue;
		}
		name = itemp->getName();
		strings_map_t::iterator sit = settings.find(name);
		if (sit == settings.end())
		{
			settings[name] = sEnvSettingsTypes[type];
		}
		else if (sit->second.find(sEnvSettingsTypes[type]) == std::string::npos)
		{
			sit->second += "," + sEnvSettingsTypes[type];
		}
	}

	if (settings.empty())
	{
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);
	for (strings_map_t::iterator it = settings.begin(), end = settings.end();
		 it != end; ++it)
	{
		lua_pushstring(statep, it->first.c_str());
		lua_pushstring(statep, it->second.c_str());
		lua_rawset(statep, -3);
	}
	return 1;
}

//static
int HBViewerAutomation::getWLSettingsList(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetWLSettingsList");
	}

	S32 n = lua_gettop(statep);
	if (n != 0 && n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	S32 wanted_type = -1;
	if (n)
	{
		wanted_type = luaL_checknumber(statep, 1);
		lua_pop(statep, n);
		if (wanted_type < 0 || wanted_type > 2)
		{
			wanted_type = -1;
		}
	}

	if (!gAgent.hasInventorySettings())
	{
		lua_pushnil(statep);
		return 1;
	}

	strings_map_t settings;

	std::vector<std::string> presets;

	if (wanted_type == 0 || wanted_type == -1)
	{
		presets = LLWLSkyParamMgr::getLoadedPresetsList();
		for (S32 i = 0, count = presets.size(); i < count; ++i)
		{
			const std::string& name = presets[i];
			settings[name] = "sky";
		}
	}

	if (wanted_type == 1 || wanted_type == -1)
	{
		presets = LLWLWaterParamMgr::getLoadedPresetsList();
		for (S32 i = 0, count = presets.size(); i < count; ++i)
		{
			const std::string& name = presets[i];
			strings_map_t::iterator sit = settings.find(name);
			if (sit == settings.end())
			{
				settings[name] = "water";
			}
			else if (sit->second.find("water") == std::string::npos)
			{
				sit->second += ",water";
			}
		}
	}

	if (wanted_type == 2 || wanted_type == -1)
	{
		presets = LLWLDayCycle::getLoadedPresetsList();
		for (S32 i = 0, count = presets.size(); i < count; ++i)
		{
			const std::string& name = presets[i];
			strings_map_t::iterator sit = settings.find(name);
			if (sit == settings.end())
			{
				settings[name] = "day";
			}
			else if (sit->second.find("day") == std::string::npos)
			{
				sit->second += ",day";
			}
		}
	}

	if (settings.empty())
	{
		lua_pushnil(statep);
		return 1;
	}

	lua_newtable(statep);
	for (strings_map_t::iterator it = settings.begin(), end = settings.end();
		 it != end; ++it)
	{
		lua_pushstring(statep, it->first.c_str());
		lua_pushstring(statep, it->second.c_str());
		lua_rawset(statep, -3);
	}
	return 1;
}

//static
int HBViewerAutomation::getEnvironmentStatus(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("GetEnvironmentStatus");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	lua_newtable(statep);

#if 1	// *TODO: remove ?
	lua_pushstring(statep, "enhanced rendering");
	lua_pushboolean(statep, true);
	lua_rawset(statep, -3);

	lua_pushstring(statep, "windlight override");
	lua_pushboolean(statep, true);
	lua_rawset(statep, -3);
#endif

	static LLCachedControl<bool> local(gSavedSettings, "UseLocalEnvironment");
	lua_pushstring(statep, "local environment");
	lua_pushboolean(statep, (bool)local);
	lua_rawset(statep, -3);

	static LLCachedControl<bool> parcel(gSavedSettings,
										"UseParcelEnvironment");
	lua_pushstring(statep, "parcel environment");
	lua_pushboolean(statep, (bool)parcel);
	lua_rawset(statep, -3);

	static LLCachedControl<bool> estatep(gSavedSettings, "UseWLEstateTime");
	bool region_time = estatep;
	if (region_time)
	{
		region_time = gWLSkyParamMgr.mAnimator.mIsRunning;
	}
	lua_pushstring(statep, "region time");
	lua_pushboolean(statep, region_time);
	lua_rawset(statep, -3);

	lua_pushstring(statep, "rlv locked");
	lua_pushboolean(statep, gRLenabled && gRLInterface.mContainsSetenv);
	lua_rawset(statep, -3);

	return 1;
}

void HBViewerAutomation::onAutoPilotFinished(const std::string& type,
											 bool reached, bool user_cancel)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	if (!mHasOnAutoPilotFinished || !mLuaState)
	{
		return;
	}

	LL_DEBUGS("Lua") << "Invoking OnAutoPilotFinished Lua callback. type="
					 << type << " - reached=" << reached << " - user_cancel="
					 << user_cancel << LL_ENDL;

	lua_getglobal(mLuaState, "OnAutoPilotFinished");
	lua_pushstring(mLuaState, type.c_str());
	lua_pushboolean(mLuaState, reached);
	lua_pushboolean(mLuaState, user_cancel);
	resetTimer();
	if (lua_pcall(mLuaState, 3, 0, 0) != LUA_OK)
	{
		reportError();
	}
}

//static
int HBViewerAutomation::agentAutoPilotToPos(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || !isAgentAvatarValid()) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotToPos");
	}

	S32 n = lua_gettop(statep);
	if (n < 2 || n > 5)
	{
		luaL_error(statep, "%d arguments passed; expected 2 to 5.", n);
	}

	F64 pos_x = luaL_checknumber(statep, 1);
	F64 pos_y = luaL_checknumber(statep, 2);
	F64 pos_z = -1.0;
	if (n >= 3)
	{
		pos_z = luaL_checknumber(statep, 3);
	}
	if (pos_z < 0.0)
	{
		pos_z = gAgentAvatarp->getPositionGlobal().mdV[VZ];
	}
	bool allow_flying = n >= 4 && lua_toboolean(statep, 4);

	F32 stop_distance = 1.f;
	if (n >= 5)
	{
		stop_distance = luaL_checknumber(statep, 5);
	}

	lua_pop(statep, n);

	static S32 counter = 0;
	std::string type = llformat("Lua auto-pilot %d", ++counter);

	gAgentPilot.startAutoPilotGlobal(LLVector3d(pos_x, pos_y, pos_z),
									 type, NULL, NULL, NULL, stop_distance,
									 0.03f, allow_flying);

	lua_pushstring(statep, type.c_str());

	return 1;
}

//static
int HBViewerAutomation::agentAutoPilotFollow(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotFollow");
	}

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 3)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 3.", n);
	}

	LLUUID id(luaL_checkstring(statep, 1), false);
	bool allow_flying = n >= 2 && lua_toboolean(statep, 2);
	F32 stop_distance = 1.f;
	if (n >= 3)
	{
		stop_distance = luaL_checknumber(statep, 3);
	}
	lua_pop(statep, n);

	bool success = gAgentPilot.startFollowPilot(id, allow_flying,
												stop_distance);
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::agentAutoPilotStop(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotStop");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	gAgentPilot.stopAutoPilot();

	return 0;
}

//static
int HBViewerAutomation::agentAutoPilotLoad(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotLoad");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	std::string filename;
	if (n)
	{
		filename = luaL_checkstring(statep, 1);
		lua_pop(statep, 1);
	}
	else
	{
		filename = gSavedSettings.getString("AutoPilotFile");
	}

	lua_pushboolean(statep, gAgentPilot.load(filename));

	return 1;
}

//static
int HBViewerAutomation::agentAutoPilotSave(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotSave");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	std::string filename;
	if (n)
	{
		filename = luaL_checkstring(statep, 1);
		lua_pop(statep, 1);
	}
	else
	{
		filename = gSavedSettings.getString("AutoPilotFile");
	}

	lua_pushboolean(statep, gAgentPilot.save(filename));

	return 1;
}

//static
int HBViewerAutomation::agentAutoPilotRemove(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotRemove");
	}

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}
	std::string filename;
	if (n)
	{
		filename = luaL_checkstring(statep, 1);
		lua_pop(statep, 1);
	}
	else
	{
		filename = gSavedSettings.getString("AutoPilotFile");
	}

	LLAgentPilot::remove(filename);

	return 0;
}

//static
int HBViewerAutomation::agentAutoPilotRecord(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotRecord");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	bool start = lua_toboolean(statep, 1);
	lua_pop(statep, 1);

	bool success = start ? gAgentPilot.startRecord()
						 : gAgentPilot.stopRecord();
	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::agentAutoPilotReplay(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentAutoPilotReplay");
	}

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 3)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 3.", n);
	}

	bool start = lua_toboolean(statep, 1);
	if (!start && n > 1)
	{
		luaL_error(statep,
				   "%d arguments passed; expected only 1 for a stop action.",
				   n);
	}

	S32 runs = -1;
	if (n >= 2)
	{
		runs = lua_tointeger(statep, 2);
	}

	bool allow_flying = n >= 3 && lua_toboolean(statep, 3);

	lua_pop(statep, n);

	bool success = start ? gAgentPilot.startPlayback(runs, allow_flying)
						 : gAgentPilot.stopPlayback();
	lua_pushboolean(statep, success);

	return 1;
}

#if LL_PUPPETRY
//static
int HBViewerAutomation::agentPuppetryStart(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	S32 n = lua_gettop(statep);
	if (n > 2)
	{
		luaL_error(statep, "%d arguments passed; expected 0 to 2.", n);
	}

	bool is_plugin_filename = n >= 2 && lua_toboolean(statep, 2);

	bool is_saved_cmd;
	std::string command;
	if (n)
	{
		is_saved_cmd = false;
		command = luaL_checkstring(statep, 1);
		lua_pop(statep, n);
	}
	else
	{
		is_saved_cmd = true;
		command = gSavedSettings.getString("PuppetryLastCommand");
	}

	bool success = false;

	if (!command.empty() && LLPuppetMotion::enabled())
	{
		LLPuppetModule* modulep = LLPuppetModule::getInstance();
		// Only try and launch when no module is already running
		if (!modulep->havePuppetModule())
		{
			if (is_plugin_filename)
			{
				success = LLFile::exists(command);
				if (success)
				{
					success = modulep->launchLeapPlugin(command);
				}
			}
			else
			{
				success = modulep->launchLeapCommand(command);
				if (!success && is_saved_cmd)
				{
					// Clear the command, since it is obviously invalid... HB
					gSavedSettings.setString("PuppetryLastCommand", "");
				}
			}
		}
	}

	lua_pushboolean(statep, success);

	return 1;
}

//static
int HBViewerAutomation::agentPuppetryStop(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	if (LLPuppetMotion::enabled())
	{
		LLPuppetModule* modulep = LLPuppetModule::getInstance();
		if (modulep->havePuppetModule())
		{
			modulep->setSending(false);
			modulep->setEcho(false);
			modulep->clearLeapModule();
		}
	}

	return 0;
}
#endif

//static
int HBViewerAutomation::agentRotate(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("AgentRotate");
	}

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	F32 angle = luaL_checknumber(statep, 1);
	if (angle > 360.f)
	{
		angle = fmodf(angle, 360.f);
	}
	else if (angle < 0.f)
	{
		angle = 360.f - fmodf(-angle, 360.f);
	}
	lua_pop(statep, 1);

	gAgent.startCameraAnimation();
	LLVector3 rot(0.f, 1.f, 0.f);
	rot = rot.rotVec(-angle * DEG_TO_RAD, LLVector3::z_axis);
	rot.normalize();
	gAgent.resetAxes(rot);

	return 0;
}

//static
int HBViewerAutomation::getAgentRotation(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 0)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	const LLVector3& at_axis = gAgent.getAtAxis();
	F32 rotation = atan2f(at_axis.mV[VX], at_axis.mV[VY]) * RAD_TO_DEG;
	if (rotation < 0.f)
	{
		rotation += 360.f;
	}
	lua_pushnumber(statep, rotation);

	return 1;
}

//static
int HBViewerAutomation::teleportAgentHome(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("TeleportAgentHome");
	}

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	gAgent.teleportHome();

	return 0;
}

//static
int HBViewerAutomation::teleportAgentToPos(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return 0;

	if (self->isThreaded())
	{
		HBAutomationThread* threadp = (HBAutomationThread*)self;
		return threadp->callMainFunction("TeleportAgentToPos");
	}

	F64 pos_x, pos_y, pos_z;
	bool preserve_look_at;
	S32 n = lua_gettop(statep);
	if ((n == 1 || n == 2) && lua_type(statep, 1) == LUA_TSTRING)
	{
		std::string pos_str(luaL_checkstring(statep, 1));
		LLVector3d global_pos;
		if (!LLVector3d::parseVector3d(pos_str, &global_pos))
		{
			luaL_error(statep, "Invalid position string: %s", pos_str.c_str());
		}
		pos_x = global_pos.mdV[VX];
		pos_y = global_pos.mdV[VY];
		pos_z = global_pos.mdV[VZ];
		preserve_look_at = n == 2 && lua_toboolean(statep, 2);
	}
	else if (n >= 2 && n <= 4)
	{
		pos_x = luaL_checknumber(statep, 1);
		pos_y = luaL_checknumber(statep, 2);
		pos_z = n >= 3 ? luaL_checknumber(statep, 3) : 0.0;
		preserve_look_at = n == 4 && lua_toboolean(statep, 4);
	}
	else
	{
		luaL_error(statep, "%d arguments passed; expected 2 to 4.", n);
		return 0;	// To avoid "<var> may be used uninitialized" gcc warnings
	}
	lua_pop(statep, n);

	if (preserve_look_at)
	{
		gAgent.teleportViaLocationLookAt(LLVector3d(pos_x, pos_y, pos_z));
	}
	else
	{
		gAgent.teleportViaLocation(LLVector3d(pos_x, pos_y, pos_z));
	}

	return 0;
}

//static
void HBViewerAutomation::onIdleSimChange(void* datap)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	HBViewerAutomation* self = (HBViewerAutomation*)datap;
	if (!self || self != gAutomationp || !self->mHasOnFailedTPSimChange ||
		// Is a teleport in progress ?
		gAgent.teleportInProgress())
	{
		return;
	}

	// Are there a failed teleported sim handle and valid TP coordinates ?
	U64 handle = gAgent.getTeleportedSimHandle();
	if (!handle || gAgent.getTeleportedPosGlobal().isExactlyZero())
	{
		return;
	}

	LLSimInfo* siminfo = gWorldMap.simInfoFromHandle(handle);
	if (!siminfo)
	{
		return;
	}

	bool sim_is_down = siminfo->mAccess == SIM_ACCESS_DOWN;
	F64 update_interval = sim_is_down ? 15.0 : 4.0;
	F64 current_time = LLTimer::getElapsedSeconds();
	F64 delta = current_time - siminfo->mAgentsUpdateTime;
	if (delta > update_interval)
	{
		// Time to update our sim info
		siminfo->mAgentsUpdateTime = current_time;
		if (sim_is_down)
		{
			gWorldMap.sendHandleRegionRequest(handle);
		}
		else
		{
			gWorldMap.sendItemRequest(MAP_ITEM_AGENT_LOCATIONS, handle);
		}
	}
	else if (!sim_is_down)
	{
		// Count the number of agents in sim, if that data is available
		LLWorldMap::agent_list_map_t::iterator counts_iter =
			gWorldMap.mAgentLocationsMap.find(handle);
		if (counts_iter != gWorldMap.mAgentLocationsMap.end())
		{
			S32 sim_agent_count = 0;
			LLWorldMap::item_info_list_t& agentcounts = counts_iter->second;
			for (LLWorldMap::item_info_list_t::iterator
					iter = agentcounts.begin(), end = agentcounts.end();
				 iter != end; ++iter)
			{
				sim_agent_count += iter->mExtra;
			}
			// If the number of agents in the sim changed then fire the
			// OnFailedTPSimChange() Lua callback.
			if (sim_agent_count != siminfo->mAgentsCount)
			{
				siminfo->mAgentsCount = sim_agent_count;
				self->onFailedTPSimChange(sim_agent_count);
			}
		}
	}
}

//static
int HBViewerAutomation::callbackAfter(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n < 2)
	{
		luaL_error(statep, "%d arguments passed; expected at least 2.", n);
	}

	F32 delay = llclamp((F64)luaL_checknumber(statep, 1), 1.0, (F64)F32_MAX);

	if (lua_type(statep, 2) != LUA_TFUNCTION)
	{
		luaL_error(statep, "The second argument must be a function");
	}

	// Store the function and the parameters into a table
	lua_newtable(statep);
	for (S32 i = 2; i <= n; ++i)
	{
		lua_pushvalue(statep, i);
		lua_rawseti(statep, -2, i - 1);
	}
	// Store the number of elements in the table
	lua_pushliteral(statep, "n");
	lua_pushinteger(statep, n - 1);
	lua_rawset(statep, -3);

	// Store the table into registry and get the corresponding unique reference
	int ref = luaL_ref(statep, LUA_REGISTRYINDEX);

	lua_settop(statep, 0);

	LL_DEBUGS("Lua") << "Queuing Lua callback with reference: " << ref
					 << " - Number of function arguments: " << n - 2
					 << LL_ENDL;

	doAfterInterval(std::bind(&doAfterIntervalCallback, statep, ref), delay);
	return 0;
}

//static
void HBViewerAutomation::doAfterIntervalCallback(lua_State* statep, int ref)
{
	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	HBViewerAutomation* self = findInstance(statep);
	if (!self) return;

	LL_DEBUGS("Lua") << "Invoking Lua callback associated with reference: "
					 << ref << LL_ENDL;

	// Get our table back from registry
	int type = lua_rawgeti(statep, LUA_REGISTRYINDEX, ref);
	if (type != LUA_TTABLE)
	{
		llwarns << "Bad type (" << lua_typename(statep, type)
				<< ") for object referenced at: " << ref << ". Aborting."
				<< llendl;
		lua_settop(statep, 0);
		return;
	}

	// Get the number of elements in the table
	lua_pushliteral(statep, "n");
	type = lua_rawget(statep, -2);
	if (type != LUA_TNUMBER)
	{
		llwarns << "Bad callback table format ('n' is missing or bears an invalid type). Aborting."
				<< llendl;
		lua_settop(statep, 0);
		return;
	}
	S32 n = lua_tointeger(statep, -1);
	lua_pop(statep, 1);

	// Copy each table element back onto the stack
	LL_DEBUGS("Lua") << "Retrieving the function and " << n - 1
					 << " argument(s)" << LL_ENDL;
	for (S32 i = 1; i <= n; ++i)
	{
		lua_rawgeti(statep, 1, i);
		if (i == 1 && lua_type(statep, -1) != LUA_TFUNCTION)
		{
			llwarns << "Invalid callback table (no function). Aborting."
					<< llendl;
			return;
		}
	}

	// Remove the table
	lua_remove(statep, -n - 1);

	// Dereference the callback data from LUA_REGISTRYINDEX
	luaL_unref(statep, LUA_REGISTRYINDEX, ref);

	LL_DEBUGS("Lua") << "Calling the Lua function with " << n - 1
					 << " argument(s)" << LL_ENDL;
	self->resetTimer();
	if (lua_pcall(statep, n - 1, 0, 0) != LUA_OK)
	{
		self->reportError();
	}
}

static void lua_force_quit()
{
	gAppViewerp->forceQuit();
}

//static
int HBViewerAutomation::forceQuit(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	HBViewerAutomation* self = findInstance(statep);
	if (!self || self != gAutomationp) return 0;

	S32 n = lua_gettop(statep);
	if (n > 1)
	{
		luaL_error(statep, "%d arguments passed; expected 0 or 1.", n);
	}

	S32 exit_code = 0;
	if (n)
	{
		exit_code = luaL_checknumber(statep, 1);
		lua_pop(statep, 1);
	}
	if (exit_code &&
		(exit_code < LLAppViewer::VIEWER_EXIT_CODES || exit_code > 125))
	{
		luaL_error(statep,
				   "Invalid exit code (must be 0 or in the range [%d-125]).",
				   LLAppViewer::VIEWER_EXIT_CODES);
	}
	gExitCode = exit_code;

	LLSD args;
	args["CODE"] = exit_code;
	gNotifications.add("LuaForceQuit", args);
	doAfterInterval(lua_force_quit, 5.f);

	return 0;
}

//static
int HBViewerAutomation::minimizeWindow(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep || !gWindowp) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n)
	{
		luaL_error(statep, "%d arguments passed; expected 0.", n);
	}

	gWindowp->minimize();

	return 0;
}

// Helper function. Returns true on success and updates 'path' to contain the
// actual, full path of the file. Returns false on failure, with the error
// message in 'path'. When 'directory_only' is true, we accept a path without
// a file name and we do not create a subdirectory if specified (this is for
// when we want to deal with sub-directories themselves, including removing
// them).
static bool validate_path(std::string& path, bool directory_only = false)
{
	if (path.empty())
	{
		path = "Empty file name.";
		return false;
	}

	// Remove illegal directory characters and make sure the proper directory
	// delimiter is used for the OS we are running under.
	path = LLDir::getScrubbedDirName(path);

	// This may only be a sub-directory of user_settings/lua_data/
	std::string subdir = LLDir::getDirName(path);

	// Get the filename, and scrub it from any illegal characters.
	std::string filename = LLDir::getBaseFileName(path);
	filename = LLDir::getScrubbedFileName(filename);
	if (!directory_only && filename.empty())
	{
		path = "Empty file name.";
		return false;
	}

	// Make sure user_settings/lua_data/ exists.
	std::string basedir = gDirUtil.getFullPath(LL_PATH_USER_SETTINGS,
											   "lua_data", "");
	basedir.pop_back();	// Remove trailing directory separator
	if (!LLFile::isdir(basedir) && !LLFile::mkdir(basedir))
	{
		path = "Could not create directory: " + basedir;
		return false;
	}
	basedir += LL_DIR_DELIM_CHR;	// Re-add separator

	if (!subdir.empty())
	{
		basedir += subdir;
		// If a sub-directory path was specifed, make sure it exists as well.
		// Note that we do not support creating two or more sub-directory
		// levels at once and LLFile::mkdir() would fail in that case.
		if (!directory_only && !LLFile::isdir(basedir) &&
			!LLFile::mkdir(basedir))
		{
			path = "Could not create directory: " + basedir;
			return false;
		}
		basedir += LL_DIR_DELIM_CHR;
	}

	path = basedir + filename;
	if (directory_only && path.back() == LL_DIR_DELIM_CHR)
	{
		path.pop_back();	// Remove trailing directory separator
	}

	return true;	
}

//static
int HBViewerAutomation::readTextFromFile(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 3)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 3.", n);
	}

	std::string fpath(luaL_checkstring(statep, 1));

	S64 offset = 0;
	if (n > 1)
	{
		offset = luaL_checknumber(statep, 2);
		if (offset < 0)
		{
			luaL_error(statep,
					   "Negative offset passed (%d): cannot read before the start of a file.",
					   offset);
		}
	}

	S64 nbytes = -1;
	if (n > 2)
	{
		 nbytes = luaL_checknumber(statep, 3);
	}

	lua_pop(statep, n);

	if (!validate_path(fpath))
	{
		// Log returned error message.
		llwarns << fpath << llendl;
		lua_pushnil(statep);
		return 1;
	}

	if (!LLFile::exists(fpath))
	{
		llwarns << "No such file:" << fpath << llendl;
		lua_pushnil(statep);
		return 1;
	}

	S64 size;
	LLFile in(fpath, "rb", &size);
	if (!in)
	{
		llwarns << "Cannot open file \"" << fpath << "\" for reading."
				<< llendl;
		lua_pushnil(statep);
		return 1;
	}

	std::string result;
	if (offset >= size)
	{
		// Trying to read past the end of file: accept and return an empty
		// string, but do warn once.
		llwarns_once << "Offset is past the end of file: " << fpath << llendl;
		lua_pushstring(statep, result.c_str());
		return 1;
	}

	S64 bufsize = size;
	if (offset > 0 && in.seek(offset) < 0)
	{
		llwarns << "Could not seek to position " << offset << " in file: "
				<< fpath << llendl;
		lua_pushnil(statep);
		return 1;
	}

	if (nbytes >= 0)
	{
		bufsize = llmin(size - offset, nbytes);
		if (bufsize != nbytes)
		{
			// Trying to read past the end of file: accept and return all we
			// got, but do warn once.
			llwarns_once << "Attempted to read past the end of file: " << fpath
						 << llendl;
		}
	}

	if (bufsize > 0)
	{
		std::vector<U8> buffer(bufsize, 0);
		in.read(buffer.data(), bufsize);
		result.assign((const char*)buffer.data(), bufsize);
	}
	lua_pushstring(statep, result.c_str());
	return 1;
}

//static
int HBViewerAutomation::writeTextToFile(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 2 && n != 3)
	{
		luaL_error(statep, "%d arguments passed; expected 2 or 3.", n);
	}

	std::string fpath(luaL_checkstring(statep, 1));

	std::string text(luaL_checkstring(statep, 2));

	const char* mode = n > 2 && lua_toboolean(statep, 2) ? "ab" : "wb";

	lua_pop(statep, n);

	if (!validate_path(fpath))
	{
		// Log returned error message.
		llwarns << fpath << llendl;
		lua_pushnil(statep);
		return 1;
	}

	bool log_creation = !LLFile::exists(fpath);
	S64 written = 0;
	LLFile out(fpath, mode);
	if (!out)
	{
		llwarns << "Cannot open file \"" << fpath << "\" for writing."
				<< llendl;
		written = -1;
	}
	else if (log_creation)
	{
		llinfos << "Created file: " << fpath << llendl;
	}

	if (written != -1 && !text.empty())
	{
		written = out.write((const U8*)text.c_str(), text.size());
	}
	lua_pushinteger(statep, written);
	return 1;
}

//static
int HBViewerAutomation::getFileSize(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string fpath(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	bool valid = validate_path(fpath, true);
	if (!valid || !LLFile::exists(fpath))
	{
		if (!valid)
		{
			// Log returned error message.
			llwarns << fpath << llendl;
		}
		// No such file/directory
		lua_pushnil(statep);
		return 1;
	}

	S64 size = -1;
	if (!LLFile::isdir(fpath))
	{
		size = LLFile::getFileSize(fpath);
	}
	lua_pushinteger(statep, size);
	return 1;
}

//static
int HBViewerAutomation::removeFile(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string fpath(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	bool success = validate_path(fpath);
	if (success)
	{
		if (LLFile::exists(fpath))
		{
			success = LLFile::remove(fpath);
			if (success)
			{
				llinfos << "Removed file: " << fpath << llendl;
			}
		}
	}
	else
	{
		// Log returned error message.
		llwarns << fpath << llendl;
	}
	lua_pushboolean(statep, success);
	return 1;
}

//static
int HBViewerAutomation::createDirectory(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string fpath(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	bool success = validate_path(fpath, true);
	if (success)
	{
		if (!LLFile::isdir(fpath))
		{
			success = LLFile::mkdir(fpath);
			if (success)
			{
				llinfos << "Created directory: " << fpath << llendl;
			}
		}
	}
	else
	{
		// Log returned error message.
		llwarns << fpath << llendl;
	}
	lua_pushboolean(statep, success);
	return 1;
}

//static
int HBViewerAutomation::removeDirectory(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string fpath(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	bool success = validate_path(fpath, true);
	if (success)
	{
		if (LLFile::exists(fpath))
		{
			success = LLFile::isdir(fpath) && LLFile::rmdir(fpath);
			if (success)
			{
				llinfos << "Removed directory: " << fpath << llendl;
			}
		}
	}
	else
	{
		// Log returned error message.
		llwarns << fpath << llendl;
	}
	lua_pushboolean(statep, success);
	return 1;
}

// Helper function to determine if a Lua table at 'index' is a genuine array
// (numerically indexed from 1 to n, with contiguous indices).
static bool is_lua_array(lua_State* statep, int index)
{
	bool is_array = true;
	int expected_key = 1;
	lua_pushnil(statep);
	while (lua_next(statep, index) != 0)
	{
		int key_type = lua_type(statep, -2);
		if (key_type != LUA_TNUMBER)
		{
			is_array = false;
			lua_pop(statep, 2);	// Pop value and key
			break;
		}
		int key = lua_tointeger(statep, -2);
		if (key != expected_key)
		{
			// Not a contiguous array (this is a sparse array or object).
			is_array = false;
			lua_pop(statep, 2);	// Pop value and key
			break;
		}
		++expected_key;
		lua_pop(statep, 1);		// Pop value. Keep key for lua_next()
	}
	return is_array;
}

// Converts a Lua table into a JSON object
static lljson lua_table_to_json(lua_State* statep, int index)
{
	lljson json;
	if (is_lua_array(statep, index))
	{
		// The Lua table is a genuine array, use JSON array.
		lua_pushnil(statep);
		while (lua_next(statep, index) != 0)
		{
			int type = lua_type(statep, -1);
			switch (type)
			{
				case LUA_TBOOLEAN:
					json.push_back(lua_toboolean(statep, -1) ? true : false);
					break;

				case LUA_TNUMBER:
					if (lua_isinteger(statep, -1))
					{
						json.push_back(lua_tointeger(statep, -1));
					}
					else
					{
						json.push_back(lua_tonumber(statep, -1));
					}
					break;

				case LUA_TSTRING:
					json.push_back(lua_tostring(statep, -1));
					break;

				case LUA_TTABLE:
					json.push_back(lua_table_to_json(statep,
													 lua_gettop(statep)));
					break;

				default:
					if (type != LUA_TNIL)
					{
						llwarns << "Unsupported type: " << type << llendl;
					}
					json.push_back(nullptr);
			}
			lua_pop(statep, 1);
		}
	}
	else
	{
		// The Lua table is an object (key-value pairs) or a sparse array, use
		// JSON object.
		lua_pushnil(statep);
		std::string key;
		while (lua_next(statep, index) != 0)
		{
			// Get the key as a string for the next JSON key/value pair.
			// IMPORTANT: we cannot rely on lua_tostring() to convert a Lua
			// number or boolean on the stack here, because, as per the Lua
			// manual: "While traversing a table, avoid calling lua_tolstring
			// directly on a key, unless you know that the key is actually a
			// string. Recall that lua_tolstring may change the value at the
			// given index; this confuses the next call to lua_next."
			int type = lua_type(statep, -2);		// Type of the key
			switch (type)
			{
				case LUA_TBOOLEAN:
					// Strange key, but why not ?...
					key = lua_toboolean(statep, -2) ? "true" : "false";
					break;

				case LUA_TNUMBER:
					if (lua_isinteger(statep, -2))
					{
						key = std::to_string(lua_tointeger(statep, -2));
					}
					else
					{
						key = std::to_string(lua_tonumber(statep, -2));
					}
					break;

				case LUA_TSTRING:
					key = lua_tostring(statep, -2);
					break;

				default:
					llwarns << "Invalid key type: " << type
							<< " - Using 'null'." << llendl;
					key = "null";
			}

			// Handle value based on its type
			type = lua_type(statep, -1);			// type of the value
			switch (type)
			{
				case LUA_TBOOLEAN:
					json[key] = lua_toboolean(statep, -1) ? true : false;
					break;

				case LUA_TNUMBER:
					if (lua_isinteger(statep, -1))
					{
						json[key] = lua_tointeger(statep, -1);
					}
					else
					{
						json[key] = lua_tonumber(statep, -1);
					}
					break;

				case LUA_TSTRING:
					json[key] = lua_tostring(statep, -1);
					break;

				case LUA_TTABLE:
					json[key] = lua_table_to_json(statep, lua_gettop(statep));
					break;

				default:
					if (type != LUA_TNIL)
					{
						llwarns << "Unsupported type: " << type << llendl;
					}
					json[key] = nullptr;
			}
			lua_pop(statep, 1);	// Pop the value, keep the key for lua_next()
		}
	}
	return json;
}

//static
int HBViewerAutomation::encodeJSON(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	try
	{
		std::string json_str = lua_table_to_json(statep, 1).dump();
		lua_settop(statep, 0);
		lua_pushstring(statep, json_str.c_str());
	}
	catch (const lljson::exception& e)
	{
		llwarns << "Encountered nlohmann::json error: " << e.what() << llendl;
		lua_settop(statep, 0);
		lua_pushnil(statep);
	}
	return 1;
}

// Converts a JSON object into a Lua table
static void json_to_lua_table(lua_State* statep, const lljson& json)
{
	if (json.is_object())
	{
		lua_newtable(statep);
		// Iterate over the JSON object key-value pairs
		for (const auto& [key, value] : json.items())
		{
			if (key.find_first_not_of("0123456789") == std::string::npos)
			{
				// It is a numeric key, treat it as an array
				lua_pushinteger(statep, atoi(key.c_str()));
			}
			else
			{
				// It is a string key, treat as an object
				lua_pushstring(statep, key.c_str());
			}
			// Recursively process the value
			json_to_lua_table(statep, value);
			// Store the value at the key
			lua_settable(statep, -3);
		}
	}
	else if (json.is_array())
	{
		lua_newtable(statep);
		int idx = 1;
		for (const auto& element : json)
		{
			// Recursively process the element
			json_to_lua_table(statep, element);
			lua_rawseti(statep, -2, idx++);
		}
	}
	else if (json.is_string())
	{
		// Avoid a string copy.
		const auto& str_val = json.template get_ref<const lljson::string_t&>();
		lua_pushstring(statep, str_val.c_str());
	}
	else if (json.is_boolean())
	{
		lua_pushboolean(statep, json.get<bool>());
	}
	else if (json.is_number_integer())
	{
		lua_pushinteger(statep, json.get<int>());
	}
	else if (json.is_number())
	{
		lua_pushnumber(statep, json.get<double>());
	}
	else
	{
		if (!json.is_null())
		{
			llwarns << "Unsupported type: " << json.type_name() << llendl;
		}
		lua_pushnil(statep);
	}
}

//static
int HBViewerAutomation::decodeJSON(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}

	std::string json_str(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);

	// *HACK: to solve JSON map-like objects not encoded as objects but
	// as arrays. Seen in KoboldCPP JSON replies, for example. HB
	LLStringUtil::replaceString(json_str, "[{", "{");
	LLStringUtil::replaceString(json_str, "}]", "}");

	try
	{
		lljson json = lljson::parse(json_str);
		json_to_lua_table(statep, json);
	}
	catch (const lljson::exception& e)
	{
		llwarns << "Encountered nlohmann::json error: " << e.what() << llendl;
		lua_pushnil(statep);
	}

	return 1;
}

//static
int HBViewerAutomation::encodeBase64(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	std::string data(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);
	std::string base64 = LLBase64::encode(data);
	lua_pushstring(statep, base64.c_str());
	return 1;
}

//static
int HBViewerAutomation::decodeBase64(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n != 1)
	{
		luaL_error(statep, "%d arguments passed; expected 1.", n);
	}
	std::string base64(luaL_checkstring(statep, 1));
	lua_pop(statep, 1);
	std::string data = LLBase64::decode(base64.c_str());
	lua_pushstring(statep, data.c_str());
	return 1;
}

//static
void HBViewerAutomation::getHTTPCoro(lua_State* statep, U32 handle,
									 std::string uri, U32 timeout,
									 const std::string& accept)
{
	LLCore::HttpOptions::ptr_t optionsp(new LLCore::HttpOptions);
	if (timeout)
	{
		optionsp->setTimeout(timeout);
		optionsp->setTransferTimeout(timeout);
	}

	LLCore::HttpHeaders::ptr_t headersp(new LLCore::HttpHeaders);
	// This will cause the Accept and Content-Type header fields to be
	// initialized with proper values for a POST to a "normal" HTTP server
	// (alwo works for a GET, even if Content-Type is then redundant).
	headersp->setStandardHeader();
	if (!accept.empty())
	{
		headersp->append(HTTP_OUT_HEADER_ACCEPT, accept);
	}

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("getHTTPCoro");
	LLSD result = adapter.getRawAndSuspend(uri, optionsp, headersp);

	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	HBViewerAutomation* self = findInstance(statep);
	if (!self)
	{
		LL_DEBUGS("Lua") << "Lua instance gone. Cannot process the HTTP reply from: "
						 << uri << LL_ENDL;
		return;
	}
	if (!self->mHasOnHTTPReply)
	{
		llwarns << "No OnHTTPReply() callback to process the HTTP reply from: "
				<< uri << llendl;
		return;
	}

	bool success = true;
	std::string reply;
	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (status)
	{
		const LLSD::Binary& raw =
			result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
		size_t size = raw.size();
		if (size > 0)
		{
			reply.assign((const char*)raw.data(), size);
		}
	}
	else
	{
		success = false;
		reply = status.toString();
	}

	LL_DEBUGS("Lua") << "Received HTTP reply from: " << uri << LL_ENDL;

	lua_getglobal(statep, "OnHTTPReply");
	lua_pushinteger(statep, handle);
	lua_pushboolean(statep, success);
	lua_pushstring(statep, reply.c_str());
	self->resetTimer();
	if (lua_pcall(statep, 3, 0, 0) != LUA_OK)
	{
		self->reportError();
	}
}

//static
int HBViewerAutomation::getHTTP(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n < 1 || n > 3)
	{
		luaL_error(statep, "%d arguments passed; expected 1 to 3.", n);
	}
	std::string uri(lua_tostring(statep, 1));
	std::string accept;
	U32 timeout = 0;
	if (n > 1)
	{
		timeout = llmax(0, lua_tointeger(statep, 2));
	}
	if (n > 2)
	{
		accept = lua_tostring(statep, 3);
	}
	lua_pop(statep, n);

	U32 handle = ++sCurrentHTTPHandle;
	gCoros.launch("HBViewerAutomation::getHTTPCoro",
			   	  std::bind(&HBViewerAutomation::getHTTPCoro, statep, handle,
							uri, timeout, accept));
	lua_pushinteger(statep, handle);

	return 1;
}

//static
void HBViewerAutomation::postHTTPCoro(lua_State* statep, U32 handle,
									  std::string uri,
									  const std::string& data, U32 timeout,
									  const std::string& accept,
									  const std::string& content_type)
{
	LLCore::HttpOptions::ptr_t optionsp(new LLCore::HttpOptions);
	if (timeout)
	{
		optionsp->setTimeout(timeout);
		optionsp->setTransferTimeout(timeout);
	}

	LLCore::HttpHeaders::ptr_t headersp(new LLCore::HttpHeaders);
	// This will cause the Accept and Content-Type header fields to be
	// initialized with proper values for a POST to a "normal" HTTP server.
	headersp->setStandardHeader();
	if (!accept.empty())
	{
		headersp->append(HTTP_OUT_HEADER_ACCEPT, accept);
	}
	if (!content_type.empty())
	{
		headersp->append(HTTP_OUT_HEADER_CONTENT_TYPE, content_type);
	}

	LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);
	LLCore::BufferArrayStream bas(rawbody.get());
	bas << std::noskipws << data;

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("posttHTTPCoro");
	LLSD result = adapter.postRawAndSuspend(uri, rawbody, optionsp, headersp);

	LL_TRACY_TIMER(TRC_LUA_CALLBACK);

	HBViewerAutomation* self = findInstance(statep);
	if (!self)
	{
		LL_DEBUGS("Lua") << "Lua instance gone. Cannot process the HTTP reply from: "
						 << uri << LL_ENDL;
		return;
	}
	if (!self->mHasOnHTTPReply)
	{
		llwarns << "No OnHTTPReply() callback to process the HTTP reply from: "
				<< uri << llendl;
		return;
	}

	bool success = true;
	std::string reply;
	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (status)
	{
		const LLSD::Binary& raw =
			result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
		size_t size = raw.size();
		if (size > 0)
		{
			reply.assign((const char*)raw.data(), size);
		}
	}
	else
	{
		success = false;
		reply = status.toString();
	}

	LL_DEBUGS("Lua") << "Received HTTP reply from: " << uri << LL_ENDL;

	lua_getglobal(statep, "OnHTTPReply");
	lua_pushinteger(statep, handle);
	lua_pushboolean(statep, success);
	lua_pushstring(statep, reply.c_str());
	self->resetTimer();
	if (lua_pcall(statep, 3, 0, 0) != LUA_OK)
	{
		self->reportError();
	}
}

//static
int HBViewerAutomation::postHTTP(lua_State* statep)
{
	LL_TRACY_TIMER(TRC_LUA_FUNCTION);

	if (!statep) return 0;	// Paranoia

	S32 n = lua_gettop(statep);
	if (n < 2 || n > 5)
	{
		luaL_error(statep, "%d arguments passed; expected 2 to 5.", n);
	}
	std::string uri(lua_tostring(statep, 1));
	std::string data(lua_tostring(statep, 2));
	std::string accept, content_type;
	U32 timeout = 0;
	if (n > 2)
	{
		timeout = llmax(0, lua_tointeger(statep, 3));
	}
	if (n > 3)
	{
		accept = lua_tostring(statep, 4);
	}
	if (n > 4)
	{
		content_type = lua_tostring(statep, 5);
	}
	lua_pop(statep, n);

	U32 handle = ++sCurrentHTTPHandle;
	gCoros.launch("HBViewerAutomation::postHTTPCoro",
			   	  std::bind(&HBViewerAutomation::postHTTPCoro, statep, handle,
							uri, data, timeout, accept, content_type));
	lua_pushinteger(statep, handle);

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// HBLuaSideBar class
///////////////////////////////////////////////////////////////////////////////

// This is the maximum number of buttons in the Lua side bar. The buttons must
// be named "btnN" with N=1 to 20 and appear in a sequence without hole in the
// numbering.
constexpr U32 MAX_NUMBER_OF_BUTTONS = 20;

HBLuaSideBar* gLuaSideBarp = NULL;

HBLuaSideBar::HBLuaSideBar()
:	LLPanel("lua side bar", LLRect(), BORDER_NO),
	mNumberOfButtons(0),
	mLeftSide(false),
	mHidden(false),
	mHideOnRightClick(false)
{
	llassert_always(gLuaSideBarp == NULL);	// Only one instance allowed

	LLUICtrlFactory::getInstance()->buildPanel(this,
											   "panel_lua_sidebar.xml");

	LLControlVariable* ctrlp = gSavedSettings.getControl("LuaSideBarOnLeft");
	if (ctrlp)
	{
		ctrlp->getSignal()->connect(std::bind(&HBLuaSideBar::handleSideChanged,
											  _2));
		mLeftSide = ctrlp->getValue().asBoolean();
	}
	if (!mLeftSide)
	{
		setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
	}

	setMouseOpaque(false);
	setIsChrome(true);
	setFocusRoot(true);
	setShape();

	std::string name;
	mCommands.resize(MAX_NUMBER_OF_BUTTONS);
	for (U32 i = 1; i <= MAX_NUMBER_OF_BUTTONS; ++i)
	{
		name = llformat("btn%d", i);
		LLButton* button = getChild<LLButton>(name.c_str(), true, false);
		if (!button) break;

		++mNumberOfButtons;
		mCommands.emplace_back("");
		button->setClickedCallback(onButtonClicked, (void*)(intptr_t)i);
		button->setVisible(false);
		button->setImageDisabled("square_button_disabled.tga");
		button->setImageUnselected("square_button_enabled.tga");
		button->setImageSelected("square_button_selected.tga");
	}
	LL_DEBUGS("Lua") << "Found " << mNumberOfButtons << " in the side bar"
					 << LL_ENDL;

	gLuaSideBarp = this;
}

//virtual
HBLuaSideBar::~HBLuaSideBar()
{
	gLuaSideBarp = NULL;
}

//virtual
void HBLuaSideBar::draw()
{
	if (!mActiveButtons.empty() && LLStartUp::isLoggedIn())
	{
		LLPanel::draw();
	}
}

//virtual
void HBLuaSideBar::reshape(S32 width, S32 height, bool called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);
	setShape();
}

//virtual
void HBLuaSideBar::setVisible(bool visible)
{
	LLPanel::setVisible(visible && !mHidden);
}

//virtual
bool HBLuaSideBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (mHideOnRightClick)
	{
		setHidden(true);
		return true;
	}

	return LLPanel::handleRightMouseDown(x, y, mask);
}

void HBLuaSideBar::setShape()
{
	if (gViewerWindowp)
	{
		LLRect rect = getRect();
		S32 height = rect.getHeight();
		S32 width = rect.getWidth();
		rect.mBottom = CHAT_BAR_HEIGHT +
					   (gViewerWindowp->getWindowHeight() - height) / 2;
		rect.mTop = rect.mBottom + height;
		if (mLeftSide)
		{
			rect.mLeft = 1;
			rect.mRight = rect.mLeft + width;
		}
		else
		{
			rect.mRight = gViewerWindowp->getWindowWidth() - 1;
			rect.mLeft = rect.mRight - width;
		}
		setRect(rect);
		updateBoundingRect();
	}
}

void HBLuaSideBar::setHidden(bool hidden)
{
	mHidden = hidden;
	LLPanel::setVisible(!hidden && !gAgent.cameraMouselook());
	if (gAutomationp)
	{
		gAutomationp->onSideBarVisibilityChange(!hidden);
	}
}

U32 HBLuaSideBar::setButton(U32 number, std::string icon, std::string command,
							const std::string& tooltip)
{
	if (number > mNumberOfButtons)
	{
		llwarns << "Invalid button number: " << number
				<< ". Valid range is 1 to " << mNumberOfButtons
				<< ", inclusive (and 0 for auto slot affectation)." << llendl;
		return 0;
	}
	if (!number)
	{
		// Find the first empty button slot, if any.
		for (U32 i = 1; i <= mNumberOfButtons; ++i)
		{
			if (!mActiveButtons.count(i))
			{
				number = i;
				break;
			}
		}
		if (!number)
		{
			llwarns << "No free button slot left: all "
				<< mNumberOfButtons << " are in use in the side bar."
				<< llendl;
			return 0;
		}
	}

	std::string name = llformat("btn%d", number);
	LLButton* buttonp = getChild<LLButton>(name.c_str(), true, false);
	if (!buttonp) return 0;

	if (command.empty() && !mCommands[number - 1].empty())
	{
		command = mCommands[number - 1];
	}
	else
	{
		// Reset any existing debug setting control and toggle state
		buttonp->setControlName("", NULL);
		buttonp->setToggleState(false);
		buttonp->setIsToggle(false);
	}

	bool visible = !icon.empty() && !command.empty();
	if (visible)
	{
		// If first character is an UTF-8 one, or there are only 1 or 2 ASCII
		// characters, interpret the icon name as a text label.
		if ((U8)icon[0] > 127 || icon.length() < 3)
		{
			buttonp->setLabel(icon);
			buttonp->setImageOverlay(LLUIImagePtr(NULL));
		}
		else
		{
			LLFontGL::HAlign alignment = LLFontGL::HCENTER;
			size_t i = icon.find('|');
			if (i != std::string::npos && i < icon.length() - 1)
			{
				std::string align_str = icon.substr(0, i);
				icon = icon.substr(i + 1);
				if (align_str == "left")
				{
					alignment = LLFontGL::LEFT;
				}
				else if (align_str == "right")
				{
					alignment = LLFontGL::RIGHT;
				}
			}
			LLUIImagePtr image = LLUI::getUIImage(icon);
			if (image.notNull())
			{
				buttonp->setLabel(LLStringUtil::null);
				buttonp->setImageOverlay(image, alignment);
			}
		}
		mActiveButtons.insert(number);
		mCommands[number - 1] = command;
		buttonp->setToolTip(tooltip);
	}
	else
	{
		mActiveButtons.erase(number);
		mCommands[number - 1].clear();
		buttonp->setToolTip(LLStringUtil::null);
	}
	buttonp->setVisible(visible);
	buttonp->setEnabled(visible);
	LL_DEBUGS("Lua") << (visible ? "Set" : "Reset") << " button " << number
					 << LL_ENDL;
	return number;
}

S32 HBLuaSideBar::buttonToggle(U32 number, S32 toggle)
{
	if (number == 0 || number > mNumberOfButtons)
	{
		llwarns << "Invalid button number: " << number
				<< ". Valid range is 1 to " << mNumberOfButtons
				<< ", inclusive." << llendl;
		return -1;
	}

	std::string name = llformat("btn%d", number);
	LLButton* buttonp = getChild<LLButton>(name.c_str(), true, false);
	if (!buttonp || mCommands[number - 1].empty())
	{
		return -1;
	}

	S32 result = toggle;
	switch (toggle)
	{
		case 0:
		case 1:
			buttonp->setIsToggle(true);
			buttonp->setToggleState(toggle == 1);
			break;

		default:
			result = buttonp->getIsToggle() ? buttonp->getToggleState() : -1;
	}

	return result;
}

void HBLuaSideBar::buttonSetControl(U32 number, LLControlVariable* controlp)
{
	if (number == 0 || number > mNumberOfButtons)
	{
		llwarns << "Invalid button number: " << number
				<< ". Valid range is 1 to " << mNumberOfButtons
				<< ", inclusive." << llendl;
		return;
	}

	std::string name = llformat("btn%d", number);
	LLButton* buttonp = getChild<LLButton>(name.c_str(), true, false);
	if (buttonp && !mCommands[number - 1].empty())
	{
		// Avoid changing the control debug setting value
		if (controlp)
		{
			buttonp->setIsToggle(true);
			buttonp->setToggleState(controlp->getValue().asBoolean());
			buttonp->setControlName(controlp->getName().c_str(), NULL);
		}
		else
		{
			buttonp->setControlName(NULL, NULL);
			buttonp->setIsToggle(false);
			buttonp->setToggleState(false);
		}
	}
}

void HBLuaSideBar::setButtonEnabled(U32 number, bool enabled)
{
	if (number == 0 || number > mNumberOfButtons)
	{
		llwarns << "Invalid button number: " << number
				<< ". Valid range is 1 to " << mNumberOfButtons
				<< ", inclusive." << llendl;
		return;
	}

	std::string name = llformat("btn%d", number);
	LLButton* buttonp = getChild<LLButton>(name.c_str(), true, false);
	if (buttonp && !mCommands[number - 1].empty())
	{
		buttonp->setEnabled(enabled);
	}
}

void HBLuaSideBar::setButtonVisible(U32 number, bool visible)
{
	if (number == 0 || number > mNumberOfButtons)
	{
		llwarns << "Invalid button number: " << number
				<< ". Valid range is 1 to " << mNumberOfButtons
				<< ", inclusive." << llendl;
		return;
	}

	std::string name = llformat("btn%d", number);
	LLButton* buttonp = getChild<LLButton>(name.c_str(), true, false);
	if (buttonp && !mCommands[number - 1].empty())
	{
		buttonp->setVisible(visible);
	}
}

void HBLuaSideBar::removeAllButtons()
{
	std::string name;
	for (U32 i = 1; i <= mNumberOfButtons; ++i)
	{
		name = llformat("btn%d", i);
		LLButton* buttonp = getChild<LLButton>(name.c_str(), true, false);
		if (buttonp)
		{
			mCommands[i - 1].clear();
			buttonp->setEnabled(false);
			buttonp->setVisible(false);
			buttonp->setControlName("", NULL);
			buttonp->setToggleState(false);
			buttonp->setIsToggle(false);
		}
	}
	mActiveButtons.clear();
}

//static
bool HBLuaSideBar::handleSideChanged(const LLSD& newvalue)
{
	if (gLuaSideBarp)
	{
		gLuaSideBarp->mLeftSide = newvalue.asBoolean();
		if (gLuaSideBarp->mLeftSide)
		{
			gLuaSideBarp->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
		}
		else
		{
			gLuaSideBarp->setFollows(FOLLOWS_TOP | FOLLOWS_RIGHT);
		}
		gLuaSideBarp->setShape();
	}

	return true;
}

//static
void HBLuaSideBar::onButtonClicked(void* user_data)
{
	U32 button = (U32)(intptr_t)user_data;
	if (gLuaSideBarp && button > 0 && button <= gLuaSideBarp->mNumberOfButtons)
	{
		const std::string& command = gLuaSideBarp->mCommands[button - 1];
		if (!command.empty() && command != "nop")
		{
			LL_DEBUGS("Lua") << "Executing command associated with button "
							 << button << LL_ENDL;
			HBViewerAutomation::eval(command);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// HBLuaPieMenu class
///////////////////////////////////////////////////////////////////////////////

HBLuaPieMenu* gLuaPiep = NULL;

HBLuaPieMenu::HBLuaPieMenu()
:	LLPieMenu("Lua pie menu"),
	mLastPickType(0)
{
	llassert_always(gLuaPiep == NULL);	// Only one instance allowed
	if (!gMenuHolderp)
	{
		llwarns << "Menu holder is NULL !  Aborted." << llendl;
		return;
	}

	const std::string filename = "menu_pie_lua.xml";
	LLXMLNodePtr root;
	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return;
	}
	if (!root->hasName(LL_PIE_MENU_TAG))
	{
		llwarns << "Root node should be named " << LL_PIE_MENU_TAG << " in: "
				<< filename  << ". Aborted." << llendl;
		return;
	}

	gMenuHolderp->addChild(this);
	initXML(root, gMenuHolderp, LLUICtrlFactory::getInstance());

	if (LLUI::sShowXUINames)
	{
		setToolTip(filename);
	}

	mLabels.reserve(48);
	mCommands.reserve(48);
	for (S32 i = 0; i < 48; ++i)
	{
		mLabels.emplace_back("");
		mCommands.emplace_back("");
	}

	gLuaPiep = this;
}

//virtual
HBLuaPieMenu::~HBLuaPieMenu()
{
	gLuaPiep = NULL;
}

void HBLuaPieMenu::removeAllSlices()
{
	for (S32 i = 0; i < 48; ++i)
	{
		mLabels[i].clear();
		mCommands[i].clear();
	}
}

// Here, we duplicate the same logic for pie menu types selection as found in
// LLToolPie::handleRightClickPick()
S32 HBLuaPieMenu::getPickedType(const LLPickInfo& pick, LLViewerObject* objp)
{
	S32 type = PICKED_INVALID;
	if ((!objp || !objp->isHUDAttachment()) &&
		pick.mPickParticle && pick.mParticleOwnerID.notNull())
	{
		type = PICKED_PARTICLE;
	}
	else if (pick.mPickType == LLPickInfo::PICK_LAND)
	{
		type = PICKED_LAND;
	}
	else if (pick.mObjectID == gAgentID)
	{
		type = PICKED_SELF;
	}
	else if (objp)
	{
		if (objp->isAvatar())
		{
			type = PICKED_AVATAR;
		}
		else if (objp->isAttachment())
		{
			type = PICKED_ATTACHMENT;
		}
		else
		{
			type = PICKED_OBJECT;
		}
	}
	return type;
}

S32 HBLuaPieMenu::getPickedType(const LLPickInfo& pick)
{
	const LLUUID& object_id = pick.mObjectID;
	if (!mLastPickType || mLastPickId.isNull() || mLastPickId != object_id)
	{
		mLastPickId = object_id;
		LLViewerObject* objp = gObjectList.findObject(object_id);
		if (objp && objp->isAttachment() && !objp->isHUDAttachment() &&
			!objp->permYouOwner())
		{
			// Find the avatar corresponding to any attachment object we do not
			// own
			while (objp->isAttachment())
			{
				objp = (LLViewerObject*)objp->getParent();
				if (!objp) return PICKED_INVALID;	// Orphaned object ?
			}
		}
		mLastPickType = getPickedType(pick, objp);
	}

	return mLastPickType;
}

bool HBLuaPieMenu::onPieMenu(const LLPickInfo& pick, LLViewerObject* objp)
{
	mLastPickId = pick.mObjectID;

	mLastPickType = getPickedType(pick, objp);
	if (mLastPickType == PICKED_INVALID)
	{
		return false;
	}

	LL_DEBUGS("Lua") << "Considering Lua pie menu type " << mLastPickType
					 << " for object " << mLastPickId << LL_ENDL;

	if (objp && mLastPickType >= PICKED_OBJECT)
	{
//MK
		if (gRLenabled && !objp->isAvatar() && LLFloaterTools::isVisible() &&
			!gRLInterface.canEdit(objp))
		{
			gFloaterToolsp->close();
		}
//mk
		gMenuHolderp->setObjectSelection(gSelectMgr.getSelection());
	}

	bool got_slice = false;

	if (mLastPickType)
	{
		// Setup the pie slices, if any, according to the pick type
		std::string name;
		for (S32 i = 0; i < 8; ++i)
		{
			S32 j = 8 * mLastPickType + i;
			const std::string& label = mLabels[j];

			bool enabled = !label.empty() && !mCommands[j].empty();
			if (enabled)
			{
				got_slice = true;
			}

			name = llformat("slice%d", i + 1);
			LLMenuItemGL* item = getChild<LLMenuItemGL>(name.c_str(), true,
														false);
			if (item)
			{
				item->setValue(label);
				item->setEnabled(enabled);
			}
			else
			{
				llwarns_once << "Malformed menu_pie_lua.xml file" << llendl;
			}
		}
	}

	return got_slice;
}

void HBLuaPieMenu::onPieSliceClick(U32 slice, const LLPickInfo& pick)
{
	if (slice < 1 || slice > 8) return;

	S32 type = getPickedType(pick);
	if (type == PICKED_INVALID) return;

	S32 i = 8 * type + slice - 1;
	const std::string& command = mCommands[i];
	if (!command.empty() && command != "nop")
	{
		LL_DEBUGS("Lua") << "Executing command associated with pie slice "
						 << slice << " for pick type " << type << LL_ENDL;
		// Setup a pie menu specific Lua global variable
		std::string functions = "V_PIE_OBJ_ID=\"" + mLastPickId.asString() +
								"\";";
		// Setup a pie menu specific Lua function using the global variable
		functions += "function GetPickedObjectID();return V_PIE_OBJ_ID;end;";
		HBViewerAutomation::eval(functions + command);
	}

	if (gAutomationp)
	{
		gAutomationp->onLuaPieMenu(slice, mLastPickType, pick);
	}
}

void HBLuaPieMenu::setSlice(S32 type, U32 slice, const std::string& label,
							const std::string& command)
{
	if (type < 0 || type >= PICKED_INVALID)
	{
		llwarns << "Invalid type value: " << type << ". Valid range is "
				<< PICKED_LAND << " to " << PICKED_INVALID - 1
				<< ", inclusive." << llendl;
		return;
	}

	if (!slice)
	{
		LL_DEBUGS("Lua") << "Resetting pie type " << type << LL_ENDL;
		for (S32 i = 8 * type; i < 8 * type + 8; ++i)
		{
			mLabels[i].clear();
			mCommands[i].clear();
		}
		return;
	}

	if (slice > 8)
	{
		llwarns << "Invalid slice number: " << slice
				<< ". Valid range is 0 to 8, inclusive." << llendl;
		return;
	}

	S32 i = 8 * type + slice - 1;
	if (label.empty())
	{
		mLabels[i].clear();
		mCommands[i].clear();
		LL_DEBUGS("Lua") << "Reset slice " << slice << " for pie type " << type
						 << LL_ENDL;
	}
	else
	{
		mLabels[i] = label;
		if (!command.empty())
		{
			mCommands[i] = command;
		}
		LL_DEBUGS("Lua") << "Set slice " << slice << " for pie type " << type
						 << LL_ENDL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// HBLuaConsole class (floater to execute Lua code and print its output)
///////////////////////////////////////////////////////////////////////////////

//static
std::vector<std::string> HBLuaConsole::sHistoryStorage;

constexpr U32 SIZE_FUDGE = 3;	// *HACK: to account for lazy resizing...

HBLuaConsole::HBLuaConsole(const LLSD&)
:	mHistoryIndex(sHistoryStorage.size()),
	mSizeDelta(0),
	mResizingAttempts(0)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_lua_console.xml");

	// If the user did not yet change the floater position, center the latter.
	LLControlVariable* controlp =
		gSavedSettings.getControl(getRectControl().c_str());
	if (!controlp || controlp->isDefault())
	{
		center();
	}

	// See if we need to resize the input layout panel.
	S32 height = gSavedSettings.getU32("LuaConsoleInputHeight");
	if (height)
	{
		height = llmax(mInputLayout->getMinHeight(), height);
		if (abs(mInputLayoutRect.getHeight() - height) > 4)
		{
			mResizingAttempts = 50;
		}
	}
}

//virtual
HBLuaConsole::~HBLuaConsole()
{
	const LLRect& rect = mInputLayout->getRect();
	if (rect != mInputLayoutRect)
	{
		// Not applicable any more: this panel has since been resized by the
		// user !
		mSizeDelta = 0;
	}
	U32 height = llmax(mInputLayout->getMinHeight(),
					   rect.getHeight() + mSizeDelta);
	gSavedSettings.setU32("LuaConsoleInputHeight", height);
}

//virtual
bool HBLuaConsole::postBuild()
{
	mInputLayout = (LLLayoutStack*)getChild<LLView>("layout_input");

	mOutputText = getChild<LLTextEditor>("output_text");

	mInputCode = getChild<LLTextEditor>("input_text");
	mInputCode->setHandleEditKeysDirectly(true);
	mInputCode->makePristine();

	mClearOutput = getChild<LLButton>("clear_output_btn");
	mClearOutput->setCommitCallback(onClearButton);
	mClearOutput->setCallbackUserData(this);

	mClearInput = getChild<LLButton>("clear_input_btn");
	mClearInput->setCommitCallback(onClearButton);
	mClearInput->setCallbackUserData(this);

	mLoadCode = getChild<LLButton>("load_btn");
	mLoadCode->setCommitCallback(onLoadButton);
	mLoadCode->setCallbackUserData(this);

	mBackward = getChild<LLButton>("previous_btn");
	mBackward->setCommitCallback(onBrowseButton);
	mBackward->setCallbackUserData(this);

	mForward = getChild<LLButton>("next_btn");
	mForward->setCommitCallback(onBrowseButton);
	mForward->setCallbackUserData(this);

	mExecute = getChild<LLButton>("execute_btn");
	mExecute->setCommitCallback(onExecuteButton);
	mExecute->setCallbackUserData(this);

	LLButton* buttonp = getChild<LLButton>("help_btn");
	buttonp->setCommitCallback(onHelpButton);
	buttonp->setCallbackUserData(this);

	buttonp = getChild<LLButton>("list_btn");
	buttonp->setCommitCallback(onListButton);
	buttonp->setCallbackUserData(this);

	return true;
}

//virtual
void HBLuaConsole::draw()
{
	// *HACK: for some reason, we need to issue two endOfDoc()'s at one frame
	// interval to actually get this damned thing to scroll down to the end.
	// *FIXME: find out why and fix the underlying UI bug.
	static bool scroll_again = false;
	if (scroll_again)
	{
		scroll_again = false;
		mOutputText->endOfDoc();
	}
	if (!mTextBuffer.empty())
	{
		bool prepend_newline = mOutputText->getLength() > 0;
		mOutputText->appendText(mTextBuffer, false, prepend_newline);
		mOutputText->setCursorAndScrollToEnd();	// Issues a 1st endOfDoc()
		mTextBuffer.clear();
		scroll_again = true;
	}

	mClearOutput->setEnabled(mOutputText->getLength() > 0);
	bool has_code = mInputCode->getLength() > 0;
	mClearInput->setEnabled(has_code);
	mExecute->setEnabled(has_code);
	mLoadCode->setEnabled(!HBFileSelector::isInUse());
	size_t history_size = sHistoryStorage.size();
	mBackward->setEnabled(history_size && mHistoryIndex > 0);
	mForward->setEnabled(mHistoryIndex + 1 < history_size);

	LLFloater::draw();

	// *HACK: due to how layout stacks are resized progressively at each frame,
	// we need to keep asking for a resize for as long as the height of our
	// layout panel has not reached the saved height (or close enough to it);
	// sadly, even with this method, the requested size is not always reached
	// (likely because of rounding bugs in the layout stack code), so we also
	// limit the number of attempts to avoid retrying forever and counter-
	//acting possible manual resizing by the user...
	if (mResizingAttempts)
	{
		static LLCachedControl<U32> height(gSavedSettings,
										   "LuaConsoleInputHeight");
		LLRect orig_rect = mInputLayout->getRect();
		LLRect scaled_rect = orig_rect;
		scaled_rect.mTop = scaled_rect.mBottom + height + SIZE_FUDGE;
		mInputLayout->setRect(scaled_rect);
		mInputLayout->setRect(orig_rect);
		mInputLayout->reshape(scaled_rect.getWidth(), scaled_rect.getHeight(),
							  false);
		LLFloater::draw();	// Propagate the changes.
		// Remember this delta to adjust the saved size on floater closing.
		mSizeDelta = (S32)height - mInputLayout->getRect().getHeight();
		// Remember the rect we reached for use in the destructor, to verify
		// that the above delta is still valid.
		mInputLayoutRect = mInputLayout->getRect();
		if (abs(mSizeDelta) <= SIZE_FUDGE)
		{
			mResizingAttempts = 0;
		}
		else
		{
			--mResizingAttempts;
		}
	}
}

//virtual
void HBLuaConsole::onFocusReceived()
{
	// Focus our input text editor
	mInputCode->setFocus(true);
	// Short-circuit LLUICtrl::onFocusReceived()
	LLFocusableElement::onFocusReceived();
}

//virtual
bool HBLuaConsole::handleKeyHere(KEY key, MASK mask)
{
	if (mask == MASK_CONTROL)
	{
		switch (key)
		{
			case KEY_RETURN:
				onExecuteButton(mExecute, (void*)this);
				return true;

			case KEY_UP:
				onBrowseButton(mBackward, (void*)this);
				return true;

			case KEY_DOWN:
				onBrowseButton(mForward, (void*)this);
				return true;

			default:
				break;
		}
	}
	return LLFloater::handleKeyHere(key, mask);
}

void HBLuaConsole::loadFile(const std::string& filename)
{
	if (!filename.empty())
	{
		llifstream file(filename.c_str());
		if (file.is_open())
		{
			mInputCode->clear();
			std::string line, text;
			while (!file.eof())
			{
				getline(file, line);
				text += line + "\n";
			}
			file.close();
			LLWString wtext = utf8str_to_wstring(text);
			LLWStringUtil::replaceTabsWithSpaces(wtext, 4);
			text = wstring_to_utf8str(wtext);
			mInputCode->appendText(text, false, false);
			mInputCode->makePristine();
			mHistoryIndex = sHistoryStorage.size();
		}
	}
}

//static
bool HBLuaConsole::print(const std::string& text)
{
	HBLuaConsole* self = findInstance();
	if (self)
	{
		if (!self->mTextBuffer.empty())
		{
			self->mTextBuffer += "\n";
		}
		self->mTextBuffer += text;
		return true;
	}
	return false;
}

//static
void HBLuaConsole::onClearButton(LLUICtrl* ctrlp, void* datap)
{
	HBLuaConsole* self = (HBLuaConsole*)datap;
	if (!self || !ctrlp) return;

	if (ctrlp == (LLUICtrl*)self->mClearOutput)
	{
		self->mOutputText->clear();
	}
	else if (ctrlp == (LLUICtrl*)self->mClearInput)
	{
		self->mInputCode->clear();
		self->mHistoryIndex = sHistoryStorage.size();
	}
}

//static
void HBLuaConsole::onBrowseButton(LLUICtrl* ctrlp, void* datap)
{
	HBLuaConsole* self = (HBLuaConsole*)datap;
	if (!self || !ctrlp || sHistoryStorage.empty()) return;

	if (ctrlp == (LLUICtrl*)self->mBackward)
	{
		if (self->mHistoryIndex > 0)
		{
			self->mInputCode->clear();
			self->mInputCode->setText(sHistoryStorage[--self->mHistoryIndex]);
			self->mInputCode->setCursorAndScrollToEnd();
		}
	}
	else if (ctrlp == (LLUICtrl*)self->mForward)
	{
		if (self->mHistoryIndex + 1 < sHistoryStorage.size())
		{
			self->mInputCode->clear();
			self->mInputCode->setText(sHistoryStorage[++self->mHistoryIndex]);
			self->mInputCode->setCursorAndScrollToEnd();
		}
	}
}

//static
void HBLuaConsole::onExecuteButton(LLUICtrl*, void* datap)
{
	HBLuaConsole* self = (HBLuaConsole*)datap;
	if (!self || !self->mInputCode->getLength())
	{
		return;
	}

	HBViewerAutomation lua(false, true);
	std::string code = self->mInputCode->getText();
	if (lua.loadString(code))
	{
		// Store the successful command in history, if different from last one.
		if (sHistoryStorage.empty() || sHistoryStorage.back() != code)
		{
			sHistoryStorage.emplace_back(code);
		}
		self->mHistoryIndex = sHistoryStorage.size();
		// Clear input text editor.
		self->mInputCode->clear();
		self->mInputCode->makePristine();
	}
}

static void load_file_cb(HBFileSelector::ELoadFilter, std::string& filename,
						 void*)
{
	if (!filename.empty())
	{
		HBLuaConsole* floaterp = HBLuaConsole::showInstance();
		if (floaterp)	// This should always be true
		{
			floaterp->loadFile(filename);
		}
	}
}

//static
void HBLuaConsole::onLoadButton(LLUICtrl*, void*)
{
	HBFileSelector::loadFile(HBFileSelector::FFLOAD_LUA, load_file_cb, NULL);
}

//static
void HBLuaConsole::onHelpButton(LLUICtrl*, void* datap)
{
	HBLuaConsole* self = (HBLuaConsole*)datap;
	if (self)
	{
		LLWeb::loadURL(self->getString("lua_manual_url"));
	}
}

//static
void HBLuaConsole::onListButton(LLUICtrl*, void* datap)
{
	HBLuaConsole* self = (HBLuaConsole*)datap;
	if (self)
	{
		// Focus back the input text editor
		self->mInputCode->setFocus(true);
		// Execute our Lua snippet to list our custom viewer functions.
		HBViewerAutomation lua(false, true);
		lua.loadString(self->getString("print_lua_funcs"));
	}
}
