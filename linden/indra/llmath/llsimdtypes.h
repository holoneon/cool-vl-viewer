/**
 * @file llsimdtypes.h
 * @brief Declaration of basic SIMD math related types
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

#if LL_DEBUG
# include "llmemory.h"		// For ll_assert_aligned()
#endif

typedef __m128	LLQuad;

#if LL_WINDOWS
# pragma warning(push)
// Disable warning about casting int to bool for this class.
# pragma warning( disable : 4800 3 )
#endif // LL_WINDOWS

class LLBool32
{
public:
	LL_INLINE LLBool32()
	: mBool(0)
	{
	}

	LL_INLINE LLBool32(int rhs)
	:	mBool(rhs)
	{
	}

	LL_INLINE LLBool32(unsigned int rhs)
	:	mBool(rhs)
	{
	}

	LL_INLINE LLBool32(bool rhs)
	: mBool((int)rhs)
	{
	}

	LL_INLINE LLBool32& operator=(bool rhs)
	{
		mBool = (int)rhs;
		return *this;
	}

	LL_INLINE bool operator==(bool rhs) const
	{
		return (const bool&)mBool == rhs;
	}

	LL_INLINE bool operator!=(bool rhs) const
	{
		return (const bool&)mBool != rhs;
	}

	LL_INLINE operator bool() const
	{
		return (const bool&)mBool;
	}

private:
	int mBool;
};

#if LL_WINDOWS
# pragma warning(pop)
#endif

class alignas(16) LLSimdScalar
{
public:
	LL_INLINE LLSimdScalar() noexcept
	:	mQ{}
	{
	}

	LL_INLINE LLSimdScalar(LLQuad q) noexcept
	{
		mQ = q;
	}

	LL_INLINE LLSimdScalar(F32 f) noexcept
	{
		mQ = _mm_set_ss(f);
	}

	LL_INLINE F32 getF32() const
	{
		F32 ret;
		_mm_store_ss(&ret, mQ);
		return ret;
	}

	LL_INLINE LLSimdScalar getAbs() const
	{
		alignas(16) thread_local const U32 F_ABS_MASK_4A[4] = {
			0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF
		};
#if LL_DEBUG
		ll_assert_aligned(F_ABS_MASK_4A, 16);
#endif
		return _mm_and_ps(mQ, *reinterpret_cast<const LLQuad*>(F_ABS_MASK_4A));
	}

	LL_INLINE void setMax(const LLSimdScalar& a, const LLSimdScalar& b)
	{
		mQ = _mm_max_ss(a, b);
	}

	LL_INLINE void setMin(const LLSimdScalar& a, const LLSimdScalar& b)
	{
		mQ = _mm_min_ss(a, b);
	}

	LL_INLINE LLSimdScalar& operator=(F32 rhs)
	{
		mQ = _mm_set_ss(rhs);
		return *this;
	}

	LL_INLINE LLSimdScalar& operator+=(const LLSimdScalar& rhs)
	{
		mQ = _mm_add_ss(mQ, rhs);
		return *this;
	}

	LL_INLINE LLSimdScalar& operator-=(const LLSimdScalar& rhs)
	{
		mQ = _mm_sub_ss(mQ, rhs);
		return *this;
	}

	LL_INLINE LLSimdScalar& operator*=(const LLSimdScalar& rhs)
	{
		mQ = _mm_mul_ss(mQ, rhs);
		return *this;
	}

	LL_INLINE LLSimdScalar& operator/=(const LLSimdScalar& rhs)
	{
		mQ = _mm_div_ss(mQ, rhs);
		return *this;
	}

	LL_INLINE operator LLQuad() const
	{
		return mQ;
	}

	LL_INLINE const LLQuad& getQuad() const
	{
		return mQ;
	}

	LL_INLINE LLBool32 isApproximatelyEqual(const LLSimdScalar& rhs,
											F32 tolerance = F_APPROXIMATELY_ZERO) const
	{
		const LLSimdScalar tol(tolerance);
		const LLSimdScalar diff = _mm_sub_ss(mQ, rhs.mQ);
		const LLSimdScalar abs_diff = diff.getAbs();
		return _mm_comile_ss(abs_diff, tol);	// return abs_diff <= tol;
	}

	static LL_INLINE const LLSimdScalar& getZero()
	{
		extern const LLQuad F_ZERO_4A;
		return reinterpret_cast<const LLSimdScalar&>(F_ZERO_4A);
	}

private:
	LLQuad mQ;
};

LL_INLINE LLSimdScalar operator+(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t += b;
	return t;
}

LL_INLINE LLSimdScalar operator-(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t -= b;
	return t;
}

LL_INLINE LLSimdScalar operator*(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t *= b;
	return t;
}

LL_INLINE LLSimdScalar operator/(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t /= b;
	return t;
}

LL_INLINE LLSimdScalar operator-(const LLSimdScalar& a)
{
	alignas(16) thread_local const U32 signMask[4] = {
		0x80000000, 0x80000000, 0x80000000, 0x80000000
	};
#if LL_DEBUG
	ll_assert_aligned(signMask, 16);
#endif
	return _mm_xor_ps(*reinterpret_cast<const LLQuad*>(signMask), a);
}

LL_INLINE LLBool32 operator==(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comieq_ss(a, b);
}

LL_INLINE LLBool32 operator!=(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comineq_ss(a, b);
}

LL_INLINE LLBool32 operator<(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comilt_ss(a, b);
}

LL_INLINE LLBool32 operator<=(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comile_ss(a, b);
}

LL_INLINE LLBool32 operator>(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comigt_ss(a, b);
}

LL_INLINE LLBool32 operator>=(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comige_ss(a, b);
}
