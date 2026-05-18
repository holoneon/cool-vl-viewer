/**
 * @file llsettingswater.h
 * @brief The water settings asset support class.
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 *
 * Copyright (c) 2001-2019, Linden Research, Inc.
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

#include "llsettingsbase.h"

class LLSettingsWater : public LLSettingsBase
{
protected:
	LOG_CLASS(LLSettingsWater);

	LLSettingsWater();

public:
	LLSettingsWater(const LLSD& data);
	~LLSettingsWater() override							{}

	typedef std::shared_ptr<LLSettingsWater> ptr_t;
	virtual ptr_t buildClone() const = 0;

	LL_INLINE LLSettingsBase::ptr_t buildDerivedClone() const override
	{
		return buildClone();
	}

	LL_INLINE std::string getSettingsType() const override
	{
		return "water";
	}

	LL_INLINE LLSettingsType::EType getSettingsTypeValue() const override
	{
		return LLSettingsType::ST_WATER;
	}

	// Settings status
	void blend(const LLSettingsBase::ptr_t& end, F64 blendf) override;

	void replaceSettings(const LLSD& settings) override;
	void replaceWithWater(LLSettingsWater::ptr_t other);

	static LLSD defaults(F32 position = 0.f);
	// Used to create a day cycle out of an existing sky setting. HB
	static void adjustTime(LLSD& settings, F32 position);

	LL_INLINE F32 getBlurMultiplier() const
	{
		update();
		return mBlurMultiplier;
	}

	LL_INLINE void setBlurMultiplier(F32 val)
	{
		setValue(SETTING_BLUR_MULTIPLIER, val);
	}

	LL_INLINE LLColor3 getWaterFogColor() const
	{
		update();
		return mWaterFogColor;
	}

	LL_INLINE void setWaterFogColor(LLColor3 val)
	{
		setValue(SETTING_FOG_COLOR, val);
	}

	LL_INLINE F32 getWaterFogDensity() const
	{
		update();
		return mWaterFogDensity;
	}

	F32 getModifiedWaterFogDensity(bool underwater) const;

	LL_INLINE void setWaterFogDensity(F32 val)
	{
		setValue(SETTING_FOG_DENSITY, val);
	}

	// NOTE: getFogMod() is not actually used for rendering... HB
	LL_INLINE F32 getFogMod() const
	{
		return mSettings[SETTING_FOG_MOD].asReal();
	}

	// NOTE: setFogMod() is not actually used for rendering... HB
	LL_INLINE void setFogMod(F32 val)
	{
		setValue(SETTING_FOG_MOD, val);
	}

	LL_INLINE F32 getFresnelOffset() const
	{
		update();
		return mFresnelOffset;
	}

	LL_INLINE void setFresnelOffset(F32 val)
	{
		setValue(SETTING_FRESNEL_OFFSET, val);
	}

	LL_INLINE F32 getFresnelScale() const
	{
		update();
		return mFresnelScale;
	}

	LL_INLINE void setFresnelScale(F32 val)
	{
		setValue(SETTING_FRESNEL_SCALE, val);
	}

	LL_INLINE LLUUID getTransparentTextureID() const
	{
		return mSettings[SETTING_TRANSPARENT_TEXTURE].asUUID();
	}

	LL_INLINE void setTransparentTextureID(const LLUUID& val)
	{
		setValue(SETTING_TRANSPARENT_TEXTURE, LLSD(val));
	}

	LLUUID getNormalMapID() const
	{
		return mSettings[SETTING_NORMAL_MAP].asUUID();
	}

	LL_INLINE void setNormalMapID(const LLUUID& val)
	{
		setValue(SETTING_NORMAL_MAP, LLSD(val));
	}

	LL_INLINE LLVector3 getNormalScale() const
	{
		update();
		return mNormalScale;
	}

	LL_INLINE void setNormalScale(LLVector3 val)
	{
		setValue(SETTING_NORMAL_SCALE, val);
	}

	LL_INLINE F32 getScaleAbove() const
	{
		update();
		return mScaleAbove;
	}

	LL_INLINE void setScaleAbove(F32 val)
	{
		setValue(SETTING_SCALE_ABOVE, val);
	}

	LL_INLINE F32 getScaleBelow() const
	{
		update();
		return mScaleBelow;
	}

	LL_INLINE void setScaleBelow(F32 val)
	{
		setValue(SETTING_SCALE_BELOW, val);
	}

	LL_INLINE LLVector2 getWave1Dir() const
	{
		update();
		return mWave1Dir;
	}

	LL_INLINE void setWave1Dir(LLVector2 val)
	{
		setValue(SETTING_WAVE1_DIR, val);
	}

	LL_INLINE LLVector2 getWave2Dir() const
	{
		update();
		return mWave2Dir;
	}

	LL_INLINE void setWave2Dir(LLVector2 val)
	{
		setValue(SETTING_WAVE2_DIR, val);
	}

	LL_INLINE LLUUID getNextNormalMapID() const
	{
		return mNextNormalMapID;
	}

	LL_INLINE LLUUID getNextTransparentTextureID() const
	{
		return mNextTransparentTextureID;
	}

	void updateSettings() override;

	const validation_list_t& getValidationList() const override;
	static const validation_list_t& validationList();

	static LLSD translateLegacySettings(const LLSD& legacy);
	static const LLUUID& getDefaultAssetId();
	static const LLUUID& getDefaultWaterNormalAssetId();
	static const LLUUID& getDefaultTransparentTextureAssetId();
	static const LLUUID& getDefaultOpaqueTextureAssetId();

public:
	static const std::string	SETTING_BLUR_MULTIPLIER;
	static const std::string	SETTING_FOG_COLOR;
	static const std::string	SETTING_FOG_DENSITY;
	static const std::string	SETTING_FOG_MOD;
	static const std::string	SETTING_FRESNEL_OFFSET;
	static const std::string	SETTING_FRESNEL_SCALE;
	static const std::string	SETTING_TRANSPARENT_TEXTURE;
	static const std::string	SETTING_NORMAL_MAP;
	static const std::string	SETTING_NORMAL_SCALE;
	static const std::string	SETTING_SCALE_ABOVE;
	static const std::string	SETTING_SCALE_BELOW;
	static const std::string	SETTING_WAVE1_DIR;
	static const std::string	SETTING_WAVE2_DIR;

	static const LLUUID			DEFAULT_ASSET_ID;

protected:
	static const std::string	SETTING_LEGACY_BLUR_MULTIPLIER;
	static const std::string	SETTING_LEGACY_FOG_COLOR;
	static const std::string	SETTING_LEGACY_FOG_DENSITY;
	static const std::string	SETTING_LEGACY_FOG_MOD;
	static const std::string	SETTING_LEGACY_FRESNEL_OFFSET;
	static const std::string	SETTING_LEGACY_FRESNEL_SCALE;
	static const std::string	SETTING_LEGACY_NORMAL_MAP;
	static const std::string	SETTING_LEGACY_NORMAL_SCALE;
	static const std::string	SETTING_LEGACY_SCALE_ABOVE;
	static const std::string	SETTING_LEGACY_SCALE_BELOW;
	static const std::string	SETTING_LEGACY_WAVE1_DIR;
	static const std::string	SETTING_LEGACY_WAVE2_DIR;

	LLUUID						mNextTransparentTextureID;
	LLUUID						mNextNormalMapID;

private:
	LLColor3					mWaterFogColor;
	LLVector3					mNormalScale;
	LLVector2					mWave1Dir;
	LLVector2					mWave2Dir;
	F32							mWaterFogDensity;
	F32							mBlurMultiplier;
	F32							mFresnelOffset;
	F32							mFresnelScale;
	F32							mScaleAbove;
	F32							mScaleBelow;
};
