/**
 * @file lldxhardware.h
 * @brief LLDXHardware definition
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

#include "llsd.h"

extern void (*gWriteDebug)(const char* msg);

class LLDXHardware
{
protected:
	LOG_CLASS(LLDXHardware);

public:
	LLDXHardware() = default;

	// Returns info found via DirectX about the display adapter.
	LLSD getDisplayInfo();

	// Gets the VRAM amount of best GPU in MB: returns memory on success, 0 on
	// failure.
	static S32 getMBVideoMemoryViaDXGI();

	LL_INLINE static void setWriteDebugFunc(void (*func)(const char*))
	{
		gWriteDebug = func;
	}

	// Methods to request and hold a high-performance GPU on Windows 10+
	// Laptops can dynamically switch between integrated and discrete GPUs.
	// The viewer has GPU-specific optimizations, and this switching can cause
	// problems and crashes. The login screen requires low performance, which
	// can lead to the OS deciding to switch to the integrated GPU; to avoid
	// this, we request and hold a high-performance GPU using A D3D11 context
	// until login. For diagnostics, we also log GPU changes.
	static void requestHighPerformanceGPU();
	static void detectGPUChange();
	static void clearHighPerformanceGPURequest();

private:
	LLSD mInfo;
};

extern LLDXHardware gDXHardware;
