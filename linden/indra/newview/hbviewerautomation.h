/**
 * @file hbviewerautomation.h
 * @brief HBViewerAutomation class definition
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

#pragma once

#include <deque>
#include <set>

#include "boost/signals2.hpp"

#include "llmenugl.h"
#include "llmutex.h"
#include "llfloater.h"
#include "lltimer.h"
#include "llvector3d.h"

class HBAutomationThread;
class LLButton;
class LLColor4;
class LLControlVariable;
class LLFriendObserver;
class LLMessageSystem;
class LLPickInfo;
class LLSimInfo;
class LLTextEditor;
class LLViewerObject;
struct lua_Debug;
struct lua_State;

class HBViewerAutomation
{
	friend class HBLuaConsole;
	friend class HBLuaDialog;
	friend class HBLuaFloater;
	friend class HBLuaPieMenu;
	friend class HBIgnoreCallback;

protected:
	LOG_CLASS(HBViewerAutomation);

public:
	// Methods used for the resident gAutomationp interpreter in which the
	// automation script gets loaded
	static void start(std::string file_name = LLStringUtil::null);
	static void cleanup();

	// Method used to spawn transient interpreters for commands sent via the
	// chat input line, the llOwnersay() and llInstantMessage() LSL functions,
	// and the D-Bus commands. When use_print_buffer is true (used for D-Bus
	// commands), all print() and reportError() outputs are redirected to the
	// mPrintBuffer string and the contents of that string is returned by this
	// function (and otherwise an empty string).
	// When the command comes from an object script, that object's 'id' and
	// 'name' are passed (by checkLuaCommand()).
	// Note that viewer events are not transmitted to this type of interpreter.
	static std::string eval(const std::string& chunk,
							bool use_print_buffer = false,
							const LLUUID& id = LLUUID::null,
							const std::string& name = LLStringUtil::null);

	// Method used to interpret possible Lua commands in script messages.
	// Returns true if the message was indeed a Lua command.
	static bool checkLuaCommand(const std::string& message,
								const LLUUID& from_object_id,
								const std::string& from_object_name);

	// Method used to execute a Lua script file
	static void execute(const std::string& file_name);

	// Events transmitted by the viewer to the Lua automation script
	void onLogin();
	void onRegionChange();
	void onParcelChange();
	void onPositionChange(const LLVector3& pos_local,
						  const LLVector3d& pos_global);
	void onAveragedFPS(F32 fps, bool limited, F32 frame_render_time);
	void onAgentOccupationChange(S32 type);
	void onAgentPush(const LLUUID& id, S32 type, F32 mag);
	bool onSendChat(std::string& text);
	void onReceivedChat(U8 chat_type, const LLUUID& from_id,
						const std::string& name, const std::string& text);
	bool onChatTextColoring(const LLUUID& from_id, const std::string& name,
							const std::string& text, LLColor4& color);
	void onInstantMsg(const LLUUID& session_id, const LLUUID& origin_id,
					  const std::string& name, const std::string& text);
	void onAlertDialog(const std::string& dialog_name, const LLUUID& alert_id,
					   const std::string& message,
					   const std::vector<std::string>& buttons);
	void onScriptDialog(const LLUUID& notif_id, const std::string& message,
						const std::vector<std::string>& buttons);
	void onNotification(const std::string& dialog_name, const LLUUID& notif_id,
						const std::string& message);
	void onGroupNotification(const std::string& group_name,
							 const LLUUID& group_id, const LLUUID& notif_id,
							 F64 timestamp, const std::string& sender,
							 const std::string& subject,
							 const std::string& message,
							 const std::string& inventory);
	bool onSLURLDispatch(const std::string& slurl, const std::string& nav_type,
						 bool trusted);
	U32 onURLDispatch(const std::string& url, const std::string& target,
					  bool external_browser);
	void onFriendStatusChange(const LLUUID& id, U32 mask, bool is_online);
	void onAvatarRezzing(const LLUUID& id);
	void onAgentBaked();
	void onObjectAttached(const LLUUID& obj_id, const LLUUID& inv_id);
	void onRadar(const LLUUID& id, const std::string& name, S32 range,
				 bool marked);
	void onRadarSelection(const uuid_vec_t& ids);
	void onRadarMark(const LLUUID& id, const std::string& name, bool marked);
	void onRadarTrack(const LLUUID& id, const std::string& name, bool tracked);
	void onSideBarVisibilityChange(bool visible);
	void onAutoPilotFinished(const std::string& type, bool reached,
							 bool user_cancel);
	void onTPStateChange(S32 statep, const std::string& reason);
	void onFailedTPSimChange(S32 agents_count);
	void onWindlightChange(const std::string& sky_settings_name,
						   const std::string& water_settings_name,
						   const std::string& day_settings_name);
	void onCameraModeChange(S32 mode);
	void onJoystickButtons(S32 old_statep, S32 new_statep);
	void onRLVHandleCommand(const LLUUID& object_id, const std::string& behav,
							const std::string& option,
							const std::string& param);
	void onRLVAnswerOnChat(const LLUUID& object_id, S32 channel,
						   const std::string& text);

	// Called by llviermessage.cpp when receiving object properties messages
	static void processObjectPropertiesFamily(LLMessageSystem* msg);

	// Called from llagent.cpp to keep track of the agent positions history
	static void addToAgentPosHistory(const LLVector3d& global_pos);

protected:
	HBViewerAutomation(bool use_print_buffer = false,
					   bool for_console = false);
	virtual ~HBViewerAutomation();

	LL_INLINE virtual bool isThreaded() const		{ return false; }
	LL_INLINE virtual U32 getLuaThreadID() const	{ return 0; }

	static HBViewerAutomation* findInstance(lua_State* statep);

	void resetCallbackFlags();

	bool load(const std::string& file_name);
	bool loadString(const std::string& chunk);

	void reportError();
	// Callback method for the Lua warn() function
	static void reportWarning(void* data, const char* msg, int to_continue);

	bool registerCFunctions();	// Returns true on success
	S32 getGlobal(const std::string& global);

	void resetTimer();

	void pushGridSimAndPos();
	void pushParcelInfo();

	// Method used to pre-process Lua source files
	std::string preprocess(const std::string& file_name);

	// Callback for use with HBPreprocessor to load #include Lua files
	static S32 loadInclude(std::string& include_name, const std::string& path,
						   std::string& buffer, void*);

	// Callback for use with HBPreprocessor to report warning and error
	// messages
	static void preprocessorMessageCB(const std::string& message,
									  bool is_warning, void*);

	static bool callAutomationFunc(HBAutomationThread* threadp);

	// Idle callbacks.
	static void onIdleThread(void* datap);
	static void onIdleSimChange(void* datap);

	// Helper method to request object details. 'reason' must be 0 for muting,
	// 1 for un-muting, anything else for Lua GetObjectInfo().
	static bool requestObjectPropertiesFamily(const LLUUID& object_id,
											  U32 reason);

	// Helper method to find an item or category UUID from its full path name
	// in the inventory. The path separator is the pipe symbol ('|') and was
	// choosen because it is not a valid/accepted character for inventory
	// objects names).
	static const LLUUID& getInventoryObjectId(const std::string& name,
											  bool& is_category);

	// Watchdog timeout hook
	static void watchdog(lua_State* statep, lua_Debug*);

	// Overridden print() Lua function
	static int print(lua_State* statep);

	// New viewer-related Lua functions:
	static int hasThread(lua_State* statep);
	static int startThread(lua_State* statep);
	static int stopThread(lua_State* statep);
	static int sendSignal(lua_State* statep);
	static int getSourceFileName(lua_State* statep);
	static int getWatchdogState(lua_State* statep);
	static int isUUID(lua_State* statep);
	static int isAvatar(lua_State* statep);
	static int isObject(lua_State* statep);
	static int isAgentFriend(lua_State* statep);
	static int isAgentGroup(lua_State* statep);
	static int getAvatarName(lua_State* statep);
	static int getGroupName(lua_State* statep);
	static int getAgentFriends(lua_State* statep);
	static int getAgentGroups(lua_State* statep);
	static int isAdmin(lua_State* statep);
	static int getRadarList(lua_State* statep);
	static int getRadarData(lua_State* statep);
	static int setRadarTracking(lua_State* statep);
	static int setRadarToolTip(lua_State* statep);
	static int setRadarMarkChar(lua_State* statep);
	static int setRadarMarkColor(lua_State* statep);
	static int setRadarNameColor(lua_State* statep);
	static int setAvatarMinimapColor(lua_State* statep);
	static int setAvatarNameTagColor(lua_State* statep);
	static int getAgentPosHistory(lua_State* statep);
	static int getAgentInfo(lua_State* statep);
	static int setAgentOccupation(lua_State* statep);
	static int getAgentGroupData(lua_State* statep);
	static int setAgentGroup(lua_State* statep);
	static int agentGroupInvite(lua_State* statep);
	static int agentSit(lua_State* statep);
	static int agentStand(lua_State* statep);
	static int setAgentTyping(lua_State* statep);
	static int sendChat(lua_State* statep);
	static int getIMSession(lua_State* statep);
	static int closeIMSession(lua_State* statep);
	static int sendIM(lua_State* statep);
	static int alertDialogResponse(lua_State* statep);
	static int scriptDialogResponse(lua_State* statep);
	static int groupNotificationAcceptOffer(lua_State* statep);
	static int cancelNotification(lua_State* statep);
	static int getObjectInfo(lua_State* statep);
	static int browseToURL(lua_State* statep);
	static int dispatchSLURL(lua_State* statep);
	static int executeRLV(lua_State* statep);
	static int openNotification(lua_State* statep);
	static int openFloater(lua_State* statep);
	static int closeFloater(lua_State* statep);
	static int getFloaterInstances(lua_State* statep);
	static int getFloaterControls(lua_State* statep);
	static int getFloaterCtrlState(lua_State* statep);
	static int getFloaterList(lua_State* statep);
	static int showFloater(lua_State* statep);
	static int makeDialog(lua_State* statep);
	static int openLuaFloater(lua_State* statep);
	static int showLuaFloater(lua_State* statep);
	static int setLuaFloaterCommand(lua_State* statep);
	static int getLuaFloaterListLine(lua_State* statep);
	static int getLuaFloaterValue(lua_State* statep);
	static int getLuaFloaterValues(lua_State* statep);
	static int setLuaFloaterValue(lua_State* statep);
	static int setLuaFloaterInvFilter(lua_State* statep);
	static int setLuaFloaterEnabled(lua_State* statep);
	static int setLuaFloaterVisible(lua_State* statep);
	static int closeLuaFloater(lua_State* statep);
	static int overlayBarLuaButton(lua_State* statep);
	static int statusBarLuaIcon(lua_State* statep);
	static int sideBarButton(lua_State* statep);
	static int sideBarButtonToggle(lua_State* statep);
	static int sideBarHide(lua_State* statep);
	static int sideBarHideOnRightClick(lua_State* statep);
	static int sideBarButtonHide(lua_State* statep);
	static int sideBarButtonDisable(lua_State* statep);
	static int luaPieMenuSlice(lua_State* statep);
	static int luaContextMenu(lua_State* statep);
	static int pasteToContextHandler(lua_State* statep);
	static int automationMessage(lua_State* statep);
	static int automationRequest(lua_State* statep);
	static int playUISound(lua_State* statep);
	static int renderDebugInfo(lua_State* statep);
	static int getDebugSetting(lua_State* statep);
	static int setDebugSetting(lua_State* statep);
	static int getFrameTimeSeconds(lua_State* statep);
	static int getTimeSinceEpoch(lua_State* statep);
	static int getTimeStamp(lua_State* statep);
	static int getClipBoardString(lua_State* statep);
	static int setClipBoardString(lua_State* statep);
	static int findInventoryObject(lua_State* statep);
	static int giveInventory(lua_State* statep);
	static int makeInventoryLink(lua_State* statep);
	static int deleteInventoryLink(lua_State* statep);
	static int newInventoryFolder(lua_State* statep);
	static int listInventoryFolder(lua_State* statep);
	static int moveToInventoryFolder(lua_State* statep);
	static int pickInventoryItem(lua_State* statep);
	static int pickAvatar(lua_State* statep);
	static int getAgentAttachments(lua_State* statep);
	static int removeAgentAttachment(lua_State* statep);
	static int getAgentWearables(lua_State* statep);
	static int agentAutoPilotToPos(lua_State* statep);
	static int agentAutoPilotFollow(lua_State* statep);
	static int agentAutoPilotStop(lua_State* statep);
	static int agentAutoPilotLoad(lua_State* statep);
	static int agentAutoPilotSave(lua_State* statep);
	static int agentAutoPilotRemove(lua_State* statep);
	static int agentAutoPilotRecord(lua_State* statep);
	static int agentAutoPilotReplay(lua_State* statep);
#if LL_PUPPETRY
	static int agentPuppetryStart(lua_State* statep);
	static int agentPuppetryStop(lua_State* statep);
#endif
	static int agentRotate(lua_State* statep);
	static int getAgentRotation(lua_State* statep);
	static int teleportAgentHome(lua_State* statep);
	static int teleportAgentToPos(lua_State* statep);
	static int getGridSimAndPos(lua_State* statep);
	static int getParcelInfo(lua_State* statep);
	static int getCameraMode(lua_State* statep);
	static int setCameraMode(lua_State* statep);
	static int setCameraFocus(lua_State* statep);
	static int setVisualMute(lua_State* statep);
	static int addMute(lua_State* statep);
	static int removeMute(lua_State* statep);
	static int isMuted(lua_State* statep);
	static int blockSound(lua_State* statep);
	static int isBlockedSound(lua_State* statep);
	static int getBlockedSounds(lua_State* statep);
	static int derenderObject(lua_State* statep);
	static int getDerenderedObjects(lua_State* statep);
	static int getAgentPushes(lua_State* statep);
	static int applyDaySettings(lua_State* statep);
	static int applySkySettings(lua_State* statep);
	static int applyWaterSettings(lua_State* statep);
	static int setDayTime(lua_State* statep);
	static int getEESettingsList(lua_State* statep);
	static int getWLSettingsList(lua_State* statep);
	static int getEnvironmentStatus(lua_State* statep);
	static int getGlobalData(lua_State* statep);
	static int setGlobalData(lua_State* statep);
	static int getPerAccountData(lua_State* statep);
	static int setPerAccountData(lua_State* statep);
	static int callbackAfter(lua_State* statep);
	static int forceQuit(lua_State* statep);
	static int minimizeWindow(lua_State* statep);
	static int readTextFromFile(lua_State* statep);
	static int writeTextToFile(lua_State* statep);
	static int getFileSize(lua_State* statep);
	static int removeFile(lua_State* statep);
	static int createDirectory(lua_State* statep);
	static int removeDirectory(lua_State* statep);

	// HTTP communications related functions Lua.
	static int encodeJSON(lua_State* statep);
	static int decodeJSON(lua_State* statep);
	static int encodeBase64(lua_State* statep);
	static int decodeBase64(lua_State* statep);
	static int getHTTP(lua_State* statep);
	static void getHTTPCoro(lua_State* statep, U32 handle, std::string uri,
							U32 timeout, const std::string& accept);
	static int postHTTP(lua_State* statep);
	static void postHTTPCoro(lua_State* statep, U32 handle, std::string uri,
							 const std::string& data, U32 timeout,
							 const std::string& accept,
							 const std::string& content_type);

	// This is the callback used by callbackAfter() via doAfterInterval()
	static void doAfterIntervalCallback(lua_State* statep, int ref);

	// This is the callback used by onAgentBaked() via doAfterInterval()
	static void doCallOnAgentBaked(lua_State* statep);

	// This is the callback for the inventory item picker.
	static void onPickInventoryItem(const std::vector<std::string>& names,
									const uuid_vec_t& ids, void* datap,
									bool on_close);

	// This is the callback for the avatar picker.
	static void onPickAvatar(const std::vector<std::string>& names,
							 const uuid_vec_t& ids, void* datap);

	// Helper methods used to de/serialize simple Lua tables from/into strings
	static bool serializeTable(lua_State* statep, S32 stack_level = 1,
							   std::string* output = NULL);
	static bool deserializeTable(lua_State* statep, std::string data);

	void onObjectInfoReply(const LLUUID& object_id, const std::string& name,
						   const std::string& desc, const LLUUID& owner_id,
						   const LLUUID& group_id);

	// Events transmitted to the Lua automation script by Lua UI elements
	void onLuaDialogClose(const std::string& title, S32 button,
						  const std::string& text);
	void onLuaFloaterAction(const std::string& floater_name,
							const std::string& ctrl_name,
							const std::string& value);
	void onLuaFloaterOpen(const std::string& floater_name,
						  const std::string& parameter);
	void onLuaFloaterClose(const std::string& floater_name,
						   const std::string& parameter);
	void onLuaPieMenu(U32 slice, S32 type, const LLPickInfo& pick);

	bool onContextMenu(U32 handler_id, S32 operation, const std::string& type);
	// This is the method passed as a callback to LLEditMenuHandler for custom
	// context menu entries.
	static void contextMenuCallback(HBContextMenuData* datap);

protected:
	lua_State*					mLuaState;

	// mFromObjectId is gAgentID unless the Lua interpreter is one set up for
	// a scripted object command, or (under Linux) for a D-Bus Lua command (in
	// which case mFromObjectId is set to sLuaDBusFakeObjectId).
	LLUUID						mFromObjectId;
	std::string					mFromObjectName;

	std::string					mSourceFileName;

	LLTimer						mWatchdogTimer;
	F32							mWatchdogTimeout;

	boost::signals2::connection	mRegionChangedConnection;
	boost::signals2::connection	mParcelChangedConnection;
	boost::signals2::connection	mPositionChangedConnection;

	// These are used only in the automation script, by GetObjectInfo()
	uuid_list_t					mObjectInfoRequests;

	// Internal print buffer for D-Bus or threaded Lua instances
	std::string					mPrintBuffer;

	// Used to deal Lua warnings
	std::string					mWarningPrefix;
	std::string					mPendingWarningText;
	bool						mPausedWarnings;
	bool						mForceWarningsToChat;
	// 'true' when using the print buffer (D-Bus or threaded Lua instances)
	bool						mUsePrintBuffer;
	// 'true' when using the Lua console output text for printing.
	bool						mUseConsoleOutput;

	// Flags used to speed-up callbacks when they are not used in the
	// automation script, which is the only instance using them...
	bool						mHasCallbacks;
	bool						mHasOnSignal;
	bool						mHasOnLogin;
	bool						mHasOnRegionChange;
	bool						mHasOnParcelChange;
	bool						mHasOnPositionChange;
	bool						mHasOnAveragedFPS;
	bool						mHasOnAgentOccupationChange;
	bool						mHasOnAgentPush;
	bool						mHasOnSendChat;
	bool						mHasOnReceivedChat;
	bool						mHasOnChatTextColoring;
	bool						mHasOnInstantMsg;
	bool						mHasOnAlertDialog;
	bool						mHasOnScriptDialog;
	bool						mHasOnNotification;
	bool						mHasOnGroupNotification;
	bool						mHasOnSLURLDispatch;
	bool						mHasOnURLDispatch;
	bool						mHasOnFriendStatusChange;
	bool						mHasOnAvatarRezzing;
	bool						mHasOnAgentBaked;
	bool						mHasOnObjectAttached;
	bool						mHasOnRadar;
	bool						mHasOnRadarSelection;
	bool						mHasOnRadarMark;
	bool						mHasOnRadarTrack;
	bool						mHasOnLuaDialogClose;
	bool						mHasOnLuaFloaterAction;
	bool						mHasOnLuaFloaterOpen;
	bool						mHasOnLuaFloaterClose;
	bool						mHasOnSideBarVisibilityChange;
	bool						mHasOnAutomationMessage;
	bool						mHasOnAutomationRequest;
	bool						mHasOnAutoPilotFinished;
	bool						mHasOnTPStateChange;
	bool						mHasOnFailedTPSimChange;
	bool						mHasOnWindlightChange;
	bool						mHasOnCameraModeChange;
	bool						mHasOnJoystickButtons;
	bool						mHasOnLuaPieMenu;
	bool						mHasOnContextMenu;
	bool						mHasOnRLVHandleCommand;
	bool						mHasOnRLVAnswerOnChat;
	bool						mHasOnObjectInfoReply;
	bool						mHasOnPickInventoryItem;
	bool						mHasOnPickAvatar;
	bool						mHasOnHTTPReply;

	typedef fast_hmap<lua_State*, HBViewerAutomation*> instances_map_t;
	static instances_map_t		sInstances;

	// We must protect the instance pointers and pending signals with a mutex !
	static LLMutex				sThreadsMutex;

	typedef fast_hmap<U32, HBAutomationThread*> threads_list_t;
	static threads_list_t		sThreadsInstances;
	static threads_list_t		sDeadThreadsInstances;

	struct HBThreadSignals
	{
		std::vector<std::string>	mSignals;
		U32							mThreadID;
	};
	typedef fast_hmap<HBAutomationThread*, HBThreadSignals*> signals_map_t;
	static signals_map_t		sThreadsSignals;

	// Array of flags/counters used to avoid infinite recursion when dealing
	// with some Lua callbacks. They are used only by the automation script and
	// must be set (and automatically reset) via the HBIgnoreCallback helper
	// class.
	enum {
		E_ONSENDCHAT,
		E_ONINSTANTMSG,
		E_ONDISPATCHSLURL,
		E_ONDISPATCHURL,
		E_ONRADARTRACK,
		E_ONAGENTOCCUPATIONCHANGE,
		E_ONCAMERAMODECHANGE,
		E_ONWINDLIGHTCHANGE,
		E_IGN_CB_COUNT
	};
	static S32					sIgnoredCallbacks[E_IGN_CB_COUNT];

	// Used for the automation script, to observe friends status changes
	static LLFriendObserver*	sFriendsObserver;

	// These are used internally; for any script, in order to fulfill the
	// objects (un)muting needs.
	static uuid_list_t			sMuteObjectRequests;
	static uuid_list_t			sUnmuteObjectRequests;

	static std::string			sLastAutomationScriptFile;

	typedef std::deque<LLVector3d> pos_history_t;
	static pos_history_t		sPositionsHistory;

#if LL_LINUX
public:
	static LLUUID				sLuaDBusFakeObjectId;
#endif
};

class HBLuaSideBar final : public LLPanel
{
protected:
	LOG_CLASS(HBLuaSideBar);

public:
	HBLuaSideBar();
	~HBLuaSideBar() override;

	void draw() override;
	void setVisible(bool visible) override;
	void reshape(S32 width, S32 height, bool from_parent = true) override;
	bool handleRightMouseDown(S32 x, S32 y, MASK mask) override;

	void setHidden(bool hidden);

	U32 setButton(U32 number, std::string icon, std::string command,
				  const std::string& tooltip);
	S32 buttonToggle(U32 number, S32 toggle);
	void buttonSetControl(U32 number, LLControlVariable* controlp);
	void setButtonEnabled(U32 number, bool enabled);
	void setButtonVisible(U32 number, bool visible);

	void removeAllButtons();

	LL_INLINE void hideOnRightClick(bool b)		{ mHideOnRightClick = b; }

private:
	void setShape();

	static bool handleSideChanged(const LLSD&);

	static void onButtonClicked(void* user_data);

private:
	std::vector<std::string>	mCommands;
	std::set<U32>				mActiveButtons;
	U32							mNumberOfButtons;
	bool						mLeftSide;
	bool						mHidden;
	bool						mHideOnRightClick;
};

class HBLuaPieMenu final : public LLPieMenu
{
protected:
	LOG_CLASS(HBLuaPieMenu);

public:
	HBLuaPieMenu();
	~HBLuaPieMenu() override;

	bool onPieMenu(const LLPickInfo& pick, LLViewerObject* objectp);
	void onPieSliceClick(U32 slice, const LLPickInfo& pick);

	void setSlice(S32 type, U32 slice, const std::string& label,
				  const std::string& command);

	void removeAllSlices();

private:
	// We have two methods so to avoid re-running here the same (costly) code
	// as the one executed in LLToolPie::handleRightClickPick() for finding the
	// avatar associated with an attachment...

	// This is the simpler method where "object" must never be another avatar's
	// attachment. It is called from onPieMenu() (istelf called from
	// LLToolPie::handleRightClickPick()) and from the method below.
	S32 getPickedType(const LLPickInfo& pick, LLViewerObject* objectp);

	// This is the full method (which calls the above one when needed) and it
	// uses a caching scheme.
	S32 getPickedType(const LLPickInfo& pick);

private:
	std::vector<std::string>	mCommands;
	std::vector<std::string>	mLabels;
	LLUUID						mLastPickId;
	S32							mLastPickType;
};

class HBLuaConsole final : public LLFloater,
						   public LLFloaterSingleton<HBLuaConsole>
{
	friend class LLUISingleton<HBLuaConsole, VisibilityPolicy<LLFloater> >;
	friend class HBViewerAutomation;

protected:
	LOG_CLASS(HBLuaConsole);

public:
	void loadFile(const std::string& filemane);

protected:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBLuaConsole(const LLSD&);
	~HBLuaConsole() override;

	// LLFloater overrides
	bool postBuild() override;
	void draw() override;
	bool handleKeyHere(KEY key, MASK mask) override;

	// LLFocusableElement override
	void onFocusReceived() override;

	// Returns true if the console exists and the text could be printed in it,
	// or false otherwise.
	static bool print(const std::string& text);

private:
	static void onClearButton(LLUICtrl* ctrlp, void* datap);
	static void onLoadButton(LLUICtrl*, void*);
	static void onBrowseButton(LLUICtrl* ctrlp, void* datap);
	static void onExecuteButton(LLUICtrl*, void* datap);
	static void onHelpButton(LLUICtrl*, void* datap);
	static void onListButton(LLUICtrl*, void* datap);

private:
	LLLayoutStack*					mInputLayout;
	LLTextEditor*					mOutputText;
	LLTextEditor*					mInputCode;
	LLButton*						mClearOutput;
	LLButton*						mClearInput;
	LLButton*						mLoadCode;
	LLButton*						mBackward;
	LLButton*						mForward;
	LLButton*						mExecute;
	LLRect							mInputLayoutRect;
	std::string						mTextBuffer;
	size_t							mHistoryIndex;
	U32								mResizingAttempts;
	S32								mSizeDelta;

	// Static to keep the Lua commands history over the whole viewer session.
	static std::vector<std::string>	sHistoryStorage;
};

// A mere wrapper function to avoid declaring HBLuaFloater in this header.
// This function is used exclusively by LLUICtrl to register Lua commands from
// UI controls in Lua floaters with a "lua_command" attribute.
void register_ui_lua_command(LLUICtrl* ctrlp, LLView* parentp,
							 const std::string& command);
// Another wrapper, for the alert dialogs callback.
void lua_alert_callback(const std::string& dialog_name, const LLUUID& alert_id,
						const std::string& message,
					    const std::vector<std::string>& buttons);

extern HBViewerAutomation*	gAutomationp;
extern HBLuaSideBar*		gLuaSideBarp;
extern HBLuaPieMenu*		gLuaPiep;
