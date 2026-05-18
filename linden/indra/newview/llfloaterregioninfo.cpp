/**
 * @file llfloaterregioninfo.cpp
 * @author Aaron Brashears
 * @brief Implementation of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
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

#include "llfloaterregioninfo.h"

#include "llalertdialog.h"
#include "llbutton.h"
#include "llcachename.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llcorehttputil.h"
#include "lldir.h"
#include "lldispatcher.h"
#include "llexperiencecache.h"
#include "llfilesystem.h"
#include "llfontgl.h"
#include "llglheaders.h"
#include "llgridmanager.h"					// For gIsInSecondLife
#include "llinventory.h"
#include "lllineeditor.h"
#include "llnamelistctrl.h"
#include "llnotifications.h"
#include "llradiogroup.h"
#include "llrenderutils.h"					// For gl_draw_scaled_image()
#include "llsdutil.h"						// For ll_pretty_print_sd()
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llworkqueue.h"
#include "llxfermanager.h"
#include "llmessage.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llenvironment.h"
#include "llfloateravatarpicker.h"
#include "llfloatergodtools.h"				// For send_sim_wide_deletes()
#include "llfloatergroups.h"
#include "hbfloaterinvitemspicker.h"
#include "llfloatertelehub.h"
#include "llfloatertopobjects.h"
#include "llgltfmateriallist.h"
#include "llinventorymodel.h"
#include "llpanelexperiencelisteditor.h"
#include "hbpanellandenvironment.h"
#include "llpipeline.h"
#include "lltexturectrl.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerinventory.h"
#include "llviewertexteditor.h"
#include "llviewertexturelist.h"
#include "llvlcomposition.h"
#include "llworld.h"

using namespace std::placeholders;

#define ELAR_ENABLED 0 // Enable when server support is implemented

constexpr U32 CORNER_COUNT = 4;
constexpr U32 MAX_LISTED_NAMES = 100;

bool gEstateDispatchInitialized = false;

std::string LLEstateInfoModel::sEstateName;
LLUUID LLEstateInfoModel::sOwnerId;
U32 LLEstateInfoModel::sEstateId = 0;
U64 LLEstateInfoModel::sEstateFlags = 0;
F32 LLEstateInfoModel::sSunHour = 0.f;

///////////////////////////////////////////////////////////////////////////////
// LLDispatchEstateUpdateInfo class 
///////////////////////////////////////////////////////////////////////////////

class LLDispatchEstateUpdateInfo final : public LLDispatchHandler
{
public:
	LLDispatchEstateUpdateInfo() = default;

	bool operator()(const LLDispatcher*, const std::string& key,
					const LLUUID& invoice,
					const strings_vec_t& strings) override;
};

// key = "estateupdateinfo"
// strings[0] = estate name
// strings[1] = str(owner_id)
// strings[2] = str(estate_id)
// strings[3] = str(estate_flags)
// strings[4] = str((S32)(sun_hour * 1024))
// strings[5] = str(parent_estate_id)
// strings[6] = str(covenant_id)
// strings[7] = str(covenant_timestamp)
// strings[8] = str(send_to_agent_only)
// strings[9] = str(abuse_email_addr)
bool LLDispatchEstateUpdateInfo::operator()(const LLDispatcher*,
											const std::string& key,
											const LLUUID&,
											const strings_vec_t& strings)
{
	// Unconditionnally fill-up the LLEstateInfoModel member variables.

	// NOTE: LLDispatcher extracts strings with an extra \0 at the end. If we
	// pass the std::string direct to the UI/renderer it draws with a weird
	// character at the end of the string. Therefore do preserve the c_str()
	// calls !
	LLEstateInfoModel::sEstateName = strings[0].c_str();
	LLEstateInfoModel::sOwnerId.set(strings[1]);
	LLEstateInfoModel::sEstateId = strtoul(strings[2].c_str(), NULL, 10);
	U64 flags = strtoul(strings[3].c_str(), NULL, 10);
	LLEstateInfoModel::sEstateFlags = flags;
	LLEstateInfoModel::sSunHour = F32(strtod(strings[4].c_str(),
											 NULL)) / 1024.0f;

	// Then update the agent region if any (and if none, we got disconnected,
	// so give up)
	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp) return true;
	regionp->setOwner(LLEstateInfoModel::sOwnerId);

	// Update the estate-related panels in region info floater if any.

	LLPanelEstateInfo* estatep = LLFloaterRegionInfo::getPanelEstate();
	if (!estatep) return true;

	estatep->setEstateName(LLEstateInfoModel::sEstateName);
	// Update estate owner name in UI
	if (gCacheNamep)
	{
		gCacheNamep->get(LLEstateInfoModel::sOwnerId, false,
						 LLPanelEstateInfo::callbackCacheName);
	}
	estatep->setEstateID(LLEstateInfoModel::sEstateId);
	estatep->setEstateFlags(flags);
	if (LLEstateInfoModel::sSunHour == 0.f &&
		!(flags & REGION_FLAGS_SUN_FIXED))
	{
		estatep->setGlobalTime(true);
	}
	else
	{
		estatep->setGlobalTime(false);
		estatep->setSunHour(LLEstateInfoModel::sSunHour);
	}

	LLPanelEstateAccess* accessp = LLFloaterRegionInfo::getPanelAccess();
	if (!accessp) return true;

	accessp->updateControls();
	// Gods need to be able to edit the access list for Linden estates,
	// regardless of visibility, to allow object and L$ transfers.
	// In OpenSim, ignore Linden estate flag.
	bool linden_estate = LLEstateInfoModel::sEstateId <= ESTATE_LAST_LINDEN;
	bool enable_agent = !linden_estate || gAgent.isGodlike() ||
						!gIsInSecondLife;
	bool enable_ban = !linden_estate || !gIsInSecondLife;
	accessp->setAccessAllowedEnabled(enable_agent, enable_agent, enable_ban);

	HBPanelLandEnvironment* envp = LLFloaterRegionInfo::getPanelEnvironment();
	if (envp)
	{
		envp->refresh();
	}

	return true;
}

class LLDispatchSetEstateAccess final : public LLDispatchHandler
{
public:
	LLDispatchSetEstateAccess() = default;

	bool operator()(const LLDispatcher*, const std::string& key,
					const LLUUID& invoice,
					const strings_vec_t& strings) override;
};


// key = "setaccess"
// strings[0] = str(estate_id)
// strings[1] = str(packed_access_lists)
// strings[2] = str(num allowed agent ids)
// strings[3] = str(num allowed group ids)
// strings[4] = str(num banned agent ids)
// strings[5] = str(num estate manager agent ids)
// strings[6...] = bin(uuid)
bool LLDispatchSetEstateAccess::operator()(const LLDispatcher*,
										   const std::string& key,
										   const LLUUID&,
										   const strings_vec_t& strings)
{
	LLPanelEstateAccess* panelp = LLFloaterRegionInfo::getPanelAccess();
	if (!panelp) return true;	// We are since gone !

	if (gAgent.hasRegionCapability("EstateAccess"))
	{
		if (panelp->getPendingUpdate())
		{
			panelp->setPendingUpdate(false);
			panelp->updateLists();
		}
		return true;
	}

	// Old, non-capability based code, kept for OpenSIM compatibility

	S32 index = 1;	// skip estate_id
	U32 access_flags = strtoul(strings[index++].c_str(), NULL, 10);
	S32 num_allowed_agents = strtol(strings[index++].c_str(), NULL, 10);
	S32 num_allowed_groups = strtol(strings[index++].c_str(), NULL, 10);
	S32 num_banned_agents = strtol(strings[index++].c_str(), NULL, 10);
	S32 num_estate_managers = strtol(strings[index++].c_str(), NULL, 10);

	// sanity ckecks
	if (num_allowed_agents > 0 &&
		!(access_flags & ESTATE_ACCESS_ALLOWED_AGENTS))
	{
		llwarns << "non-zero count for allowed agents, but no corresponding flag"
				<< llendl;
	}
	if (num_allowed_groups > 0 &&
		!(access_flags & ESTATE_ACCESS_ALLOWED_GROUPS))
	{
		llwarns << "non-zero count for allowed groups, but no corresponding flag"
				<< llendl;
	}
	if (num_banned_agents > 0 && !(access_flags & ESTATE_ACCESS_BANNED_AGENTS))
	{
		llwarns << "non-zero count for banned agents, but no corresponding flag"
				<< llendl;
	}
	if (num_estate_managers > 0 && !(access_flags & ESTATE_ACCESS_MANAGERS))
	{
		llwarns << "non-zero count for managers, but no corresponding flag"
				<< llendl;
	}

	LLNameListCtrl* name_list;
	// Grab the UUIDs out of the string fields
	if (access_flags & ESTATE_ACCESS_ALLOWED_AGENTS)
	{
		name_list = panelp->mAllowedAvatars;

		S32 total = num_allowed_agents;

		if (name_list)
		{
			total += name_list->getItemCount();
		}

		std::string msg = llformat("Allowed residents: (%d, max %d)", total,
								   ESTATE_MAX_ACCESS_IDS);
		panelp->childSetValue("allow_resident_label", LLSD(msg));

		if (name_list)
		{
#if 0
			name_list->deleteAllItems();
#endif
			for (S32 i = 0;
				 i < num_allowed_agents && i < ESTATE_MAX_ACCESS_IDS; ++i)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);
				name_list->addNameItem(id);
			}
			panelp->childSetEnabled("remove_allowed_avatar_btn",
									name_list->getFirstSelected() != NULL);
			name_list->sortByName(true);
		}
	}

	if (access_flags & ESTATE_ACCESS_ALLOWED_GROUPS)
	{
		name_list = panelp->mAllowedGroups;

		std::string msg = llformat("Allowed groups: (%d, max %d)",
									num_allowed_groups,
									(S32)ESTATE_MAX_GROUP_IDS);
		panelp->childSetValue("allow_group_label", LLSD(msg));

		if (name_list)
		{
			name_list->deleteAllItems();
			for (S32 i = 0; i < num_allowed_groups && i < ESTATE_MAX_GROUP_IDS;
				 ++i)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);
				name_list->addGroupNameItem(id);
			}
			panelp->childSetEnabled("remove_allowed_group_btn",
								    name_list->getFirstSelected() != NULL);
			name_list->sortByName(true);
		}
	}

	if (access_flags & ESTATE_ACCESS_BANNED_AGENTS)
	{
		name_list = panelp->mBannedAvatars;

		S32 total = num_banned_agents;

		if (name_list)
		{
			total += name_list->getItemCount();
		}

		std::string msg = llformat("Banned residents: (%d, max %d)", total,
								   ESTATE_MAX_BANNED_IDS);
		panelp->childSetValue("ban_resident_label", LLSD(msg));

		if (name_list)
		{
#if 0
			name_list->deleteAllItems();
#endif
			std::string na = LLTrans::getString("na");
			for (S32 i = 0; i < num_banned_agents && i < ESTATE_MAX_BANNED_IDS;
				 ++i)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);

				LLSD item;
				item["id"] = id;

				LLSD& columns = item["columns"];

				columns[0]["column"] = "name"; // value is auto-populated

				columns[1]["column"] = "last_login_date";
				columns[1]["value"] = na;

				columns[2]["column"] = "ban_date";
				columns[2]["value"] = na;

				columns[3]["column"] = "bannedby";
				columns[3]["value"] = na;

				name_list->addElement(item);
			}
			panelp->childSetEnabled("remove_banned_avatar_btn",
									name_list->getFirstSelected() != NULL);
			name_list->sortByName(true);
		}
	}

	if (access_flags & ESTATE_ACCESS_MANAGERS)
	{
		std::string msg = llformat("Estate Managers: (%d, max %d)",
								   num_estate_managers,
								   ESTATE_MAX_MANAGERS);
		panelp->childSetValue("estate_manager_label", LLSD(msg));

		name_list = panelp->mEstateManagers;
		if (name_list)
		{
			// Clear existing entries
			name_list->deleteAllItems();

			// There should be only ESTATE_MAX_MANAGERS people in the list,
			// but if the database gets more (SL-46107) don't truncate the
			// list unless it's really big.  Go ahead and show the extras so
			// the user doesn't get confused, and they can still remove them.
			for (S32 i = 0;
				 i < num_estate_managers && i < ESTATE_MAX_MANAGERS * 4; ++i)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);
				name_list->addNameItem(id);
			}
			panelp->childSetEnabled("remove_estate_manager_btn",
									name_list->getFirstSelected() != NULL);
			name_list->sortByName(true);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// LLEstateAccessChangeInfo class 
///////////////////////////////////////////////////////////////////////////////

class LLEstateAccessChangeInfo
{
public:
	LLEstateAccessChangeInfo(const LLSD& sd)
	{
		mDialogName = sd["dialog_name"].asString();
		mOperationFlag = (U32)sd["operation"].asInteger();
		for (LLSD::array_const_iterator it = sd["allowed_ids"].beginArray(),
										end = sd["allowed_ids"].endArray();
			 it != end; ++it)
		{
			mAgentOrGroupIDs.emplace_back(it->asUUID());
		}
	}

	const LLSD asLLSD() const
	{
		LLSD sd;
		sd["name"] = mDialogName;
		sd["operation"] = (S32)mOperationFlag;
		for (U32 i = 0, count = mAgentOrGroupIDs.size(); i < count; ++i)
		{
			sd["allowed_ids"].append(mAgentOrGroupIDs[i]);
		}
		return sd;
	}

public:
	// ESTATE_ACCESS_BANNED_AGENT_ADD, _REMOVE, etc.
	U32							mOperationFlag;

	std::string					mDialogName;

	// List of agent IDs to apply to this change
	uuid_vec_t					mAgentOrGroupIDs;
};

///////////////////////////////////////////////////////////////////////////////
// LLDispatchSetEstateExperience class 
///////////////////////////////////////////////////////////////////////////////

class LLDispatchSetEstateExperience final : public LLDispatchHandler
{
public:
	LLDispatchSetEstateExperience() = default;

	bool operator()(const LLDispatcher*, const std::string& key,
					const LLUUID& invoice,
					const strings_vec_t& strings) override;

	static LLSD getIDs(strings_vec_t::const_iterator it,
					   strings_vec_t::const_iterator end, S32 count);
};

//static
LLSD LLDispatchSetEstateExperience::getIDs(strings_vec_t::const_iterator it,
										   strings_vec_t::const_iterator end,
										   S32 count)
{
	LLSD ids = LLSD::emptyArray();
	LLUUID id;
	while (count-- > 0 && it < end)
	{
		memcpy(id.mData, (*(it++)).data(), UUID_BYTES);
		ids.append(id);
	}
	return ids;
}

// key = "setexperience"
// strings[0] = str(estate_id)
// strings[1] = str(send_to_agent_only)
// strings[2] = str(num blocked)
// strings[3] = str(num trusted)
// strings[4] = str(num allowed)
// strings[5] = bin(uuid) ...
bool LLDispatchSetEstateExperience::operator()(const LLDispatcher*,
											   const std::string& key,
											   const LLUUID&,
											   const strings_vec_t& strings)
{
	LLPanelRegionExperiences* panelp =
		LLFloaterRegionInfo::getPanelExperiences();
	if (!panelp)
	{
		return true;
	}

	constexpr size_t MIN_SIZE = 5;
	if (strings.size() < MIN_SIZE)
	{
		return true;
	}

	// Skip 2 parameters
	strings_vec_t::const_iterator it = strings.begin();
	++it; // U32 estate_id = strtol((*it).c_str(), NULL, 10);
	++it; // U32 send_to_agent_only = strtoul((*(++it)).c_str(), NULL, 10);

	// Read 3 parameters
	S32 blocked = strtol((*(it++)).c_str(), NULL, 10);
	S32 trusted = strtol((*(it++)).c_str(), NULL, 10);
	S32 allowed = strtol((*(it++)).c_str(), NULL, 10);

	LLSD ids = LLSD::emptyMap()
		.with("blocked", getIDs(it, strings.end(),  blocked))
		.with("trusted", getIDs(it + blocked, strings.end(), trusted))
		.with("allowed", getIDs(it + blocked + trusted, strings.end(),
			  allowed));

	panelp->processResponse(ids);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// LLPBRTerrainFeatures class 
///////////////////////////////////////////////////////////////////////////////

//static
void LLPBRTerrainFeatures::queueQuery(LLViewerRegion* regionp, done_cb_t cb)
{
	if (!regionp) return;	// Paranoia

	const LLUUID& region_id = regionp->getRegionID();

	const std::string& cap_url = regionp->getCapability("ModifyRegion");
	if (cap_url.empty())
	{
		llwarns << "No ModifyRegion capability for region " << region_id
				<< ". Aborted." << llendl;
		return;
	}
	gCoros.launch("LLPBRTerrainFeatures::queueQueryCoro",
				  std::bind(&LLPBRTerrainFeatures::queryRegionCoro, cap_url,
							region_id, cb));
}

//static
void LLPBRTerrainFeatures::queueModify(LLViewerRegion* regionp,
									   const LLTerrain* terrainp)
{
	if (!regionp || !terrainp) return;	// Paranoia

	const LLUUID& region_id = regionp->getRegionID();

	const std::string& cap_url = regionp->getCapability("ModifyRegion");
	if (cap_url.empty())
	{
		llwarns << "No ModifyRegion capability for region " << region_id
				<< ". Aborted." << llendl;
		return;
	}

	LLSD updates = LLSD::emptyMap();
	LLSD overrides = LLSD::emptyArray();
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		const LLGLTFMaterial* matp = terrainp->getMaterialOverride(i);
		LLSD omatllsd;
		if (matp)
		{
			omatllsd = LLGLTFMaterial::sDefault.getOverrideLLSD(*matp);
		}
		else
		{
			omatllsd = LLSD::emptyMap();
		}
		overrides.append(omatllsd);
	}
	updates["overrides"] = overrides;
	gCoros.launch("LLPBRTerrainFeatures::modifyRegionCoro",
				  std::bind(&LLPBRTerrainFeatures::modifyRegionCoro, cap_url,
							region_id, updates));
}

//static
void LLPBRTerrainFeatures::queryRegionCoro(const std::string& cap_url,
										   LLUUID region_id, done_cb_t cb)
{
	LLCore::HttpOptions::ptr_t options(new LLCore::HttpOptions);
	options->setFollowRedirects(true);

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("queryRegion");

	LLSD result = adapter.getAndSuspend(cap_url, options);

	if (!gWorld.getRegionFromID(region_id))
	{
		return;	// Stale callback, region is gone... HB
	}

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);

	bool success = true;
	if (!status || !result.has("success") || !result["success"].asBoolean())
	{
		success = false;
		llwarns << "Failed to query PBR terrain features for region: "
					<< region_id;
		if (result.has("message"))
		{
			llcont << " - Error: " << result["message"];
		}
		llcont << llendl;
		if (!cb)
		{
			return;	// Nothing else to do. HB
		}
	}

	LLTerrain* terrainp = new LLTerrain();
	if (success)
	{
		const LLSD& overrides = result["overrides"];
		if (!overrides.isArray() || overrides.size() < LLTerrain::ASSET_COUNT)
		{
			success = false;
			llwarns << "Bad composition format with misssing or invalid terrain overrides data for region: "
					<< region_id << llendl;
			LL_DEBUGS("RegionTexture") << "Invalid overrides data:\n"
									   << ll_pretty_print_sd(overrides)
									   << LL_ENDL;
		}
		else
		{
			for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
			{
				const LLSD& override_llsd = overrides[i];
				// This is always the case for non-PBR terrain... HB
				if (override_llsd.isUndefined())
				{
					terrainp->setMaterialOverride(i, NULL);
					continue;
				}

				LLPointer<LLGLTFMaterial> matp = new LLGLTFMaterial();
				matp->applyOverrideLLSD(override_llsd);
				if (*matp == LLGLTFMaterial::sDefault)
				{
					matp = NULL;
				}
				terrainp->setMaterialOverride(i, matp.get());
			}
		}
	}

	if (cb && gMainloopWorkp)
	{
		gMainloopWorkp->post([=]()
							{
								cb(region_id, success, terrainp);
								delete terrainp;
							});
	}
	else
	{
		delete terrainp;
	}
}

//static
void LLPBRTerrainFeatures::modifyRegionCoro(const std::string& cap_url,
											LLUUID region_id,
											const LLSD& updates)
{
	LLCore::HttpOptions::ptr_t options(new LLCore::HttpOptions);
	options->setFollowRedirects(true);

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("modifyRegion");

	LLSD result = adapter.postAndSuspend(cap_url, updates, options);

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (!status || !result.has("success") || !result["success"].asBoolean())
	{
		llwarns << "Failed to modify PBR terrain features for region: "
					<< region_id;
		if (result.has("message"))
		{
			llcont << " - Error: " << result["message"];
		}
		llcont << llendl;
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLFloaterRegionInfo class proper
///////////////////////////////////////////////////////////////////////////////

LLUUID LLFloaterRegionInfo::sRequestInvoice;
S32 LLFloaterRegionInfo::sLastTab = 0;

LLFloaterRegionInfo::LLFloaterRegionInfo(const LLSD& seed)
:	mPanelEnvironment(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_region_info.xml",
												 NULL, false);
}

bool LLFloaterRegionInfo::postBuild()
{
	mTabs = getChild<LLTabContainer>("region_panels");

	// Construct the panels
	LLPanelRegionInfo* panelp = new LLPanelRegionGeneralInfo;
	mInfoPanels.push_back(panelp);
	LLUICtrlFactory::getInstance()->buildPanel(panelp,
											   "panel_region_general.xml");
	mTabs->addTabPanel(panelp, panelp->getLabel(), true);

	panelp = new LLPanelRegionDebugInfo;
	mInfoPanels.push_back(panelp);
	LLUICtrlFactory::getInstance()->buildPanel(panelp,
											   "panel_region_debug.xml");
	mTabs->addTabPanel(panelp, panelp->getLabel());

	panelp = new LLPanelRegionTextureInfo;
	mInfoPanels.push_back(panelp);
	LLUICtrlFactory::getInstance()->buildPanel(panelp,
											   "panel_region_texture.xml");
	mTabs->addTabPanel(panelp, panelp->getLabel());

	panelp = new LLPanelRegionTerrainInfo;
	mInfoPanels.push_back(panelp);
	LLUICtrlFactory::getInstance()->buildPanel(panelp,
											   "panel_region_terrain.xml");
	mTabs->addTabPanel(panelp, panelp->getLabel());

	panelp = new LLPanelEstateInfo;
	mInfoPanels.push_back(panelp);
	LLUICtrlFactory::getInstance()->buildPanel(panelp,
											   "panel_region_estate.xml");
	mTabs->addTabPanel(panelp, panelp->getLabel());

	panelp = new LLPanelEstateAccess;
	mInfoPanels.push_back(panelp);
	LLUICtrlFactory::getInstance()->buildPanel(panelp,
											   "panel_region_access.xml");
	mTabs->addTabPanel(panelp, panelp->getLabel());

	panelp = new LLPanelEstateCovenant;
	mInfoPanels.push_back(panelp);
	LLUICtrlFactory::getInstance()->buildPanel(panelp,
											   "panel_region_covenant.xml");
	mTabs->addTabPanel(panelp, panelp->getLabel());

	if (gAgent.hasRegionCapability("RegionExperiences"))
	{
		panelp = new LLPanelRegionExperiences;
		mInfoPanels.push_back(panelp);
		mTabs->addTabPanel(panelp, panelp->getLabel());
	}

	// Add the environment tab if needed
	if (gAgent.hasInventorySettings())
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		mPanelEnvironment =
			new HBPanelLandEnvironment(regionp ? regionp->getHandle() : 0);
		mTabs->addTabPanel(mPanelEnvironment, mPanelEnvironment->getLabel());
	}

	gMessageSystemp->setHandlerFunc(_PREHASH_EstateOwnerMessage,
									&processEstateOwnerRequest);

	if (sLastTab < mTabs->getTabCount())
	{
		mTabs->selectTab(sLastTab);
	}
	else
	{
		sLastTab = 0;
	}

	return true;
}

LLFloaterRegionInfo::~LLFloaterRegionInfo()
{
	sLastTab = mTabs->getCurrentPanelIndex();
}

void LLFloaterRegionInfo::onOpen()
{
	LLRect rect = gSavedSettings.getRect("FloaterRegionInfoRect");
	S32 left, top;
	gFloaterViewp->getNewFloaterPosition(&left, &top);
	rect.translate(left, top);

	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		refreshFromRegion(gAgent.getRegion());
		requestRegionInfo();
	}
	LLFloater::onOpen();
}

//static
void LLFloaterRegionInfo::requestRegionInfo()
{
	LLFloaterRegionInfo* self = findInstance();
	if (!self) return;

	// Disable all but Covenant panels
	LLPanel* panelp = (LLPanel*)getPanelGeneral();
	if (panelp)
	{
		panelp->setCtrlsEnabled(false);
	}
	panelp = (LLPanel*)getPanelDebug();
	if (panelp)
	{
		panelp->setCtrlsEnabled(false);
	}
	panelp = (LLPanel*)getPanelTerrain();
	if (panelp)
	{
		panelp->setCtrlsEnabled(false);
	}
	panelp = (LLPanel*)getPanelEstate();
	if (panelp)
	{
		panelp->setCtrlsEnabled(false);
	}
	panelp = (LLPanel*)getPanelAccess();
	if (panelp)
	{
		panelp->setCtrlsEnabled(false);
	}
	panelp = (LLPanel*)getPanelExperiences();
	if (panelp)
	{
		panelp->setCtrlsEnabled(false);
	}
	if (self->mPanelEnvironment)
	{
		self->mPanelEnvironment->setEnabled(false);
	}

	// Must allow anyone to request the RegionInfo data so non-owners/non-gods
	// can see the values. We therefore cannot use an EstateOwnerMessage. JC
	LLMessageSystem* msg = gMessageSystemp;
	if (msg)	// Paranoia
	{
		msg->newMessage(_PREHASH_RequestRegionInfo);
		msg->nextBlock(_PREHASH_AgentData);
		msg->addUUID(_PREHASH_AgentID, gAgentID);
		msg->addUUID(_PREHASH_SessionID, gAgentSessionID);
		gAgent.sendReliableMessage();
	}
}

//static
void LLFloaterRegionInfo::processEstateOwnerRequest(LLMessageSystem* msg,
													void**)
{
	static LLDispatcher dispatch;

	if (!findInstance()) return;

	if (!gEstateDispatchInitialized)
	{
		LLPanelEstateInfo::initDispatch(dispatch);
	}

	// Unpack the message
	std::string request;
	LLUUID invoice;
	strings_vec_t strings;
	LLDispatcher::unpackMessage(msg, request, invoice, strings);
	if (invoice != getLastInvoice())
	{
		LL_DEBUGS("RegionInfo") << "Mismatched estate message: " << request
								<< " - Invoice: " << invoice << LL_ENDL;
		return;
	}

	// Dispatch the message
	dispatch.dispatch(request, invoice, strings);

	LLPanelEstateInfo* panelp = getPanelEstate();
	if (panelp)
	{
		panelp->updateControls();
	}
}

//static
void LLFloaterRegionInfo::updateFromRegionInfo()
{
	LLFloaterRegionInfo* self = findInstance();
	if (!self) return;

	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp) return;

	bool allow_modify = gAgent.isGodlike() || regionp->canManageEstate();
	U64 region_flags = LLRegionInfoModel::sRegionFlags;

	// GENERAL PANEL
	LLPanel* panelp = (LLPanel*)getPanelGeneral();
	if (panelp)
	{
		getPanelGeneral()->refreshFromRegion(regionp);

		const LLVector3d& coords = regionp->getOriginGlobal();
		S32 x = coords[VX] / REGION_WIDTH_UNITS;
		S32 y = coords[VY] / REGION_WIDTH_UNITS;
		static std::string name_fmt = panelp->getString("name_coords_format");
		std::string sim = llformat(name_fmt.c_str(),
								   LLRegionInfoModel::sSimName.c_str(), x, y);
		panelp->childSetValue("region_text", LLSD(sim));
		panelp->childSetValue("region_type", LLSD(LLRegionInfoModel::sSimType));
		panelp->childSetValue("version_channel_text", gLastVersionChannel);

		LLSpinCtrl* spinp = panelp->getChild<LLSpinCtrl>("agent_limit_spin");
		spinp->setMaxValue(LLRegionInfoModel::sHardAgentLimit);
		spinp->setValue(LLSD((F32)LLRegionInfoModel::sAgentLimit));

		panelp->childSetValue("object_bonus_spin",
							  LLSD(LLRegionInfoModel::sObjectBonusFactor));
		panelp->childSetValue("access_combo",
							  LLSD(LLRegionInfoModel::sSimAccess));

		panelp->childSetValue("allow_land_resell_check",
							  (region_flags &
							   REGION_FLAGS_BLOCK_LAND_RESELL) == 0);
		panelp->childSetValue("allow_parcel_changes_check",
							  (region_flags &
							   REGION_FLAGS_ALLOW_PARCEL_CHANGES) != 0);
		panelp->childSetValue("block_terraform_check",
							  (region_flags &
							   REGION_FLAGS_BLOCK_TERRAFORM) != 0);
		panelp->childSetValue("block_parcel_search_check",
							  (region_flags &
							   REGION_FLAGS_BLOCK_PARCEL_SEARCH) != 0);
		panelp->childSetValue("block_fly_check",
							  (region_flags & REGION_FLAGS_BLOCK_FLY) != 0);
		panelp->childSetValue("block_fly_over_check",
							  (region_flags & REGION_FLAGS_BLOCK_FLYOVER) != 0);
		panelp->childSetValue("restrict_pushobject",
							  (region_flags &
							   REGION_FLAGS_RESTRICT_PUSHOBJECT) != 0);
		bool has_damage = region_flags & REGION_FLAGS_ALLOW_DAMAGE;
		panelp->childSetValue("allow_damage_check", has_damage);
		bool has_combat2 = LLRegionInfoModel::sSupportsCombat2;
		panelp->childSetVisible("combat_restrict_log",
								has_combat2 && has_damage);
		panelp->childSetVisible("combat_allow_damage_adjust",
								has_combat2 && has_damage);
		panelp->childSetVisible("combat_restore_health",
								has_combat2 && has_damage);
		panelp->childSetVisible("on_death_text", has_combat2 && has_damage);
		panelp->childSetVisible("combat_on_death", has_combat2 && has_damage);
		panelp->childSetVisible("combat_dps_spin", has_combat2 && has_damage);
		panelp->childSetVisible("combat_hps_spin", has_combat2 && has_damage);
		panelp->childSetVisible("combat_invuln_spin",
								has_combat2 && has_damage);
		panelp->childSetVisible("combat_damage_limit",
								has_combat2 && has_damage);
		if (has_combat2)
		{
			panelp->childSetValue("combat_restrict_log",
								  (LLRegionInfoModel::sCombatFlags &
								   REGION_COMBAT_FLAG_RESTRICT_LOG) != 0);
			panelp->childSetValue("combat_allow_damage_adjust",
								  (LLRegionInfoModel::sCombatFlags &
								   REGION_COMBAT_FLAG_DAMAGE_ADJUST) != 0);
			panelp->childSetValue("combat_restore_health",
								  (LLRegionInfoModel::sCombatFlags &
								   REGION_COMBAT_FLAG_RESTORE_HEALTH) != 0);
			panelp->childSetValue("combat_on_death",
								  LLRegionInfoModel::sOnDeath);
			panelp->childSetValue("combat_dps_spin",
								  LLRegionInfoModel::sDamageThrottle);
			panelp->childSetValue("combat_hps_spin",
								  LLRegionInfoModel::sRegenerationRate);
			panelp->childSetValue("combat_invuln_spin",
								  LLRegionInfoModel::sInvulnerabilityTime);
			panelp->childSetValue("combat_damage_limit",
								  LLRegionInfoModel::sDamageLimit);
		}

	 	// Detect teen grid for maturity
		// *TODO add field to estate table and test that
		bool teen_grid = LLRegionInfoModel::sParentEstateID == 5;
		panelp->childSetEnabled("access_combo",
								gAgent.isGodlike() ||
								(!teen_grid &&
								 regionp && regionp->canManageEstate()));
		panelp->setCtrlsEnabled(allow_modify);
	}

	// DEBUG PANEL
	panelp = (LLPanel*)getPanelDebug();
	if (panelp)
	{
		getPanelDebug()->refreshFromRegion(regionp);

		panelp->childSetValue("region_text", LLSD(LLRegionInfoModel::sSimName));
		panelp->childSetValue("disable_scripts_check",
							  LLSD((region_flags &
									REGION_FLAGS_SKIP_SCRIPTS) != 0));
		panelp->childSetValue("disable_collisions_check",
							  LLSD((region_flags &
									REGION_FLAGS_SKIP_COLLISIONS) != 0));
		panelp->childSetValue("disable_physics_check",
							  LLSD((region_flags &
								REGION_FLAGS_SKIP_PHYSICS) != 0));
		panelp->setCtrlsEnabled(allow_modify);
	}

	// TERRAIN PANEL
	panelp = (LLPanel*)getPanelTerrain();
	if (panelp)
	{
		getPanelTerrain()->refreshFromRegion(regionp);

		panelp->childSetValue("region_text", LLSD(LLRegionInfoModel::sSimName));
		panelp->childSetValue("water_height_spin",
							  LLSD(LLRegionInfoModel::sWaterHeight));
		panelp->childSetValue("terrain_raise_spin",
							  LLSD(LLRegionInfoModel::sTerrainRaiseLimit));
		panelp->childSetValue("terrain_lower_spin",
							  LLSD(LLRegionInfoModel::sTerrainLowerLimit));
		panelp->childSetValue("use_estate_sun_check",
							  LLSD(LLRegionInfoModel::sUseEstateSun));

		panelp->childSetValue("fixed_sun_check",
							  LLSD((region_flags &
									REGION_FLAGS_SUN_FIXED) != 0));
		panelp->childSetEnabled("fixed_sun_check",
							    allow_modify &&
							    !LLRegionInfoModel::sUseEstateSun);
		panelp->childSetValue("sun_hour_slider",
							  LLSD(LLRegionInfoModel::sSunHour));
		panelp->childSetEnabled("sun_hour_slider",
							    allow_modify &&
							    !LLRegionInfoModel::sUseEstateSun);
		panelp->setCtrlsEnabled(allow_modify);
	}

	// ESTATE PANEL
	LLPanelEstateInfo* estate_panelp = getPanelEstate();
	if (estate_panelp)
	{
		estate_panelp->refreshFromRegion(regionp);

		estate_panelp->setEstateName(LLEstateInfoModel::sEstateName);
		estate_panelp->setEstateID(LLEstateInfoModel::sEstateId);
		// Update estate owner name in UI
		if (gCacheNamep)
		{
			gCacheNamep->get(LLEstateInfoModel::sOwnerId, false,
							 LLPanelEstateInfo::callbackCacheName);
		}
		estate_panelp->setEstateFlags(LLEstateInfoModel::sEstateFlags);

		if (LLEstateInfoModel::sSunHour == 0 &&
			(LLEstateInfoModel::sEstateFlags & REGION_FLAGS_SUN_FIXED) == 0)
		{
			estate_panelp->setGlobalTime(true);
		}
		else
		{
			estate_panelp->setGlobalTime(false);
			estate_panelp->setSunHour(LLEstateInfoModel::sSunHour);
		}
	}

	self->refreshFromRegion(regionp);

	// *TODO: this frequently results in one more request than we need. It is
	// not breaking, but should be nicer. We need to know new env version to
	// fix this, without it we can only do full re-request. They happens on
	// updates, on opening LLFloaterRegionInfo, on region crossing if info
	// floater is open.
	LLEnvironment::requestRegion();
}

//static
LLPanelRegionGeneralInfo* LLFloaterRegionInfo::getPanelGeneral()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	if (!self)
	{
		return NULL;
	}
	LLPanel* panelp = self->mTabs->getChild<LLPanel>("General", true, false);
	return (LLPanelRegionGeneralInfo*)panelp;
}

//static
LLPanelRegionDebugInfo* LLFloaterRegionInfo::getPanelDebug()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	if (!self)
	{
		return NULL;
	}
	LLPanel* panelp = self->mTabs->getChild<LLPanel>("Debug", true, false);
	return (LLPanelRegionDebugInfo*)panelp;
}

//static
LLPanelEstateInfo* LLFloaterRegionInfo::getPanelEstate()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	if (!self)
	{
		return NULL;
	}
	LLPanel* panelp = self->mTabs->getChild<LLPanel>("Estate", true, false);
	return (LLPanelEstateInfo*)panelp;
}

//static
LLPanelEstateAccess* LLFloaterRegionInfo::getPanelAccess()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	if (!self)
	{
		return NULL;
	}
	LLPanel* panelp = self->mTabs->getChild<LLPanel>("Access", true, false);
	return (LLPanelEstateAccess*)panelp;
}

//static
LLPanelEstateCovenant* LLFloaterRegionInfo::getPanelCovenant()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	if (!self)
	{
		return NULL;
	}
	LLPanel* panelp = self->mTabs->getChild<LLPanel>("Covenant", true, false);
	return (LLPanelEstateCovenant*)panelp;
}

//static
LLPanelRegionTerrainInfo* LLFloaterRegionInfo::getPanelTerrain()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	if (!self)
	{
		return NULL;
	}
	LLPanel* panelp = self->mTabs->getChild<LLPanel>("Terrain", true, false);
	return (LLPanelRegionTerrainInfo*)panelp;
}

//static
LLPanelRegionExperiences* LLFloaterRegionInfo::getPanelExperiences()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	if (!self)
	{
		return NULL;
	}
	LLPanel* panelp = self->mTabs->getChild<LLPanel>("Experiences", true,
													 false);
	return (LLPanelRegionExperiences*)panelp;
}

//static
HBPanelLandEnvironment* LLFloaterRegionInfo::getPanelEnvironment()
{
	LLFloaterRegionInfo* self = LLFloaterRegionInfo::findInstance();
	return self ? self->mPanelEnvironment : NULL;
}

void LLFloaterRegionInfo::refreshFromRegion(LLViewerRegion* regionp)
{
	if (!regionp) return;

	// Call refresh from region on all panels
	for (S32 i = 0, count = mInfoPanels.size(); i < count; ++i)
	{
		LLPanelRegionInfo* panelp = mInfoPanels[i];
		panelp->refreshFromRegion(regionp);
	}

	if (mPanelEnvironment)
	{
		mPanelEnvironment->setRegionHandle(regionp->getHandle());
	}
}

void LLFloaterRegionInfo::refresh()
{
	for (info_panels_t::iterator iter = mInfoPanels.begin();
		 iter != mInfoPanels.end(); ++iter)
	{
		(*iter)->refresh();
	}
	if (mPanelEnvironment)
	{
		mPanelEnvironment->refresh();
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelRegionInfo
///////////////////////////////////////////////////////////////////////////////

LLPanelRegionInfo::LLPanelRegionInfo()
:	LLPanel(),
	mComposition(NULL),
	mApplyBtn(NULL),
	mOwner(false),
	mCanManage(false),
	mEstateManager(false)
{
}

//virtual
bool LLPanelRegionInfo::postBuild()
{
	mApplyBtn = getChild<LLButton>("apply_btn", true, false);
	if (mApplyBtn)
	{
		mApplyBtn->setClickedCallback(onBtnSet, this);
		mApplyBtn->setEnabled(false);
	}

	refresh();

	return true;
}

//virtual
bool LLPanelRegionInfo::refreshFromRegion(LLViewerRegion* regionp)
{
	if (!regionp)
	{
		mComposition = NULL;
		mOwner = mCanManage = mEstateManager = false;
		return false;
	}
	mComposition = regionp->getComposition();
	mHost = regionp->getHost();
	mOwner = regionp->getOwner() == gAgentID;
	mCanManage = regionp->canManageEstate();
	mEstateManager = regionp->isEstateManager();
	return true;
}

void LLPanelRegionInfo::enableApplyBtn(bool enable)
{
	if (mApplyBtn)
	{
		mApplyBtn->setEnabled(enable);
	}
}

void LLPanelRegionInfo::disableApplyBtn()
{
	if (mApplyBtn)
	{
		mApplyBtn->setEnabled(false);
	}
}

void LLPanelRegionInfo::initCtrl(const char* name)
{
	childSetCommitCallback(name, onChangeAnything, this);
}

void LLPanelRegionInfo::initHelpBtn(const char* name,
									const std::string& xml_alert)
{
	childSetAction(name, onClickHelp, new std::string(xml_alert));
}

void LLPanelRegionInfo::sendEstateOwnerMessage(const std::string& request,
											   const strings_vec_t& strings)
{
	LLMessageSystem* msg = gMessageSystemp;
	if (!msg) return;	// Paranoia

	llinfos << "Sending estate request '" << request << "' - Invoice: "
			<< LLFloaterRegionInfo::getLastInvoice() << llendl;
	msg->newMessage(_PREHASH_EstateOwnerMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); // Not used
	msg->nextBlock(_PREHASH_MethodData);
	msg->addString(_PREHASH_Method, request);
	msg->addUUID(_PREHASH_Invoice, LLFloaterRegionInfo::getLastInvoice());
	if (strings.empty())
	{
		msg->nextBlock(_PREHASH_ParamList);
		msg->addString(_PREHASH_Parameter, NULL);
	}
	else
	{
		strings_vec_t::const_iterator it = strings.begin();
		strings_vec_t::const_iterator end = strings.end();
		for ( ; it != end; ++it)
		{
			msg->nextBlock(_PREHASH_ParamList);
			msg->addString(_PREHASH_Parameter, *it);
		}
	}
	msg->sendReliable(mHost);
}

//static
void LLPanelRegionInfo::onBtnSet(void* user_data)
{
	LLPanelRegionInfo* panelp = (LLPanelRegionInfo*)user_data;
	if (panelp && panelp->sendUpdate())
	{
		panelp->disableApplyBtn();
	}
}

// Enables the "Apply" button if it is not already enabled
//static
void LLPanelRegionInfo::onChangeAnything(LLUICtrl*, void* user_data)
{
	LLPanelRegionInfo* panelp = (LLPanelRegionInfo*)user_data;
	if (panelp)
	{
		panelp->enableApplyBtn();
		panelp->refresh();
	}
}

//static
void LLPanelRegionInfo::onClickHelp(void* data)
{
	std::string* xml_alert = (std::string*)data;
	gNotifications.add(*xml_alert);
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelRegionGeneralInfo
///////////////////////////////////////////////////////////////////////////////

//virtual
bool LLPanelRegionGeneralInfo::refreshFromRegion(LLViewerRegion* regionp)
{
	LLPanelRegionInfo::refreshFromRegion(regionp);
	bool allow_modify = mCanManage || gAgent.isGodlike();

	setCtrlsEnabled(allow_modify);
	disableApplyBtn();
	childSetEnabled("access_text", allow_modify);
	// childSetEnabled("access_combo", allow_modify);
	// now set in processRegionInfo for teen grid detection
	childSetEnabled("kick_btn", allow_modify);
	childSetEnabled("kick_all_btn", allow_modify);
	childSetEnabled("im_btn", allow_modify);
	childSetEnabled("manage_telehub_btn", allow_modify);

	// Data gets filled in by processRegionInfo

	return true;
}

bool LLPanelRegionGeneralInfo::postBuild()
{
	// Enable the "Apply" button if something is changed. JC
	initCtrl("access_combo");
	initCtrl("agent_limit_spin");
	initCtrl("object_bonus_spin");
	initCtrl("allow_land_resell_check");
	initCtrl("allow_parcel_changes_check");
	initCtrl("block_terraform_check");
	initCtrl("block_parcel_search_check");
	initCtrl("block_fly_check");
	initCtrl("block_fly_over_check");
	initCtrl("restrict_pushobject");
	initCtrl("combat_restrict_log");
	initCtrl("combat_damage_limit");
	initCtrl("combat_allow_damage_adjust");
	initCtrl("combat_restore_health");
	initCtrl("combat_on_death");
	initCtrl("combat_dps_spin");
	initCtrl("combat_hps_spin");
	initCtrl("combat_invuln_spin");
	// Special callback to enable combat 2 related controls
	childSetCommitCallback("allow_damage_check", onChangeCombat, this);

	initHelpBtn("object_bonus_help", "HelpRegionObjectBonus");
	initHelpBtn("access_help", "HelpRegionMaturity");
	initHelpBtn("parcel_changes_help", "HelpParcelChanges");

	childSetAction("kick_btn", onClickKick, this);
	childSetAction("kick_all_btn", onClickKickAll, this);
	childSetAction("im_btn", onClickMessage, this);
	childSetAction("manage_telehub_btn", onClickManageTelehub, this);

	return LLPanelRegionInfo::postBuild();
}

//static
void LLPanelRegionGeneralInfo::onChangeCombat(LLUICtrl* ctrlp, void* userdata)
{
	LLPanelRegionGeneralInfo* panelp = (LLPanelRegionGeneralInfo*)userdata;
	if (!panelp || !ctrlp) return;

	bool enabled = ctrlp->getValue().asBoolean() &&
					 LLRegionInfoModel::sSupportsCombat2;
	panelp->childSetVisible("combat_restrict_log", enabled);
	panelp->childSetVisible("combat_allow_damage_adjust", enabled);
	panelp->childSetVisible("combat_restore_health", enabled);
	panelp->childSetVisible("on_death_text", enabled);
	panelp->childSetVisible("combat_on_death", enabled);
	panelp->childSetVisible("combat_dps_spin", enabled);
	panelp->childSetVisible("combat_hps_spin", enabled);
	panelp->childSetVisible("combat_invuln_spin", enabled);
	panelp->childSetVisible("combat_damage_limit", enabled);

	onChangeAnything(NULL, panelp);
}

//static
void LLPanelRegionGeneralInfo::onClickKick(void* userdata)
{
	LLPanelRegionGeneralInfo* panelp = (LLPanelRegionGeneralInfo*)userdata;
	if (!panelp) return;

	LLFloater* childp = LLFloaterAvatarPicker::show(onKickCommit, userdata,
													false, true);
	if (childp && gFloaterViewp)
	{
		// This depends on the grandparent view being a floater in order to set
		// up floater dependency
		LLFloater* parentp = gFloaterViewp->getParentFloater(panelp);
		if (parentp)
		{
			parentp->addDependentFloater(childp);
		}
	}
}

//static
void LLPanelRegionGeneralInfo::onKickCommit(const std::vector<std::string>& names,
											const uuid_vec_t& ids,
											void* userdata)
{
	LLPanelRegionGeneralInfo* self = (LLPanelRegionGeneralInfo*)userdata;
	if (self && !names.empty() && !ids.empty() && ids[0].notNull())
	{
		strings_vec_t strings;
		// [0] = our agent id
		// [1] = target agent id
		std::string buffer;
		gAgentID.toString(buffer);
		strings.push_back(buffer);

		ids[0].toString(buffer);
		strings.push_back(strings_vec_t::value_type(buffer));

		self->sendEstateOwnerMessage("teleporthomeuser", strings);
	}
}

//static
void LLPanelRegionGeneralInfo::onClickKickAll(void* userdata)
{
	gNotifications.add("KickUsersFromRegion", LLSD(), LLSD(),
					   std::bind(&LLPanelRegionGeneralInfo::onKickAllCommit,
								 (LLPanelRegionGeneralInfo*)userdata, _1, _2));
}

bool LLPanelRegionGeneralInfo::onKickAllCommit(const LLSD& notification,
											   const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		strings_vec_t strings;
		// [0] = our agent id
		std::string buffer;
		gAgentID.toString(buffer);
		strings.push_back(buffer);

		// Historical message name
		sendEstateOwnerMessage("teleporthomeallusers", strings);
	}
	return false;
}

//static
void LLPanelRegionGeneralInfo::onClickMessage(void* userdata)
{
	gNotifications.add("MessageRegion", LLSD(), LLSD(),
					   std::bind(&LLPanelRegionGeneralInfo::onMessageCommit,
								 (LLPanelRegionGeneralInfo*)userdata, _1, _2));
}

//static
bool LLPanelRegionGeneralInfo::onMessageCommit(const LLSD& notification,
											   const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) != 0)
	{
		return false;
	}

	std::string text = response["message"].asString();
	if (text.empty()) return false;

	llinfos << "Message to everyone: " << text << llendl;
	strings_vec_t strings;
	// [0] grid_x, unused here
	// [1] grid_y, unused here
	// [2] agent_id of sender
	// [3] sender name
	// [4] message
	strings.emplace_back("-1");
	strings.emplace_back("-1");
	std::string buffer;
	gAgentID.toString(buffer);
	strings.emplace_back(buffer);
	std::string name;
	gAgent.buildFullname(name);
	strings.emplace_back(name);
	strings.emplace_back(text);
	sendEstateOwnerMessage("simulatormessage", strings);
	return false;
}

//static
void LLPanelRegionGeneralInfo::onClickManageTelehub(void* data)
{
	LLFloaterTelehub::showInstance();
	LLFloaterRegionInfo::getInstance()->close();
}

#define YESORNO(C) llformat("%s", childGetValue(C).asBoolean() ? "Y" : "N")
#define ASFLOAT(C) llformat("%f", childGetValue(C).asReal())
#define ASINTEGER(C) llformat("%d", childGetValue(C).asInteger())

// setregioninfo
// strings[0] = 'Y' - block terraform, 'N' - not
// strings[1] = 'Y' - block fly, 'N' - not
// strings[2] = 'Y' - allow damage, 'N' - not
// strings[3] = 'Y' - allow land sale, 'N' - not
// strings[4] = agent limit
// strings[5] = object bonus
// strings[6] = sim access (0 = unknown, 13 = PG, 21 = Mature, 42 = Adult)
// strings[7] = restrict pushobject
// strings[8] = 'Y' - allow parcel subdivide, 'N' - not
// strings[9] = 'Y' - block parcel search, 'N' - allow
bool LLPanelRegionGeneralInfo::sendUpdate()
{
	// First try using a Cap. If that fails use the old method.
	LLSD body;
	std::string url = gAgent.getRegionCapability("DispatchRegionInfo");
	if (!url.empty())
	{
		body["block_terraform"] = childGetValue("block_terraform_check");
		body["block_fly"] = childGetValue("block_fly_check");
		body["block_fly_over"] = childGetValue("block_fly_over_check");
		body["allow_damage"] = childGetValue("allow_damage_check");
		body["allow_land_resell"] = childGetValue("allow_land_resell_check");
		body["agent_limit"] = childGetValue("agent_limit_spin");
		body["prim_bonus"] = childGetValue("object_bonus_spin");
		body["sim_access"] = childGetValue("access_combo");
		body["restrict_pushobject"] = childGetValue("restrict_pushobject");
		body["allow_parcel_changes"] =
			childGetValue("allow_parcel_changes_check");
		body["block_parcel_search"] =
			childGetValue("block_parcel_search_check");
		using namespace LLCoreHttpUtil;
		HttpCoroutineAdapter::messageHttpPost(url, body,
											  "Region info update posted.",
											  "Failure to post region info update.");
		if (LLRegionInfoModel::sSupportsCombat2 &&
			childGetValue("allow_damage_check").asBoolean())
		{
			U32 combat_flags = 0;
			if (childGetValue("combat_restrict_log").asBoolean())
			{
				combat_flags |= REGION_COMBAT_FLAG_RESTRICT_LOG;
			}
			if (childGetValue("combat_allow_damage_adjust").asBoolean())
			{
				combat_flags |= REGION_COMBAT_FLAG_DAMAGE_ADJUST;
			}
			if (childGetValue("combat_restore_health").asBoolean())
			{
				combat_flags |= REGION_COMBAT_FLAG_RESTORE_HEALTH;
			}
			body["combat_flags"] = LLSD::Integer(combat_flags);
			body["combat_on_death"] = childGetValue("combat_on_death");
			body["combat_dps_throttle"] = childGetValue("combat_dps_spin");
			body["combat_hps_rate"] = childGetValue("combat_hps_spin");
			body["combat_invuln_time"] = childGetValue("combat_invuln_spin");
			body["combat_damage_limit"] = childGetValue("combat_damage_limit");
		}
	}
	else
	{
		strings_vec_t strings;
		strings.emplace_back(YESORNO("block_terraform_check"));
		strings.emplace_back(YESORNO("block_fly_check"));
		strings.emplace_back(YESORNO("allow_damage_check"));
		strings.emplace_back(YESORNO("allow_land_resell_check"));
		strings.emplace_back(ASFLOAT("agent_limit_spin"));
		strings.emplace_back(ASFLOAT("object_bonus_spin"));
		strings.emplace_back(ASINTEGER("access_combo"));
		strings.emplace_back(YESORNO("restrict_pushobject"));
		strings.emplace_back(YESORNO("allow_parcel_changes_check"));
		sendEstateOwnerMessage("setregioninfo", strings);
	}

	// If we changed access levels, tell user about it
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp &&
		childGetValue("access_combo").asInteger() != regionp->getSimAccess())
	{
		gNotifications.add("RegionMaturityChange");
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelRegionDebugInfo
///////////////////////////////////////////////////////////////////////////////

bool LLPanelRegionDebugInfo::postBuild()
{
	LLPanelRegionInfo::postBuild();
	initCtrl("disable_scripts_check");
	initCtrl("disable_collisions_check");
	initCtrl("disable_physics_check");

	initHelpBtn("disable_scripts_help", "HelpRegionDisableScripts");
	initHelpBtn("disable_collisions_help", "HelpRegionDisableCollisions");
	initHelpBtn("disable_physics_help", "HelpRegionDisablePhysics");
	initHelpBtn("top_colliders_help", "HelpRegionTopColliders");
	initHelpBtn("top_scripts_help", "HelpRegionTopScripts");
	initHelpBtn("restart_help", "HelpRegionRestart");

	childSetAction("choose_avatar_btn", onClickChooseAvatar, this);
	childSetAction("return_btn", onClickReturn, this);
	childSetAction("top_colliders_btn", onClickTopColliders, this);
	childSetAction("top_scripts_btn", onClickTopScripts, this);
	childSetAction("restart_btn", onClickRestart, this);
	childSetAction("cancel_restart_btn", onClickCancelRestart, this);

	return true;
}

//virtual
bool LLPanelRegionDebugInfo::refreshFromRegion(LLViewerRegion* regionp)
{
	LLPanelRegionInfo::refreshFromRegion(regionp);
	bool allow_modify = mCanManage || gAgent.isGodlike();

	bool got_target_avatar = mTargetAvatar.notNull();

	setCtrlsEnabled(allow_modify);
	disableApplyBtn();
	childDisable("target_avatar_name");

	childSetEnabled("choose_avatar_btn", allow_modify);
	childSetEnabled("return_scripts", allow_modify && got_target_avatar);
	childSetEnabled("return_other_land", allow_modify && got_target_avatar);
	childSetEnabled("return_estate_wide", allow_modify && got_target_avatar);
	childSetEnabled("return_btn", allow_modify && got_target_avatar);
	childSetEnabled("top_colliders_btn", allow_modify);
	childSetEnabled("top_scripts_btn", allow_modify);
	childSetEnabled("restart_btn", allow_modify);
	childSetEnabled("cancel_restart_btn", allow_modify);

	return true;
}

//virtual
bool LLPanelRegionDebugInfo::sendUpdate()
{
	strings_vec_t strings;
	strings.emplace_back(YESORNO("disable_scripts_check"));
	strings.emplace_back(YESORNO("disable_collisions_check"));
	strings.emplace_back(YESORNO("disable_physics_check"));
	sendEstateOwnerMessage("setregiondebug", strings);
	return true;
}

void LLPanelRegionDebugInfo::onClickChooseAvatar(void* data)
{
	LLFloaterAvatarPicker::show(callbackAvatarID, data, false, true);
}

//static
void LLPanelRegionDebugInfo::callbackAvatarID(const std::vector<std::string>& names,
											  const uuid_vec_t& ids,
											  void* data)
{
	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
	if (self && !ids.empty() && !names.empty())
	{
		self->mTargetAvatar = ids[0];
		self->childSetValue("target_avatar_name", LLSD(names[0]));
		self->refreshFromRegion(gAgent.getRegion());
	}
}

//static
void LLPanelRegionDebugInfo::onClickReturn(void* data)
{
	LLPanelRegionDebugInfo* panelp = (LLPanelRegionDebugInfo*)data;
	if (!panelp || panelp->mTargetAvatar.isNull()) return;

	LLSD args;
	args["USER_NAME"] = panelp->childGetValue("target_avatar_name").asString();
	LLSD payload;
	payload["avatar_id"] = panelp->mTargetAvatar;

	U32 flags = SWD_ALWAYS_RETURN_OBJECTS;

	if (panelp->childGetValue("return_scripts").asBoolean())
	{
		flags |= SWD_SCRIPTED_ONLY;
	}

	if (panelp->childGetValue("return_other_land").asBoolean())
	{
		flags |= SWD_OTHERS_LAND_ONLY;
	}
	payload["flags"] = LLSD::Integer(flags);
	payload["return_estate_wide"] =
		panelp->childGetValue("return_estate_wide").asBoolean();
	gNotifications.add("EstateObjectReturn", args, payload,
					   std::bind(&LLPanelRegionDebugInfo::callbackReturn,
								 panelp, _1, _2));
}

bool LLPanelRegionDebugInfo::callbackReturn(const LLSD& notification,
											const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response))
	{
		return false;
	}

	LLUUID target_avatar = notification["payload"]["avatar_id"].asUUID();
	if (target_avatar.notNull())
	{
		U32 flags = notification["payload"]["flags"].asInteger();
		bool return_estate_wide = notification["payload"]["return_estate_wide"];
		if (return_estate_wide)
		{
			// Send as estate message - routed by spaceserver to all regions in
			// estate
			strings_vec_t strings;
			strings.emplace_back(llformat("%d", flags));
			strings.emplace_back(target_avatar.asString());
			sendEstateOwnerMessage("estateobjectreturn", strings);
		}
		else
		{
			// Send to this simulator only
  			send_sim_wide_deletes(target_avatar, flags);
  		}
	}
	return false;
}

//static
void LLPanelRegionDebugInfo::onClickTopColliders(void* data)
{
	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
	if (!self) return;

	strings_vec_t strings;
	strings.emplace_back("1");	// one physics step
	LLFloaterTopObjects::showInstance();
	LLFloaterTopObjects::clearList();
	self->sendEstateOwnerMessage("colliders", strings);
}

//static
void LLPanelRegionDebugInfo::onClickTopScripts(void* data)
{
	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
	if (!self) return;

	strings_vec_t strings;
	strings.emplace_back("6");	// top 5 scripts
	LLFloaterTopObjects::showInstance();
	LLFloaterTopObjects::clearList();
	self->sendEstateOwnerMessage("scripts", strings);
}

//static
void LLPanelRegionDebugInfo::onClickRestart(void* data)
{
	gNotifications.add("ConfirmRestart", LLSD(), LLSD(),
					   std::bind(&LLPanelRegionDebugInfo::callbackRestart,
								 (LLPanelRegionDebugInfo*)data, _1, _2));
}

bool LLPanelRegionDebugInfo::callbackRestart(const LLSD& notification,
											 const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		strings_vec_t strings;
		strings.emplace_back("120");
		sendEstateOwnerMessage("restart", strings);
	}
	return false;
}

//static
void LLPanelRegionDebugInfo::onClickCancelRestart(void* data)
{
	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
	if (!self) return;

	strings_vec_t strings;
	strings.emplace_back("-1");
	self->sendEstateOwnerMessage("restart", strings);
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelRegionTextureInfo
///////////////////////////////////////////////////////////////////////////////

LLPanelRegionTextureInfo::LLPanelRegionTextureInfo()
:	mHasTerrainTransforms(false)
{
}

//virtual
LLPanelRegionTextureInfo::~LLPanelRegionTextureInfo()
{
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		clearMaterialPreview(mMatAssetIds[i]);
	}
}

//virtual
bool LLPanelRegionTextureInfo::postBuild()
{
	mTerrainTypeCtrl = getChild<LLRadioGroup>("terrain_type");
	mTerrainTypeCtrl->setCommitCallback(onTerrainTypeCommit);
	mTerrainTypeCtrl->setCallbackUserData(this);

	// For each view border enclosing our future material preview, get the
	// corresponding rectangle and setup a clickable text box in that rectangle
	// to that we can catch the user clicks and open the corresponding material
	// selector. We also store the texture pickers pointers in an array. HB
	std::string buffer;
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		buffer = llformat("material_detail_%d", i);
		mMatBorders[i] = getChild<LLView>(buffer.c_str());
		LLRect rect = mMatBorders[i]->getRect();
		// Adjust to keep the view border showing while we will draw the
		// thumbnail inside it, over the clickable text box.
		rect.mBottom += 2;
		rect.mTop -= 2;
		rect.mLeft += 2;
		rect.mRight -= 2;
		mMatClickTexts[i] = new LLTextBox(llformat("%d", i), rect, "",
										  LLFontGL::getFontSansSerifSmall(),
										  true); // Opaque text box
		mMatClickTexts[i]->setColor(LLColor4::white);
		mThumbnailRect[i] = rect;
		// Add it as our child.
		addChild(mMatClickTexts[i]);
		// Add our clicked callback.
		mMatClickTexts[i]->setClickedCallback(onMatPreviewClicked,
											  (void*)mMatClickTexts[i]);
		// Keep texture pickers and spinners in arrays, to avoid calling again
		// getChild() with their name...
		buffer = llformat("texture_detail_%d", i);
		mTexPickers[i] = getChild<LLTextureCtrl>(buffer.c_str());
		buffer = llformat("u_scale_detail_%d", i);
		mMaterialScaleUCtrl[i] = getChild<LLSpinCtrl>(buffer.c_str());
		mMaterialScaleUCtrl[i]->setCommitCallback(onChangeAnything);
		mMaterialScaleUCtrl[i]->setCallbackUserData(this);
		buffer = llformat("v_scale_detail_%d", i);
		mMaterialScaleVCtrl[i] = getChild<LLSpinCtrl>(buffer.c_str());
		mMaterialScaleVCtrl[i]->setCommitCallback(onChangeAnything);
		mMaterialScaleVCtrl[i]->setCallbackUserData(this);
		buffer = llformat("u_offset_detail_%d", i);
		mMaterialOffsetUCtrl[i] = getChild<LLSpinCtrl>(buffer.c_str());
		mMaterialOffsetUCtrl[i]->setCommitCallback(onChangeAnything);
		mMaterialOffsetUCtrl[i]->setCallbackUserData(this);
		buffer = llformat("v_offset_detail_%d", i);
		mMaterialOffsetVCtrl[i] = getChild<LLSpinCtrl>(buffer.c_str());
		mMaterialOffsetVCtrl[i]->setCommitCallback(onChangeAnything);
		mMaterialOffsetVCtrl[i]->setCallbackUserData(this);
		buffer = llformat("rotation_detail_%d", i);
		mMaterialRotationCtrl[i] = getChild<LLSpinCtrl>(buffer.c_str());
		mMaterialRotationCtrl[i]->setCommitCallback(onChangeAnything);
		mMaterialRotationCtrl[i]->setCallbackUserData(this);
	}

	mStatusStrings[NEEDS_PBR_MODE] = getString("needs_pbr_text");
	mStatusStrings[NEEDS_PROBES] = getString("needs_probes_text");
	mStatusStrings[NO_PBR_MAT] = getString("no_mat_text");
	mStatusStrings[LOADING_PBR_MAT] = getString("loading_mat_text");
	mStatusStrings[FAILED_PBR_MAT] = getString("failed_mat_text");

	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		buffer = llformat("texture_detail_%d", i);
		initCtrl(buffer.c_str());
	}

	for (U32 i = 0; i < CORNER_COUNT; ++i)
	{
		buffer = llformat("height_start_spin_%d", i);
		initCtrl(buffer.c_str());
		buffer = llformat("height_range_spin_%d", i);
		initCtrl(buffer.c_str());
	}

	return LLPanelRegionInfo::postBuild();
}

// This is for now only used to draw the material preview thumbnails, when
// needed and possible. HB
//virtual
void LLPanelRegionTextureInfo::draw()
{
	if (!mComposition || mTerrainTypeCtrl->getSelectedIndex() == 0)
	{
		// No region set or textures-based terrain: no material preview to
		// render; just draw the UI elements. HB
		LLPanel::draw();
		return;
	}

	U32 status = STATUS_COUNT;
	if (!gUsePBRShaders)
	{
		status = NEEDS_PBR_MODE;
	}
	else if (!LLPipeline::sReflectionProbesEnabled)
	{
		status = NEEDS_PROBES;
	}

	// Update the text in the preview tiles and see if preview are ready. HB
	LLPointer<LLViewerTexture> previews[LLTerrain::ASSET_COUNT];
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		if (status > NEEDS_PROBES)
		{
			const LLUUID& mat_id = mMatAssetIds[i];
			LLFetchedGLTFMaterial* matp = NULL;
			if (mat_id.notNull())
			{
				matp = gGLTFMaterialList.getMaterial(mat_id);
			}
			if (!matp)
			{
				status = NO_PBR_MAT;
			}
			else if (matp->isFetching())
			{
				status = LOADING_PBR_MAT;
			}
			else if (!matp->isLoaded())
			{
				status = FAILED_PBR_MAT;
			}
			else
			{
				status = NTR;
				// Get the preview for this material.
				previews[i] = matp->getPreview();
				if (previews[i].isNull())	// Preview not ready yet...
				{
					status = LOADING_PBR_MAT;
				}
			}
		}
		mMatClickTexts[i]->setText(mStatusStrings[status]);
	}

	// Draw all UI elements before we would draw the previews(may be). HB
	LLPanel::draw();
	if (status <= NEEDS_PROBES)
	{
		return;	// Cannot render material previews, so skip the rest. HB
	}

	// Draw the previews which are ready.
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		if (previews[i].notNull())
		{
			LLRect rect = mThumbnailRect[i];
			rect.translate(0, 154);	// *BUG: why do I need this offset ?  O.O
			gl_draw_scaled_image(rect.mLeft, rect.mBottom, rect.getWidth(),
								 rect.getHeight(), previews[i]);
		}
	}
}

void LLPanelRegionTextureInfo::clearMaterialPreview(const LLUUID& mat_id)
{
	if (mat_id.isNull())
	{
		return;
	}
	LLFetchedGLTFMaterial* matp = gGLTFMaterialList.getMaterial(mat_id);
	if (matp)
	{
		matp->clearPreview();
	}
}

//virtual
bool LLPanelRegionTextureInfo::refreshFromRegion(LLViewerRegion* regionp)
{
	LLPanelRegionInfo::refreshFromRegion(regionp);
	bool allow_modify = mCanManage || gAgent.isGodlike();

	setCtrlsEnabled(allow_modify);
	disableApplyBtn();

	if (!mComposition)	// No region set !
	{
		childSetValue("region_text", LLSD(""));
		return false;
	}

	childSetValue("region_text", LLSD(regionp->getName()));

	mHasTerrainTransforms = regionp->hasPBRTerrainTransforms();

	bool is_pbr = mComposition->isPBR();
	mTerrainTypeCtrl->setSelectedIndex(is_pbr ? 1 : 0);
	mTerrainTypeCtrl->setEnabled(regionp->hasPBRTerrain());

	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		mTexPickers[i]->setVisible(!is_pbr);
		mMatBorders[i]->setVisible(is_pbr);
		mMatClickTexts[i]->setVisible(is_pbr);
		mMaterialScaleUCtrl[i]->setVisible(is_pbr && mHasTerrainTransforms);
		mMaterialScaleVCtrl[i]->setVisible(is_pbr && mHasTerrainTransforms);
		mMaterialOffsetUCtrl[i]->setVisible(is_pbr && mHasTerrainTransforms);
		mMaterialOffsetVCtrl[i]->setVisible(is_pbr && mHasTerrainTransforms);
		mMaterialRotationCtrl[i]->setVisible(is_pbr && mHasTerrainTransforms);
		const LLUUID& asset_id = mComposition->getDetailAssetID(i);
		if (is_pbr)
		{
			LL_DEBUGS("RegionTexture") << "Detail material " << i << ": "
									   << asset_id << LL_ENDL;
			// Clear any old preview.
			clearMaterialPreview(mMatAssetIds[i]);
			// Assign the material.
			mMatAssetIds[i] = asset_id;
		}
		else
		{
			LL_DEBUGS("RegionTexture") << "Detail texture " << i << ": "
									   << asset_id << LL_ENDL;
			mTexPickers[i]->setImageAssetID(asset_id);
		}
		if (!is_pbr)
		{
			continue;
		}
		const LLGLTFMaterial* omatp = mComposition->getMaterialOverride(i);
		if (!omatp)
		{
			omatp = &LLGLTFMaterial::sDefault;
		}
		// Note: we assume all texture transforms have the same value.
		const auto& tfr = omatp->mTextureTransform[BASECOLIDX];
		mMaterialScaleUCtrl[i]->setValue(tfr.mScale.mV[VX]);
		mMaterialScaleVCtrl[i]->setValue(tfr.mScale.mV[VY]);
		mMaterialOffsetUCtrl[i]->setValue(tfr.mOffset.mV[VX]);
		mMaterialOffsetVCtrl[i]->setValue(tfr.mOffset.mV[VY]);
		mMaterialRotationCtrl[i]->setValue(tfr.mRotation * RAD_TO_DEG);
	}

	std::string buffer;
	for (U32 i = 0; i < CORNER_COUNT; ++i)
    {
		buffer = llformat("height_start_spin_%d", i);
		childSetValue(buffer.c_str(), LLSD(mComposition->getStartHeight(i)));
		buffer = llformat("height_range_spin_%d", i);
		childSetValue(buffer.c_str(), LLSD(mComposition->getHeightRange(i)));
	}

	return true;
}

//virtual
bool LLPanelRegionTextureInfo::sendUpdate()
{
	bool is_pbr = mTerrainTypeCtrl->getSelectedIndex() == 1;

	// Make sure user has not chosen wacky assets.
	if ((!is_pbr && !validateTextureSizes()) ||
		(is_pbr && !validateMaterials()))
	{
		return false;
	}

	strings_vec_t strings;

	std::string buffer;
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		if (is_pbr)
		{
			mMatAssetIds[i].toString(buffer);
		}
		else
		{
			const LLUUID& tex_id = mTexPickers[i]->getImageAssetID();
			tex_id.toString(buffer);
		}
		strings.emplace_back(llformat("%d %s", i, buffer.c_str()));
		LL_DEBUGS("RegionInfo") << "Terrain layer " << i + 1 << ", using "
								<< (is_pbr ? "material" : "texture")
								<< " Id: " << buffer << LL_ENDL;
	}
	sendEstateOwnerMessage("texturedetail", strings);

	strings.clear();
	std::string buffer2, buffer3;
	for (U32 i = 0; i < CORNER_COUNT; ++i)
	{
		buffer = llformat("height_start_spin_%d", i);
		buffer2 = llformat("height_range_spin_%d", i);
		buffer3 = llformat("%d %f %f", i,
						   (F32)childGetValue(buffer.c_str()).asReal(),
						   (F32)childGetValue(buffer2.c_str()).asReal());
		strings.emplace_back(buffer3);
	}
	sendEstateOwnerMessage("textureheights", strings);
	strings.clear();
	sendEstateOwnerMessage("texturecommit", strings);

	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp && is_pbr && mHasTerrainTransforms)
	{
		LLTerrain terrain;
		for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
		{
			LLPointer<LLGLTFMaterial> omatp = new LLGLTFMaterial();
			for (U32 j = 0; j < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++j)
			{
				// Note: we set the same transforms on all maps
				auto& tfr = omatp->mTextureTransform[j];
				tfr.mScale.mV[VX] = mMaterialScaleUCtrl[i]->get();
				tfr.mScale.mV[VY] = mMaterialScaleVCtrl[i]->get();
				tfr.mOffset.mV[VX] = mMaterialOffsetUCtrl[i]->get();
				tfr.mOffset.mV[VY] = mMaterialOffsetVCtrl[i]->get();
				tfr.mRotation = mMaterialRotationCtrl[i]->get() * DEG_TO_RAD;
			}
			if (*omatp == LLGLTFMaterial::sDefault)
			{
				omatp = NULL;
			}
			terrain.setMaterialOverride(i, omatp.get());
		}
		LLPBRTerrainFeatures::queueModify(regionp, &terrain);
	}

	return true;
}

bool LLPanelRegionTextureInfo::validateTextureSizes() const
{
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		const LLUUID& tex_id = mTexPickers[i]->getImageAssetID();
		LLViewerTexture* texp =
			LLViewerTextureManager::getFetchedTexture(tex_id);
		if (!texp) return false;

		S32 components = texp->getComponents();
		if (components != 3)
		{
			LLSD args;
			args["TEXTURE_NUM"] = i + 1;
			args["TEXTURE_BIT_DEPTH"] = llformat("%d", components * 8);
			args["MAX_SIZE"] = gMaxImageSizeDefault;
			gNotifications.add("InvalidTerrainBitDepth", args);
			return false;
		}

		S32 width = texp->getFullWidth();
		S32 height = texp->getFullHeight();
		if (width > gMaxImageSizeDefault || height > gMaxImageSizeDefault)
		{
			LLSD args;
			args["TEXTURE_NUM"] = i + 1;
			args["TEXTURE_SIZE_X"] = width;
			args["TEXTURE_SIZE_Y"] = height;
			args["MAX_SIZE"] = gMaxImageSizeDefault;
			gNotifications.add("InvalidTerrainSize", args);
			return false;
		}
	}

	return true;
}

bool LLPanelRegionTextureInfo::validateMaterials() const
{
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		const LLUUID mat_id = mMatAssetIds[i];
		const LLFetchedGLTFMaterial* matp =
			gGLTFMaterialList.getMaterial(mat_id);
		if (!matp) return false;

		if (mat_id == BLANK_MATERIAL_ASSET_ID)
		{
			// Default/Blank material is valid by default
			continue;
		}

		if (!matp->isLoaded())
		{
			const char* notification =
				matp->isFetching() ? "InvalidTerrainMaterialNotLoaded"
								   : "InvalidTerrainMaterialLoadFailed";
			LLSD args;
			args["MATERIAL_NUM"] = i + 1;
			gNotifications.add(notification, args);
			return false;
		}

		if (matp->mDoubleSided)
		{
			LLSD args;
			args["MATERIAL_NUM"] = i + 1;
			gNotifications.add("InvalidTerrainMaterialDoubleSided", args);
			return false;
		}

		if (matp->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_OPAQUE &&
			matp->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_MASK)
		{
			LLSD args;
			args["MATERIAL_NUM"] = i + 1;
			args["MATERIAL_ALPHA_MODE"] = LLSD::String(matp->getAlphaMode());
			gNotifications.add("InvalidTerrainMaterialAlphaMode", args);
			return false;
		}
	}

	return true;
}

//static
void LLPanelRegionTextureInfo::onTerrainTypeCommit(LLUICtrl*, void* userdata)
{
	LLPanelRegionTextureInfo* self = (LLPanelRegionTextureInfo*)userdata;
	if (!self) return;	// Paranoia
	
	bool is_pbr = self->mTerrainTypeCtrl->getSelectedIndex() == 1;
	bool has_transforms = self->mHasTerrainTransforms;
	for (U32 i = 0; i < LLTerrain::ASSET_COUNT; ++i)
	{
		self->mTexPickers[i]->setVisible(!is_pbr);
		self->mMatBorders[i]->setVisible(is_pbr);
		self->mMatClickTexts[i]->setVisible(is_pbr);
		self->mMaterialScaleUCtrl[i]->setVisible(is_pbr && has_transforms);
		self->mMaterialScaleVCtrl[i]->setVisible(is_pbr && has_transforms);
		self->mMaterialOffsetUCtrl[i]->setVisible(is_pbr && has_transforms);
		self->mMaterialOffsetVCtrl[i]->setVisible(is_pbr && has_transforms);
		self->mMaterialRotationCtrl[i]->setVisible(is_pbr && has_transforms);
	}

	self->enableApplyBtn();
}

//static
void LLPanelRegionTextureInfo::onMatPreviewClicked(void* userdata)
{
	// Note: userdata is a pointer on the clicked LLTextBox corresponding to
	// the material preview, which parent is LLPanelRegionTextureInfo and which
	// name is equal to the index of the terrain material. HB
	HBFloaterInvItemsPicker* pickerp =
		new HBFloaterInvItemsPicker((LLView*)userdata, onSelectInventoryMat,
									userdata);
	pickerp->setAssetType(LLAssetType::AT_MATERIAL);
	// Terrain materials must be at least copy OK...
	pickerp->setFilterPermMask(PERM_COPY);
}

//static
void LLPanelRegionTextureInfo::onSelectInventoryMat(const std::vector<std::string>&,
													const uuid_vec_t& ids,
													void* userdata, bool)
{
	if (ids.empty() || ids[0].isNull())
	{
		return;	// Nothing to do (was a Cancel, most likely).
	}

	LLViewerInventoryItem* itemp = gInventory.getItem(ids[0]);
	if (!itemp)
	{
		llwarns << "Could not find inventory item " << ids[0] << llendl;
		return;
	}

	LLTextBox* textp = (LLTextBox*)userdata;
	if (!textp) return;	// Paranoia

	LLPanelRegionTextureInfo* self =
		(LLPanelRegionTextureInfo*)textp->getParent();
	if (!self) return;	// Paranoia

	S32 i = atoi(textp->getName().c_str());
	if (i < 0 || i >= (S32)LLTerrain::ASSET_COUNT)
	{
		llerrs << "Erroneous LLTextBox name (should be equal to the material index): "
			   << textp->getName() << llendl;
	}

	// Clear any old preview.
	self->clearMaterialPreview(self->mMatAssetIds[i]);
	// Assign the new material.
	self->mMatAssetIds[i] = itemp->getAssetUUID();

	self->enableApplyBtn();
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelRegionTerrainInfo
///////////////////////////////////////////////////////////////////////////////

bool LLPanelRegionTerrainInfo::postBuild()
{
	LLPanelRegionInfo::postBuild();

	initHelpBtn("water_height_help", "HelpRegionWaterHeight");
	initHelpBtn("terrain_raise_help", "HelpRegionTerrainRaise");
	initHelpBtn("terrain_lower_help", "HelpRegionTerrainLower");
	initHelpBtn("upload_raw_help", "HelpRegionUploadRaw");
	initHelpBtn("download_raw_help", "HelpRegionDownloadRaw");
	initHelpBtn("use_estate_sun_help", "HelpRegionUseEstateSun");
	initHelpBtn("fixed_sun_help", "HelpRegionFixedSun");
	initHelpBtn("bake_terrain_help", "HelpRegionBakeTerrain");

	initCtrl("water_height_spin");
	initCtrl("terrain_raise_spin");
	initCtrl("terrain_lower_spin");

	initCtrl("fixed_sun_check");
	childSetCommitCallback("fixed_sun_check", onChangeFixedSun, this);
	childSetCommitCallback("use_estate_sun_check", onChangeUseEstateTime, this);
	childSetCommitCallback("sun_hour_slider", onChangeSunHour, this);

	childSetAction("download_raw_btn", onClickDownloadRaw, this);
	childSetAction("upload_raw_btn", onClickUploadRaw, this);
	childSetAction("bake_terrain_btn", onClickBakeTerrain, this);

	return true;
}

//virtual
bool LLPanelRegionTerrainInfo::refreshFromRegion(LLViewerRegion* regionp)
{
	if (!LLPanelRegionInfo::refreshFromRegion(regionp))
	{
		return false;
	}

	bool owner_or_god = mOwner || gAgent.isGodlike();

	setCtrlsEnabled(owner_or_god || mEstateManager);
	disableApplyBtn();

	childSetEnabled("download_raw_btn", owner_or_god);
	childSetEnabled("upload_raw_btn", owner_or_god);
	childSetEnabled("bake_terrain_btn", owner_or_god);

	return true;
}

//virtual
bool LLPanelRegionTerrainInfo::sendUpdate()
{
	strings_vec_t strings;

	LLRegionInfoModel::sWaterHeight =
		childGetValue("water_height_spin").asReal();
	strings.emplace_back(llformat("%f", LLRegionInfoModel::sWaterHeight));

	LLRegionInfoModel::sTerrainRaiseLimit =
		childGetValue("terrain_raise_spin").asReal();
	strings.emplace_back(llformat("%f",
								  LLRegionInfoModel::sTerrainRaiseLimit));

	LLRegionInfoModel::sTerrainLowerLimit =
		childGetValue("terrain_lower_spin").asReal();
	strings.emplace_back(llformat("%f", LLRegionInfoModel::sTerrainLowerLimit));

	LLRegionInfoModel::sUseEstateSun =
		childGetValue("use_estate_sun_check").asBoolean();
	strings.emplace_back(llformat("%s",
								  LLRegionInfoModel::sUseEstateSun ? "Y"
																   : "N"));

	bool fixed_sun = childGetValue("fixed_sun_check").asBoolean();
	LLRegionInfoModel::setUseFixedSun(fixed_sun);
	strings.emplace_back(llformat("%s", fixed_sun ? "Y" : "N"));

	LLRegionInfoModel::sSunHour = childGetValue("sun_hour_slider").asReal();
	strings.emplace_back(llformat("%f", LLRegionInfoModel::sSunHour));

	// Grab estate information in case the user decided to set the region back
	// to estate time. JC
	LLPanelEstateInfo* panelp = LLFloaterRegionInfo::getPanelEstate();
	if (!panelp) return true;

	bool estate_global_time = panelp->getGlobalTime();
	bool estate_fixed_sun = panelp->getFixedSun();
	F32 estate_sun_hour;
	if (estate_global_time)
	{
		estate_sun_hour = 0.f;
	}
	else
	{
		estate_sun_hour = panelp->getSunHour();
	}

	strings.emplace_back(llformat("%s", estate_global_time ? "Y" : "N"));
	strings.emplace_back(llformat("%s", estate_fixed_sun ? "Y" : "N"));
	strings.emplace_back(llformat("%f", estate_sun_hour));

	sendEstateOwnerMessage("setregionterrain", strings);
	return true;
}

//static
void LLPanelRegionTerrainInfo::onChangeUseEstateTime(LLUICtrl* ctrl,
													 void* user_data)
{
	LLPanelRegionTerrainInfo* panelp = (LLPanelRegionTerrainInfo*)user_data;
	if (!panelp) return;

	bool use_estate_sun =
		panelp->childGetValue("use_estate_sun_check").asBoolean();
	panelp->childSetEnabled("fixed_sun_check", !use_estate_sun);
	panelp->childSetEnabled("sun_hour_slider", !use_estate_sun);
	if (use_estate_sun)
	{
		panelp->childSetValue("fixed_sun_check", LLSD(false));
		panelp->childSetValue("sun_hour_slider", LLSD(0.f));
	}
	panelp->enableApplyBtn();
}

//static
void LLPanelRegionTerrainInfo::onChangeFixedSun(LLUICtrl* ctrl, void* data)
{
	LLPanelRegionTerrainInfo* panelp = (LLPanelRegionTerrainInfo*)data;
	if (panelp)
	{
		// Just enable the apply button. We let the sun-hour slider be enabled
		// for both fixed-sun and non-fixed-sun. JC
		panelp->enableApplyBtn();
	}
}

//static
void LLPanelRegionTerrainInfo::onChangeSunHour(LLUICtrl* ctrl, void*)
{
	// Cannot use userdata to get panel, slider uses it internally
	LLPanelRegionTerrainInfo* panelp =
		(LLPanelRegionTerrainInfo*)ctrl->getParent();
	if (panelp)
	{
		panelp->enableApplyBtn();
	}
}

//static
void LLPanelRegionTerrainInfo::downloadRawCallback(HBFileSelector::ESaveFilter filter,
												   std::string& filepath,
												   void* data)
{
	LLPanelRegionTerrainInfo* self = LLFloaterRegionInfo::getPanelTerrain();
	if (!self || data != self || !gXferManagerp) return;

	gXferManagerp->expectFileForRequest(filepath);
	strings_vec_t strings;
	strings.emplace_back("download filename");
	strings.emplace_back(filepath);
	self->sendEstateOwnerMessage("terrain", strings);
}

//static
void LLPanelRegionTerrainInfo::onClickDownloadRaw(void* data)
{
	HBFileSelector::saveFile(HBFileSelector::FFSAVE_RAW, "terrain.raw",
							 downloadRawCallback, data);
}

//static
void LLPanelRegionTerrainInfo::uploadRawCallback(HBFileSelector::ELoadFilter filter,
												 std::string& filepath,
												 void* data)
{
	LLPanelRegionTerrainInfo* self = LLFloaterRegionInfo::getPanelTerrain();
	if (!self || data != self || !gXferManagerp) return;

	gXferManagerp->expectFileForTransfer(filepath);
	strings_vec_t strings;
	strings.emplace_back("upload filename");
	strings.emplace_back(filepath);
	self->sendEstateOwnerMessage("terrain", strings);

	gNotifications.add("RawUploadStarted");
}

//static
void LLPanelRegionTerrainInfo::onClickUploadRaw(void* data)
{
	HBFileSelector::loadFile(HBFileSelector::FFLOAD_TERRAIN,
							 uploadRawCallback, data);
}

//static
void LLPanelRegionTerrainInfo::onClickBakeTerrain(void* data)
{
	gNotifications.add(LLNotification::Params("ConfirmBakeTerrain").functor(
		std::bind(&LLPanelRegionTerrainInfo::callbackBakeTerrain,
				  (LLPanelRegionTerrainInfo*)data, _1, _2)));
}

bool LLPanelRegionTerrainInfo::callbackBakeTerrain(const LLSD& notification,
												   const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response))
	{
		return false;
	}

	strings_vec_t strings;
	strings.emplace_back("bake");
	sendEstateOwnerMessage("terrain", strings);
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelEstateInfo
///////////////////////////////////////////////////////////////////////////////

LLPanelEstateInfo::LLPanelEstateInfo()
:	LLPanelRegionInfo(),
	mEstateID(0)			// 0 = invalid
{
}

//virtual
bool LLPanelEstateInfo::postBuild()
{
	// Set up the callbacks for the generic controls
	initCtrl("public_access_check");
	initCtrl("use_global_time_check");
	initCtrl("fixed_sun_check");
	initCtrl("allow_direct_teleport");
	initCtrl("limit_payment");
	initCtrl("limit_age_verified");
	initCtrl("voice_chat_check");
	initCtrl("parcel_access_override");
	initCtrl("limit_bots");

	initHelpBtn("use_global_time_help", "HelpEstateUseGlobalTime");
	initHelpBtn("fixed_sun_help", "HelpEstateFixedSun");
	initHelpBtn("public_access_help", "HelpEstatePublicAccess");
	initHelpBtn("allow_direct_teleport_help", "HelpEstateAllowDirectTeleport");
	initHelpBtn("voice_chat_help", "HelpEstateVoiceChat");

	// Set up the use global time checkbox
	childSetCommitCallback("use_global_time_check", onChangeUseGlobalTime,
						   this);
	childSetCommitCallback("fixed_sun_check", onChangeFixedSun, this);
	childSetCommitCallback("sun_hour_slider", updateChild, this);

	childSetAction("message_estate_btn", onClickMessageEstate, this);
	childSetAction("kick_user_from_estate_btn", onClickKickUser, this);

	return LLPanelRegionInfo::postBuild();
}

//static
void LLPanelEstateInfo::updateChild(LLUICtrl* ctrlp, void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)user_data;
	if (self && ctrlp)
	{
		self->checkSunHourSlider(ctrlp);
		// Ensure appropriate state of the management ui.
		self->updateControls();
	}
}

//virtual
bool LLPanelEstateInfo::estateUpdate(LLMessageSystem*)
{
	llinfos << "No operation..." << llendl;
	return false;
}

void LLPanelEstateInfo::updateControls()
{
	LL_DEBUGS("RegionInfo") << "Udpating controls" << LL_ENDL;
	bool allow_modify = mOwner || mEstateManager || gAgent.isGodlike();
	setCtrlsEnabled(allow_modify);
	disableApplyBtn();
	childSetEnabled("message_estate_btn", allow_modify);
	childSetEnabled("kick_user_from_estate_btn", allow_modify);
}

//virtual
bool LLPanelEstateInfo::refreshFromRegion(LLViewerRegion* regionp)
{
	LL_DEBUGS("RegionInfo") << "Refreshing info from region" << LL_ENDL;
	LLPanelRegionInfo::refreshFromRegion(regionp);

	updateControls();

	// We want estate info. To make sure it works across region boundaries and
	// multiple packets, we add a serial number to the integers and track
	// against that on update.
	LLFloaterRegionInfo::nextInvoice();
	strings_vec_t strings;
	sendEstateOwnerMessage("getinfo", strings);

	refresh();

	return true;
}

void LLPanelEstateInfo::refresh()
{
	bool public_access = childGetValue("public_access_check").asBoolean();
	childSetEnabled("Only Allow", public_access);
	childSetEnabled("limit_payment", public_access);
	childSetEnabled("limit_age_verified", public_access);
	childSetEnabled("limit_bots", public_access);
	if (!public_access)
	{
		// If not public access, then the limit fields are meaningless and
		// should be turned off
		childSetValue("limit_payment", false);
		childSetValue("limit_age_verified", false);
		childSetValue("limit_bots", false);
	}
}

bool LLPanelEstateInfo::sendUpdate()
{
	LLNotification::Params params("ChangeLindenEstate");
	params.functor(std::bind(&LLPanelEstateInfo::callbackChangeLindenEstate,
							 this, _1, _2));

	if (getEstateID() <= ESTATE_LAST_LINDEN)
	{
		// Trying to change reserved estate, warn
		gNotifications.add(params);
	}
	else
	{
		// For normal estates, just make the change
		gNotifications.forceResponse(params, 0);
	}
	return true;
}

bool LLPanelEstateInfo::callbackChangeLindenEstate(const LLSD& notification,
												   const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		// Send the update
		if (!commitEstateInfoCaps())
		{
			// The caps method failed, try the old way
			LLFloaterRegionInfo::nextInvoice();
			commitEstateInfoDataserver();
		}
#if 0	// We do not want to do this because we will get it automatically from
		// the sim after the spaceserver processes it
		else
		{
			// Caps method does not automatically send this info
			LLFloaterRegionInfo::requestRegionInfo();
		}
#endif
	}
	else	// Cancelling action
	{
		HBPanelLandEnvironment* panelp =
			LLFloaterRegionInfo::getPanelEnvironment();
		if (panelp)
		{
			// This will (re)set the environment override check to its
			// former (or last) value
			panelp->resetOverride();
		}
	}
	return false;
}

#if 0
// Request = "getowner"
// SParam[0] = "" (empty string)
// IParam[0] = serial
void LLPanelEstateInfo::getEstateOwner()
{
	// *TODO: disable the panel and call this method whenever we cross a region
	// boundary; re-enable when owner matches, and get new estate info.
	LLMessageSystem* msg = gMessageSystemp;
	msg->newMessageFast(_PREHASH_EstateOwnerRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);

	msg->nextBlockFast(_PREHASH_RequestData);
	msg->addStringFast(_PREHASH_Request, "getowner");

	// We send an empty string so that the variable block is not empty
	msg->nextBlockFast(_PREHASH_StringData);
	msg->addStringFast(_PREHASH_SParam, "");

	msg->nextBlockFast(_PREHASH_IntegerData);
	msg->addS32Fast(_PREHASH_IParam, LLFloaterRegionInfo::getSerial());

	gAgent.sendMessage();
}
#endif

// Tries to send estate info using a cap; returns true if it succeeded
bool LLPanelEstateInfo::commitEstateInfoCaps()
{
	std::string url = gAgent.getRegionCapability("EstateChangeInfo");
	if (url.empty())
	{
		// Whoops, could not find the capability, so bail out
		return false;
	}

	gCoros.launch("LLPanelEstateInfo::commitEstateInfoCaps",
				  std::bind(&LLPanelEstateInfo::commitEstateInfoCapsCoro,
							this, url));
	return true;
}

void LLPanelEstateInfo::commitEstateInfoCapsCoro(const std::string& url)
{
	LL_DEBUGS("RegionInfo") << "Commiting estate info via capability"
						    << LL_ENDL;
	LLSD body;
	body["estate_name"] = getEstateName();
	body["is_externally_visible"] =
		childGetValue("public_access_check").asBoolean();
	body["allow_direct_teleport"] =
		childGetValue("allow_direct_teleport").asBoolean();
	body["is_sun_fixed"] = childGetValue("fixed_sun_check").asBoolean();
	body["deny_anonymous"] = childGetValue("limit_payment").asBoolean();
	body["deny_age_unverified"] =
		childGetValue("limit_age_verified").asBoolean();
	body["block_bots"] = childGetValue("limit_bots").asBoolean();
	body["allow_voice_chat"] = childGetValue("voice_chat_check").asBoolean();
	body["override_public_access"] =
		childGetValue("override_public_access").asBoolean();
	// For potential EE support in OpenSIM. This is not in LLPanelEstateInfo's
	// UI: it is (re)set by HBPanelLandEnvironment directly in sEstateFlags. HB
	body["override_environment"] =
		LLEstateInfoModel::getAllowEnvironmentOverride();
	body["invoice"] = LLFloaterRegionInfo::getLastInvoice();

#if 0	// Block fly is in estate database but not in estate UI, so we are not
		// supporting it
	body["block_fly"] = childGetValue("").asBoolean();
#endif

	F32 sun_hour = getSunHour();
	if (childGetValue("use_global_time_check").asBoolean())
	{
		sun_hour = 0.f;			// 0 = global time
	}
	body["sun_hour"] = sun_hour;

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("EstateChangeInfo");
	LLSD result = adapter.postAndSuspend(url, body);

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (status)
	{
		refresh();
	}
	else
	{
		llwarns << "Failed to commit estate info: " << status.toString()
				<< llendl;
	}
}

// This is the old way of doing things, is deprecated, and should be deleted
// when the dataserver model can be removed.
// key = "estatechangeinfo"
// strings[0] = str(estate_id) (added by simulator before relay - not here)
// strings[1] = estate_name
// strings[2] = str(estate_flags)
// strings[3] = str((S32)(sun_hour * 1024.f))
void LLPanelEstateInfo::commitEstateInfoDataserver()
{
	LL_DEBUGS("RegionInfo") << "Commiting estate info via legacy UDP message"
						    << LL_ENDL;
	LLMessageSystem* msg = gMessageSystemp;
	msg->newMessage(_PREHASH_EstateOwnerMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); // not used

	msg->nextBlock(_PREHASH_MethodData);
	msg->addString(_PREHASH_Method, "estatechangeinfo");
	msg->addUUID(_PREHASH_Invoice, LLFloaterRegionInfo::getLastInvoice());

	msg->nextBlock(_PREHASH_ParamList);
	msg->addString(_PREHASH_Parameter, getEstateName());

	std::string buffer;
	buffer = llformat("%u", computeEstateFlags());
	msg->nextBlock(_PREHASH_ParamList);
	msg->addString(_PREHASH_Parameter, buffer);

	F32 sun_hour = getSunHour();
	if (childGetValue("use_global_time_check").asBoolean())
	{
		sun_hour = 0.f;	// 0 = global time
	}

	buffer = llformat("%d", (S32)(sun_hour * 1024.f));
	msg->nextBlock(_PREHASH_ParamList);
	msg->addString(_PREHASH_Parameter, buffer);

	gAgent.sendMessage();
}

void LLPanelEstateInfo::setEstateFlags(U32 flags)
{
	childSetValue("public_access_check",
				  LLSD((flags & REGION_FLAGS_EXTERNALLY_VISIBLE) != 0));
	childSetValue("fixed_sun_check",
				  LLSD((flags & REGION_FLAGS_SUN_FIXED) != 0));
	childSetValue("voice_chat_check",
				  LLSD((flags & REGION_FLAGS_ALLOW_VOICE) != 0));
	childSetValue("allow_direct_teleport",
				  LLSD((flags & REGION_FLAGS_ALLOW_DIRECT_TELEPORT) != 0));
	childSetValue("limit_payment",
				  LLSD((flags & REGION_FLAGS_DENY_ANONYMOUS) != 0));
	childSetValue("limit_age_verified",
				  LLSD((flags & REGION_FLAGS_DENY_AGEUNVERIFIED) != 0));
	childSetValue("parcel_access_override",
				  LLSD((flags & REGION_FLAGS_ALLOW_ACCESS_OVERRIDE) != 0));
	childSetValue("limit_bots", LLSD((flags & REGION_FLAGS_DENY_BOTS) != 0));

	refresh();
}

U32 LLPanelEstateInfo::computeEstateFlags()
{
	U32 flags = 0;

	// This is not in LLPanelEstateInfo's UI: it is (re)set by
	// HBPanelLandEnvironment directly in sEstateFlags...
	if (LLEstateInfoModel::getAllowEnvironmentOverride())
	{
		flags |= REGION_FLAGS_ALLOW_ENVIRONMENT_OVERRIDE;
	}

	if (childGetValue("public_access_check").asBoolean())
	{
		flags |= REGION_FLAGS_EXTERNALLY_VISIBLE;
	}

	if (childGetValue("voice_chat_check").asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_VOICE;
	}

	if (childGetValue("parcel_access_override").asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_ACCESS_OVERRIDE;
	}

	if (childGetValue("allow_direct_teleport").asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_DIRECT_TELEPORT;
	}

	if (childGetValue("fixed_sun_check").asBoolean())
	{
		flags |= REGION_FLAGS_SUN_FIXED;
	}

	if (childGetValue("limit_payment").asBoolean())
	{
		flags |= REGION_FLAGS_DENY_ANONYMOUS;
	}

	if (childGetValue("limit_age_verified").asBoolean())
	{
		flags |= REGION_FLAGS_DENY_AGEUNVERIFIED;
	}

	if (childGetValue("limit_bots").asBoolean())
	{
		flags |= REGION_FLAGS_DENY_BOTS;
	}

	// Store in LLEstateInfoModel
	LLEstateInfoModel::sEstateFlags = flags;

	return flags;
}

bool LLPanelEstateInfo::getGlobalTime()
{
	return childGetValue("use_global_time_check").asBoolean();
}

void LLPanelEstateInfo::setGlobalTime(bool b)
{
	childSetValue("use_global_time_check", LLSD(b));
	childSetEnabled("fixed_sun_check", LLSD(!b));
	childSetEnabled("sun_hour_slider", LLSD(!b));
	if (b)
	{
		childSetValue("sun_hour_slider", LLSD(0.f));
	}
}

bool LLPanelEstateInfo::getFixedSun()
{
	return childGetValue("fixed_sun_check").asBoolean();
}

void LLPanelEstateInfo::setSunHour(F32 sun_hour)
{
	if (sun_hour < 6.f)
	{
		sun_hour = 24.f + sun_hour;
	}
	childSetValue("sun_hour_slider", LLSD(sun_hour));
}

F32 LLPanelEstateInfo::getSunHour()
{
	if (childIsEnabled("sun_hour_slider"))
	{
		return (F32)childGetValue("sun_hour_slider").asReal();
	}
	return 0.f;
}

std::string LLPanelEstateInfo::getEstateName() const
{
	return childGetValue("estate_name").asString();
}

void LLPanelEstateInfo::setEstateName(const std::string& name)
{
	childSetValue("estate_name", LLSD(name));
}

void LLPanelEstateInfo::setEstateID(U32 estate_id)
{
	mEstateID = estate_id;
	childSetValue("estate_id", LLSD(llformat("%d", (S32)estate_id)));
}

std::string LLPanelEstateInfo::getOwnerName() const
{
	return childGetValue("estate_owner").asString();
}

void LLPanelEstateInfo::setOwnerName(const std::string& name)
{
	childSetValue("estate_owner", LLSD(name));
}

bool LLPanelEstateInfo::checkSunHourSlider(LLUICtrl* child_ctrl)
{
	bool found_child_ctrl = false;
	if (child_ctrl->getName() == "sun_hour_slider")
	{
		enableApplyBtn();
		found_child_ctrl = true;
	}
	return found_child_ctrl;
}

bool LLPanelEstateInfo::kickUserConfirm(const LLSD& notification,
										const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		// Kick User
		strings_vec_t strings;
		strings.emplace_back(notification["payload"]["agent_id"].asString());
		sendEstateOwnerMessage("kickestate", strings);
	}
	return false;
}


//static
void LLPanelEstateInfo::onClickMessageEstate(void* userdata)
{
	gNotifications.add("MessageEstate", LLSD(), LLSD(),
					   std::bind(&LLPanelEstateInfo::onMessageCommit,
								 (LLPanelEstateInfo*)userdata, _1, _2));
}

//static
bool LLPanelEstateInfo::onMessageCommit(const LLSD& notification,
										const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string text = response["message"].asString();
	if (option != 0) return false;
	if (text.empty()) return false;
	llinfos << "Message to everyone: " << text << llendl;
	strings_vec_t strings;
	//integers_t integers;
	std::string name;
	gAgent.buildFullname(name);
	strings.emplace_back(strings_vec_t::value_type(name));
	strings.emplace_back(strings_vec_t::value_type(text));
	sendEstateOwnerMessage("instantmessage", strings);
	return false;
}

//static
void LLPanelEstateInfo::initDispatch(LLDispatcher& dispatch)
{
	std::string name = "estateupdateinfo";
	static LLDispatchEstateUpdateInfo estate_update_info;
	dispatch.addHandler(name, &estate_update_info);

	name = "setaccess";
	static LLDispatchSetEstateAccess set_access;
	dispatch.addHandler(name, &set_access);

	name = "setexperience";
	static LLDispatchSetEstateExperience set_experience;
	dispatch.addHandler(name, &set_experience);

	gEstateDispatchInitialized = true;
}

//static
void LLPanelEstateInfo::callbackCacheName(const LLUUID& id,
										  const std::string& full_name,
										  bool is_group)
{
	LLPanelEstateInfo* self = LLFloaterRegionInfo::getPanelEstate();
	if (!self) return;

	std::string name;

	if (id.isNull())
	{
		name = "(none)";
	}
	else
	{
		name = full_name;
	}

	self->setOwnerName(name);
}

// Disables the sun-hour slider and the use fixed time check if the use global
// time is check
//static
void LLPanelEstateInfo::onChangeUseGlobalTime(LLUICtrl* ctrl, void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)user_data;
	if (!self) return;

	bool enabled = !self->childGetValue("use_global_time_check").asBoolean();
	self->childSetEnabled("sun_hour_slider", enabled);
	self->childSetEnabled("fixed_sun_check", enabled);
	self->childSetValue("fixed_sun_check", LLSD(false));
	self->enableApplyBtn();
}

// Enables the sun-hour slider if the fixed-sun checkbox is set
//static
void LLPanelEstateInfo::onChangeFixedSun(LLUICtrl* ctrl, void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*) user_data;
	if (!self) return;

	bool enabled = !self->childGetValue("fixed_sun_check").asBoolean();
	self->childSetEnabled("use_global_time_check", enabled);
	self->childSetValue("use_global_time_check", LLSD(false));
	self->enableApplyBtn();
}

//static
void LLPanelEstateInfo::onClickKickUser(void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)user_data;
	if (!self) return;

	LLFloater* picker = LLFloaterAvatarPicker::show(onKickUserCommit,
													user_data, false, true);
	if (picker && gFloaterViewp)
	{
		// This depends on the grandparent view being a floater in order to set
		// up floater dependency
		LLFloater* parentp = gFloaterViewp->getParentFloater(self);
		if (parentp)
		{
			parentp->addDependentFloater(picker);
		}
	}
}

struct LLKickFromEstateInfo
{
	LLPanelEstateInfo*	mEstatePanelp;
	LLUUID      		mAgentID;
};

//static
void LLPanelEstateInfo::onKickUserCommit(const std::vector<std::string>& names,
										 const uuid_vec_t& ids,
										 void* userdata)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)userdata;
	if (!self || names.empty() || ids.empty() ||
		// Check to make sure there is one valid user and id
		ids[0].isNull() || names[0].length() == 0)
	{
		return;
	}

	// Keep track of what user they want to kick and other misc info
	LLKickFromEstateInfo* kick_info = new LLKickFromEstateInfo();
	kick_info->mEstatePanelp = self;
	kick_info->mAgentID = ids[0];

	// Bring up a confirmation dialog
	LLSD args;
	args["EVIL_USER"] = names[0];
	LLSD payload;
	payload["agent_id"] = ids[0];
	gNotifications.add("EstateKickUser", args, payload,
					   std::bind(&LLPanelEstateInfo::kickUserConfirm, self, _1,
								 _2));
}

//static
bool LLPanelEstateInfo::isLindenEstate()
{
	LLPanelEstateInfo* self = LLFloaterRegionInfo::getPanelEstate();
	return self && self->getEstateID() <= ESTATE_LAST_LINDEN;
}

//static
void LLPanelEstateInfo::updateEstateOwnerName(const std::string& name)
{
	LLPanelEstateInfo* self = LLFloaterRegionInfo::getPanelEstate();
	if (self)
	{
		self->setOwnerName(name);
	}
}

//static
void LLPanelEstateInfo::updateEstateName(const std::string& name)
{
	LLPanelEstateInfo* self = LLFloaterRegionInfo::getPanelEstate();
	if (self)
	{
		self->setEstateName(name);
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelEstateAccess
///////////////////////////////////////////////////////////////////////////////

//static
S32 LLPanelEstateAccess::sLastActiveTab = 0;

LLPanelEstateAccess::LLPanelEstateAccess()
:	LLPanelRegionInfo(),
	mPendingUpdate(false),
	mCtrlsEnabled(false)
{
}

//virtual
bool LLPanelEstateAccess::postBuild()
{
	mTabContainer = getChild<LLTabContainer>("access_tabs");
	LLPanel* tab = mTabContainer->getChild<LLPanel>("estate_managers");
	mTabContainer->setTabChangeCallback(tab, onTabChanged);
	mTabContainer->setTabUserData(tab, this);
	tab = mTabContainer->getChild<LLPanel>("allowed_groups");
	mTabContainer->setTabChangeCallback(tab, onTabChanged);
	mTabContainer->setTabUserData(tab, this);
	tab = mTabContainer->getChild<LLPanel>("allowed_resident");
	mTabContainer->setTabChangeCallback(tab, onTabChanged);
	mTabContainer->setTabUserData(tab, this);
	tab = mTabContainer->getChild<LLPanel>("banned_residents");
	mTabContainer->setTabChangeCallback(tab, onTabChanged);
	mTabContainer->setTabUserData(tab, this);

	mTabContainer->selectTab(sLastActiveTab);

	initHelpBtn("estate_manager_help", "HelpEstateEstateManager");
	initHelpBtn("allow_group_help", "HelpEstateAllowGroup");
	initHelpBtn("allow_resident_help", "HelpEstateAllowResident");
	initHelpBtn("ban_resident_help", "HelpEstateBanResident");

	mEstateManagers = getChild<LLNameListCtrl>("estate_manager_name_list");
	mEstateManagers->setCommitCallback(updateChild);
	mEstateManagers->setCallbackUserData(this);
	mEstateManagers->setCommitOnSelectionChange(true);
	// Allow extras for dupe issue
	mEstateManagers->setMaxItemCount(ESTATE_MAX_MANAGERS * 4);

	childSetAction("add_estate_manager_btn", onClickAddEstateManager, this);
	childSetAction("remove_estate_manager_btn", onClickRemoveEstateManager,
				   this);

	mAllowedGroups = getChild<LLNameListCtrl>("allowed_group_name_list");
	mAllowedGroups->setCommitCallback(updateChild);
	mAllowedGroups->setCallbackUserData(this);
	mAllowedGroups->setCommitOnSelectionChange(true);
	mAllowedGroups->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);

	childSetAction("add_allowed_group_btn", onClickAddAllowedGroup, this);
	childSetAction("remove_allowed_group_btn", onClickRemoveAllowedGroup,
				   this);

	mAllowedAvatars = getChild<LLNameListCtrl>("allowed_avatar_name_list");
	mAllowedAvatars->setCommitCallback(updateChild);
	mAllowedAvatars->setCallbackUserData(this);
	mAllowedAvatars->setCommitOnSelectionChange(true);
	mAllowedAvatars->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);

	childSetAction("add_allowed_avatar_btn", onClickAddAllowedAgent, this);
	childSetAction("remove_allowed_avatar_btn", onClickRemoveAllowedAgent,
				   this);

	mBannedAvatars = getChild<LLNameListCtrl>("banned_avatar_name_list");
	mBannedAvatars->setCommitCallback(updateChild);
	mBannedAvatars->setCallbackUserData(this);
	mBannedAvatars->setCommitOnSelectionChange(true);
	mBannedAvatars->setMaxItemCount(ESTATE_MAX_BANNED_IDS);

	childSetAction("add_banned_avatar_btn", onClickAddBannedAgent, this);
	childSetAction("remove_banned_avatar_btn", onClickRemoveBannedAgent, this);

#if 0	// *TODO: implement (backport from LL's viewer-release)
	childSetCommitCallback("allowed_search_input", onAllowedSearchEdit, this);
	childSetCommitCallback("banned_search_input", onBannedSearchEdit, this);
	childSetCommitCallback("allowed_group_search_input",
						   onAllowedGroupsSearchEdit, this);
	childSetAction("copy_allowed_list_btn", onClickCopyAllowedList, this);
	childSetAction("copy_allowed_group_list_btn", onClickCopyAllowedGroupList,
				   this);
	childSetAction("copy_banned_list_btn", onClickCopyBannedList, this);
#endif

	// Note: no apply button, so we do not call LLPanelRegionInfo::postBuild()
	return true;
}

//static
void LLPanelEstateAccess::updateChild(LLUICtrl* ctrlp, void* user_data)
{
	LLPanelEstateAccess* self = (LLPanelEstateAccess*)user_data;
	if (self && ctrlp)
	{
		self->checkRemovalButton(ctrlp->getName());
		// Ensure appropriate state of the management ui.
		self->updateControls();
	}
}

//virtual
bool LLPanelEstateAccess::refreshFromRegion(LLViewerRegion* regionp)
{
	LL_DEBUGS("RegionInfo") << "Refreshing from region" << LL_ENDL;
	LLPanelRegionInfo::refreshFromRegion(regionp);
	updateLists();
	return true;
}

void LLPanelEstateAccess::updateLists()
{
	LL_DEBUGS("RegionInfo") << "Requesting access info from region" << LL_ENDL;
	std::string cap_url = gAgent.getRegionCapability("EstateAccess");
	if (cap_url.empty())
	{
		strings_vec_t strings;
		LLFloaterRegionInfo::nextInvoice();
		sendEstateOwnerMessage("getinfo", strings);
		return;
	}
	// Use the capability
	gCoros.launch("LLPanelEstateAccess::requestEstateGetAccessCoro",
				  std::bind(LLPanelEstateAccess::requestEstateGetAccessCoro,
							cap_url));
}

void LLPanelEstateAccess::requestEstateGetAccessCoro(const std::string& url)
{
	LL_DEBUGS("RegionInfo") << "Requesting access info via capability"
							<< LL_ENDL;

	LLCoreHttpUtil::HttpCoroutineAdapter adapter("requestEstateGetAccessoCoro");

	LLSD result = adapter.getAndSuspend(url);

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);

	LLPanelEstateAccess* self = LLFloaterRegionInfo::getPanelAccess();
	if (!self) return;		// We have since been closed...

	LLNameListCtrl* name_list;

	if (result.has("AllowedAgents"))
	{
		name_list = self->mAllowedAvatars;
		const LLSD& allowed = result["AllowedAgents"];

		LLStringUtil::format_map_t args;
		args["[ALLOWEDAGENTS]"] = llformat("%d", allowed.size());
		args["[MAXACCESS]"] = llformat("%d", ESTATE_MAX_ACCESS_IDS);
		std::string msg = self->getString("RegionInfoAllowedResidents", args);
		self->getChild<LLUICtrl>("allow_resident_label")->setValue(LLSD(msg));

		name_list->clearSortOrder();
		name_list->deleteAllItems();
		for (LLSD::array_const_iterator it = allowed.beginArray(),
										end = allowed.endArray();
			 it != end; ++it)
		{
			LLUUID id = (*it)["id"].asUUID();
			name_list->addNameItem(id);
		}
		name_list->sortByName(true);
	}

	if (result.has("BannedAgents"))
	{
		name_list = self->mBannedAvatars;
		const LLSD& banned = result["BannedAgents"];

		LLStringUtil::format_map_t args;
		args["[BANNEDAGENTS]"] = llformat("%d", banned.size());
		args["[MAXBANNED]"] = llformat("%d", ESTATE_MAX_BANNED_IDS);
		std::string msg = self->getString("RegionInfoBannedResidents", args);
		self->getChild<LLUICtrl>("ban_resident_label")->setValue(LLSD(msg));

		name_list->clearSortOrder();
		name_list->deleteAllItems();
		std::string fullname;
		std::string na = LLTrans::getString("na");
		for (LLSD::array_const_iterator it = banned.beginArray(),
										end = banned.endArray();
			 it != end; ++it)
		{
			LLSD item;
			item["id"] = (*it)["id"].asUUID();
			LLSD& columns = item["columns"];

			columns[0]["column"] = "name"; // value is auto-populated

			columns[1]["column"] = "last_login_date";
			columns[1]["value"] =
													// Cut the seconds
				(*it)["last_login_date"].asString().substr(0, 16);

			std::string ban_date = (*it)["ban_date"].asString();
			columns[2]["column"] = "ban_date";
			 // The server returns the "0000-00-00 00:00:00" date in case it
			// does not know it
			columns[2]["value"] = ban_date[0] ? ban_date.substr(0, 16) : na;

			columns[3]["column"] = "bannedby";
			LLUUID banning_id = (*it)["banning_id"].asUUID();
			if (banning_id.isNull())
			{
				columns[3]["value"] = na;
			}
			else if (gCacheNamep &&
					 gCacheNamep->getFullName(banning_id, fullname))
			{
				// *TODO: fetch the name if it was not cached
				columns[3]["value"] = fullname;
			}

			name_list->addElement(item);
		}
		name_list->sortByName(true);
	}

	if (result.has("AllowedGroups"))
	{
		name_list = self->mAllowedGroups;
		const LLSD& groups = result["AllowedGroups"];

		LLStringUtil::format_map_t args;
		args["[ALLOWEDGROUPS]"] = llformat("%d", groups.size());
		args["[MAXACCESS]"] = llformat("%d", ESTATE_MAX_GROUP_IDS);
		std::string msg = self->getString("RegionInfoAllowedGroups", args);
		self->getChild<LLUICtrl>("allow_group_label")->setValue(LLSD(msg));

		name_list->clearSortOrder();
		name_list->deleteAllItems();
		for (LLSD::array_const_iterator it = groups.beginArray(),
										end = groups.endArray();
			 it != end; ++it)
		{
			LLUUID id = (*it)["id"].asUUID();
			name_list->addGroupNameItem(id);
		}
		name_list->sortByName(true);
	}

	if (result.has("Managers"))
	{
		name_list = self->mEstateManagers;
		const LLSD& managers = result["Managers"];

		LLStringUtil::format_map_t args;
		args["[ESTATEMANAGERS]"] = llformat("%d", managers.size());
		args["[MAXMANAGERS]"] = llformat("%d", ESTATE_MAX_MANAGERS);
		std::string msg = self->getString("RegionInfoEstateManagers", args);
		self->getChild<LLUICtrl>("estate_manager_label")->setValue(LLSD(msg));

		name_list->clearSortOrder();
		name_list->deleteAllItems();
		for (LLSD::array_const_iterator it = managers.beginArray(),
										end = managers.endArray();
			 it != end; ++it)
		{
			LLUUID id = (*it)["agent_id"].asUUID();
			name_list->addNameItem(id);
		}
		name_list->sortByName(true);
	}

	self->updateControls();
}

//static
void LLPanelEstateAccess::onTabChanged(void* user_data, bool)
{
	LLPanelEstateAccess* self = (LLPanelEstateAccess*)user_data;
	if (self)
	{
		sLastActiveTab = self->mTabContainer->getCurrentPanelIndex();
	}
}

//static
void LLPanelEstateAccess::onClickAddAllowedAgent(void* user_data)
{
	LLPanelEstateAccess* self = (LLPanelEstateAccess*)user_data;
	if (!self) return;

	if (self->mAllowedAvatars->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		LLSD args;
		args["MAX_AGENTS"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		gNotifications.add("MaxAllowedAgentOnRegion", args);
	}
	else
	{
		accessAddCore(ESTATE_ACCESS_ALLOWED_AGENT_ADD);
	}
}

//static
void LLPanelEstateAccess::onClickRemoveAllowedAgent(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_ALLOWED_AGENT_REMOVE);
}

//static
void LLPanelEstateAccess::onClickAddAllowedGroup(void* user_data)
{
	LLPanelEstateAccess* self = (LLPanelEstateAccess*)user_data;
	if (!self) return;

	if (self->mAllowedGroups->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		LLSD args;
		args["MAX_GROUPS"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		gNotifications.add("MaxAllowedGroupsOnRegion", args);
		return;
	}

	LLNotification::Params params("ChangeLindenAccess");
	params.functor(std::bind(&LLPanelEstateAccess::addAllowedGroup, self, _1,
							 _2));
	if (LLPanelEstateInfo::isLindenEstate())
	{
		gNotifications.add(params);
	}
	else
	{
		gNotifications.forceResponse(params, 0);
	}
}

//static
bool LLPanelEstateAccess::addAllowedGroup(const LLSD& notification,
										  const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response))
	{
		return false;
	}

	LLFloaterGroupPicker* picker = LLFloaterGroupPicker::show(addAllowedGroup2,
															  NULL);
	LLFloater* parentp = gFloaterViewp->getParentFloater(this);
	if (picker && parentp && gFloaterViewp)
	{
		LLRect new_rect = gFloaterViewp->findNeighboringPosition(parentp,
																 picker);
		picker->setOrigin(new_rect.mLeft, new_rect.mBottom);
		parentp->addDependentFloater(picker);
	}

	return false;
}

//static
void LLPanelEstateAccess::onClickRemoveAllowedGroup(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_ALLOWED_GROUP_REMOVE);
}

//static
void LLPanelEstateAccess::onClickAddBannedAgent(void* user_data)
{
	LLPanelEstateAccess* self = (LLPanelEstateAccess*)user_data;
	if (!self) return;

	if (self->mBannedAvatars->getItemCount() >= ESTATE_MAX_BANNED_IDS)
	{
		LLSD args;
		args["MAX_BANNED"] = llformat("%d",ESTATE_MAX_BANNED_IDS);
		gNotifications.add("MaxBannedAgentsOnRegion", args);
	}
	else
	{
		accessAddCore(ESTATE_ACCESS_BANNED_AGENT_ADD);
	}
}

//static
void LLPanelEstateAccess::onClickRemoveBannedAgent(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_BANNED_AGENT_REMOVE);
}

//static
void LLPanelEstateAccess::onClickAddEstateManager(void* user_data)
{
	LLPanelEstateAccess* self = (LLPanelEstateAccess*)user_data;
	if (!self) return;

	if (self->mEstateManagers->getItemCount() >= ESTATE_MAX_MANAGERS)
	{
		// Tell user they cannot add more managers
		LLSD args;
		args["MAX_MANAGER"] = llformat("%d", ESTATE_MAX_MANAGERS);
		gNotifications.add("MaxManagersOnRegion", args);
	}
	else
	{
		// Go pick managers to add
		accessAddCore(ESTATE_ACCESS_MANAGER_ADD);
	}
}

//static
void LLPanelEstateAccess::onClickRemoveEstateManager(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_MANAGER_REMOVE);
}

//static
std::string LLPanelEstateAccess::allEstatesText()
{
	LLPanelEstateAccess* self = LLFloaterRegionInfo::getPanelAccess();
	LLPanelEstateInfo* panelp = LLFloaterRegionInfo::getPanelEstate();
	LLViewerRegion* region = gAgent.getRegion();
	if (!self || !panelp || !region)
	{
		return "(error)";
	}

	if (gAgent.isGodlike())
	{
		LLStringUtil::format_map_t args;
		args["[OWNER]"] = panelp->getOwnerName();
		return self->getString("all_estates_owned_by", args);
	}

	if (region->getOwner() == gAgentID)
	{
		return self->getString("all_estates_you_own");
	}

	if (region->isEstateManager())
	{
		LLStringUtil::format_map_t args;
		args["[OWNER]"] = panelp->getOwnerName();
		return self->getString("all_estates_you_manage_for", args);
	}

	return self->getString("error");
}

// Special case callback for groups, since it has different callback format
// than names
//static
void LLPanelEstateAccess::addAllowedGroup2(LLUUID id, void* user_data)
{
	LLPanelEstateAccess* self = LLFloaterRegionInfo::getPanelAccess();
	if (!self) return;

	LLScrollListItem* item = self->mAllowedGroups->getItemById(id);
	if (item)
	{
		LLSD args;
		args["GROUP"] = item->getColumn(0)->getValue().asString();
		gNotifications.add("GroupIsAlreadyInList", args);
		return;
	}

	LLSD payload;
	payload["operation"] = (S32)ESTATE_ACCESS_ALLOWED_GROUP_ADD;
	payload["dialog_name"] = "EstateAllowedGroupAdd";
	payload["allowed_ids"].append(id);

	LLSD args;
	args["ALL_ESTATES"] = allEstatesText();

	LLNotification::Params params("EstateAllowedGroupAdd");
	params.payload(payload).substitutions(args).functor(accessCoreConfirm);
	if (LLPanelEstateInfo::isLindenEstate())
	{
		gNotifications.forceResponse(params, 0);
	}
	else
	{
		gNotifications.add(params);
	}
}

//static
void LLPanelEstateAccess::accessAddCore(U32 operation_flag)
{
	std::string dialog_name;
	switch (operation_flag)
	{
		case ESTATE_ACCESS_MANAGER_ADD:
			dialog_name = "EstateManagerAdd";
			break;

		case ESTATE_ACCESS_ALLOWED_AGENT_ADD:
			dialog_name = "EstateAllowedAgentAdd";
			break;

		case ESTATE_ACCESS_BANNED_AGENT_ADD:
			dialog_name = "EstateBannedAgentAdd";
			break;

		default:
			llwarns << "Invalid remove operation requested: " << operation_flag
					<< llendl;
			llassert(false);
			return;
	}

	LLSD payload;
	payload["operation"] = (S32)operation_flag;
	payload["dialog_name"] = dialog_name;
	// Avatar id filled in after avatar picker

	LLNotification::Params params("ChangeLindenAccess");
	params.payload(payload).functor(accessAddCore2);

	if (LLPanelEstateInfo::isLindenEstate())
	{
		gNotifications.add(params);
	}
	else
	{
		// Same as clicking "OK"
		gNotifications.forceResponse(params, 0);
	}
}

//static
bool LLPanelEstateAccess::accessAddCore2(const LLSD& notification,
										 const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response) == 0)
	{
		LLEstateAccessChangeInfo* change_info =
			new LLEstateAccessChangeInfo(notification["payload"]);
		// Avatar picker yes multi-select, yes close-on-select
		LLFloaterAvatarPicker::show(accessAddCore3, (void*)change_info, true,
									true);
	}
	return false;
}

//static
void LLPanelEstateAccess::accessAddCore3(const std::vector<std::string>& names,
										 const uuid_vec_t& ids, void* data)
{
	LLEstateAccessChangeInfo* change_info = (LLEstateAccessChangeInfo*)data;
	if (!change_info) return;

	LLPanelEstateAccess* self = LLFloaterRegionInfo::getPanelAccess();
	LLViewerRegion* region = gAgent.getRegion();
	if (!self || !region || ids.empty())
	{
		delete change_info;
		return;
	}

	// User did select a name. Note: cannot put estate owner on ban list.
	change_info->mAgentOrGroupIDs = ids;

	if (change_info->mOperationFlag & ESTATE_ACCESS_ALLOWED_AGENT_ADD)
	{
		LLNameListCtrl*	name_list = self->mAllowedAvatars;
		S32 list_count = name_list->getItemCount();
		S32 total = ids.size() + list_count;
		if (total > ESTATE_MAX_ACCESS_IDS)
		{
			LLSD args;
			args["NUM_ADDED"] = llformat("%d", ids.size());
			args["MAX_AGENTS"] = llformat("%d", ESTATE_MAX_ACCESS_IDS);
			args["LIST_TYPE"] = "Allowed Residents";
			args["NUM_EXCESS"] = llformat("%d", total - ESTATE_MAX_ACCESS_IDS);
			gNotifications.add("MaxAgentOnRegionBatch", args);
			delete change_info;
			return;
		}

		uuid_vec_t ids_allowed;
		std::string already_allowed, first, last;
		bool single = true;
		for (U32 i = 0, count = ids.size(); i < count; ++i)
		{
			const LLUUID& id = ids[i];
			LLScrollListItem* item = name_list->getItemById(id);
			if (item)
			{
				if (!already_allowed.empty())
				{
					already_allowed += ", ";
					single = false;
				}
				already_allowed += item->getColumn(0)->getValue().asString();
			}
			else
			{
				ids_allowed.emplace_back(id);
				// Used to trigger a name caching request, in anticipation
				// for confirmation dialogs.
				gCacheNamep->getName(id, first, last);
			}
		}
		if (!already_allowed.empty())
		{
			LLSD args;
			args["AGENT"] = already_allowed;
			args["LIST_TYPE"] =
				self->getString("RegionInfoListTypeAllowedAgents");
			std::string dialog = single ? "AgentIsAlreadyInList"
										: "AgentsAreAlreadyInList";
			gNotifications.add(dialog, args);
			if (ids_allowed.empty())
			{
				delete change_info;
				return;
			}
		}
		change_info->mAgentOrGroupIDs = ids_allowed;
	}

	if (change_info->mOperationFlag & ESTATE_ACCESS_BANNED_AGENT_ADD)
	{
		LLNameListCtrl*	name_list = self->mBannedAvatars;
		S32 list_count = name_list->getItemCount();
		S32 total = ids.size() + list_count;
		if (total > ESTATE_MAX_BANNED_IDS)
		{
			LLSD args;
			args["NUM_ADDED"] = llformat("%d", ids.size());
			args["MAX_AGENTS"] = llformat("%d", ESTATE_MAX_BANNED_IDS);
			args["LIST_TYPE"] = "Banned Residents";
			args["NUM_EXCESS"] = llformat("%d", total - ESTATE_MAX_BANNED_IDS);
			gNotifications.add("MaxAgentOnRegionBatch", args);
			delete change_info;
			return;
		}

		LLNameListCtrl*	em_list = self->mEstateManagers;
		uuid_vec_t ids_banned;
		std::string already_banned, em_ban, first, last;
		bool single = true;
		for (U32 i = 0, count = ids.size(); i < count; ++i)
		{
			const LLUUID& id = ids[i];
			bool can_ban = true;
			LLScrollListItem* em_item = em_list->getItemById(id);
			if (em_item)
			{
				if (!em_ban.empty())
				{
					em_ban += ", ";
				}
				em_ban += em_item->getColumn(0)->getValue().asString();
				can_ban = false;
			}

			LLScrollListItem* item = name_list->getItemById(id);
			if (item)
			{
				if (!already_banned.empty())
				{
					already_banned += ", ";
					single = false;
				}
				already_banned += item->getColumn(0)->getValue().asString();
				can_ban = false;
			}

			if (can_ban)
			{
				ids_banned.emplace_back(id);
				// Used to trigger a name caching request, in anticipation
				// for confirmation dialogs.
				gCacheNamep->getName(id, first, last);
			}
		}
		if (!em_ban.empty())
		{
			LLSD args;
			args["AGENT"] = em_ban;
			gNotifications.add("ProblemBanningEstateManager", args);
			if (ids_banned.empty())
			{
				delete change_info;
				return;
			}
		}
		if (!already_banned.empty())
		{
			LLSD args;
			args["AGENT"] = already_banned;
			args["LIST_TYPE"] =
				self->getString("RegionInfoListTypeBannedAgents");
			std::string dialog = single ? "AgentIsAlreadyInList"
										: "AgentsAreAlreadyInList";
			gNotifications.add(dialog, args);
			if (ids_banned.empty())
			{
				delete change_info;
				return;
			}
		}
		change_info->mAgentOrGroupIDs = ids_banned;
	}

	LLSD args;
	args["ALL_ESTATES"] = allEstatesText();
	LLNotification::Params params(change_info->mDialogName);
	params.substitutions(args).payload(change_info->asLLSD()).functor(accessCoreConfirm);
	if (LLPanelEstateInfo::isLindenEstate())
	{
		// Just apply to this estate
		gNotifications.forceResponse(params, 0);
	}
	else
	{
		// Ask if this estate or all estates with this owner
		gNotifications.add(params);
	}
}

//static
void LLPanelEstateAccess::accessRemoveCore(U32 operation_flag)
{
	LLPanelEstateAccess* self = LLFloaterRegionInfo::getPanelAccess();
	if (!self) return;

	std::string dialog_name;
	LLNameListCtrl* name_list;
	switch (operation_flag)
	{
		case ESTATE_ACCESS_MANAGER_REMOVE:
			dialog_name = "EstateManagerRemove";
			name_list = self->mEstateManagers;
			break;

		case ESTATE_ACCESS_ALLOWED_GROUP_REMOVE:
			dialog_name = "EstateAllowedGroupRemove";
			name_list = self->mAllowedGroups;
			break;

		case ESTATE_ACCESS_ALLOWED_AGENT_REMOVE:
			dialog_name = "EstateAllowedAgentRemove";
			name_list = self->mAllowedAvatars;
			break;

		case ESTATE_ACCESS_BANNED_AGENT_REMOVE:
			dialog_name = "EstateBannedAgentRemove";
			name_list = self->mBannedAvatars;
			break;

		default:
			llwarns << "Invalid remove operation requested: " << operation_flag
					<< llendl;
			llassert(false);
			return;
	}

	LLScrollListCtrl::items_vec_t list_vector = name_list->getAllSelected();
	if (list_vector.empty())
	{
		return;
	}

	LLSD payload;
	payload["operation"] = (S32)operation_flag;
	payload["dialog_name"] = dialog_name;

	for (U32 i = 0, count = list_vector.size(); i < count; ++i)
	{
		LLScrollListItem* itemp = list_vector[i];	// Cannot be NULL. HB
		payload["allowed_ids"].append(itemp->getUUID());
	}

	LLNotification::Params params("ChangeLindenAccess");
	params.payload(payload).functor(accessRemoveCore2);

	if (LLPanelEstateInfo::isLindenEstate())
	{
		// Warn on change linden estate
		gNotifications.add(params);
	}
	else
	{
		// Just proceed, as if clicking OK
		gNotifications.forceResponse(params, 0);
	}
}

//static
bool LLPanelEstateAccess::accessRemoveCore2(const LLSD& notification,
											const LLSD& response)
{
	if (LLNotification::getSelectedOption(notification, response))
	{
		return false;
	}

	// If Linden estate, can only apply to "this" estate, not all estates
	// owned by NULL.
	if (LLPanelEstateInfo::isLindenEstate())
	{
		accessCoreConfirm(notification, response);
	}
	else
	{
		LLSD args;
		args["ALL_ESTATES"] = allEstatesText();
		gNotifications.add(notification["payload"]["dialog_name"], args,
						   notification["payload"], accessCoreConfirm);
	}

	return false;
}

// Used for both access add and remove operations, depending on the
// mOperationFlag passed in (ESTATE_ACCESS_BANNED_AGENT_ADD,
// ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, etc.)
//static
bool LLPanelEstateAccess::accessCoreConfirm(const LLSD& notification,
											const LLSD& response)
{
	LL_DEBUGS("RegionInfo") << "Sending access changes" << LL_ENDL;

	LLPanelEstateAccess* self = LLFloaterRegionInfo::getPanelAccess();
	LLViewerRegion* region = gAgent.getRegion();
	if (!self || !region || !gCacheNamep) return false;

	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 2)	// Cancel button
	{
		return false;
	}

	const LLSD& payload = notification["payload"];
	const U32 orig_flags = (U32)payload["operation"].asInteger();
	U32 flags = orig_flags;

	if (option == 1)
	{
		// All estates, either than I own or manage for this owner; this will
		// be verified on simulator. JC
		if (region->getOwner() == gAgentID || gAgent.isGodlike())
		{
			flags |= ESTATE_ACCESS_APPLY_TO_ALL_ESTATES;
		}
		else if (region->isEstateManager())
		{
			flags |= ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES;
		}
	}

	std::string names, fullname;
	U32 listed_names = 0;
	const LLSD& allowed_ids = payload["allowed_ids"];
	for (U32 i = 0, count = allowed_ids.size(); i < count; ++i)
	{
		if (i != count - 1)
		{
			flags |= ESTATE_ACCESS_NO_REPLY;
		}
		else
		{
			flags &= ~ESTATE_ACCESS_NO_REPLY;
		}

		const LLUUID id = allowed_ids[i].asUUID();
		if ((orig_flags & ESTATE_ACCESS_BANNED_AGENT_ADD) &&
			region->getOwner() == id)
		{
			gNotifications.add("OwnerCanNotBeDenied");
			break;
		}

		sendEstateAccessDelta(flags, id);

		if ((flags & (ESTATE_ACCESS_ALLOWED_GROUP_ADD |
					  ESTATE_ACCESS_ALLOWED_GROUP_REMOVE)) == 0)
		{
			// Fill the name list for confirmation
			if (listed_names < MAX_LISTED_NAMES)
			{
				if (!names.empty())
				{
					names += ", ";
				}
				gCacheNamep->getFullName(id, fullname);
				names += fullname;
			}
			++listed_names;
		}
	}

	if (listed_names > MAX_LISTED_NAMES)
	{
		LLStringUtil::format_map_t args;
		args["EXTRA_COUNT"] = llformat("%d", listed_names - MAX_LISTED_NAMES);
		names += " " + self->getString("AndNMore", args);
	}

	if (!names.empty())
	{
		// Show the confirmation
		LLSD args;
		args["AGENT"] = names;
		if (flags & (ESTATE_ACCESS_ALLOWED_AGENT_ADD |
					 ESTATE_ACCESS_ALLOWED_AGENT_REMOVE))
		{
			args["LIST_TYPE"] =
				self->getString("RegionInfoListTypeAllowedAgents");
		}
		else if (flags & (ESTATE_ACCESS_BANNED_AGENT_ADD |
						  ESTATE_ACCESS_BANNED_AGENT_REMOVE))
		{
			args["LIST_TYPE"] =
				self->getString("RegionInfoListTypeBannedAgents");
		}

		if (flags & ESTATE_ACCESS_APPLY_TO_ALL_ESTATES)
		{
			args["ESTATE"] = self->getString("RegionInfoAllEstates");
		}
		else if (flags & ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES)
		{
			args["ESTATE"] = self->getString("RegionInfoManagedEstates");
		}
		else
		{
			args["ESTATE"] = self->getString("RegionInfoThisEstate");
		}

		std::string dialog;
		if (flags & (ESTATE_ACCESS_ALLOWED_AGENT_ADD |
					 ESTATE_ACCESS_BANNED_AGENT_ADD))
		{
			dialog = listed_names == 1 ? "AgentWasAddedToList"
									   : "AgentsWereAddedToList";
		}
		else if (flags & (ESTATE_ACCESS_ALLOWED_AGENT_REMOVE |
						  ESTATE_ACCESS_BANNED_AGENT_REMOVE))
		{
			dialog = listed_names == 1 ? "AgentWasRemovedFromList"
									   : "AgentsWereRemovedFromList";
		}
		if (!dialog.empty())
		{
			gNotifications.add(dialog, args);
		}
	}

	self->setPendingUpdate(true);

	return false;
}

// key = "estateaccessdelta"
// str(estate_id) will be added to front of list by
//                forward_EstateOwnerRequest_to_dataserver
// str[0] = str(agent_id) requesting the change
// str[1] = str(flags) (ESTATE_ACCESS_DELTA_*)
// str[2] = str(agent_id) to add or remove
//static
void LLPanelEstateAccess::sendEstateAccessDelta(U32 flags, const LLUUID& id)
{
	LLMessageSystem* msg = gMessageSystemp;
	if (!msg) return;

	msg->newMessage(_PREHASH_EstateOwnerMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); // not used

	msg->nextBlock(_PREHASH_MethodData);
	msg->addString(_PREHASH_Method, "estateaccessdelta");
	msg->addUUID(_PREHASH_Invoice, LLFloaterRegionInfo::getLastInvoice());

	std::string buf;
	gAgentID.toString(buf);
	msg->nextBlock(_PREHASH_ParamList);
	msg->addString(_PREHASH_Parameter, buf);

	buf = llformat("%u", flags);
	msg->nextBlock(_PREHASH_ParamList);
	msg->addString(_PREHASH_Parameter, buf);

	id.toString(buf);
	msg->nextBlock(_PREHASH_ParamList);
	msg->addString(_PREHASH_Parameter, buf);

	gAgent.sendReliableMessage();

	// This was part of the old pre-capability code, so do it when the
	// capability is not in use; the deleteAllItems() are disabled in the
	// LLDispatchSetEstateAccess() code for the capability-less case (likely
	// because we could receive several UDP packets, each contaning a part of
	// the full list), so we need to deleteAllItems() here instead... HB.
	if ((flags & (ESTATE_ACCESS_ALLOWED_AGENT_ADD |
				  ESTATE_ACCESS_ALLOWED_AGENT_REMOVE |
		          ESTATE_ACCESS_BANNED_AGENT_ADD |
				  ESTATE_ACCESS_BANNED_AGENT_REMOVE)) &&
		!gAgent.hasRegionCapability("EstateAccess"))
	{

		LLPanelEstateAccess* self = LLFloaterRegionInfo::getPanelAccess();
		if (self)
		{
			self->mAllowedAvatars->deleteAllItems();
			self->mBannedAvatars->deleteAllItems();
		}
	}
}

void LLPanelEstateAccess::updateControls()
{
	bool god_or_owner = mOwner || gAgent.isGodlike();
	bool can_control = god_or_owner || mEstateManager;
	LL_DEBUGS("RegionInfo") << " - god_or_owner = "
							<< (god_or_owner ? "true" : "false")
			  				<< " - can_control = "
							<< (can_control ? "true" : "false") << LL_ENDL;
	setCtrlsEnabled(can_control);

	childSetEnabled("add_allowed_group_btn", can_control);
	childSetEnabled("remove_allowed_group_btn",
					can_control && mAllowedGroups->getFirstSelected());
	childSetEnabled("add_allowed_avatar_btn", can_control);
	childSetEnabled("remove_allowed_avatar_btn",
					can_control && mAllowedAvatars->getFirstSelected());
	childSetEnabled("add_banned_avatar_btn",  can_control);
	childSetEnabled("remove_banned_avatar_btn",
					can_control && mBannedAvatars->getFirstSelected());

	// Estate managers cannot add estate managers
	childSetEnabled("add_estate_manager_btn", god_or_owner);
	childSetEnabled("remove_estate_manager_btn",
					god_or_owner && mEstateManagers->getFirstSelected());
	childSetEnabled("estate_manager_name_list", god_or_owner);

	if (mCtrlsEnabled != can_control)
	{
		mCtrlsEnabled = can_control;
		// Update the lists on the agent's access level change
		updateLists();
	}
}

void LLPanelEstateAccess::setAccessAllowedEnabled(bool enable_agent,
												  bool enable_group,
												  bool enable_ban)
{
	LL_DEBUGS("RegionInfo") << "enable_agent = "
							<< (enable_agent ? "true" : "false")
							<< " - enable_group = "
							<< (enable_group ? "true" : "false")
							<< " - enable_ban = "
							<< (enable_ban ? "true" : "false") << LL_ENDL;
	childSetEnabled("allow_group_label", enable_group);
	childSetEnabled("add_allowed_group_btn", enable_group);
	childSetEnabled("remove_allowed_group_btn", enable_group);
	mAllowedGroups->setEnabled(enable_group);

	childSetEnabled("allow_resident_label", enable_agent);
	childSetEnabled("add_allowed_avatar_btn", enable_agent);
	childSetEnabled("remove_allowed_avatar_btn", enable_agent);
	mAllowedAvatars->setEnabled(enable_agent);

	childSetEnabled("ban_resident_label", enable_ban);
	childSetEnabled("add_banned_avatar_btn", enable_ban);
	childSetEnabled("remove_banned_avatar_btn", enable_ban);
	mBannedAvatars->setEnabled(enable_ban);

	// Update removal buttons if needed
	if (enable_group)
	{
		checkRemovalButton("allowed_group_name_list");
	}
	if (enable_agent)
	{
		checkRemovalButton("allowed_avatar_name_list");
	}
	if (enable_ban)
	{
		checkRemovalButton("banned_avatar_name_list");
	}
}

// Enables/disables the "remove" button for the various allow/ban lists
bool LLPanelEstateAccess::checkRemovalButton(std::string name)
{
	std::string btn_name;
	if (name == "allowed_avatar_name_list")
	{
		btn_name = "remove_allowed_avatar_btn";
	}
	else if (name == "allowed_group_name_list")
	{
		btn_name = "remove_allowed_group_btn";
	}
	else if (name == "banned_avatar_name_list")
	{
		btn_name = "remove_banned_avatar_btn";
	}
	else if (name == "estate_manager_name_list")
	{
		// ONLY OWNER CAN ADD / DELETE ESTATE MANAGER
		LLViewerRegion* region = gAgent.getRegion();
		if (region && region->getOwner() == gAgentID)
		{
			btn_name = "remove_estate_manager_btn";
		}
	}

	// Enable the remove button if something is selected
	LLNameListCtrl* name_list = getChild<LLNameListCtrl>(name.c_str(),
														 true, false);
	if (name_list && !btn_name.empty())
	{
		childSetEnabled(btn_name.c_str(),
						name_list->getFirstSelected() != NULL);
	}

	return !btn_name.empty();
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelEstateCovenant
///////////////////////////////////////////////////////////////////////////////

LLPanelEstateCovenant::LLPanelEstateCovenant()
:	LLPanelRegionInfo(),
	mCovenantID(LLUUID::null)
{
}

//virtual
bool LLPanelEstateCovenant::postBuild()
{
	initHelpBtn("covenant_help", "HelpEstateCovenant");
	mEstateNameText = getChild<LLTextBox>("estate_name_text");
	mEstateOwnerText = getChild<LLTextBox>("estate_owner_text");
	mLastModifiedText = getChild<LLTextBox>("covenant_timestamp_text");
	mEditor = getChild<LLViewerTextEditor>("covenant_editor");
	mEditor->setHandleEditKeysDirectly(true);
	LLButton* reset_button = getChild<LLButton>("reset_covenant");
	reset_button->setEnabled(gAgent.canManageEstate());
	reset_button->setClickedCallback(resetCovenantID, this);

	return LLPanelRegionInfo::postBuild();
}

//virtual
bool LLPanelEstateCovenant::refreshFromRegion(LLViewerRegion* regionp)
{
	if (!LLPanelRegionInfo::refreshFromRegion(regionp))
	{
		return false;
	}

	LLTextBox* textp = getChild<LLTextBox>("region_name_text", true, false);
	if (textp)
	{
		textp->setText(regionp->getName());
	}

	textp = getChild<LLTextBox>("resellable_clause", true, false);
	if (textp)
	{
		if (regionp->getRegionFlag(REGION_FLAGS_BLOCK_LAND_RESELL))
		{
			textp->setText(getString("can_not_resell"));
		}
		else
		{
			textp->setText(getString("can_resell"));
		}
	}

	textp = getChild<LLTextBox>("changeable_clause", true, false);
	if (textp)
	{
		if (regionp->getRegionFlag(REGION_FLAGS_ALLOW_PARCEL_CHANGES))
		{
			textp->setText(getString("can_change"));
		}
		else
		{
			textp->setText(getString("can_not_change"));
		}
	}

	textp = getChild<LLTextBox>("region_maturity_text", true, false);
	if (textp)
	{
		textp->setText(regionp->getSimAccessString());
	}

	textp = getChild<LLTextBox>("region_landtype_text", true, false);
	if (textp)
	{
		textp->setText(regionp->getSimProductName());
	}

	regionp->sendEstateCovenantRequest();

	return true;
}

//virtual
bool LLPanelEstateCovenant::estateUpdate(LLMessageSystem* msg)
{
	llinfos << "No operation..." << llendl;
	return true;
}

//virtual
bool LLPanelEstateCovenant::handleDragAndDrop(S32 x, S32 y, MASK mask,
											  bool drop,
											  EDragAndDropType cargo_type,
											  void* cargo_data,
											  EAcceptance* accept,
											  std::string& tooltip_msg)
{
	if (!gAgent.canManageEstate())
	{
		*accept = ACCEPT_NO;
		return true;
	}

	if (cargo_type == DAD_NOTECARD)
	{
		*accept = ACCEPT_YES_COPY_SINGLE;
		LLInventoryItem* item = (LLInventoryItem*)cargo_data;
		if (item && drop)
		{
			LLSD payload;
			payload["item_id"] = item->getUUID();
			gNotifications.add("EstateChangeCovenant", LLSD(), payload,
							   confirmChangeCovenantCallback);
		}
	}
	else
	{
		*accept = ACCEPT_NO;
	}

	return true;
}

//static
bool LLPanelEstateCovenant::confirmChangeCovenantCallback(const LLSD& notification,
														  const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLInventoryItem* item = gInventory.getItem(notification["payload"]["item_id"].asUUID());
	LLPanelEstateCovenant* self = LLFloaterRegionInfo::getPanelCovenant();
	if (!item || !self) return false;

	if (option == 0)
	{
		self->loadInvItem(item);
	}
	return false;
}

//static
void LLPanelEstateCovenant::resetCovenantID(void*)
{
	gNotifications.add("EstateChangeCovenant", LLSD(), LLSD(),
					   confirmResetCovenantCallback);
}

//static
bool LLPanelEstateCovenant::confirmResetCovenantCallback(const LLSD& notification,
														 const LLSD& response)
{
	LLPanelEstateCovenant* self = LLFloaterRegionInfo::getPanelCovenant();
	if (!self) return false;

	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		self->loadInvItem(NULL);
	}
	return false;
}

void LLPanelEstateCovenant::loadInvItem(LLInventoryItem* itemp)
{
	if (!gAssetStoragep)
	{
		llwarns << "No valid asset storage. Aborted." << llendl;
		return;
	}
	if (itemp)
	{
		gAssetStoragep->getInvItemAsset(gAgent.getRegionHost(),
										gAgentID, gAgentSessionID,
										itemp->getPermissions().getOwner(),
										LLUUID::null, itemp->getUUID(),
										itemp->getAssetUUID(),
										itemp->getType(), onLoadComplete,
										(void*)this, true); // high priority
		mAssetStatus = ASSET_LOADING;
	}
	else
	{
		mAssetStatus = ASSET_LOADED;
		setCovenantTextEditor("There is no Covenant provided for this Estate.");
		sendChangeCovenantID(LLUUID::null);
	}
}

//static
void LLPanelEstateCovenant::onLoadComplete(const LLUUID& asset_id,
										   LLAssetType::EType type,
										   void* user_data,
										   S32 status, LLExtStat)
{
	LLPanelEstateCovenant* panelp = (LLPanelEstateCovenant*)user_data;
	if (!panelp)
	{
		return;
	}
	if (status == 0)
	{
		LLFileSystem file(asset_id);
		S32 file_length = file.getSize();

		char* buffer = new char[file_length + 1];
		if (!buffer)
		{
			llerrs << "Memory Allocation Failed" << llendl;
		}
		file.read((U8*)buffer, file_length);
		// Put a EOS at the end
		buffer[file_length] = 0;

		if (file_length > 19 && !strncmp(buffer, "Linden text version", 19))
		{
			if (!panelp->mEditor->importBuffer(buffer, file_length + 1))
			{
				llwarns << "Problem importing estate covenant." << llendl;
				gNotifications.add("ProblemImportingEstateCovenant");
			}
			else
			{
				panelp->sendChangeCovenantID(asset_id);
			}
		}
		else
		{
			// Version 0 (just text, doesn't include version number)
			panelp->sendChangeCovenantID(asset_id);
		}
		delete[] buffer;
	}
	else
	{
		gViewerStats.incStat(LLViewerStats::ST_DOWNLOAD_FAILED);

		if (LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
			LL_ERR_FILE_EMPTY == status)
		{
			gNotifications.add("MissingNotecardAssetID");
		}
		else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
		{
			gNotifications.add("NotAllowedToViewNotecard");
		}
		else
		{
			gNotifications.add("UnableToLoadNotecardAsset");
		}

		llwarns << "Problem loading notecard: " << status << llendl;
	}
	panelp->mAssetStatus = ASSET_LOADED;
	panelp->setCovenantID(asset_id);
}

// key = "estatechangecovenantid"
// strings[0] = str(estate_id) (added by simulator before relay - not here)
// strings[1] = str(covenant_id)
void LLPanelEstateCovenant::sendChangeCovenantID(const LLUUID& asset_id)
{
	if (asset_id != getCovenantID())
	{
        setCovenantID(asset_id);

		LLMessageSystem* msg = gMessageSystemp;
		msg->newMessage(_PREHASH_EstateOwnerMessage);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
		msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); // Not used

		msg->nextBlock(_PREHASH_MethodData);
		msg->addString(_PREHASH_Method, "estatechangecovenantid");
		msg->addUUID(_PREHASH_Invoice, LLFloaterRegionInfo::getLastInvoice());

		msg->nextBlock(_PREHASH_ParamList);
		msg->addString(_PREHASH_Parameter, getCovenantID().asString());
		gAgent.sendReliableMessage();
	}
}

//virtual
bool LLPanelEstateCovenant::sendUpdate()
{
	return true;
}

const std::string& LLPanelEstateCovenant::getEstateName() const
{
	return mEstateNameText->getText();
}

void LLPanelEstateCovenant::setEstateName(const std::string& name)
{
	mEstateNameText->setText(name);
}

//static
void LLPanelEstateCovenant::updateCovenantText(const std::string& string,
											   const LLUUID& asset_id)
{
	LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
	if (panelp)
	{
		panelp->mEditor->setText(string);
		panelp->setCovenantID(asset_id);
	}
}

//static
void LLPanelEstateCovenant::updateEstateName(const std::string& name)
{
	LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
	if (panelp)
	{
		panelp->mEstateNameText->setText(name);
	}
}

//static
void LLPanelEstateCovenant::updateLastModified(const std::string& text)
{
	LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
	if (panelp)
	{
		panelp->mLastModifiedText->setText(text);
	}
}

//static
void LLPanelEstateCovenant::updateEstateOwnerName(const std::string& name)
{
	LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
	if (panelp)
	{
		panelp->mEstateOwnerText->setText(name);
	}
}

const std::string& LLPanelEstateCovenant::getOwnerName() const
{
	return mEstateOwnerText->getText();
}

void LLPanelEstateCovenant::setOwnerName(const std::string& name)
{
	mEstateOwnerText->setText(name);
}

void LLPanelEstateCovenant::setCovenantTextEditor(const std::string& text)
{
	mEditor->setText(text);
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelRegionExperiences
///////////////////////////////////////////////////////////////////////////////

// static
void* LLPanelRegionExperiences::createAllowedExperiencesPanel(void* data)
{
	LLPanelRegionExperiences* self = (LLPanelRegionExperiences*)data;
	self->mAllowed = new LLPanelExperienceListEditor();
	return self->mAllowed;
}

// static
void* LLPanelRegionExperiences::createTrustedExperiencesPanel(void* data)
{
	LLPanelRegionExperiences* self = (LLPanelRegionExperiences*)data;
	self->mTrusted = new LLPanelExperienceListEditor();
	return self->mTrusted;
}

// static
void* LLPanelRegionExperiences::createBlockedExperiencesPanel(void* data)
{
	LLPanelRegionExperiences* self = (LLPanelRegionExperiences*)data;
	self->mBlocked = new LLPanelExperienceListEditor();
	return self->mBlocked;
}

LLPanelRegionExperiences::LLPanelRegionExperiences()
:	LLPanelRegionInfo()
{
	LLCallbackMap::map_t factory_map;
	factory_map["panel_allowed"] = LLCallbackMap(createAllowedExperiencesPanel,
												 this);
	factory_map["panel_trusted"] = LLCallbackMap(createTrustedExperiencesPanel,
												 this);
	factory_map["panel_blocked"] = LLCallbackMap(createBlockedExperiencesPanel,
												 this);
	LLUICtrlFactory::getInstance()->buildPanel(this,
											   "panel_region_experiences.xml",
											   &factory_map);
}

bool LLPanelRegionExperiences::postBuild()
{
	if (!mAllowed || !mTrusted || !mBlocked) return false;

	setupList(mAllowed, "panel_allowed",
			  ESTATE_EXPERIENCE_ALLOWED_ADD, ESTATE_EXPERIENCE_ALLOWED_REMOVE);
	setupList(mTrusted, "panel_trusted",
			  ESTATE_EXPERIENCE_TRUSTED_ADD, ESTATE_EXPERIENCE_TRUSTED_REMOVE);
	setupList(mBlocked, "panel_blocked",
			  ESTATE_EXPERIENCE_BLOCKED_ADD, ESTATE_EXPERIENCE_BLOCKED_REMOVE);

	getChild<LLPanel>("help_text_layout_panel")->setVisible(true);
	getChild<LLPanel>("trusted_layout_panel")->setVisible(true);
	mTrusted->getChild<LLTextBox>("text_name")->setToolTip(getString("trusted_estate_text"));
	mAllowed->getChild<LLTextBox>("text_name")->setToolTip(getString("allowed_estate_text"));
	mBlocked->getChild<LLTextBox>("text_name")->setToolTip(getString("blocked_estate_text"));

	// Note: no apply button, so we do not call LLPanelRegionInfo::postBuild()
	return true;
}

void LLPanelRegionExperiences::setupList(LLPanelExperienceListEditor* panelp,
										 const std::string& control_name,
										 U32 add_id, U32 remove_id)
{
	if (!panelp) return;

	panelp->getChild<LLTextBox>("text_name")->setText(panelp->getString(control_name));
	panelp->setMaxExperienceIDs(ESTATE_MAX_EXPERIENCE_IDS);
	panelp->setAddedCallback(std::bind(&LLPanelRegionExperiences::itemChanged,
									   this, add_id, _1));
	panelp->setRemovedCallback(std::bind(&LLPanelRegionExperiences::itemChanged,
										this, remove_id, _1));
}

void LLPanelRegionExperiences::processResponse(const LLSD& content)
{
	if (content.has("default"))
	{
		mDefaultExperience = content["default"].asUUID();
	}

	mAllowed->setExperienceIds(content["allowed"]);
	mBlocked->setExperienceIds(content["blocked"]);

	LLSD trusted = content["trusted"];
	if (mDefaultExperience.notNull())
	{
		mTrusted->setStickyFunction(std::bind(LLExperienceCache::FilterMatching,
											  _1, mDefaultExperience));
		trusted.append(mDefaultExperience);
	}

	mTrusted->setExperienceIds(trusted);

	mAllowed->refreshExperienceCounter();
	mBlocked->refreshExperienceCounter();
	mTrusted->refreshExperienceCounter();
}

// Used for both access add and remove operations, depending on the flag passed
// in (ESTATE_EXPERIENCE_ALLOWED_ADD, ESTATE_EXPERIENCE_ALLOWED_REMOVE, etc.)
//static
bool LLPanelRegionExperiences::experienceCoreConfirm(const LLSD& notification,
													 const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	const U32 orig_flags = (U32)notification["payload"]["operation"].asInteger();

	LLViewerRegion* region = gAgent.getRegion();

	for (LLSD::array_const_iterator
			iter = notification["payload"]["allowed_ids"].beginArray(),
			end_it = notification["payload"]["allowed_ids"].endArray();
		 iter != end_it; ++iter)
	{
		U32 flags = orig_flags;
		if (iter + 1 != end_it)
		{
			flags |= ESTATE_ACCESS_NO_REPLY;
		}

		const LLUUID id = iter->asUUID();
		switch (option)
		{
			case 0:
			    // This estate
			    sendEstateExperienceDelta(flags, id);
			    break;

			case 1:
			{
				// All estates, either than I own or manage for this owner.
				// This will be verified on simulator. JC
				if (!region) break;
				if (region->getOwner() == gAgentID || gAgent.isGodlike())
				{
					flags |= ESTATE_ACCESS_APPLY_TO_ALL_ESTATES;
					sendEstateExperienceDelta(flags, id);
				}
				else if (region->isEstateManager())
				{
					flags |= ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES;
					sendEstateExperienceDelta(flags, id);
				}
				break;
			}

			default:
			    break;
		}
	}
	return false;
}

// Send the actual "estateexperiencedelta" message
void LLPanelRegionExperiences::sendEstateExperienceDelta(U32 flags,
														 const LLUUID& experience_id)
{
	LLPanelRegionExperiences* panelp =
		LLFloaterRegionInfo::getPanelExperiences();
	if (panelp)
	{
		strings_vec_t str(3, std::string());
		gAgentID.toString(str[0]);
		str[1] = llformat("%u", flags);
		experience_id.toString(str[2]);
		panelp->sendEstateOwnerMessage("estateexperiencedelta", str);
	}
}

void LLPanelRegionExperiences::infoCallback(LLHandle<LLPanelRegionExperiences> handle,
											const LLSD& content)
{
	if (handle.isDead()) return;

	LLPanelRegionExperiences* floater = handle.get();
	if (floater)
	{
		floater->processResponse(content);
	}
}

//static
const std::string& LLPanelRegionExperiences::regionCapabilityQuery(LLViewerRegion* region,
																   const char* cap)
{
	return region ? region->getCapability(cap) : LLStringUtil::null;
}

//virtual
bool LLPanelRegionExperiences::refreshFromRegion(LLViewerRegion* regionp)
{
	if (!LLPanelRegionInfo::refreshFromRegion(regionp))
	{
		return false;
	}

	bool allow_modify = mCanManage || gAgent.isGodlike();

	mAllowed->setDisabled(false);
	mAllowed->setReadonly(!allow_modify);
	mAllowed->loading();
	// Remove grid-wide experiences
	mAllowed->addFilter(std::bind(LLExperienceCache::FilterWithProperty, _1,
								  LLExperienceCache::PROPERTY_GRID));
	// Remove default experience
	mAllowed->addFilter(std::bind(LLExperienceCache::FilterMatching, _1,
								  mDefaultExperience));

	mBlocked->setDisabled(false);
	mBlocked->setReadonly(!allow_modify);
	mBlocked->loading();
	// Only grid-wide experiences
	mBlocked->addFilter(std::bind(LLExperienceCache::FilterWithoutProperty, _1,
								  LLExperienceCache::PROPERTY_GRID));
	// But not privileged ones
	mBlocked->addFilter(std::bind(LLExperienceCache::FilterWithProperty, _1,
								  LLExperienceCache::PROPERTY_PRIVILEGED));
	// Remove default experience
	mBlocked->addFilter(std::bind(LLExperienceCache::FilterMatching, _1,
								  mDefaultExperience));

	mTrusted->setDisabled(false);
	mTrusted->setReadonly(!allow_modify);
	mTrusted->loading();

	LLExperienceCache* exp = LLExperienceCache::getInstance();
	exp->getRegionExperiences(std::bind(&LLPanelRegionExperiences::regionCapabilityQuery,
										regionp, _1),
							  std::bind(&LLPanelRegionExperiences::infoCallback,
										getDerivedHandle<LLPanelRegionExperiences>(),
										_1));

	return true;
}

LLSD LLPanelRegionExperiences::addIds(LLPanelExperienceListEditor* panelp)
{
	LLSD ids;
	const uuid_list_t& id_list = panelp->getExperienceIds();
	for (uuid_list_t::const_iterator it = id_list.begin(),
									 end = id_list.end();
		 it != end; ++it)
	{
		ids.append(*it);
	}
	return ids;
}

bool LLPanelRegionExperiences::sendUpdate()
{
	if (!gAgent.hasRegionCapability("RegionExperiences")) return false;

	LLSD content;
	content["allowed"] = addIds(mAllowed);
	content["blocked"] = addIds(mBlocked);
	content["trusted"] = addIds(mTrusted);
	LLExperienceCache* exp = LLExperienceCache::getInstance();
	exp->setRegionExperiences(std::bind(&LLPanelRegionExperiences::regionCapabilityQuery,
										gAgent.getRegion(), _1), content,
							  std::bind(&LLPanelRegionExperiences::infoCallback,
										getDerivedHandle<LLPanelRegionExperiences>(),
										_1));
	return true;
}

void LLPanelRegionExperiences::itemChanged(U32 event_type, const LLUUID& id)
{
	std::string dialog_name;
	switch (event_type)
	{
		case ESTATE_EXPERIENCE_ALLOWED_ADD:
			dialog_name = "EstateAllowedExperienceAdd";
			break;

		case ESTATE_EXPERIENCE_ALLOWED_REMOVE:
			dialog_name = "EstateAllowedExperienceRemove";
			break;

		case ESTATE_EXPERIENCE_TRUSTED_ADD:
			dialog_name = "EstateTrustedExperienceAdd";
			break;

		case ESTATE_EXPERIENCE_TRUSTED_REMOVE:
			dialog_name = "EstateTrustedExperienceRemove";
			break;

		case ESTATE_EXPERIENCE_BLOCKED_ADD:
			dialog_name = "EstateBlockedExperienceAdd";
			break;

		case ESTATE_EXPERIENCE_BLOCKED_REMOVE:
			dialog_name = "EstateBlockedExperienceRemove";
			break;

		default:
			return;
	}

	LLSD payload;
	payload["operation"] = (S32)event_type;
	payload["dialog_name"] = dialog_name;
	payload["allowed_ids"].append(id);

	LLSD args;
	args["ALL_ESTATES"] = LLPanelEstateAccess::allEstatesText();

	LLNotification::Params p(dialog_name);
	p.substitutions(args).payload(payload)
						 .functor(std::bind(&LLPanelRegionExperiences::experienceCoreConfirm,
											_1, _2));
	if (LLPanelEstateInfo::isLindenEstate())
	{
		gNotifications.forceResponse(p, 0);
	}
	else
	{
		gNotifications.add(p);
	}

	onChangeAnything(NULL, this);
}
