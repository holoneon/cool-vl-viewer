/** 
 * @file llviewerprecompiledheaders.h
 * @brief precompiled headers for newview project
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

// This file MUST be the first one included by each .cpp file in viewer. It is
// used to precompile headers for improved build speed.

#include "linden_common.h"

// Headers from llcommon
#include "indra_constants.h"
#include "llassettype.h"
#include "llcriticaldamp.h"
#include "llframetimer.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llsingleton.h"
#include "llstrider.h"
#include "llstring.h"
#include "lltimer.h"

// Headers from llmath
#include "llmath.h"
#include "llcamera.h"
#include "llcoord.h"
#include "llplane.h"
#include "llquantize.h"
#include "llrand.h"
#include "llrect.h"
#include "llmatrix4.h"
#include "llvector2.h"
#include "llcolor3.h"
#include "llvector3d.h"
#include "llvector3.h"
#include "llcolor4.h"
#include "llcolor4u.h"
#include "llvector4.h"
#include "llxform.h"
