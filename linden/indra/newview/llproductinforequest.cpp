/**
 * @file llproductinforequest.cpp
 * @author Kent Quirk
 * @brief Get region type descriptions (translation from SKU to description)
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llproductinforequest.h"

#include "llcorehttputil.h"
#include "lltrans.h"

#include "llagent.h"  // for gAgent

LLProductInfoRequestManager::LLProductInfoRequestManager()
:	mSkuDescriptions()
{
}

void LLProductInfoRequestManager::initSingleton()
{
	const std::string& url = gAgent.getRegionCapability("ProductInfoRequest");
	if (url.empty())
	{
		return;
	}
	gCoros.launch("LLProductInfoRequestManager::getLandDescriptionsCoro",
				  std::bind(&LLProductInfoRequestManager::getLandDescriptionsCoro,
							this, url));
}

std::string LLProductInfoRequestManager::getDescriptionForSku(const std::string& sku)
{
	// The description LLSD is an array of maps; each array entry has a map
	// with 3 fields: description, name, and sku
	for (LLSD::array_const_iterator it = mSkuDescriptions.beginArray(),
									end = mSkuDescriptions.endArray();
		 it != end; ++it)
	{
		LL_DEBUGS("ProductInfoRequestManager") <<  (*it)["sku"].asString()
											   << " = "
											   << (*it)["description"].asString()
											   << LL_ENDL;
		if ((*it)["sku"].asString() == sku)
		{
			return (*it)["description"].asString();
		}
	}
	return LLTrans::getString("unknown");
}

void LLProductInfoRequestManager::getLandDescriptionsCoro(const std::string& url)
{
	LLCoreHttpUtil::HttpCoroutineAdapter adapter("ProductInfoRequest");
	LLSD result = adapter.getAndSuspend(url);

	LLCore::HttpStatus status =
		LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result);
	if (!status)
	{
		llwarns << "Failure to fetch land SKU: " << status.toString()
				<< llendl;
	}
	else if (result.has(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT) &&
			 result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT].isArray())
	{
		mSkuDescriptions = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT];
	}
	else
	{
		llwarns << "Land SKU description response is malformed" << llendl;
	}
}
