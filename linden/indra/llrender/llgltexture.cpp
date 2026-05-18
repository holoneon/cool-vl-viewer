/**
 * @file llgltexture.cpp
 * @brief OpenGL texture implementation
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

#include "linden_common.h"

#include "llgltexture.h"

#include "llimagegl.h"

// Made this a global variable to cover differences between grids (SL or
// OpenSim). HB
S32 gMaxImageSizeDefault = 1024;

LLGLTexture::LLGLTexture(bool usemipmaps)
{
	init();
	mUseMipMaps = usemipmaps;
}

LLGLTexture::LLGLTexture(U32 width, U32 height, U8 components, bool usemipmaps)
{
	init();
	mFullWidth = width;
	mFullHeight = height;
	mUseMipMaps = usemipmaps;
	mComponents = components;
	setTexelsPerImage();
}

LLGLTexture::LLGLTexture(const LLImageRaw* rawp, bool usemipmaps)
{
	init();
	mUseMipMaps = usemipmaps;
	// Create an empty image of the specified size and width
	mImageGLp = new LLImageGL(rawp, usemipmaps);
	mImageGLp->setOwner(this);
	mFullWidth = mImageGLp->getWidth();
	mFullHeight = mImageGLp->getHeight();
	mComponents = mImageGLp->getComponents();
	setTexelsPerImage();
}

LLGLTexture::~LLGLTexture()
{
	cleanup();
	mImageGLp = NULL;
}

void LLGLTexture::init()
{
	mBoostLevel = BOOST_NONE;

	mFullWidth = 0;
	mFullHeight = 0;
	mTexelsPerImage = 0;
	mUseMipMaps = false;
	mComponents = 0;

	mTextureState = NO_DELETE;
	mDontDiscard = false;
	mNeedsGLTexture = false;

	mIsMegaTexture = false;
}

void LLGLTexture::cleanup()
{
	if (mImageGLp)
	{
		mImageGLp->cleanup();
	}
}

// virtual
void LLGLTexture::dump()
{
	if (mImageGLp)
	{
		mImageGLp->dump();
	}
}

void LLGLTexture::setBoostLevel(U32 level)
{
	// Do not downgrade UI textures, ever !  HB
	if (mBoostLevel == BOOST_UI)
	{
		return;
	}
	if (level == BOOST_UI)
	{
		mBoostLevel = level;
		// UI textures must always be kept in memory for the whole duration of
		// the viewer session. HB
		mTextureState = ALWAYS_KEEP;
		// Also, never allow to discard UI textures. HB
		mDontDiscard = true;
		return;
	}
	mBoostLevel = level;
#if LL_IMPLICIT_SETNODELETE
	if (level != BOOST_NONE && level != BOOST_ALM && level != BOOST_SELECTED)
	{
		mTextureState = NO_DELETE;
	}
#else
	// Make map textures no-delete, always.
	if (level == BOOST_MAP)
	{
		mTextureState = NO_DELETE;
	}
#endif
}

void LLGLTexture::generateGLTexture()
{
	if (mImageGLp.isNull())
	{
		mImageGLp = new LLImageGL(mFullWidth, mFullHeight, mComponents,
								  mUseMipMaps);
		mImageGLp->setOwner(this);
	}
}

LLImageGL* LLGLTexture::getGLImage() const
{
	llassert(mImageGLp.notNull());
	return mImageGLp;
}

bool LLGLTexture::createGLTexture()
{
	if (mImageGLp.isNull())
	{
		generateGLTexture();
	}
	return mImageGLp->createGLTexture();
}

bool LLGLTexture::createGLTexture(S32 discard_level, const LLImageRaw* rawimg,
								  S32 usename, bool to_create, bool defer_copy,
								  U32* tex_name)
{
	if (mImageGLp.isNull())
	{
		llwarns << "NULL GL image for GL texture 0x" << std::hex
				<< (intptr_t)this << std::dec << llendl;
		llassert(false);
		return false;
	}

	bool ret = mImageGLp->createGLTexture(discard_level, rawimg, usename,
										  to_create, defer_copy, tex_name);
	if (ret)
	{
		mFullWidth = mImageGLp->getCurrentWidth();
		mFullHeight = mImageGLp->getCurrentHeight();
		mComponents = mImageGLp->getComponents();
		setTexelsPerImage();
	}

	return ret;
}

void LLGLTexture::setExplicitFormat(S32 internal_format, U32 primary_format,
									U32 type_format, bool swap_bytes)
{
	llassert_always(mImageGLp.notNull());
	mImageGLp->setExplicitFormat(internal_format, primary_format, type_format,
								 swap_bytes);
}

void LLGLTexture::setAddressMode(LLTexUnit::eTextureAddressMode mode)
{
	llassert_always(mImageGLp.notNull());
	mImageGLp->setAddressMode(mode);
}

void LLGLTexture::setFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
	llassert_always(mImageGLp.notNull());
	mImageGLp->setFilteringOption(option);
}

//virtual
S32	LLGLTexture::getWidth(S32 discard_level) const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getWidth(discard_level);
}

//virtual
S32	LLGLTexture::getHeight(S32 discard_level) const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getHeight(discard_level);
}

S32 LLGLTexture::getMaxDiscardLevel() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getMaxDiscardLevel();
}

S32 LLGLTexture::getDiscardLevel() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getDiscardLevel();
}

S8 LLGLTexture::getComponents() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getComponents();
}

U32 LLGLTexture::getTexName() const
{
	return mImageGLp.notNull() ? mImageGLp->getTexName() : 0;
}

bool LLGLTexture::hasGLTexture() const
{
	return mImageGLp.notNull() && mImageGLp->getHasGLTexture();
}

bool LLGLTexture::getBoundRecently() const
{
	return mImageGLp.notNull() && mImageGLp->getBoundRecently();
}

LLTexUnit::eTextureType LLGLTexture::getTarget() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getTarget();
}

bool LLGLTexture::setSubImage(const LLImageRaw* rawimg, S32 x_pos, S32 y_pos,
							  S32 width, S32 height, U32 use_name)
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->setSubImage(rawimg, x_pos, y_pos, width, height, 0,
								  use_name);
}

bool LLGLTexture::setSubImage(const U8* datap, S32 data_width, S32 data_height,
							  S32 x_pos, S32 y_pos, S32 width, S32 height,
							  U32 use_name)
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->setSubImage(datap, data_width, data_height, x_pos, y_pos,
								  width, height, 0, use_name);
}

void LLGLTexture::setGLTextureCreated (bool initialized)
{
	llassert_always(mImageGLp.notNull());
	mImageGLp->setGLTextureCreated (initialized);
}

#if 0	// Not used
void LLGLTexture::setTexName(U32 name)
{
	llassert_always(mImageGLp.notNull());
	mImageGLp->setTexName(name);
}

void LLGLTexture::setTarget(U32 target, LLTexUnit::eTextureType bind_target)
{
	llassert_always(mImageGLp.notNull());
	mImageGLp->setTarget(target, bind_target);
}
#endif

LLTexUnit::eTextureAddressMode LLGLTexture::getAddressMode() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getAddressMode();
}

S32 LLGLTexture::getTextureMemory() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->mTextureMemory;
}

U32 LLGLTexture::getPrimaryFormat() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getPrimaryFormat();
}

bool LLGLTexture::getIsAlphaMask() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getIsAlphaMask();
}

bool LLGLTexture::getMask(const LLVector2 &tc)
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getMask(tc);
}

F32 LLGLTexture::getTimePassedSinceLastBound()
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->getTimePassedSinceLastBound();
}

bool LLGLTexture::isJustBound() const
{
	llassert_always(mImageGLp.notNull());
	return mImageGLp->isJustBound();
}

void LLGLTexture::forceUpdateBindStats() const
{
	llassert_always(mImageGLp.notNull());
	mImageGLp->forceUpdateBindStats();
}

void LLGLTexture::destroyGLTexture()
{
	if (mImageGLp.notNull() && mImageGLp->getHasGLTexture())
	{
		mImageGLp->destroyGLTexture();
	}
	mTextureState = DELETED;
}

void LLGLTexture::setTexelsPerImage()
{
	S32 fullwidth = llmin(mFullWidth, (S32)gMaxImageSizeDefault);
	S32 fullheight = llmin(mFullHeight, (S32)gMaxImageSizeDefault);
	mTexelsPerImage = fullwidth * fullheight;
}

// Returns the minimum discard level to apply to this image, depending on its
// dimensions and the gMaxImageSizeDefault limit. HB
S32 LLGLTexture::getMinDiscardLevel() const
{
	S32 max_size = llmax(mFullWidth, mFullHeight);
	if (max_size <= gMaxImageSizeDefault)
	{
		return 0;
	}
	if (max_size > 2 * gMaxImageSizeDefault)
	{
		// E.g. 4K image and 1K max size. HB
		return 2;
	}
	// E.g. 2K image and 1K max size. HB
	return 1;
}
