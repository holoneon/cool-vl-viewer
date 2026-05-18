/** 
 * @file linden_common.h
 * @brief Includes common headers that are always safe to include
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

// *NOTE: Please keep includes here to a minimum !  Files included here are
// included in every *.cpp file...

#include "llpreprocessor.h"

#if LL_WINDOWS
# include "llwindowsheaderslean.h"
# ifdef _DEBUG
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
# endif
#endif

#include <cstring>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#if LL_OPENMP
# include <omp.h>
#endif

// Linden only libs in alpha-order
#include "llcommonmath.h"
#include "llerror.h"
#include "llfile.h"
#include "hbintrinsics.h"
#include "lluuid.h"
