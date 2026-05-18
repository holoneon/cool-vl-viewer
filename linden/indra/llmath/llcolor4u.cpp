/**
 * @file llcolor4u.cpp
 * @brief LLColor4U class implementation.
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

#include "linden_common.h"

#include "llcolor4u.h"

// LLColor4U
LLColor4U LLColor4U::white(255, 255, 255, 255);
LLColor4U LLColor4U::black(0, 0, 0, 255);
LLColor4U LLColor4U::red(255, 0, 0, 255);
LLColor4U LLColor4U::green(0, 255, 0, 255);
LLColor4U LLColor4U::blue(0, 0, 255, 255);

std::ostream& operator<<(std::ostream& s, const LLColor4U& a)
{
	s << "{ " << (S32)a.mV[VX] << ", " << (S32)a.mV[VY] << ", " << (S32)a.mV[VZ]
	  << ", " << (S32)a.mV[VW] << " }";
	return s;
}

//static
bool LLColor4U::parseColor4U(const std::string& buf, LLColor4U* value,
							 bool strict)
{
	if (buf.empty() || value == NULL)
	{
		return false;
	}

	U32 v[4];
	S32 count = sscanf(buf.c_str(), "%u, %u, %u, %u", v, v + 1, v + 2, v + 3);
	if ((count != 4 && strict) || count < 3)
	{
		// Try this format
		count = sscanf(buf.c_str(), "%u %u %u %u", v, v + 1, v + 2, v + 3);
	}
	if ((count != 4 && strict) || count < 3)
	{
		return false;
	}
	if (count == 3)
	{
		// Default to opaque
		v[3] = U8_MAX;
	}

	for (S32 i = 0; i < 4; ++i)
	{
		if (v[i] > U8_MAX)
		{
			return false;
		}
	}

	value->set(U8(v[0]), U8(v[1]), U8(v[2]), U8(v[3]));
	return true;
}

void LLColor4U::setVecScaleClamp(const LLColor4& color)
{
	F32 color_scale_factor = 255.f;
	F32 max_color = llmax(color.mV[0], color.mV[1], color.mV[2]);
	if (max_color > 1.f)
	{
		color_scale_factor /= max_color;
	}
	constexpr S32 MAX_COLOR = 255;
	S32 r = ll_roundp(color.mV[0] * color_scale_factor);
	if (r > MAX_COLOR)
	{
		r = MAX_COLOR;
	}
	else if (r < 0)
	{
		r = 0;
	}
	mV[0] = r;

	S32 g = ll_roundp(color.mV[1] * color_scale_factor);
	if (g > MAX_COLOR)
	{
		g = MAX_COLOR;
	}
	else if (g < 0)
	{
		g = 0;
	}
	mV[1] = g;

	S32 b = ll_roundp(color.mV[2] * color_scale_factor);
	if (b > MAX_COLOR)
	{
		b = MAX_COLOR;
	}
	else if (b < 0)
	{
		b = 0;
	}
	mV[2] = b;

	// Alpha should not be scaled, just clamped...
	S32 a = ll_roundp(color.mV[3] * MAX_COLOR);
	if (a > MAX_COLOR)
	{
		a = MAX_COLOR;
	}
	else if (a < 0)
	{
		a = 0;
	}
	mV[3] = a;
}

void LLColor4U::setVecScaleClamp(const LLColor3& color)
{
	F32 color_scale_factor = 255.f;
	F32 max_color = llmax(color.mV[0], color.mV[1], color.mV[2]);
	if (max_color > 1.f)
	{
		color_scale_factor /= max_color;
	}

	constexpr S32 MAX_COLOR = 255;
	S32 r = ll_roundp(color.mV[0] * color_scale_factor);
	if (r > MAX_COLOR)
	{
		r = MAX_COLOR;
	}
	else if (r < 0)
	{
		r = 0;
	}
	mV[0] = r;

	S32 g = ll_roundp(color.mV[1] * color_scale_factor);
	if (g > MAX_COLOR)
	{
		g = MAX_COLOR;
	}
	else if (g < 0)
	{
		g = 0;
	}
	mV[1] = g;

	S32 b = ll_roundp(color.mV[2] * color_scale_factor);
	if (b > MAX_COLOR)
	{
		b = MAX_COLOR;
	}
	if (b < 0)
	{
		b = 0;
	}
	mV[2] = b;

	mV[3] = 255;
}
