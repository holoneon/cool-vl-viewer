/**
 * @file llimagetga.h
 * @brief Image implementation to compresses and decompressed TGA files.
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

#pragma once

#include "llimage.h"

class LLFile;

class LLImageTGA final : public LLImageFormatted
{
protected:
	LOG_CLASS(LLImageTGA);

	~LLImageTGA() override;

public:
	LLImageTGA();
	LLImageTGA(const std::string& file_name);

	std::string getExtension() override			{ return "tga"; }
	bool updateData() override;
	bool decode(LLImageRaw* raw_imagep) override;
	bool encode(const LLImageRaw* raw_imagep) override;

	bool decodeAndProcess(LLImageRaw* raw_imagep, F32 domain, F32 weight);

	static bool getDimensions(LLFile& filename, S32& width, S32& height);

private:
	bool decodeTruecolor(LLImageRaw* raw_imagep, bool rle, bool flipped);

	bool decodeTruecolorRle8(LLImageRaw* raw_imagep);
	bool decodeTruecolorRle15(LLImageRaw* raw_imagep);
	bool decodeTruecolorRle24(LLImageRaw* raw_imagep);
	bool decodeTruecolorRle32(LLImageRaw* raw_imagep, bool& alpha_opaque);

	void decodeTruecolorPixel15(U8* dstp, const U8* srcp);

	bool decodeTruecolorNonRle(LLImageRaw* raw_imagep, bool& alpha_opaque);

	bool decodeColorMap(LLImageRaw* raw_imagep, bool rle, bool flipped);

	void decodeColorMapPixel8(U8* dstp, const U8* srcp);
	void decodeColorMapPixel15(U8* dstp, const U8* srcp);
	void decodeColorMapPixel24(U8* dstp, const U8* srcp);
	void decodeColorMapPixel32(U8* dstp, const U8* srcp);

	bool loadFile(const std::string& file_name);

private:
	U32 mDataOffset; // Offset from start of data to the actual header.

	// Data from header
	U8 mIDLength;		// Length of identifier string
	U8 mColorMapType;	// 0 = No Map
	// Supported: 2 = Uncompressed true color, 3 = uncompressed monochrome
	// without colormap:
	U8 mImageType;
	U8 mColorMapIndexLo;	// First color map entry (low order byte)
	U8 mColorMapIndexHi;	// First color map entry (high order byte)
	U8 mColorMapLengthLo;	// Color map length (low order byte)
	U8 mColorMapLengthHi;	// Color map length (high order byte)
	U8 mColorMapDepth;	// Size of color map entry (15, 16, 24, or 32 bits)
	U8 mXOffsetLo;		// X offset of image (low order byte)
	U8 mXOffsetHi;		// X offset of image (hi order byte)
	U8 mYOffsetLo;		// Y offset of image (low order byte)
	U8 mYOffsetHi;		// Y offset of image (hi order byte)
	U8 mWidthLo;		// Width (low order byte)
	U8 mWidthHi;		// Width (hi order byte)
	U8 mHeightLo;		// Height (low order byte)
	U8 mHeightHi;		// Height (hi order byte)
	U8 mPixelSize;		// 8, 16, 24, 32 bits per pixel
	U8 mAttributeBits;	// 4 bits: number of attributes per pixel
	U8 mOriginRightBit;	// 1 bit: origin, 0 = left, 1 = right
	U8 mOriginTopBit;	// 1 bit: origin, 0 = bottom, 1 = top
	// 2 bits: interleaved flag, 0 = none, 1 = interleaved 2, 2 = interleaved 4
	U8 mInterleave;

	U8*		mColorMap;
	S32		mColorMapStart;
	S32		mColorMapLength;
	S32		mColorMapBytesPerEntry;

	bool	mIs15Bit;

	static const U8 s5to8bits[32];
};
