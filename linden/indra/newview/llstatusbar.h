/** 
 * @file llstatusbar.h
 * @brief LLStatusBar class definition
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

#include "llframetimer.h"
#include "llpanel.h"
#include "llstatgraph.h"

// "Constants" loaded from settings.xml at start time
extern S32 gStatusBarHeight;

class LLButton;
class LLLineEditor;
class LLStatGraph;
class LLTextBox;
class LLUICtrl;
class LLUUID;

class LLStatusBar final : public LLPanel
{
public:
	LLStatusBar(const LLRect& rect);
	~LLStatusBar() override;
	
	void draw() override;
	void refresh() override;

	LL_INLINE void setDirty()						{ mDirty = true; }

	void setIcons();

	// MANIPULATORS
	void setBalance(S32 balance);
	void debitBalance(S32 debit);
	void creditBalance(S32 credit);

	// Request the latest currency balance from the server
	static void sendMoneyBalanceRequest();

	void setHealth(S32 percent);

	LL_INLINE void setLandCredit(S32 credit)		{ mSquareMetersCredit = credit; }
	LL_INLINE void setLandCommitted(S32 committed)	{ mSquareMetersCommitted = committed; }

	// Causes an agent parcel properties request to be launched and the various
	// status bar icons (no-build, no-scripts, no-fly, no-voice, etc) to be
	// updated accordingly after the reply is received by the parcel manager.
	void setDirtyAgentParcelProperties();

	// Some elements should hide in mouselook
	void setVisibleForMouselook(bool visible);

	LL_INLINE S32 getBalance() const				{ return mBalance; }
	LL_INLINE S32 getHealth() const					{ return mHealth; }

	LL_INLINE bool isUserTiered() const				{ return mSquareMetersCredit > 0; }
	LL_INLINE S32 getSquareMetersCredit() const		{ return mSquareMetersCredit; }
	LL_INLINE S32 getSquareMetersCommitted() const	{ return mSquareMetersCommitted; }
	LL_INLINE S32 getSquareMetersLeft() const		{ return mSquareMetersCredit -
															 mSquareMetersCommitted; }

	LL_INLINE void incFailedEventPolls()			{ ++mAgentRegionFailedEventPolls; }
	LL_INLINE void resetFailedEventPolls()			{ mAgentRegionFailedEventPolls = 0; }

	LL_INLINE void setFrameRateLimited(bool b)		{ mFrameRateLimited = b; }

	// We collect per-frame statistics of rendered materials, so to inform the
	// user when some materials are around (and which type) and display an
	// indicator in the status bar to let them know if they are using a
	// rendering mode allowing to see these materials or not. HB
	LL_INLINE void materialsUsage(bool using_alm, bool using_pbr,
								  bool lacking_fallbacks,
								  bool lacking_basecolor)
	{
		mUsingALM = using_alm;
		mUsingPBR = using_pbr;
		mLacksLegacyFallback = lacking_fallbacks;
		mLacksBaseColorFallback = lacking_basecolor;
	}

	// We collect per-frame statistics of glTF scenes usage as well. HB
	LL_INLINE void usingGLTFScene(bool has_gltf_scene)
	{
		mUsingGLTFScene = has_gltf_scene;
	}

	// Lua status bar button support. HB
	void setLuaFunctionButton(const std::string& command,
							  const std::string& tooltip);

private:
	enum
	{
		TIME_MODE_SL,
		TIME_MODE_UTC,
		TIME_MODE_LOCAL,
		TIME_MODE_END
	};

	void setNetworkBandwidth();

	static void onClickParcelInfo(void* data);
	static void onClickTime(void* data);
	static void onClickMaterials(void*);
	static void onClickBalance(void*);
	static void onClickHealth(void* data);
	static void onClickFly(void* data);
	static void onClickPush(void* data);
	static void onClickBuild(void* data);
	static void onClickScripts(void* data);
	static void onClickPathFinding(void* data);
	static void onClickDirtyNavMesh(void* data);
	static void onClickAdult(void* data);
	static void onClickMature(void* data);
	static void onClickPG(void* data);
	static void onClickNotifications(void* data);
	static void onClickTooComplex(void* data);
	static void onClickVoice(void* data);
	static void onClickSee(void* data);
	static void onClickBuyLand(void* data);
	static void onClickScriptDebug(void* data);
	static void onCommitSearch(LLUICtrl*, void* data);
	static void onClickSearch(void* data);
	static void onClickFPS(void*);
	static void onClickStatGraph(void*);
	static void onClickDiscardGraph(void*);
	static void onClickLuaFunction(void* data);

private:
	LLColor4		mParcelTextColor;

	LLTextBox*		mTextFPS;
	LLTextBox*		mMaterials;
	LLTextBox*		mTextBalance;
	LLTextBox*		mTextHealth;
	LLTextBox*		mTextTime;
	LLTextBox*		mTextParcelName;
	LLTextBox*		mTextStat;
	LLTextBox*		mTextNotifications;
	LLTextBox*		mTextTooComplex;

	LLStatGraph*	mSGBandwidth;
	LLStatGraph*	mSGPacketLoss;
	LLStatGraph*	mSGDiscardBias;

	LLLineEditor*	mLineEditSearch;

	LLButton*		mBtnHealth;
	LLButton*		mBtnNoFly;
	LLButton*		mBtnBuyLand;
	LLButton*		mBtnNoBuild;
	LLButton*		mBtnNoScript;
	LLButton*		mBtnNoPush;
	LLButton*		mBtnNoVoice;
	LLButton*		mBtnNoSee;
	LLButton*		mBtnNoPathFinding;
	LLButton*		mBtnDirtyNavMesh;
	LLButton*		mBtnAdult;
	LLButton*		mBtnMature;
	LLButton*		mBtnPG;
	LLButton*		mBtnNotificationsOn;
	LLButton*		mBtnNotificationsOff;
	LLButton*		mBtnScriptError;
	LLButton*		mBtnRebaking;
	LLButton*		mTooComplex;
	LLButton*		mBtnSearch;
	LLButton*		mBtnSearchBevel;
	LLButton*		mBtnLuaFunction;
	LLButton*		mBtnBuyMoney;

	std::string		mAllMaterialsOK;
	std::string		mDiffuseFallback;
	std::string		mLegacyFallback;
	std::string		mBasecolorFallback;
	std::string		mMaterialMissing;
	std::string		mNoMaterial;
	std::string		mGLTFSceneInUse;
	std::string		mGLTFSceneNoShader;

	std::string		mParcelTooltipOK;
	std::string		mParcelTooltipWarn1;
	std::string		mParcelTooltipWarn2;
	std::string		mParcelTooltipWarn3;

	std::string		mLuaCommand;

	U32				mTimeMode;
	S32				mBalance;
	S32				mHealth;
	S32				mLastNotifications;
	S32				mSquareMetersCredit;
	S32				mSquareMetersCommitted;
	U32				mAgentRegionFailedEventPolls;
	F32				mLastZeroBandwidthTime;
	F32				mAbsoluteMaxBandwidth;

	LLFrameTimer	mHealthTimer;
	LLFrameTimer	mUpdateTimer;
	LLFrameTimer	mNotificationsTimer;
	LLFrameTimer	mRefreshAgentParcelTimer;

	bool			mVisibility;
	bool			mDirty;
	bool			mUseOldIcons;
	bool			mNetworkDown;
	bool			mFrameRateLimited;
	bool			mUsingALM;
	bool			mUsingPBR;
	bool			mLacksLegacyFallback;
	bool			mLacksBaseColorFallback;
	bool			mUsingGLTFScene;
};

extern LLStatusBar*	gStatusBarp;

// *HACK: Status bar owns your cached money balance. JC
LL_INLINE bool can_afford_transaction(S32 cost)
{
	return cost <= 0 || (gStatusBarp && gStatusBarp->getBalance() >= cost);
}
