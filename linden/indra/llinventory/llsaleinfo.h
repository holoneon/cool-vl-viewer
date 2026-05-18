/**
 * @file llsaleinfo.h
 * @brief LLSaleInfo class header file.
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

#include "llpermissionsflags.h"
#include "llpreprocessor.h"
#include "llsd.h"
#include "llxmlnode.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLSaleInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// L$ default price for objects
constexpr S32 DEFAULT_PRICE = 10;

class LLMessageSystem;

class LLSaleInfo
{
protected:
	LOG_CLASS(LLSaleInfo);

public:
	// Use this to avoid temporary object creation
	static const LLSaleInfo DEFAULT;

	enum EForSale
	{
		// Item is not to be considered for transactions
		FS_NOT = 0,

		// The origional is on sale
		FS_ORIGINAL = 1,

		// A copy is for sale
		FS_COPY = 2,

		// Valid only for tasks, the inventory is for sale at the price in this
		// structure.
		FS_CONTENTS = 3,

		FS_COUNT
	};

public:
	// Default constructor is fine usually
	LLSaleInfo();
	LLSaleInfo(EForSale sale_type, S32 sale_price);

	// Accessors
	LL_INLINE bool isForSale() const				{ return mSaleType != FS_NOT; }
	LL_INLINE EForSale getSaleType() const			{ return mSaleType; }
	LL_INLINE S32 getSalePrice() const				{ return mSalePrice; }
	U32 getCRC32() const;

	// Mutators
	LL_INLINE void setSaleType(EForSale type)		{ mSaleType = type; }
	void setSalePrice(S32 price);
#if 0
	LL_INLINE void setNextOwnerPermMask(U32 mask)	{ mNextOwnerPermMask = mask; }
#endif

	bool exportLegacyStream(std::ostream& output_stream) const;
	LLSD asLLSD() const;
	LL_INLINE operator LLSD() const					{ return asLLSD(); }
	bool fromLLSD(const LLSD& sd, bool& has_perm_mask, U32& perm_mask);
	bool importLegacyStream(std::istream& input_stream, bool& has_perm_mask,
							U32& perm_mask);

	LLSD packMessage() const;
	void unpackMessage(LLSD sales);

	// Message serialization
	void packMessage(LLMessageSystem* msg) const;
	void unpackMessage(LLMessageSystem* msg, const char* block);
	void unpackMultiMessage(LLMessageSystem* msg, const char* block,
							S32 block_num);

	// Static functionality to determine the "for sale" status.
	static EForSale lookup(const char* name);
	static const char* lookup(EForSale type);

	// Allows accumulation of sale info. The price of each is added, conflict
	// in sale type results in FS_NOT, and the permissions are tightened.
	void accumulate(const LLSaleInfo& sale_info);

	bool operator==(const LLSaleInfo& rhs) const;
	bool operator!=(const LLSaleInfo& rhs) const;

protected:
	EForSale	mSaleType;
	S32			mSalePrice;
};

// These functions convert between structured data and sale info as appropriate
// for serialization.
LLSD ll_create_sd_from_sale_info(const LLSaleInfo& sale);
LLSaleInfo ll_sale_info_from_sd(const LLSD& sd);
