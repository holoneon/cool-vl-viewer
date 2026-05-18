/** 
 * @file llaudiodecodemgr.h
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "lluuid.h"

class LLVorbisDecodeState;

class LLAudioDecodeMgr
{
	class Impl;

	friend class LLAudioDecodeMgr::Impl;

protected:
	LOG_CLASS(LLAudioDecodeMgr);

public:
	LLAudioDecodeMgr();
	~LLAudioDecodeMgr();

	void processQueue();
	bool addDecodeRequest(const LLUUID& id);
	void addAudioRequest(const LLUUID& id);

	LL_INLINE static void setGeneralPoolSize(U32 pool_size)
	{
		sMaxDecodes = pool_size * 2;
	}

protected:
	Impl*		mImpl;

	static U32	sMaxDecodes;
};

extern LLAudioDecodeMgr* gAudioDecodeMgrp;
