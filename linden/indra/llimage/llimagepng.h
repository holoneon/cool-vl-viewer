/*
 * @file llimagepng.h
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

#include "stdtypes.h"
#include "llimage.h"

class LLFile;

class LLImagePNG final : public LLImageFormatted
{
protected:
	LOG_CLASS(LLImagePNG);

	~LLImagePNG() override;

public:
	LLImagePNG();

	std::string getExtension() override		{ return "png"; }
	bool updateData() override;
	bool decode(LLImageRaw* raw_imagep) override;
	bool encode(const LLImageRaw* raw_imagep) override;

	static bool getDimensions(LLFile& filename, S32& width, S32& height);

private:
	U8* mTmpWriteBuffer;
};
