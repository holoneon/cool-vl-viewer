/**
 * @file hbfloaterrlv.h
 * @brief The HBFloaterRLV and HBFloaterBlacklistRLV classes declarations
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 *
 * Copyright (c) 2011-2020, Henri Beauchamp
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

#include "llfloater.h"

class LLButton;
class LLScrollListCtrl;
class LLTabContainer;

class HBFloaterRLV final : public LLFloater,
						   public LLFloaterSingleton<HBFloaterRLV>
{
	friend class LLUISingleton<HBFloaterRLV, VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBFloaterRLV);

	bool postBuild() override;
	void draw() override;
	void onOpen() override;

public:
	static void setDirty();

	// Command status
	enum { QUEUED = -1, FAILED = 0, EXECUTED = 1, IMPLICIT = 2, BLACKLISTED = 3 };

	class LoggedCommand
	{
	public:
		LoggedCommand(const LLUUID& id, const std::string& name,
					  const std::string& command, S32 status);
	public:
		LLUUID		mId;
		std::string	mName;
		std::string	mCommand;
		std::string mTimeStamp;
		S32			mStatus;
		bool		mIsLua;
		bool		mIsGone;
		bool		mIsRoot;
	};

	static void logCommand(const LLUUID& obj_id, const std::string& obj_name,
						   const std::string& command, S32 status = EXECUTED);

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBFloaterRLV(const LLSD&);

	void setButtonsStatus();

	static void onTabChanged(void* data, bool);
	static void onButtonHelp(void*);
	static void onButtonRefresh(void* data);
	static void onButtonClear(void* data);
	static void onButtonClose(void* data);
	static void onDoubleClick(void* data);

public:
	static std::string					sQueued;
	static std::string					sFailed;
	static std::string					sExecuted;
	static std::string					sBlacklisted;
	static std::string					sImplicit;
	static std::string					sUnrestrictedEmotes;

private:
	LLButton*							mRefreshButton;
	LLButton*							mClearButton;
	LLTabContainer*						mTabContainer;
	LLScrollListCtrl*					mStatusByObject;
	LLScrollListCtrl*					mRestrictions;
	LLScrollListCtrl*					mExceptions;
	LLScrollListCtrl*					mCommandsLog;
	U32									mLastCommandsLogSize;
	bool								mFirstOpen;
	bool								mIsDirty;

	static std::vector<LoggedCommand>	sLoggedCommands;
};

class HBFloaterBlacklistRLV final
:	public LLFloater, public LLFloaterSingleton<HBFloaterBlacklistRLV>
{
	friend class LLUISingleton<HBFloaterBlacklistRLV,
							   VisibilityPolicy<LLFloater> >;

protected:
	LOG_CLASS(HBFloaterBlacklistRLV);

private:
	// Open only via LLFloaterSingleton interface, i.e. showInstance() or
	// toggleInstance().
	HBFloaterBlacklistRLV(const LLSD&);

	bool postBuild() override;

	static void onButtonApply(void* data);
	static void onButtonCancel(void* data);
};
