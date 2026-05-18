/**
 * @file lltexlayerparams.h
 * @brief Texture layer parameters, used by lltexlayer.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llpointer.h"
#include "llviewervisualparam.h"
#include "llcolor4.h"

class LLAvatarAppearance;
class LLGLTexture;
class LLImageRaw;
class LLImageTGA;
class LLTexLayer;
class LLTexLayerInterface;
class LLWearable;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerParam
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class alignas(16) LLTexLayerParam : public LLViewerVisualParam
{
protected:
	LOG_CLASS(LLTexLayerParam);

	LLTexLayerParam(const LLTexLayerParam& other);

public:
	LLTexLayerParam(LLTexLayerInterface* layer);
	LLTexLayerParam(LLAvatarAppearance* appearance);
	bool setInfo(LLViewerVisualParamInfo* info, bool add_to_app);
	LLViewerVisualParam* cloneParam(LLWearable* wearable) const override = 0;

protected:
	LLTexLayerInterface*	mTexLayer;
	LLAvatarAppearance*		mAvatarAppearance;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerParamAlpha
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class alignas(16) LLTexLayerParamAlpha final : public LLTexLayerParam
{
protected:
	LOG_CLASS(LLTexLayerParamAlpha);

	LLTexLayerParamAlpha(const LLTexLayerParamAlpha& other);

public:
	LLTexLayerParamAlpha(LLTexLayerInterface* layer);
	LLTexLayerParamAlpha(LLAvatarAppearance* appearance);
	~LLTexLayerParamAlpha() override;

	LLViewerVisualParam* cloneParam(LLWearable* wearp = NULL) const override;

	// LLVisualParam Virtual functions

	void apply(ESex avatar_sex) override					{}
	void setWeight(F32 weight, bool upload_bake) override;
	void setAnimationTarget(F32 target_value, bool upload_bake) override;
	void animate(F32 delta, bool upload_bake) override;

#if 0	// Unused methods
	// LLViewerVisualParam Virtual functions

	LL_INLINE F32 getTotalDistortion() override				{ return 1.f; }
	LL_INLINE const LLVector4a& getAvgDistortion() override	{ return mAvgDistortionVec; }
	LL_INLINE F32 getMaxDistortion() override				{ return 3.f; }

	LL_INLINE LLVector4a getVertexDistortion(S32, LLPolyMesh*) override
	{
		return LLVector4a(1.f, 1.f, 1.f);
	}

	LL_INLINE const LLVector4a* getFirstDistortion(U32* index,
												   LLPolyMesh** pmesh) override
	{
		index = 0;
		pmesh = NULL;
		return &mAvgDistortionVec;
	}

	LL_INLINE const LLVector4a* getNextDistortion(U32* index,
												  LLPolyMesh** pmesh) override
	{
		index = 0;
		pmesh = NULL;
		return NULL;
	}
#endif

	// New functions
	bool render(S32 x, S32 y, S32 width, S32 height);
	bool getSkip() const;
	void deleteCaches();
	bool getMultiplyBlend() const;

private:
	LLVector4a				mAvgDistortionVec;
	LLPointer<LLGLTexture>	mCachedProcessedTexture;
	LLPointer<LLImageTGA>	mStaticImageTGA;
	LLPointer<LLImageRaw>	mStaticImageRaw;
	bool					mNeedsCreateTexture;
	bool					mStaticImageInvalid;
	F32						mCachedEffectiveWeight;

public:
	// Global list of instances for gathering statistics
	static void dumpCacheByteCount();
	static void getCacheByteCount(S32* gl_bytes);

	typedef std::list< LLTexLayerParamAlpha* > param_alpha_ptr_list_t;
	static param_alpha_ptr_list_t sInstances;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerParamAlphaInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLTexLayerParamAlphaInfo final : public LLViewerVisualParamInfo
{
	friend class LLTexLayerParamAlpha;

public:
	LLTexLayerParamAlphaInfo();

	bool parseXml(LLXmlTreeNode* node) override;

private:
	F32			mDomain;
	bool		mMultiplyBlend;
	bool		mSkipIfZeroWeight;
	std::string	mStaticImageFileName;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerParamColor
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class alignas(16) LLTexLayerParamColor : public LLTexLayerParam
{
protected:
	LOG_CLASS(LLTexLayerParamColor);

	LLTexLayerParamColor(const LLTexLayerParamColor& other);

public:
	enum EColorOperation
	{
		OP_ADD = 0,
		OP_MULTIPLY = 1,
		OP_BLEND = 2,
		OP_COUNT = 3 // Number of operations
	};

	LLTexLayerParamColor(LLTexLayerInterface* layer);
	LLTexLayerParamColor(LLAvatarAppearance* appearance);

	LLViewerVisualParam* cloneParam(LLWearable* wearp = NULL) const override;

	// LLVisualParam Virtual functions

	LL_INLINE void apply(ESex avatar_sex) override			{}
	void setWeight(F32 weight, bool upload_bake) override;
	void setAnimationTarget(F32 target_value, bool upload_bake) override;
	void animate(F32 delta, bool upload_bake) override;

#if 0	// Unused methods
	// LLViewerVisualParam Virtual functions

	LL_INLINE F32 getTotalDistortion() override				{ return 1.f; }
	LL_INLINE const LLVector4a& getAvgDistortion() override	{ return mAvgDistortionVec; }
	LL_INLINE F32 getMaxDistortion() override				{ return 3.f; }

	LL_INLINE LLVector4a getVertexDistortion(S32, LLPolyMesh*) override
	{
		return LLVector4a(1.f, 1.f, 1.f);
	}

	LL_INLINE const LLVector4a* getFirstDistortion(U32* index,
												   LLPolyMesh** pmesh) override
	{
		index = 0;
		pmesh = NULL;
		return &mAvgDistortionVec;
	}

	LL_INLINE const LLVector4a* getNextDistortion(U32* index,
												  LLPolyMesh** pmesh) override
	{
		index = 0;
		pmesh = NULL;
		return NULL;
	}
#endif

	// New functions
	LLColor4			getNetColor() const;

protected:
	LL_INLINE virtual void onGlobalColorChanged(bool upload_bake)	{}

private:
	LLVector4a	mAvgDistortionVec;
};

class LLTexLayerParamColorInfo final : public LLViewerVisualParamInfo
{
	friend class LLTexLayerParamColor;

protected:
	LOG_CLASS(LLTexLayerParamColorInfo);

public:
	LLTexLayerParamColorInfo();

	bool parseXml(LLXmlTreeNode* node);

	LL_INLINE LLTexLayerParamColor::EColorOperation getOperation() const
	{
		return mOperation;
	}

private:
	enum { MAX_COLOR_VALUES = 20 };
	LLTexLayerParamColor::EColorOperation	mOperation;
	LLColor4								mColors[MAX_COLOR_VALUES];
	S32										mNumColors;
};

typedef std::vector<LLTexLayerParamColor*> param_color_list_t;
typedef std::vector<LLTexLayerParamAlpha*> param_alpha_list_t;
typedef std::vector<LLTexLayerParamColorInfo*> param_color_info_list_t;
typedef std::vector<LLTexLayerParamAlphaInfo*> param_alpha_info_list_t;
