/**
 * @file llmaterial.h
 * @brief Material definition
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 *
 * Copyright (c) 2012, Linden Research, Inc.
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

#include "llmaterialid.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llsd.h"
#include "llcolor4u.h"

class LLMaterial : public LLRefCount
{
public:
	typedef enum
	{
		DIFFUSE_ALPHA_MODE_NONE = 0,
		DIFFUSE_ALPHA_MODE_BLEND = 1,
		DIFFUSE_ALPHA_MODE_MASK = 2,
		DIFFUSE_ALPHA_MODE_EMISSIVE = 3,
		DIFFUSE_ALPHA_MODE_DEFAULT = 4,
	} eDiffuseAlphaMode;

	enum eShaderCount : U32
	{
		SHADER_COUNT = 16,
		ALPHA_SHADER_COUNT = 4
	};

	static const LLColor4U	DEFAULT_SPECULAR_LIGHT_COLOR;
	static constexpr U8		DEFAULT_SPECULAR_LIGHT_EXPONENT = (U8)(0.2f * 255);
	static constexpr U8		DEFAULT_ENV_INTENSITY = 0;

	LLMaterial();
	LLMaterial(const LLSD& material_data);

	LLSD asLLSD() const;
	void fromLLSD(const LLSD& material_data);

	LL_INLINE const LLUUID& getNormalID() const			{ return mNormalID; }
	LL_INLINE void setNormalID(const LLUUID& id)		{ mNormalID = id; }

	LL_INLINE void getNormalOffset(F32& offset_x, F32& offset_y) const
	{
		offset_x = mNormalOffsetX;
		offset_y = mNormalOffsetY;
	}

	LL_INLINE F32 getNormalOffsetX() const				{ return mNormalOffsetX; }
	LL_INLINE F32 getNormalOffsetY() const				{ return mNormalOffsetY; }

	LL_INLINE void setNormalOffset(F32 offset_x, F32 offset_y)
	{
		mNormalOffsetX = offset_x;
		mNormalOffsetY = offset_y;
	}

	LL_INLINE void setNormalOffsetX(F32 offset_x)		{ mNormalOffsetX = offset_x; }
	LL_INLINE void setNormalOffsetY(F32 offset_y)		{ mNormalOffsetY = offset_y; }

	LL_INLINE void getNormalRepeat(F32& repeat_x, F32& repeat_y) const
	{
		repeat_x = mNormalRepeatX;
		repeat_y = mNormalRepeatY;
	}

	LL_INLINE F32 getNormalRepeatX() const				{ return mNormalRepeatX; }
	LL_INLINE F32 getNormalRepeatY() const				{ return mNormalRepeatY; }

	LL_INLINE void setNormalRepeat(F32 repeat_x, F32 repeat_y)
	{
		mNormalRepeatX = repeat_x;
		mNormalRepeatY = repeat_y;
	}

	LL_INLINE void setNormalRepeatX(F32 repeat_x)		{ mNormalRepeatX = repeat_x; }
	LL_INLINE void setNormalRepeatY(F32 repeat_y)		{ mNormalRepeatY = repeat_y; }

	LL_INLINE F32 getNormalRotation() const				{ return mNormalRotation; }
	LL_INLINE void setNormalRotation(F32 rot)			{ mNormalRotation = rot; }

	LL_INLINE const LLUUID& getSpecularID() const		{ return mSpecularID; }
	LL_INLINE void setSpecularID(const LLUUID& id)		{ mSpecularID = id; }

	LL_INLINE void getSpecularOffset(F32& offset_x, F32& offset_y) const
	{
		offset_x = mSpecularOffsetX;
		offset_y = mSpecularOffsetY;
	}

	LL_INLINE F32 getSpecularOffsetX() const			{ return mSpecularOffsetX; }
	LL_INLINE F32 getSpecularOffsetY() const			{ return mSpecularOffsetY; }

	LL_INLINE void setSpecularOffset(F32 offset_x, F32 offset_y)
	{
		mSpecularOffsetX = offset_x;
		mSpecularOffsetY = offset_y;
	}

	LL_INLINE void setSpecularOffsetX(F32 offset_x)		{ mSpecularOffsetX = offset_x; }
	LL_INLINE void setSpecularOffsetY(F32 offset_y)		{ mSpecularOffsetY = offset_y; }

	LL_INLINE void getSpecularRepeat(F32& repeat_x, F32& repeat_y) const
	{
		repeat_x = mSpecularRepeatX;
		repeat_y = mSpecularRepeatY;
	}

	LL_INLINE F32 getSpecularRepeatX() const			{ return mSpecularRepeatX; }
	LL_INLINE F32 getSpecularRepeatY() const			{ return mSpecularRepeatY; }

	LL_INLINE void setSpecularRepeat(F32 repeat_x, F32 repeat_y)
	{
		mSpecularRepeatX = repeat_x;
		mSpecularRepeatY = repeat_y;
	}

	LL_INLINE void setSpecularRepeatX(F32 repeat_x)		{ mSpecularRepeatX = repeat_x; }
	LL_INLINE void setSpecularRepeatY(F32 repeat_y)		{ mSpecularRepeatY = repeat_y; }

	LL_INLINE F32 getSpecularRotation() const			{ return mSpecularRotation; }
	LL_INLINE void setSpecularRotation(F32 rot)			{ mSpecularRotation = rot; }

	LL_INLINE const LLColor4U& getSpecularLightColor() const
	{
		return mSpecularLightColor;
	}

	LL_INLINE void setSpecularLightColor(const LLColor4U& color)
	{
		mSpecularLightColor = color;
	}

	LL_INLINE U8 getSpecularLightExponent() const		{ return mSpecularLightExponent; }
	LL_INLINE void setSpecularLightExponent(U8 e)		{ mSpecularLightExponent = e; }
	LL_INLINE U8 getEnvironmentIntensity() const		{ return mEnvironmentIntensity; }
	LL_INLINE void setEnvironmentIntensity(U8 i)		{ mEnvironmentIntensity = i; }
	LL_INLINE U8 getDiffuseAlphaMode() const			{ return mDiffuseAlphaMode; }
	LL_INLINE void setDiffuseAlphaMode(U8 mode)			{ mDiffuseAlphaMode = mode; }
	LL_INLINE U8 getAlphaMaskCutoff() const				{ return mAlphaMaskCutoff; }
	LL_INLINE void setAlphaMaskCutoff(U8 cutoff)		{ mAlphaMaskCutoff = cutoff; }

	bool isNull() const;

	bool operator ==(const LLMaterial& rhs) const;
	bool operator !=(const LLMaterial& rhs) const;

	U32 getShaderMask(U32 alpha_mode, bool is_alpha);

	LLUUID getHash() const;

public:
	static const LLMaterial null;

protected:
	// Note: before these variables, we find the 32 bits counter from
	// LLRefCount... Placing five 32 bits floats first ensures the UUIDs
	// are aligned on 64 bits (where they are faster). HB

	F32			mNormalOffsetX;
	F32			mNormalOffsetY;
	F32			mNormalRepeatX;
	F32			mNormalRepeatY;
	F32			mNormalRotation;
	LLUUID		mNormalID;

	LLUUID		mSpecularID;
	F32			mSpecularOffsetX;
	F32			mSpecularOffsetY;
	F32			mSpecularRepeatX;
	F32			mSpecularRepeatY;
	F32			mSpecularRotation;

	LLColor4U	mSpecularLightColor;
	U8			mSpecularLightExponent;
	U8			mEnvironmentIntensity;
	U8			mDiffuseAlphaMode;
	U8			mAlphaMaskCutoff;
};

typedef LLPointer<LLMaterial> LLMaterialPtr;
