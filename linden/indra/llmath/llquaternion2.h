/**
 * @file llquaternion2.h
 * @brief LLQuaternion2 class header file - SIMD-enabled quaternion class
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 *
 * Copyright (C) 2010, Linden Research, Inc.
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

/////////////////////////////
// LLQuaternion2
/////////////////////////////
// This class stores a quaternion x*i + y*j + z*k + w in <x, y, z, w> order
// (i.e., w in high order element of vector)
//
// These classes are intentionally minimal right now. If you need additional
// functionality, please contact someone with SSE experience (e.g., Falcon or
// Huseby).
/////////////////////////////

#include "llquaternion.h"

class alignas(16) LLQuaternion2
{
public:
	LL_INLINE LLQuaternion2()
	{
	}

	explicit LLQuaternion2(const class LLQuaternion& quat);

	//////////////////////////
	// Get/Set
	//////////////////////////

	// Load from an LLQuaternion
	LL_INLINE void operator=(const LLQuaternion& quat)
	{
		mQ.loadua(quat.mQ);
	}

	// Return the internal LLVector4a representation of the quaternion
	LL_INLINE const LLVector4a& getVector4a() const;
	LL_INLINE LLVector4a& getVector4aRw();

	/////////////////////////
	// Quaternion modification
	/////////////////////////

	// Set this quaternion to the conjugate of src
	LL_INLINE void setConjugate(const LLQuaternion2& src);

	// Renormalizes the quaternion. Assumes it has nonzero length.
	LL_INLINE void normalize();

	// Quantize this quaternion to 8 bit precision
	LL_INLINE void quantize8();

	// Quantize this quaternion to 16 bit precision
	LL_INLINE void quantize16();

	LL_INLINE void mul(const LLQuaternion2& b);

	/////////////////////////
	// Quaternion inspection
	/////////////////////////

	// Return true if this quaternion is equal to 'rhs'.
	// Note! Quaternions exhibit "double-cover", so any rotation has two equally valid
	// quaternion representations and they will NOT compare equal.
	LL_INLINE bool equals(const LLQuaternion2& rhs, F32 tolerance = F_APPROXIMATELY_ZERO) const;

	// Return true if all components are finite and the quaternion is normalized
	LL_INLINE bool isOkRotation() const;

protected:
	LLVector4a mQ;
};
