/**
 * @file llimagejpeg.h
 * @brief This class compresses and decompresses JPEG files
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

#include "llwindowsheaderslean.h"

#include "llimage.h"

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}

class LLFile;

class LLImageJPEG final : public LLImageFormatted
{
protected:
	LOG_CLASS(LLImageJPEG);

	~LLImageJPEG() override;

public:
	LLImageJPEG(S32 quality = 75);

	LL_INLINE std::string getExtension() override	{ return "jpg"; }
	bool updateData() override;
	bool decode(LLImageRaw* raw_imagep) override;
	bool encode(const LLImageRaw* raw_imagep) override;

	// On a scale from 1 to 100
	LL_INLINE void setEncodeQuality(S32 q)			{ mEncodeQuality = q; }
	LL_INLINE S32 getEncodeQuality()				{ return mEncodeQuality; }

	static bool getDimensions(LLFile& filename, S32& width, S32& height);

private:
	// Callbacks registered with jpeglib
	static void encodeInitDestination(j_compress_ptr cinfo);
	static boolean encodeEmptyOutputBuffer(j_compress_ptr cinfo);
	static void encodeTermDestination(j_compress_ptr cinfo);

	static void decodeInitSource(j_decompress_ptr cinfo);
	static boolean decodeFillInputBuffer(j_decompress_ptr cinfo);
	static void decodeSkipInputData(j_decompress_ptr cinfo, long num_bytes);
	static void decodeTermSource(j_decompress_ptr cinfo);

	static void errorExit(j_common_ptr cinfo);
	static void errorEmitMessage(j_common_ptr cinfo, int msg_level);
	static void errorOutputMessage(j_common_ptr cinfo);

private:
	U8* mOutputBuffer;		// Temporary buffer used during encoding
	S32 mOutputBufferSize;	// Bytes in mOuputBuffer
	S32 mEncodeQuality;		// On a scale from 1 to 100
};
