/**
 * @file llinventory.cpp
 * @brief Implementation of the inventory system.
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

#include "linden_common.h"

#include <sstream>

#include "boost/tokenizer.hpp"

#include "llinventory.h"

#include "lldbstrings.h"
#include "hbfastset.h"
#include "llmessage.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "llxorcipher.h"
#include "hbxxh.h"

static const LLUUID MAGIC_ID("3c115e51-04f4-523c-9fa6-98aff1034730");

// Keys used by agent-inventory-service
static const std::string INV_ITEM_ID_LABEL("item_id");
static const std::string INV_PARENT_ID_LABEL("parent_id");
static const std::string INV_THUMBNAIL_LABEL("thumbnail");
static const std::string INV_THUMBNAIL_ID_LABEL("thumbnail_id");
static const std::string INV_ASSET_TYPE_LABEL("type");
static const std::string INV_INVENTORY_TYPE_LABEL("inv_type");
static const std::string INV_NAME_LABEL("name");
static const std::string INV_DESC_LABEL("desc");
static const std::string INV_PERMISSIONS_LABEL("permissions");
static const std::string INV_SHADOW_ID_LABEL("shadow_id");
static const std::string INV_ASSET_ID_LABEL("asset_id");
static const std::string INV_LINKED_ID_LABEL("linked_id");
static const std::string INV_SALE_INFO_LABEL("sale_info");
static const std::string INV_FLAGS_LABEL("flags");
static const std::string INV_CREATION_DATE_LABEL("created_at");
static const std::string INV_ASSET_TYPE_LABEL_WS("type_default");
static const std::string INV_FOLDER_ID_LABEL_WS("category_id");
static const std::string INV_LANGUAGE_LABEL("language");
static const std::string INV_TARGET_LABEL("target");

///----------------------------------------------------------------------------
/// Class LLInventoryObject
///----------------------------------------------------------------------------

LLInventoryObject::LLInventoryObject(const LLUUID& uuid,
									 const LLUUID& parent_uuid,
									 LLAssetType::EType type,
									 const std::string& name)
:	mUUID(uuid),
	mParentUUID(parent_uuid),
	mType(type),
	mName(name),
	mCreationDate(0)
{
	correctInventoryName(mName);
}

LLInventoryObject::LLInventoryObject()
:	mType(LLAssetType::AT_NONE),
	mCreationDate(0)
{
}

void LLInventoryObject::copyObject(const LLInventoryObject* otherp)
{
	mUUID = otherp->mUUID;
	mParentUUID = otherp->mParentUUID;
	mThumbnailUUID = otherp->mThumbnailUUID;
	mType = otherp->mType;
	mName = otherp->mName;
}

void LLInventoryObject::rename(const std::string& n)
{
	std::string new_name(n);
	correctInventoryName(new_name);
	if (!new_name.empty() && new_name != mName)
	{
		mName = new_name;
	}
}

//virtual
bool LLInventoryObject::importLegacyStream(std::istream& input_stream)
{
	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[MAX_STRING];
	char keyword[MAX_STRING];
	char valuestr[MAX_STRING];

	keyword[0] = '\0';
	valuestr[0] = '\0';
	while (input_stream.good())
	{
		input_stream.getline(buffer, MAX_STRING);
		// This happens a lot... And it does not matter. HB
		if (!strcmp("|", buffer))
		{
			continue;
		}
		if (sscanf(buffer, " %254s %254s", keyword, valuestr) < 1)
		{
			continue;
		}
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}", keyword))
		{
			break;
		}
		else if (!strcmp("obj_id", keyword))
		{
			mUUID.set(valuestr);
		}
		else if (!strcmp("parent_id", keyword))
		{
			mParentUUID.set(valuestr);
		}
		else if (!strcmp("type", keyword))
		{
			mType = LLAssetType::lookup(valuestr);
		}
		else if (!strcmp("name", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(buffer, " %254s %254[^|]", keyword, valuestr);
			mName.assign(valuestr);
			correctInventoryName(mName);
		}
		else if (!strcmp("metadata", keyword))
		{
			LLSD metadata;
#if 0
			if (!strncmp("<llsd>", valuestr, 6))
			{
				std::istringstream stream(valuestr);
				LLSDSerialize::fromXML(metadata, stream);
			}
			else
#endif
			{
				metadata = LLSD(valuestr);
			}
			if (metadata.has("thumbnail"))
			{
				const LLSD& thumbnail = metadata["thumbnail"];
				if (thumbnail.has("asset_id"))
				{
					setThumbnailUUID(thumbnail["asset_id"].asUUID());
				}
			}
		}
		else
		{
			llwarns << "Unknown keyword '" << keyword << "' for object "
					<< mUUID << ". Buffer contents: " << buffer << llendl;
		}
	}

	return true;
}

//virtual
bool LLInventoryObject::exportLegacyStream(std::ostream& output_stream,
										   bool) const
{
	std::string uuid_str;
	output_stream <<  "\tinv_object\t0\n\t{\n";
	mUUID.toString(uuid_str);
	output_stream << "\t\tobj_id\t" << uuid_str << "\n";
	mParentUUID.toString(uuid_str);
	output_stream << "\t\tparent_id\t" << uuid_str << "\n";
	output_stream << "\t\ttype\t" << LLAssetType::lookup(mType) << "\n";
	output_stream << "\t\tname\t" << mName.c_str() << "|\n";
	if (mThumbnailUUID.notNull())
	{
		// Max length is 255 chars, will have to export differently if it gets
		// more data (e.g. use newline and toNotation (uses {}) for unlimited
		// size).
		LLSD metadata;
		metadata["thumbnail"] = LLSD().with("asset_id", mThumbnailUUID);
#if 0
		output_stream << "\t\tmetadata\t";
		LLSDSerialize::toXML(metadata, output_stream);
		output_stream << "|\n";
#else
		output_stream << "\t\tmetadata\t" << metadata << "|\n";
#endif
	}
	output_stream << "\t}\n";
	return true;
}

void LLInventoryObject::updateParentOnServer(bool) const
{
	// Do not do anything
	llwarns << "No-operation call. This method should be overridden !"
			<< llendl;
}

void LLInventoryObject::updateServer(bool) const
{
	// Do not do anything
	llwarns << "No-operation call. This method should be overridden !"
			<< llendl;
}

//static
void LLInventoryObject::correctInventoryName(std::string& name)
{
	LLStringUtil::replaceNonstandardASCII(name, ' ');
	LLStringUtil::replaceChar(name, '|', ' ');
	LLStringUtil::trim(name);
	LLStringUtil::truncate(name, DB_INV_ITEM_NAME_STR_LEN);
}

///----------------------------------------------------------------------------
/// Class LLInventoryItem
///----------------------------------------------------------------------------

LLInventoryItem::LLInventoryItem(const LLUUID& uuid,
								 const LLUUID& parent_uuid,
								 const LLPermissions& permissions,
								 const LLUUID& asset_uuid,
								 LLAssetType::EType type,
								 LLInventoryType::EType inv_type,
								 const std::string& name,
								 const std::string& desc,
								 const LLSaleInfo& sale_info,
								 U32 flags,
								 S32 creation_date_utc)
:	LLInventoryObject(uuid, parent_uuid, type, name),
	mPermissions(permissions),
	mAssetUUID(asset_uuid),
	mDescription(desc),
	mSaleInfo(sale_info),
	mInventoryType(inv_type),
	mFlags(flags)
{
	mCreationDate = creation_date_utc;

	LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
	LLStringUtil::replaceChar(mDescription, '|', ' ');
	mPermissions.initMasks(inv_type);
}

LLInventoryItem::LLInventoryItem()
:	LLInventoryObject(),
	mPermissions(),
	mAssetUUID(),
	mDescription(),
	mSaleInfo(),
	mInventoryType(LLInventoryType::IT_NONE),
	mFlags(0)
{
	mCreationDate = 0;
}

LLInventoryItem::LLInventoryItem(const LLInventoryItem* otherp)
:	LLInventoryObject()
{
	copyItem(otherp);
}

//virtual
void LLInventoryItem::copyItem(const LLInventoryItem* otherp)
{
	copyObject(otherp);
	mPermissions = otherp->mPermissions;
	mAssetUUID = otherp->mAssetUUID;
	mDescription = otherp->mDescription;
	mSaleInfo = otherp->mSaleInfo;
	mInventoryType = otherp->mInventoryType;
	mFlags = otherp->mFlags;
	mCreationDate = otherp->mCreationDate;
}

// Note: this method is used to identify a newly created copy of an inventory
// item, and avoid considering it a newly received item by inventory observers.
// We therefore only care about part of the data (e.g. we do not care about the
// parent, since the item may be copied into another folder, neither about the
// sale info which is irrelevant to copy-ok items) and discard from the hash
// any data that changes during the copy action (e.g. the last owner UUID which
// gets reset, or the thumbnail UUID which is currently not copied since the
// copy command still uses the legacy UDP messaging and not yet AISv3). HB
LLUUID LLInventoryItem::hashContents() const
{
	HBXXH128 hash;
	hash.update(mName);
	hash.update(mDescription);
	hash.update((void*)mAssetUUID.mData, UUID_BYTES);
	U32 buffer[3];
	buffer[0] = (U32)mInventoryType;
	buffer[1] = mFlags;
	buffer[2] = mPermissions.getCRC32(true);	// true = skip last owner UUID
	hash.update((void*)&buffer, 3 * sizeof(U32));
	return hash.digest();
}

// If this is a linked item, then the UUID of the base object is
// this item's assetID.
//virtual
const LLUUID& LLInventoryItem::getLinkedUUID() const
{
	if (LLAssetType::lookupIsLinkType(getActualType()))
	{
		return mAssetUUID;
	}

	return LLInventoryObject::getLinkedUUID();
}

U32 LLInventoryItem::getCRC32() const
{
	// *FIX: Not a real crc - more of a checksum.
	// *NOTE: We currently do not validate the name or description, but if they
	// change in transit, it's no big deal.
	U32 crc = mUUID.getCRC32();
	crc += mParentUUID.getCRC32();
	crc += mPermissions.getCRC32();
	crc += mAssetUUID.getCRC32();
	crc += mType;
	crc += mInventoryType;
	crc += mFlags;
	crc += mSaleInfo.getCRC32();
	crc += mCreationDate;
	crc += mThumbnailUUID.getCRC32();
	return crc;
}

//static
void LLInventoryItem::correctInventoryDescription(std::string& desc)
{
	LLStringUtil::replaceNonstandardASCII(desc, ' ');
	LLStringUtil::replaceChar(desc, '|', ' ');
}

void LLInventoryItem::setDescription(const std::string& d)
{
	std::string new_desc(d);
	LLInventoryItem::correctInventoryDescription(new_desc);
	if (new_desc != mDescription)
	{
		mDescription = new_desc;
	}
}

void LLInventoryItem::setPermissions(const LLPermissions& perm)
{
	mPermissions = perm;

	// Override permissions to unrestricted if this is a landmark
	mPermissions.initMasks(mInventoryType);
}

void LLInventoryItem::accumulatePermissionSlamBits(const LLInventoryItem& old_item)
{
	// Remove any pre-existing II_FLAGS_PERM_OVERWRITE_MASK flags
	// because we now detect when they should be set.
	setFlags(old_item.getFlags() | (getFlags() & ~II_FLAGS_PERM_OVERWRITE_MASK));

	// Enforce the PERM_OVERWRITE flags for any masks that are different
	// but only for AT_OBJECT's since that is the only asset type that can
	// exist in-world (instead of only in-inventory or in-object-contents).
	if (LLAssetType::AT_OBJECT == getType())
	{
		LLPermissions old_permissions = old_item.getPermissions();
		U32 flags_to_be_set = 0;
		if (old_permissions.getMaskNextOwner() != getPermissions().getMaskNextOwner())
		{
			flags_to_be_set |= II_FLAGS_OBJECT_SLAM_PERM;
		}
		if (old_permissions.getMaskEveryone() != getPermissions().getMaskEveryone())
		{
			flags_to_be_set |= II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
		}
		if (old_permissions.getMaskGroup() != getPermissions().getMaskGroup())
		{
			flags_to_be_set |= II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
		}
		LLSaleInfo old_sale_info = old_item.getSaleInfo();
		if (old_sale_info != getSaleInfo())
		{
			flags_to_be_set |= II_FLAGS_OBJECT_SLAM_SALE;
		}
		setFlags(getFlags() | flags_to_be_set);
	}
}

void LLInventoryItem::setTargetLanguage(const std::string& language)
{
	static const fast_hset<std::string> languages =
		{ "lsl2", "mono", "lsl-luau", "luau" };
	if (mInventoryType == LLInventoryType::IT_LSL &&
		!language.empty() && languages.count(language))
	{
		mTargetLanguage = language;
	}
}

//virtual
void LLInventoryItem::packMessage(LLMessageSystem* msg) const
{
	msg->addUUIDFast(_PREHASH_ItemID, mUUID);
	msg->addUUIDFast(_PREHASH_FolderID, mParentUUID);
	mPermissions.packMessage(msg);
	msg->addUUIDFast(_PREHASH_AssetID, mAssetUUID);
	msg->addS8Fast(_PREHASH_Type, (S8)mType);
	msg->addS8Fast(_PREHASH_InvType, (S8)mInventoryType);
	msg->addU32Fast(_PREHASH_Flags, mFlags);
	mSaleInfo.packMessage(msg);
	msg->addStringFast(_PREHASH_Name, mName);
	msg->addStringFast(_PREHASH_Description, mDescription);
	msg->addS32Fast(_PREHASH_CreationDate, mCreationDate);
	U32 crc = getCRC32();
	msg->addU32Fast(_PREHASH_CRC, crc);
}

#undef CRC_CHECK
//virtual
bool LLInventoryItem::unpackMessage(LLMessageSystem* msg, const char* block,
									S32 block_num)
{
	msg->getUUIDFast(block, _PREHASH_ItemID, mUUID, block_num);
	msg->getUUIDFast(block, _PREHASH_FolderID, mParentUUID, block_num);
	mPermissions.unpackMessage(msg, block, block_num);
	msg->getUUIDFast(block, _PREHASH_AssetID, mAssetUUID, block_num);

	S8 type;
	msg->getS8Fast(block, _PREHASH_Type, type, block_num);
	mType = (LLAssetType::EType)type;
	msg->getS8(block, "InvType", type, block_num);
	mInventoryType = (LLInventoryType::EType)type;
	mPermissions.initMasks(mInventoryType);

	msg->getU32Fast(block, _PREHASH_Flags, mFlags, block_num);

	mSaleInfo.unpackMultiMessage(msg, block, block_num);

	msg->getStringFast(block, _PREHASH_Name, mName, block_num);
	LLStringUtil::replaceNonstandardASCII(mName, ' ');

	msg->getStringFast(block, _PREHASH_Description, mDescription, block_num);
	LLStringUtil::replaceNonstandardASCII(mDescription, ' ');

	S32 date;
	msg->getS32(block, "CreationDate", date, block_num);
	mCreationDate = date;

	U32 local_crc = getCRC32();
	U32 remote_crc = 0;
	msg->getU32(block, "CRC", remote_crc, block_num);
#ifdef CRC_CHECK
	if (local_crc == remote_crc)
	{
		LL_DEBUGS("Inventory") << "CRC matches" << LL_ENDL;
		return true;
	}
	else
	{
		llwarns << "Inventory CRC mismatch: local=" << std::hex << local_crc
				<< " - remote=" << remote_crc << std::dec << llendl;
		return false;
	}
#else
	return local_crc == remote_crc;
#endif
}

//virtual
bool LLInventoryItem::importLegacyStream(std::istream& input_stream)
{
	// NOTE: changing the buffer size requires changing the scanf calls below.
	char buffer[MAX_STRING];
	char keyword[MAX_STRING];
	char valuestr[MAX_STRING];
	char junk[MAX_STRING];
	bool success = true;

	keyword[0] = '\0';
	valuestr[0] = '\0';

	mInventoryType = LLInventoryType::IT_NONE;
	mAssetUUID.setNull();
	while (success && input_stream.good())
	{
		input_stream.getline(buffer, MAX_STRING);
		// This happens a lot... And it does not matter. HB
		if (!strcmp("|", buffer))
		{
			continue;
		}
		if (sscanf(buffer, " %254s %254s", keyword, valuestr) < 1)
		{
			continue;
		}
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}", keyword))
		{
			break;
		}
		if (!strcmp("item_id", keyword))
		{
			mUUID.set(valuestr);
		}
		else if (!strcmp("parent_id", keyword))
		{
			mParentUUID.set(valuestr);
		}
		else if (!strcmp("permissions", keyword))
		{
			success = mPermissions.importLegacyStream(input_stream);
		}
		else if (!strcmp("sale_info", keyword))
		{
			// Sale info used to contain next owner perm. It is now in
			// the permissions. Thus, we read that out, and fix legacy
			// objects. It's possible this op would fail, but it
			// should pick up the vast majority of the tasks.
			bool has_perm_mask = false;
			U32 perm_mask = 0;
			success = mSaleInfo.importLegacyStream(input_stream, has_perm_mask,
												   perm_mask);
			if (has_perm_mask)
			{
				if (perm_mask == PERM_NONE)
				{
					perm_mask = mPermissions.getMaskOwner();
				}
				// fair use fix.
				if (!(perm_mask & PERM_COPY))
				{
					perm_mask |= PERM_TRANSFER;
				}
				mPermissions.setMaskNext(perm_mask);
			}
		}
		else if (!strcmp("shadow_id", keyword))
		{
			mAssetUUID.set(valuestr);
			LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
			cipher.decrypt(mAssetUUID.mData, UUID_BYTES);
		}
		else if (!strcmp("asset_id", keyword))
		{
			mAssetUUID.set(valuestr);
		}
		else if (!strcmp("type", keyword))
		{
			mType = LLAssetType::lookup(valuestr);
		}
		else if (!strcmp("inv_type", keyword))
		{
			mInventoryType = LLInventoryType::lookup(std::string(valuestr));
		}
		else if (!strcmp("flags", keyword))
		{
			sscanf(valuestr, "%x", &mFlags);
		}
		else if (!strcmp("name", keyword))
		{
			sscanf(buffer, " %254s%254[\t]%254[^|]", keyword, junk, valuestr);

			// IW: sscanf chokes and puts | in valuestr if there's no name
			if (valuestr[0] == '|')
			{
				valuestr[0] = '\000';
			}

			mName.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
		else if (!strcmp("desc", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(buffer, " %254s%254[\t]%254[^|]", keyword, junk, valuestr);

			if (valuestr[0] == '|')
			{
				valuestr[0] = '\000';
			}

			mDescription.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
		}
		else if (!strcmp("creation_date", keyword))
		{
			S32 date;
			sscanf(valuestr, "%d", &date);
			mCreationDate = date;
		}
		else if (!strcmp("metadata", keyword))
		{
			LLSD metadata;
#if 0
			if (!strncmp("<llsd>", valuestr, 6))
			{
				std::istringstream stream(valuestr);
				LLSDSerialize::fromXML(metadata, stream);
			}
			else
#endif
			{
				metadata = LLSD(valuestr);
			}
			if (metadata.has("thumbnail"))
			{
				const LLSD& thumbnail = metadata["thumbnail"];
				if (thumbnail.has("asset_id"))
				{
					setThumbnailUUID(thumbnail["asset_id"].asUUID());
				}
			}
		}
		else
		{
			llwarns << "Unknown keyword '" << keyword
					<< "' in inventory import of item " << mUUID
					<< ". Buffer contents: " << buffer << llendl;
		}
	}

	// Need to convert 1.0 simstate files to a useful inventory type and
	// potentially deal with bad inventory tyes eg, a landmark marked as a
	// texture.
	if (mInventoryType == LLInventoryType::IT_NONE ||
		!inventory_and_asset_types_match(mInventoryType, mType))
	{
		LL_DEBUGS("Inventory") << "Resetting inventory type for " << mUUID
							   << LL_ENDL;
		mInventoryType = LLInventoryType::defaultForAssetType(mType);
	}

	mPermissions.initMasks(mInventoryType);

	return success;
}

//virtual
bool LLInventoryItem::exportLegacyStream(std::ostream& output_stream,
										 bool include_asset_key) const
{
	std::string uuid_str;
	output_stream << "\tinv_item\t0\n\t{\n";
	mUUID.toString(uuid_str);
	output_stream << "\t\titem_id\t" << uuid_str << "\n";
	mParentUUID.toString(uuid_str);
	output_stream << "\t\tparent_id\t" << uuid_str << "\n";
	mPermissions.exportLegacyStream(output_stream);

	if (mThumbnailUUID.notNull())
	{
		// Max length is 255 chars, will have to export differently if it gets
		// more data (e.g. use newline and toNotation (uses {}) for unlimited
		// size).
		LLSD metadata;
		metadata["thumbnail"] = LLSD().with("asset_id", mThumbnailUUID);
#if 0
		output_stream << "\t\tmetadata\t";
		LLSDSerialize::toXML(metadata, output_stream);
		output_stream << "|\n";
#else
		output_stream << "\t\tmetadata\t" << metadata << "|\n";
#endif
	}

	// Check for permissions to see the asset id, and if so write it out as an
	// asset id. Otherwise, apply our cheesy encryption.
	if (include_asset_key)
	{
		if (mPermissions.unrestricted() || mAssetUUID.isNull())
		{
			mAssetUUID.toString(uuid_str);
			output_stream << "\t\tasset_id\t" << uuid_str << "\n";
		}
		else
		{
			LLUUID shadow_id(mAssetUUID);
			LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
			cipher.encrypt(shadow_id.mData, UUID_BYTES);
			shadow_id.toString(uuid_str);
			output_stream << "\t\tshadow_id\t" << uuid_str << "\n";
		}
	}
	else
	{
		LLUUID::null.toString(uuid_str);
		output_stream << "\t\tasset_id\t" << uuid_str << "\n";
	}
	output_stream << "\t\ttype\t" << LLAssetType::lookup(mType) << "\n";
	const std::string& inv_type_str = LLInventoryType::lookup(mInventoryType);
	if (!inv_type_str.empty())
	{
		output_stream << "\t\tinv_type\t" << inv_type_str << "\n";
	}
	std::string buffer;
	buffer = llformat( "\t\tflags\t%08x\n", mFlags);
	output_stream << buffer;
	mSaleInfo.exportLegacyStream(output_stream);
	output_stream << "\t\tname\t" << mName.c_str() << "|\n";
	output_stream << "\t\tdesc\t" << mDescription.c_str() << "|\n";
	output_stream << "\t\tcreation_date\t" << mCreationDate << "\n";
	output_stream << "\t}\n";
	return true;
}

LLSD LLInventoryItem::asLLSD() const
{
	LLSD sd = LLSD();

	sd[INV_ITEM_ID_LABEL] = mUUID;
	sd[INV_PARENT_ID_LABEL] = mParentUUID;
	sd[INV_PERMISSIONS_LABEL] = mPermissions.asLLSD();

	if (mThumbnailUUID.notNull())
	{
		sd[INV_THUMBNAIL_LABEL] =
			LLSD().with(INV_ASSET_ID_LABEL, mThumbnailUUID);
	}

	if (mPermissions.unrestricted() || mAssetUUID.isNull())
	{
		sd[INV_ASSET_ID_LABEL] = mAssetUUID;
	}
	else
	{
		// *TODO: get rid of this. Phoenix 2008-01-30
		LLUUID shadow_id(mAssetUUID);
		LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
		cipher.encrypt(shadow_id.mData, UUID_BYTES);
		sd[INV_SHADOW_ID_LABEL] = shadow_id;
	}
	sd[INV_ASSET_TYPE_LABEL] = LLAssetType::lookup(mType);
	sd[INV_INVENTORY_TYPE_LABEL] = mInventoryType;
	const std::string& inv_type_str = LLInventoryType::lookup(mInventoryType);
	if (!inv_type_str.empty())
	{
		sd[INV_INVENTORY_TYPE_LABEL] = inv_type_str;
	}
	//sd[INV_FLAGS_LABEL] = (S32)mFlags;
	sd[INV_FLAGS_LABEL] = ll_sd_from_U32(mFlags);
	sd[INV_SALE_INFO_LABEL] = mSaleInfo;
	sd[INV_NAME_LABEL] = mName;
	sd[INV_DESC_LABEL] = mDescription;
	sd[INV_CREATION_DATE_LABEL] = (S32) mCreationDate;

	if (mInventoryType == LLInventoryType::IT_LSL && !mTargetLanguage.empty())
	{
		sd[INV_LANGUAGE_LABEL] = LLSD().with(INV_TARGET_LABEL,
											 mTargetLanguage);
	}

	return sd;
}

bool LLInventoryItem::fromLLSD(const LLSD& sd, bool is_new)
{
	if (is_new)
	{
		// If we are adding LLSD to an existing object, need avoid clobbering
		// these fields.
		mInventoryType = LLInventoryType::IT_NONE;
		mAssetUUID.setNull();
	}

	for (LLSD::map_const_iterator it = sd.beginMap(), end = sd.endMap();
		 it != end; ++it)
	{
		const LLSD::String& key = it->first;
		const LLSD& value = it->second;

		if (key == INV_ITEM_ID_LABEL)
		{
			mUUID = value;
			continue;
		}
		if (key == INV_PARENT_ID_LABEL)
		{
			mParentUUID = value;
			continue;
		}
		if (key == INV_THUMBNAIL_LABEL)
		{
			if (value.has(INV_ASSET_ID_LABEL))
			{
				mThumbnailUUID = value[INV_ASSET_ID_LABEL];
			}
			continue;
		}
		if (key == INV_THUMBNAIL_ID_LABEL)
		{
			mThumbnailUUID = value;
			continue;
		}
		if (key == INV_PERMISSIONS_LABEL)
		{
			mPermissions.fromLLSD(value);
			continue;
		}
		if (key == INV_SALE_INFO_LABEL)
		{
			// Sale info used to contain next owner perm. It is now in the
			// permissions. Thus, we read that out, and fix legacy objects. It
			// is possible this op would fail, but it should pick up the vast
			// majority of the tasks.
			bool has_perm_mask = false;
			U32 perm_mask = 0;
			if (!mSaleInfo.fromLLSD(value, has_perm_mask, perm_mask))
			{
				return false;
			}
			if (has_perm_mask)
			{
				if (perm_mask == PERM_NONE)
				{
					perm_mask = mPermissions.getMaskOwner();
				}
				// Fair use fix.
				if (!(perm_mask & PERM_COPY))
				{
					perm_mask |= PERM_TRANSFER;
				}
				mPermissions.setMaskNext(perm_mask);
			}
			continue;
		}
		if (key == INV_SHADOW_ID_LABEL)
		{
			mAssetUUID = value;
			LLXORCipher cipher(MAGIC_ID.mData, UUID_BYTES);
			cipher.decrypt(mAssetUUID.mData, UUID_BYTES);
			continue;
		}
		if (key == INV_ASSET_ID_LABEL || key == INV_LINKED_ID_LABEL)
		{
			mAssetUUID = value;
			continue;
		}
		if (key == INV_ASSET_TYPE_LABEL)
		{
			if (value.isString())
			{
				mType = LLAssetType::lookup(value.asString());
			}
			else if (value.isInteger())
			{
				S8 type = (U8)value.asInteger();
				mType = (LLAssetType::EType)type;
			}
			continue;
		}
		if (key == INV_INVENTORY_TYPE_LABEL)
		{
			if (value.isString())
			{
				mInventoryType =
					LLInventoryType::lookup(value.asString().c_str());
			}
			else if (value.isInteger())
			{
				S8 type = (U8)value.asInteger();
				mInventoryType = (LLInventoryType::EType)type;
			}
			continue;
		}
		if (key == INV_FLAGS_LABEL)
		{
			if (value.isBinary())
			{
				mFlags = ll_U32_from_sd(value);
			}
			else if (value.isInteger())
			{
				mFlags = value.asInteger();
			}
			continue;
		}
		if (key == INV_NAME_LABEL)
		{
			mName = value.asString();
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
			continue;
		}
		if (key == INV_DESC_LABEL)
		{
			mDescription = value.asString();
			LLStringUtil::replaceNonstandardASCII(mDescription, ' ');
			continue;
		}
		if (key == INV_CREATION_DATE_LABEL)
		{
			mCreationDate = value.asInteger();
		}
		if (mInventoryType == LLInventoryType::IT_LSL &&
			key == INV_LANGUAGE_LABEL)
		{
			if (value.has(INV_TARGET_LABEL))
			{
				mTargetLanguage = value[INV_TARGET_LABEL].asString();
			}
		}
	}

	// Need to convert 1.0 simstate files to a useful inventory type and
	// potentially deal with bad inventory tyes eg, a landmark marked as a
	// texture.
	if (mInventoryType == LLInventoryType::IT_NONE ||
		!inventory_and_asset_types_match(mInventoryType, mType))
	{
		LL_DEBUGS("Inventory") << "Resetting inventory type for " << mUUID
							   << LL_ENDL;
		mInventoryType = LLInventoryType::defaultForAssetType(mType);
	}

	mPermissions.initMasks(mInventoryType);

	return true;
}

///----------------------------------------------------------------------------
/// Class LLInventoryCategory
///----------------------------------------------------------------------------

LLInventoryCategory::LLInventoryCategory(const LLUUID& uuid,
										 const LLUUID& parent_uuid,
										 LLFolderType::EType preferred_type,
										 const std::string& name)
:	LLInventoryObject(uuid, parent_uuid, LLAssetType::AT_CATEGORY, name),
	mPreferredType(preferred_type)
{
}

LLInventoryCategory::LLInventoryCategory()
:	mPreferredType(LLFolderType::FT_NONE)
{
	mType = LLAssetType::AT_CATEGORY;
}

LLInventoryCategory::LLInventoryCategory(const LLInventoryCategory* otherp)
:	LLInventoryObject()
{
	copyCategory(otherp);
}

//virtual
void LLInventoryCategory::copyCategory(const LLInventoryCategory* otherp)
{
	copyObject(otherp);
	mPreferredType = otherp->mPreferredType;
}

LLSD LLInventoryCategory::asLLSD() const
{
	LLSD sd = LLSD();
	sd[INV_ITEM_ID_LABEL] = mUUID;
	sd[INV_PARENT_ID_LABEL] = mParentUUID;
	sd[INV_ASSET_TYPE_LABEL] = (S8)mPreferredType;
	sd[INV_NAME_LABEL] = mName;
	if (mThumbnailUUID.notNull())
	{
		sd[INV_THUMBNAIL_LABEL] =
			LLSD().with(INV_ASSET_ID_LABEL, mThumbnailUUID);
	}
	return sd;
}

LLSD LLInventoryCategory::asAISCreateCatLLSD() const
{
	LLSD sd = LLSD();
	sd[INV_FOLDER_ID_LABEL_WS] = mUUID;
	sd[INV_PARENT_ID_LABEL] = mParentUUID;
	sd[INV_ASSET_TYPE_LABEL_WS] = (S8)mPreferredType;
	sd[INV_NAME_LABEL] = mName;
	if (mThumbnailUUID.notNull())
	{
		sd[INV_THUMBNAIL_LABEL] =
			LLSD().with(INV_ASSET_ID_LABEL, mThumbnailUUID);
	}
	return sd;
}

bool LLInventoryCategory::fromLLSD(const LLSD& sd)
{
	for (LLSD::map_const_iterator it = sd.beginMap(), end = sd.endMap();
		 it != end; ++it)
	{
		const LLSD::String& key = it->first;
		const LLSD& value = it->second;

		if (key == INV_FOLDER_ID_LABEL_WS)
		{
			mUUID = value;
			continue;
		}
		if (key == INV_PARENT_ID_LABEL)
		{
			mParentUUID = value;
			continue;
		}
		if (key == INV_THUMBNAIL_LABEL)
		{
			if (value.has(INV_ASSET_ID_LABEL))
			{
				mThumbnailUUID = value[INV_ASSET_ID_LABEL];
			}
			continue;
		}
		if (key == INV_THUMBNAIL_ID_LABEL)
		{
			mThumbnailUUID = value;
			continue;
		}
		if (key == INV_ASSET_TYPE_LABEL || key == INV_ASSET_TYPE_LABEL_WS)
		{
			S8 type = (U8)value.asInteger();
			mPreferredType = (LLFolderType::EType)type;
			continue;
		}
		if (key == INV_NAME_LABEL)
		{
			mName = value.asString();
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
	}

	return true;
}

//virtual
void LLInventoryCategory::packMessage(LLMessageSystem* msg) const
{
	msg->addUUIDFast(_PREHASH_FolderID, mUUID);
	msg->addUUIDFast(_PREHASH_ParentID, mParentUUID);
	msg->addS8Fast(_PREHASH_Type, (S8)mPreferredType);
	msg->addStringFast(_PREHASH_Name, mName);
}

//virtual
void LLInventoryCategory::unpackMessage(LLMessageSystem* msg,
										const char* block, S32 block_num)
{
	msg->getUUIDFast(block, _PREHASH_FolderID, mUUID, block_num);
	msg->getUUIDFast(block, _PREHASH_ParentID, mParentUUID, block_num);
	S8 type;
	msg->getS8Fast(block, _PREHASH_Type, type, block_num);
	mPreferredType = (LLFolderType::EType)type;
	msg->getStringFast(block, _PREHASH_Name, mName, block_num);
	LLStringUtil::replaceNonstandardASCII(mName, ' ');
}

//virtual
bool LLInventoryCategory::importLegacyStream(std::istream& input_stream)
{
	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[MAX_STRING];
	char keyword[MAX_STRING];
	char valuestr[MAX_STRING];

	keyword[0] = '\0';
	valuestr[0] = '\0';
	while (input_stream.good())
	{
		input_stream.getline(buffer, MAX_STRING);
		// This happens a lot... And it does not matter. HB
		if (!strcmp("|", buffer))
		{
			continue;
		}
		if (sscanf(buffer, " %254s %254s", keyword, valuestr) < 1)
		{
			continue;
		}
		if (!strcmp("{", keyword))
		{
			continue;
		}
		if (!strcmp("}", keyword))
		{
			break;
		}
		else if (!strcmp("cat_id", keyword))
		{
			mUUID.set(valuestr);
		}
		else if (!strcmp("parent_id", keyword))
		{
			mParentUUID.set(valuestr);
		}
		else if (!strcmp("type", keyword))
		{
			mType = LLAssetType::lookup(valuestr);
		}
		else if (!strcmp("pref_type", keyword))
		{
			mPreferredType = LLFolderType::lookup(valuestr);
		}
		else if (!strcmp("name", keyword))
		{
			//strcpy(valuestr, buffer + strlen(keyword) + 3);
			// *NOTE: Not ANSI C, but widely supported.
			sscanf(buffer, " %254s %254[^|]", keyword, valuestr);
			mName.assign(valuestr);
			LLStringUtil::replaceNonstandardASCII(mName, ' ');
			LLStringUtil::replaceChar(mName, '|', ' ');
		}
		else if (!strcmp("metadata", keyword))
		{
			LLSD metadata;
#if 0
			if (!strncmp("<llsd>", valuestr, 6))
			{
				std::istringstream stream(valuestr);
				LLSDSerialize::fromXML(metadata, stream);
			}
			else
#endif
			{
				metadata = LLSD(valuestr);
			}
			if (metadata.has("thumbnail"))
			{
				const LLSD& thumbnail = metadata["thumbnail"];
				if (thumbnail.has("asset_id"))
				{
					setThumbnailUUID(thumbnail["asset_id"].asUUID());
				}
			}
		}
		else
		{
			llwarns << "Unknown keyword '" << keyword
					<< "' in inventory import category " << mUUID
					<< ". Buffer contents: " << buffer << llendl;
		}
	}
	return true;
}

//virtual
bool LLInventoryCategory::exportLegacyStream(std::ostream& output_stream,
											 bool) const
{
	std::string uuid_str;
	output_stream << "\tinv_category\t0\n\t{\n";
	mUUID.toString(uuid_str);
	output_stream << "\t\tcat_id\t" << uuid_str << "\n";
	mParentUUID.toString(uuid_str);
	output_stream << "\t\tparent_id\t" << uuid_str << "\n";
	output_stream << "\t\ttype\t" << LLAssetType::lookup(mType) << "\n";
	output_stream << "\t\tpref_type\t" << LLFolderType::lookup(mPreferredType)
				  << "\n";
	output_stream << "\t\tname\t" << mName.c_str() << "|\n";
	if (mThumbnailUUID.notNull())
	{
		// Max length is 255 chars, will have to export differently if it gets
		// more data (e.g. use newline and toNotation (uses {}) for unlimited
		// size).
		LLSD metadata;
		metadata["thumbnail"] = LLSD().with("asset_id", mThumbnailUUID);
#if 0
		output_stream << "\t\tmetadata\t";
		LLSDSerialize::toXML(metadata, output_stream);
		output_stream << "|\n";
#else
		output_stream << "\t\tmetadata\t" << metadata << "|\n";
#endif
	}
	output_stream << "\t}\n";
	return true;
}
