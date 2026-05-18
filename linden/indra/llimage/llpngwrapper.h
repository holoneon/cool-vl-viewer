/*
 * @file llpngwrapper.h
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "png.h"
#include "llimage.h"

class LLPngWrapper
{
protected:
	LOG_CLASS(LLPngWrapper);

	void normalizeImage();
	void updateMetaData();

public:
	LLPngWrapper();
	~LLPngWrapper();

	struct ImageInfo
	{
		U16 mWidth;
		U16 mHeight;
		S8  mComponents;
	};

	bool isValidPng(U8* src);
	bool readPng(U8* src, S32 dataSize, LLImageRaw* rawImage,
				 ImageInfo* infop = NULL);
	bool writePng(const LLImageRaw* rawImage, U8* dst);
	LL_INLINE U32 getFinalSize()					{ return mFinalSize; }
	LL_INLINE const std::string& getErrorMessage()	{ return mErrorMessage; }

private:
	// Structure for writing/reading PNG data to/from memory as opposed to
	// using a file.
	struct PngDataInfo
	{
		U8* mData;
		U32 mOffset;
		S32 mDataSize;
	};

	// No-op since we are just writing to memory
	LL_INLINE static void writeFlush(png_structp png_ptr)
	{
	}

	static void errorHandler(png_structp png_ptr, png_const_charp msg);
	static void readDataCallback(png_structp png_ptr, png_bytep dest,
								 png_size_t length);
	static void writeDataCallback(png_structp png_ptr, png_bytep src,
								  png_size_t length);

	void releaseResources();

private:
	png_structp mReadPngPtr;
	png_infop mReadInfoPtr;
	png_structp mWritePngPtr;
	png_infop mWriteInfoPtr;

	U8** mRowPointers;

	png_uint_32 mWidth;
	png_uint_32 mHeight;
	S32 mBitDepth;
	S32 mColorType;
	S32 mChannels;
	S32 mInterlaceType;
	S32 mCompressionType;
	S32 mFilterMethod;

	U32 mFinalSize;

	F64 mGamma;

	std::string mErrorMessage;
};
