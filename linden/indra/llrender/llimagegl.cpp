/**
 * @file llimagegl.cpp
 * @brief Generic GL image handler
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

#include "llimagegl.h"

#include "hbfastset.h"
#include "llglslshader.h"
#include "llgltexture.h"
#include "llimage.h"
#include "llmath.h"
#include "llsys.h"					// For LLCPUInfo
#include "lltimer.h"				// For ms_sleep()
#include "hbtracy.h"
#include "lluuid.h"
#include "llvertexbuffer.h"			// For wpo2()
#include "llwindow.h"

#define FIX_MASKS 1

// Static members
LLMutex LLImageGL::sMutex;
LLMutex LLImageGL::sSyncMutex;
std::vector<U32> LLImageGL::sFreeList[LLImageGL::FREE_LIST_SIZE];
U32 LLImageGL::sUniqueCount = 0;
U32 LLImageGL::sBindCount = 0;
U32 LLImageGL::sCurrentFrame = 0;
S64 LLImageGL::sBoundTexMemBytes = 0;
LLAtomicS64 LLImageGL::sGlobalTexMemBytes(0);
LLAtomicS32 LLImageGL::sCount(0);
bool LLImageGL::sGlobalUseAnisotropic = false;
bool LLImageGL::sPreserveDiscard = false;
bool LLImageGL::sCompressTextures = false;
bool LLImageGL::sSetSubImagePerLine = false;
bool LLImageGL::sSyncInThread = false;
U32 LLImageGL::sCompressThreshold = 262144U;
F32 LLImageGL::sLastFrameTime = 0.f;
LLImageGL* LLImageGL::sDefaultGLImagep = NULL;
LLImageGL::glimage_list_t LLImageGL::sImageList;
LLImageGLThread* LLImageGL::sThread = NULL;

constexpr S8 INVALID_OFFSET = -99;
constexpr F32 STALE_IMAGES_TIMEOUT = 10.f;

///////////////////////////////////////////////////////////////////////////////
// Helper functions to track bound GL textures allocations.

static LLMutex sTexMemMutex;
typedef flat_hmap<U32, S64> alloc_map_t;
static alloc_map_t sTextureAllocs;
typedef fast_hset<U32> unused_tex_set_t;
static unused_tex_set_t sUnusedTextures;
static LLAtomicS64 sCurBoundTexBytes(0);

void image_bound(U32 width, U32 height, U32 pixformat, U32 count = 1,
				 U32 tex_name = 0)
{
	if (!tex_name)
	{
		tex_name =
			gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->getCurrTexture();
	}
	if (!tex_name)
	{
		return;
	}
	S64 new_size = S64(count) *
				   LLImageGL::dataFormatBytes(pixformat, width, height);
	S64 old_size = 0;
	sTexMemMutex.lock();

	alloc_map_t::iterator it = sTextureAllocs.find(tex_name);
	if (it == sTextureAllocs.end())
	{
		sTextureAllocs[tex_name] = new_size;
	}
	else
	{
		old_size = it->second;
		it->second = new_size;
	}
	sCurBoundTexBytes += new_size - old_size;

	// Make sure we do not have this texture in the unused textures list. HB
	unused_tex_set_t::iterator it2 = sUnusedTextures.find(tex_name);
	if (it2 != sUnusedTextures.end())
	{
		sUnusedTextures.erase(it2);
	}

	sTexMemMutex.unlock();
}

void image_unbound(U32 tex_name)
{
	if (tex_name)
	{
		S64 size = 0;
		sTexMemMutex.lock();
		alloc_map_t::iterator it = sTextureAllocs.find(tex_name);
		if (it != sTextureAllocs.end())
		{
			size = it->second;
			sTextureAllocs.erase(it);
		}
		sCurBoundTexBytes -= size;
		sTexMemMutex.unlock();
	}
}

void image_unused(U32 tex_name)
{
	if (tex_name)
	{
		sTexMemMutex.lock();
		if (!sTextureAllocs.count(tex_name))
		{
			sUnusedTextures.insert(tex_name);
		}
		sTexMemMutex.unlock();
	}
}

///////////////////////////////////////////////////////////////////////////////

//static
U32 LLImageGL::dataFormatBits(S32 dataformat, bool& valid)
{
	valid = true;
	switch (dataformat)
	{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:			return 4;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:	return 4;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:			return 8;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:	return 8;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:			return 8;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:	return 8;
		case GL_COMPRESSED_LUMINANCE:					return 8;
		case GL_LUMINANCE:								return 8;
		case GL_LUMINANCE8:								return 8;
		case GL_COMPRESSED_ALPHA:						return 8;
		case GL_ALPHA:									return 8;
		case GL_ALPHA8:									return 8;
		case GL_COLOR_INDEX:							return 8;
		case GL_COMPRESSED_RED:							return 8;
		case GL_RED:									return 8;
		case GL_R8:										return 8;
		case GL_COMPRESSED_LUMINANCE_ALPHA:				return 16;
		case GL_LUMINANCE_ALPHA:						return 16;
		case GL_LUMINANCE8_ALPHA8:						return 16;
		case GL_COMPRESSED_RG:							return 16;
		case GL_RG:										return 16;
		case GL_RG8:									return 16;
		case GL_R16F:									return 16;
		case GL_COMPRESSED_RGB:							return 24;
		case GL_RGB:									return 24;
		case GL_SRGB:									return 24;
		case GL_RGB8:									return 24;
		case GL_DEPTH_COMPONENT:						return 24;
		case GL_DEPTH_COMPONENT24:						return 24;
		case GL_R11F_G11F_B10F:							return 32;
		case GL_RGBA:									return 32;
		case GL_RGBA8:									return 32;
		case GL_RGB10_A2:								return 32;
		case GL_BGRA:									return 32;
		case GL_SRGB_ALPHA:								return 32;
		case GL_COMPRESSED_SRGB:						return 32;
		case GL_RG16F:									return 32;
		case GL_R32F:									return 32;
		case GL_RGB16F:									return 48;
		case GL_RGBA16F:								return 64;
		case GL_RG32F:									return 64;
		case GL_RGB32F:									return 96;
		case GL_RGBA32F:								return 128;

		default:
			valid = false;
			return 32;
	}
}

//static
S64 LLImageGL::dataFormatBytes(S32 dataformat, U32 width, U32 height)
{
	if (width < 4 || height < 4)
	{
		if (dataformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
			dataformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT ||
			dataformat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
			dataformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT ||
			dataformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
			dataformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT)
		{
			width = llmax(width, 4);
			height = llmax(height, 4);
		}
	}
	bool valid;
	S64 bits = dataFormatBits(dataformat, valid);
	if (!valid)
	{
		llwarns_once << "Unknown format: " << std::hex << dataformat
					 << std::dec
					 << " - 32 bits assumed, but expect a crash to happen if this assumption is wrong..."
					 << llendl;
		llassert(false);
	}
	S64 bytes = ((S64)width * (S64)height * bits + 7) >> 3;
	return (bytes + 3) & ~3L;	// Keep it a multiple of 4 bytes
}

//static
U32 LLImageGL::dataFormatComponents(S32 dataformat)
{
	switch (dataformat)
	{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:			return 3;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:	return 3;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:			return 4;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:	return 4;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:			return 4;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:	return 4;
		case GL_LUMINANCE:								return 1;
		case GL_ALPHA:									return 1;
		case GL_COLOR_INDEX:							return 1;
		case GL_LUMINANCE_ALPHA:						return 2;
		case GL_RED:									return 1;
		case GL_RG:										return 2;
		case GL_RGB:									return 3;
		case GL_SRGB:									return 3;
		case GL_RGBA:									return 4;
		case GL_SRGB_ALPHA:								return 4;
		case GL_BGRA:									return 4;

		default:
			llwarns_once << "Unknown format: " << std::hex << dataformat
						 << std::dec
						 << " - 4 components assumed, but expect a crash to happen if this assumption is wrong..."
						 << llendl;
			llassert(false);
			return 4;
	}
}

bool LLImageGL::isCompressed() const
{
	switch (mFormatPrimary)
	{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
			return true;

		default:
			return false;
	}
}

//static
void LLImageGL::destroyGL(bool save_state)
{
	// Release all textures pending for deletion. HB
	freeDeletedTextures(true);

	for (S32 stage = 0; stage < LLGLManager::sNumTextureImageUnits; ++stage)
	{
		gGL.getTexUnit(stage)->unbind(LLTexUnit::TT_TEXTURE);
	}

	sMutex.lock();
	for (glimage_list_t::iterator iter = sImageList.begin(),
								  end = sImageList.end();
		 iter != end; ++iter)
	{
		LLImageGL* imagep = *iter;
		imagep->syncTexName();
		if (imagep->mTexName)
		{
			if (save_state && imagep->isGLTextureCreated() &&
				imagep->mComponents)
			{
				imagep->mSaveData = new LLImageRaw;
				if (!imagep->readBackRaw(imagep->mCurrentDiscardLevel,
										 imagep->mSaveData, false))
				{
					imagep->mSaveData = NULL;
				}
			}

			imagep->destroyGLTexture();
			stop_glerror();
		}
	}
	sMutex.unlock();
}

//static
void LLImageGL::restoreGL()
{
	sMutex.lock();
	for (glimage_list_t::iterator iter = sImageList.begin(),
								  end = sImageList.end();
		 iter != end; ++iter)
	{
		LLImageGL* imagep = *iter;
		if (imagep->getTexName())
		{
			llwarns << "Tex name is not 0." << llendl;
		}
		if (imagep->mSaveData.notNull())
		{
			if (imagep->getComponents() &&
				imagep->mSaveData->getComponents())
			{
				imagep->createGLTexture(imagep->mCurrentDiscardLevel,
										imagep->mSaveData, 0, true);
				stop_glerror();
			}
			imagep->mSaveData = NULL; // Deletes data
		}
	}
	sMutex.unlock();
}

//static
void LLImageGL::dirtyTexOptions()
{
	sMutex.lock();
	for (glimage_list_t::iterator iter = sImageList.begin(),
								  end = sImageList.end();
		 iter != end; ++iter)
	{
		LLImageGL* imagep = *iter;
		imagep->mTexOptionsDirty = true;
	}
	sMutex.unlock();
}

// This method is used to allow releasing old NO_DELETE fetched textures which
// associated GL image have not been used for rendering in a while. It only
// affects fetched textures, and not the ones that must be kept untouched or
// the "manual"/local GL textures (UI textures, bump maps, map tiles, media
// textures, preview textures, etc). Without a periodic call to this method,
// the viewer "leaks" (keeps uselessly) GL textures around, which clobber the
// RAM, and worst, the VRAM. HB
//static
U32 LLImageGL::activateStaleTextures()
{
	U32 activated = 0;
	sMutex.lock();
	for (glimage_list_t::iterator iter = sImageList.begin(),
								  end = sImageList.end();
		 iter != end; ++iter)
	{
		LLImageGL* imagep = *iter;
		// If this image has been recently bound, then it is not a candidate.
		if (sLastFrameTime - imagep->mLastBindTime < STALE_IMAGES_TIMEOUT)
		{
			continue;
		}
		LLGLTexture* ownerp = imagep->getOwner();
		// We only care about fetched textures, i.e. the ones with an owner.
		if (!ownerp)
		{
			continue;
		}
		U32 boost_level = ownerp->getBoostLevel();
		// Do not touch avatar bakes, sculpties, UI, map, preview, bumps, media
		// textures...
		if (boost_level <= LLGLTexture::BOOST_SUPER_HIGH &&
			boost_level != LLGLTexture::BOOST_SCULPTED &&
			// And only textures in NO_DELETE state need to be re-ACTIVATEd
			ownerp->getTextureState() == LLGLTexture::NO_DELETE)
		{
			ownerp->forceActive();
			++activated;
		}
	}
	sMutex.unlock();
	return activated;
}

LLImageGL::LLImageGL(bool usemipmaps, bool allow_compression)
:	mOwnerp(NULL),
	mAutoGenMips(false)
{
	init(usemipmaps, allow_compression);
	setSize(0, 0, 0);
	sMutex.lock();
	sImageList.insert(this);
	++sCount;
	sMutex.unlock();
}

LLImageGL::LLImageGL(U32 width, U32 height, U8 components, bool usemipmaps,
					 bool allow_compression)
:	mOwnerp(NULL),
	mAutoGenMips(false)
{
	llassert(components <= 4);
	init(usemipmaps, allow_compression);
	setSize(width, height, components);
	sMutex.lock();
	sImageList.insert(this);
	++sCount;
	sMutex.unlock();
}

LLImageGL::LLImageGL(const LLImageRaw* imagerawp, bool usemipmaps,
					 bool allow_compression)
:	mOwnerp(NULL),
	mAutoGenMips(false)
{
	init(usemipmaps, allow_compression);
	setSize(0, 0, 0);
	sMutex.lock();
	sImageList.insert(this);
	++sCount;
	sMutex.unlock();

	createGLTexture(0, imagerawp);
}

LLImageGL::LLImageGL(U32 tex_name, U32 components, U32 target,
					 S32 internal_fmt, S32 primary_fmt, S32 format_type,
					 LLTexUnit::eTextureAddressMode mode)
:	mOwnerp(NULL),
	mAutoGenMips(false)
{
	init(false, true);
	mTexName = tex_name;
	mComponents = components;
	mTarget = target;
	mFormatInternal = internal_fmt;
	mFormatPrimary = primary_fmt;
	mFormatType = format_type;
	mAddressMode = mode;
	sMutex.lock();
	sImageList.insert(this);
	++sCount;
	sMutex.unlock();
}

LLImageGL::~LLImageGL()
{
	sMutex.lock();
	sImageList.erase(this);
	--sCount;
	sMutex.unlock();
	cleanup();
}

void LLImageGL::init(bool usemipmaps, bool allow_compression)
{
	mNewTexName = 0;
	mHasSync = false;
	mTexNameSync = 0;

	mTextureMemory = 0;
	mLastBindTime = 0.f;

	mPickMask = NULL;
	mPickMaskWidth = 0;
	mPickMaskHeight = 0;
	mUseMipMaps = usemipmaps;
	mHasExplicitFormat = false;

	mIsMask = false;
#if FIX_MASKS
	mNeedsAlphaAndPickMask = false;
	mAlphaOffset = INVALID_OFFSET;
#else
	mNeedsAlphaAndPickMask = true;
	mAlphaOffset = 0;
#endif
	mAlphaStride = 0;

	mGLTextureCreated = false;
	mTexName = 0;
	mWidth = 0;
	mHeight = 0;
	mCurrentDiscardLevel = -1;

	mAllowCompression = allow_compression;

	mTarget	= GL_TEXTURE_2D;
	mBindTarget	= LLTexUnit::TT_TEXTURE;
	mHasMipMaps	= false;
	mMipLevels = -1;

	mComponents	= 0;
	mMaxDiscardLevel = MAX_DISCARD_LEVEL;

	mTexOptionsDirty = true;
	mAddressMode = LLTexUnit::TAM_WRAP;
	mFilterOption = LLTexUnit::TFO_ANISOTROPIC;

	mFormatInternal = -1;
	mFormatPrimary = 0;
	mFormatType = GL_UNSIGNED_BYTE;
	mFormatSwapBytes = false;
}

void LLImageGL::cleanup()
{
	if (!LLGLManager::sIsDisabled)
	{
		destroyGLTexture();
	}
	freePickMask();
	mSaveData = NULL; // Delete data
}

// Helper function used to check the size of a texture image. So dim should be
// a positive number
LL_INLINE static bool check_power_of_two(S32 dim)
{
	return !dim || (dim > 0 && !(dim & (dim - 1)));
}

//static
bool LLImageGL::checkSize(S32 width, S32 height)
{
	return check_power_of_two(width) && check_power_of_two(height);
}

bool LLImageGL::setSize(U32 width, U32 height, U8 ncomponents,
						S32 discard_level)
{
	if (width != mWidth || height != mHeight || ncomponents != mComponents)
	{
		// Check if dimensions are a power of two
		if (!checkSize(width, height))
		{
			llwarns << "Texture has non power of two dimension: " << width
					<< "x" << height  << ". Aborted." << llendl;
			return false;
		}

		// Pick mask validity depends on old image size, delete it
		freePickMask();

		mWidth = width;
		mHeight = height;
		mComponents = ncomponents;
		if (ncomponents > 0)
		{
			mMaxDiscardLevel = 0;
			while (width > 1 && height > 1 &&
				   mMaxDiscardLevel < MAX_DISCARD_LEVEL)
			{
				++mMaxDiscardLevel;
				width >>= 1;
				height >>= 1;
			}

			if (discard_level > 0 && !sPreserveDiscard)
			{
				mMaxDiscardLevel = llmax(mMaxDiscardLevel, discard_level);
				if (mMaxDiscardLevel > MAX_DISCARD_LEVEL)
				{
					mMaxDiscardLevel = MAX_DISCARD_LEVEL;
				}
			}
		}
		else
		{
			mMaxDiscardLevel = MAX_DISCARD_LEVEL;
		}
	}

	return true;
}

//virtual
void LLImageGL::dump()
{
	llinfos << "mMaxDiscardLevel = " << S32(mMaxDiscardLevel)
			<< " - mLastBindTime = " << mLastBindTime
			<< " - mTarget = " << S32(mTarget)
			<< " - mBindTarget = " << S32(mBindTarget)
			<< " - mUseMipMaps = " << S32(mUseMipMaps)
			<< " - mHasMipMaps = " << S32(mHasMipMaps)
			<< " - mCurrentDiscardLevel = " << S32(mCurrentDiscardLevel)
			<< " - mFormatInternal = " << S32(mFormatInternal)
			<< " - mFormatPrimary = " << S32(mFormatPrimary)
			<< " - mFormatType = " << S32(mFormatType)
			<< " - mFormatSwapBytes = " << S32(mFormatSwapBytes)
			<< " - mHasExplicitFormat = " << S32(mHasExplicitFormat)
			<< " - mTextureMemory = " << mTextureMemory
			<< " - mTexName = " << mTexName
			<< llendl;
}

std::string LLImageGL::getImageId() const
{
	std::string ret = llformat("GL image 0x%x", (intptr_t)this);
	LLGLTexture* ownerp = getOwner();
	if (ownerp)
	{
		ret += " owned by GL texture " + ownerp->getID().asString();
	}
	return ret;
}

//static
void LLImageGL::dumpStaleList()
{
	U32 num_stale_images = 0;
	S64 total_stale_memory = 0;
	U32 num_scuplties = 0;
	S64 scuplties_memory = 0;
	sMutex.lock();
	for (glimage_list_t::iterator iter = sImageList.begin(),
								  end = sImageList.end();
		 iter != end; ++iter)
	{
		LLImageGL* imagep = *iter;
		if (sLastFrameTime - imagep->mLastBindTime < STALE_IMAGES_TIMEOUT)
		{
			continue;
		}
		LLGLTexture* ownerp = imagep->getOwner();
		U32 boost_level = ownerp ? ownerp->getBoostLevel() : 0;
		if (boost_level == LLGLTexture::BOOST_SCULPTED)
		{
			scuplties_memory += imagep->mTextureMemory;
			++num_scuplties;
		}
		else if (boost_level <= LLGLTexture::BOOST_SUPER_HIGH)
		{
			llinfos << "Image " << std::hex << (intptr_t)imagep << std::dec;
			if (boost_level >= 0)
			{
				llcont << " with boost level " << boost_level;
			}
			else
			{
				llcont << " not owned by a fetched texture";
			}
			llcont << ":" << llendl;

			imagep->dump();
			total_stale_memory += imagep->mTextureMemory;
			++num_stale_images;
		}
	}
	sMutex.unlock();
	llinfos << "Total number of sculpt textures: " << num_scuplties
			<< " (using " << scuplties_memory / 1024
			<< "KB) - Total number of stale images: " << num_stale_images
			<< " - Total leaked memory: " << total_stale_memory / 1024 << "KB."
			<< llendl;
}

//static
void LLImageGL::freeDeletedTextures(bool delete_all)
{
	LL_TRACY_TIMER(TRC_FREE_DELETED_IMAGE);

	U32 idx = 0;
	if (!delete_all)
	{
		++sCurrentFrame;
		idx = (sCurrentFrame + FREE_LIST_SIZE - 1) % FREE_LIST_SIZE;
	}

	// Delete textures in VRAM that have not been registered with image_bound()
	// so that we do not leak them. HB
	sTexMemMutex.lock();
	U32 sz = sUnusedTextures.size();
	if (sz && LLGLManager::sInited)
	{
		std::vector<U32> textures;
		textures.reserve(sz);
		for (unused_tex_set_t::iterator it = sUnusedTextures.begin(),
										end = sUnusedTextures.end();
			 it != end; ++it)
		{
			textures.push_back(*it);
		}
		sUnusedTextures.clear();
		glDeleteTextures(sz, textures.data());
	}
	sTexMemMutex.unlock();

	sMutex.lock();
	do
	{
		if (!sFreeList[idx].empty())
		{
			std::vector<U32>& textures = sFreeList[idx];
			U32 num_textures = textures.size();
			if (LLGLManager::sInited)
			{
				glDeleteTextures(num_textures, textures.data());
			}
			S64 size = 0;
			sTexMemMutex.lock();
			for (U32 i = 0; i < num_textures; ++i)
			{
				alloc_map_t::iterator it = sTextureAllocs.find(textures[i]);
				if (it != sTextureAllocs.end())
				{
					size += it->second;
					sTextureAllocs.erase(it);
				}
			}
			sCurBoundTexBytes -= size;
			sTexMemMutex.unlock();
			textures.clear();
		}
	}
	while (delete_all && ++idx < FREE_LIST_SIZE);

	sMutex.unlock();

	if (sCurBoundTexBytes < 0 || gDebugGL)
	{
		sTexMemMutex.lock();
		S64 total = 0;
		for (alloc_map_t::iterator it = sTextureAllocs.begin(),
								   end = sTextureAllocs.end();
			 it != end; ++it)
		{
			total += it->second;
		}
		if (total != sCurBoundTexBytes)
		{
			llwarns << "Bound textures accounting mismatch: "
					<< sCurBoundTexBytes << ", against: " << total
					<< ". Resynced." << llendl;
			sCurBoundTexBytes = total;
		}
		sTexMemMutex.unlock();
	}
}

//static
void LLImageGL::updateStats(F32 current_time)
{
	sLastFrameTime = current_time;
	sBoundTexMemBytes = sCurBoundTexBytes;
}

bool LLImageGL::updateBindStats() const
{
	const_cast<LLImageGL*>(this)->syncTexName();

	if (mTexName)
	{
		++sBindCount;
		if (mLastBindTime != sLastFrameTime)
		{
			// We have not accounted for this texture yet this frame
			++sUniqueCount;
			mLastBindTime = sLastFrameTime;
			return true;
		}
	}
	return false;
}

void LLImageGL::setExplicitFormat(S32 internal_format, U32 primary_format,
								  U32 type_format, bool swap_bytes)
{
	// Notes:
	//  - Must be called before createTexture()
	//  - It is up to the caller to ensure that the format matches the number
	//    of components.
	mHasExplicitFormat = true;
	mFormatInternal = internal_format;
	mFormatPrimary = primary_format;
	mFormatType = type_format == 0 ? GL_UNSIGNED_BYTE : type_format;
	mFormatSwapBytes = swap_bytes;

	calcAlphaChannelOffsetAndStride();
}

void LLImageGL::setImage(const LLImageRaw* imagerawp)
{
	llassert(imagerawp->getWidth() == getWidth(mCurrentDiscardLevel) &&
			 imagerawp->getHeight() == getHeight(mCurrentDiscardLevel) &&
			 imagerawp->getComponents() == getComponents());
	const U8* rawdatap = imagerawp->getData();
	setImage(rawdatap, false);
}

bool LLImageGL::setImage(const U8* data_in, bool data_hasmips, S32 usename)
{
	LL_TRACY_TIMER(TRC_SET_IMAGE);
	bool is_compressed = isCompressed();

	LLTexUnit* unit0 = gGL.getTexUnit(0);

	if (mUseMipMaps)
	{
		// Set has mip maps to true before binding image so tex parameters get
		// set properly
		unit0->unbind(mBindTarget);
		mHasMipMaps = true;
		mTexOptionsDirty = true;
		setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
	}
	else
	{
		mHasMipMaps = false;
	}

	unit0->bind(this, false, usename);

	if (!data_in)
	{
		if (!setManualImage(mTarget, 0, mFormatInternal, getWidth(),
							getHeight(), mFormatPrimary, mFormatType, NULL,
							mAllowCompression))
		{
			mGLTextureCreated = false;
			return false;
		}
	}
	else if (mUseMipMaps)
	{
		// Just in case, clamp discard level to a valid value. HB
		S32 clamped_discard = llclamp(mCurrentDiscardLevel, 0,
									  mMaxDiscardLevel);
		if (data_hasmips)
		{
			// NOTE: data_in points to largest image; smaller images are stored
			// BEFORE the largest image
			for (S32 d = clamped_discard; d <= mMaxDiscardLevel; ++d)
			{
				U32 w = getWidth(d);
				U32 h = getHeight(d);
				S32 gl_level = d - clamped_discard;

				mMipLevels = llmax(mMipLevels, gl_level);

				if (d > clamped_discard)
				{
					// See above comment
					data_in -= dataFormatBytes(mFormatPrimary, w, h);
				}
				if (is_compressed)
				{
 					GLsizei tex_size = (GLsizei)dataFormatBytes(mFormatPrimary,
																w, h);
					glCompressedTexImage2D(mTarget, gl_level, mFormatPrimary,
										   w, h, 0, tex_size,
										   (GLvoid*)data_in);
					stop_glerror();
				}
				else
				{
					if (mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
						stop_glerror();
					}

					if (!setManualImage(mTarget, gl_level, mFormatInternal, w,
										h, mFormatPrimary, GL_UNSIGNED_BYTE,
										(GLvoid*)data_in, mAllowCompression))
					{
						mGLTextureCreated = false;
						return false;
					}
					if (gl_level == 0)
					{
						analyzeAlpha(data_in, w, h);
					}
					updatePickMask(w, h, data_in);

					if (mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
						stop_glerror();
					}
				}
			}
		}
		else if (!is_compressed)
		{
			if (mAutoGenMips)
			{
				if (mFormatSwapBytes)
				{
					glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
					stop_glerror();
				}

				U32 w = getWidth(clamped_discard);
				U32 h = getHeight(clamped_discard);

				mMipLevels = wpo2(llmax(w, h));

				// Use legacy mipmap generation mode only when core profile is
				// not enabled (to avoid deprecation warnings), or GL version
				// is below 3.0 (to avoid rendering issues).
				bool use_legacy_mimap = !LLRender::sGLCoreProfile ||
										LLGLManager::sGLVersion < 3.f;
				if (use_legacy_mimap)
				{
					glTexParameteri(mTarget, GL_GENERATE_MIPMAP, GL_TRUE);
				}

				if (!setManualImage(mTarget, 0, mFormatInternal, w, h,
									mFormatPrimary, mFormatType, data_in,
									mAllowCompression))
				{
					mGLTextureCreated = false;
					return false;
				}
				analyzeAlpha(data_in, w, h);
				updatePickMask(w, h, data_in);

				if (mFormatSwapBytes)
				{
					glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
					stop_glerror();
				}

				if (!use_legacy_mimap)
				{
					glGenerateMipmap(mTarget);
					stop_glerror();
				}
			}
			else
			{
				// Create mips by hand
				// ~4x faster than gluBuild2DMipmaps
				U32 width = getWidth(clamped_discard);
				U32 height = getHeight(clamped_discard);
				U32 nummips = mMaxDiscardLevel - clamped_discard + 1;
				U32 w = width;
				U32 h = height;
				U8* prev_mip_data = NULL;
				U8* cur_mip_data = NULL;

				mMipLevels = nummips;

				for (U32 m = 0; m < nummips; ++m)
				{
					if (m == 0)
					{
						cur_mip_data = const_cast<U8*>(data_in);
					}
					else
					{
						S32 bytes = w * h * mComponents;
						llassert(prev_mip_data);

						U8* new_data = new (std::nothrow) U8[bytes];
						if (!new_data)
						{
							if (prev_mip_data)
							{
								delete[] prev_mip_data;
							}
							if (cur_mip_data && cur_mip_data != prev_mip_data)
							{
								delete[] cur_mip_data;
							}
							mGLTextureCreated = false;
							return false;
						}

						LLImageBase::generateMip(prev_mip_data, new_data, w, h,
												 mComponents);
						cur_mip_data = new_data;
					}

					if (w > 0 && h > 0 && cur_mip_data)
					{
						if (mFormatSwapBytes)
						{
							glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
							stop_glerror();
						}

						if (!setManualImage(mTarget, m, mFormatInternal, w, h,
											mFormatPrimary, mFormatType,
											cur_mip_data, mAllowCompression))
						{
							mGLTextureCreated = false;
							return false;
						}
						if (m == 0)
						{
							analyzeAlpha(data_in, w, h);
							updatePickMask(w, h, cur_mip_data);
						}

						if (mFormatSwapBytes)
						{
							glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
							stop_glerror();
						}
					}
					else
					{
						llassert(false);
					}

					if (prev_mip_data && prev_mip_data != data_in)
					{
						delete[] prev_mip_data;
					}
					prev_mip_data = cur_mip_data;
					w >>= 1;
					h >>= 1;
				}
				if (prev_mip_data && prev_mip_data != data_in)
				{
					delete[] prev_mip_data;
					prev_mip_data = NULL;
				}
			}
		}
		else
		{
			llerrs << "Compressed Image has mipmaps but data does not (can not auto generate compressed mips)"
				   << llendl;
		}
	}
	else
	{
		mMipLevels = 0;
		U32 w = getWidth();
		U32 h = getHeight();
		if (is_compressed)
		{
			GLsizei tex_size = (GLsizei)dataFormatBytes(mFormatPrimary, w, h);
			glCompressedTexImage2D(mTarget, 0, mFormatPrimary, w, h, 0,
								   tex_size, (GLvoid*)data_in);
			stop_glerror();
		}
		else
		{
			if (mFormatSwapBytes)
			{
				glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
				stop_glerror();
			}

			if (!setManualImage(mTarget, 0, mFormatInternal, w, h,
								mFormatPrimary, mFormatType, (GLvoid*)data_in,
								mAllowCompression))
			{
				mGLTextureCreated = false;
				return false;
			}
			analyzeAlpha(data_in, w, h);

			updatePickMask(w, h, data_in);

			if (mFormatSwapBytes)
			{
				glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
				stop_glerror();
			}
		}
	}
	mGLTextureCreated = true;
	return true;
}

// Breaks up glTexSubImage2D() calls to a manageable size for the GL command
// buffer.
// IMPORTANT: pixformat and pixtype must have been validated by the caller. HB
static void sub_image_lines(U32 target, S32 miplevel, S32 x_offset, S32 y_offset,
							U32 width, U32 height, U32 pixformat, U32 pixtype,
							U32 type_width, const U8* srcp, U32 data_width)
{
	U32 components = LLImageGL::dataFormatComponents(pixformat);
	U32 line_width = data_width * components * type_width;
	U32 y_offset_end = y_offset + height;

	if (width != data_width || height % 32 != 0)
	{
		for (U32 y_pos = y_offset; y_pos < y_offset_end; ++y_pos)
		{
			glTexSubImage2D(target, miplevel, x_offset, y_pos, width, 1,
							pixformat, pixtype, srcp);
			srcp += line_width;
		}
		return;
	}

	// Full width: batch multiple lines at a time, with batch size based on
	// width.
	U32 batch_size;
	if (width > 1024)
	{
		batch_size = 8;
	}
	else if (width > 512)
	{
		batch_size = 16;
	}
	else
	{
		batch_size = 32;
	}
	for (U32 y_pos = y_offset; y_pos < y_offset_end; y_pos += batch_size)
	{
		glTexSubImage2D(target, miplevel, x_offset, y_pos, width, batch_size,
						pixformat, pixtype, srcp);
		srcp += line_width * batch_size;
	}
}

static bool get_pixtype_width(U32 pixtype, U32& type_width)
{
	switch (pixtype)
	{
		case GL_UNSIGNED_BYTE:
		case GL_BYTE:
		case GL_UNSIGNED_INT_8_8_8_8_REV:
			type_width = 1;
			break;

		case GL_UNSIGNED_SHORT:
		case GL_SHORT:
			type_width = 2;
			break;

		case GL_UNSIGNED_INT:
		case GL_INT:
		case GL_FLOAT:
			type_width = 4;
			break;

		default:
			return false;
	}
	return true;
}

bool LLImageGL::setSubImage(const U8* datap, U32 data_width, U32 data_height,
							S32 x_pos, S32 y_pos, U32 width, U32 height,
							bool force_fast_update, U32 use_name)
{
	if (!width || !height)
	{
		return true;
	}

	syncTexName();

	if (use_name == 0)
	{
		use_name = mTexName;
	}
	if (!datap || use_name == 0)
	{
		// *TODO: Re-add warning ?  Ran into thread locking issues ? - DK
		return false;
	}

	// Validate formats to avoid a crash in glTexSubImage2D(). HB
	bool valid;
	dataFormatBits(mFormatPrimary, valid);
	if (!valid)
	{
		llwarns << "Unknown pixel format 0x" << std::hex << mFormatPrimary
				<< std::dec << " for " << getImageId() << ". Aborted."
				<< llendl;
		llassert(false);
		return false;
	}
	U32 type_width;
	if (!get_pixtype_width(mFormatType, type_width))
	{
		llwarns << "Unknown pixel type 0x" << std::hex << mFormatType
				<< std::dec << " for " << getImageId() << ". Aborted."
				<< llendl;
		llassert(false);
		return false;
	}

	// *HACK: allow the caller to explicitly force the fast path (i.e. using
	// glTexSubImage2D here instead of calling setImage) even when updating the
	// full texture.
	if (!force_fast_update && x_pos == 0 && y_pos == 0 &&
		data_width == width && data_height == height &&
		width == getWidth() && height == getHeight())
	{
		setImage(datap, false, use_name);
	}
	else
	{
		if (mUseMipMaps)
		{
			dump();
			llerrs << "Called with mipmapped image (not supported)" << llendl;
		}
		if (mCurrentDiscardLevel != 0)
		{
			dump();
			llerrs << "Invalid discard level (should be 0)." << llendl;
		}
		if (x_pos < 0 || y_pos < 0)
		{
			dump();
			llerrs << "Invalid sub-image position: x_pos = " << x_pos
				   << " - y_pos = " << y_pos << llendl;
		}
		if (U32(x_pos + width) > getWidth() ||
			U32(y_pos + height) > getHeight())
		{
			dump();
			llerrs << "Subimage not wholly in target image !"
				   << " x_pos " << x_pos
				   << " y_pos " << y_pos
				   << " width " << width
				   << " height " << height
				   << " getWidth() " << getWidth()
				   << " getHeight() " << getHeight()
				   << llendl;
		}

		if (U32(x_pos + width) > data_width ||
			U32(y_pos + height) > data_height)
		{
			dump();
			llerrs << "Subimage not wholly in source image !"
				   << " x_pos " << x_pos
				   << " y_pos " << y_pos
				   << " width " << width
				   << " height " << height
				   << " source_width " << data_width
				   << " source_height " << data_height
				   << llendl;
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, data_width);
		stop_glerror();

		if (mFormatSwapBytes)
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
			stop_glerror();
		}

		datap += (y_pos * data_width + x_pos) * getComponents();
		// Update the GL texture
		LLTexUnit* unit0 = gGL.getTexUnit(0);
		bool res = unit0->bindManual(mBindTarget, use_name);
		if (!res)
		{
			llerrs << "gGL.getTexUnit(0)->bindManual() failed" << llendl;
		}

		if (sSetSubImagePerLine && !isCompressed() && !LLGLManager::sIsIntel &&
			is_main_thread())
		{
			sub_image_lines(mTarget, 0, x_pos, y_pos, width, height,
							mFormatPrimary, mFormatType, type_width, datap,
							data_width);
		}
		else
		{
			glTexSubImage2D(mTarget, 0, x_pos, y_pos,
							width, height, mFormatPrimary, mFormatType, datap);
		}

		unit0->disable();
		stop_glerror();

		if (mFormatSwapBytes)
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
			stop_glerror();
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		stop_glerror();
		mGLTextureCreated = true;
	}
	return true;
}

bool LLImageGL::setSubImage(const LLImageRaw* imagerawp, S32 x_pos, S32 y_pos,
							U32 width, U32 height, bool force_fast_update,
							U32 use_name)
{
	return setSubImage(imagerawp->getData(), imagerawp->getWidth(),
					   imagerawp->getHeight(), x_pos, y_pos, width, height,
					   force_fast_update, use_name);
}

// Copy sub image from frame buffer
bool LLImageGL::setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos,
										   S32 y_pos, U32 width, U32 height)
{
	if (gGL.getTexUnit(0)->bind(this, true))
	{
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, fb_x, fb_y, x_pos, y_pos, width,
							height);
		stop_glerror();
		mGLTextureCreated = true;
		return true;
	}
	return false;
}

//static
void LLImageGL::generateTextures(U32 num_textures, U32* textures)
{
	LL_TRACY_TIMER(TRC_GENERATE_TEXTURES);

	for (U32 i = 0; i < num_textures; ++i)
	{
		image_unbound(textures[i]);
	}

	constexpr size_t pool_size = 1024;
	thread_local U32 name_pool[pool_size];	// Pool of texture names
	thread_local U32 name_count = 0;		// Available names in pool
	if (!name_count)
	{
		glGenTextures(pool_size, name_pool);
		name_count = pool_size;
	}

	if (num_textures < name_count)
	{
		memcpy((void*)textures, (void*)(name_pool + name_count - num_textures),
			   sizeof(U32) * num_textures);
		name_count -= num_textures;
	}
	else
	{
		glGenTextures(num_textures, textures);
	}
}

//static
void LLImageGL::deleteTextures(U32 num_textures, const U32* textures)
{
	LL_TRACY_TIMER(TRC_DELETE_IMAGE);

	if (LLGLManager::sInited)
	{
		sMutex.lock();
		U32 idx = sCurrentFrame % FREE_LIST_SIZE;
		for (U32 i = 0; i < num_textures; ++i)
		{
			sFreeList[idx].push_back(textures[i]);
		}
		sMutex.unlock();
	}
}

//static
bool LLImageGL::setManualImage(U32 target, S32 miplevel, S32 intformat,
							   U32 width, U32 height, U32 pixformat,
							   U32 pixtype, const void* pixels,
							   bool allow_compression)
{
	LL_TRACY_TIMER(TRC_SET_MANUAL_IMAGE);

	const U32 pixels_count = width * height;

	if (LLRender::sGLCoreProfile)
	{
		if (LLGLManager::sHasTextureSwizzle)
		{
			if (pixformat == GL_ALPHA)
			{
				// GL_ALPHA is deprecated, convert to RGBA
				const GLint mask[] = { GL_ZERO, GL_ZERO, GL_ZERO, GL_RED };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
				pixformat = GL_RED;
				intformat = GL_R8;
			}

			if (pixformat == GL_LUMINANCE)
			{
				// GL_LUMINANCE is deprecated, convert to RGBA
				const GLint mask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
				pixformat = GL_RED;
				intformat = GL_R8;
			}

			if (pixformat == GL_LUMINANCE_ALPHA)
			{
				// GL_LUMINANCE_ALPHA is deprecated, convert to RGBA
				const GLint mask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
				pixformat = GL_RG;
				intformat = GL_RG8;
			}
		}
		else
		{
			thread_local std::vector<U32> scratch;

			if (pixformat == GL_ALPHA && pixtype == GL_UNSIGNED_BYTE)
			{
				if (pixels)
				{
					// GL_ALPHA is deprecated, convert to RGBA
					scratch.reserve(pixels_count);
					for (U32 i = 0; i < pixels_count; ++i)
					{
						U8* pix = (U8*)&scratch[i];
						pix[0] = pix[1] = pix[2] = 0;
						pix[3] = ((U8*)pixels)[i];
					}
					pixels = (void*)scratch.data();
				}
				pixformat = GL_RGBA;
				intformat = GL_RGBA8;
			}

			if (pixformat == GL_LUMINANCE_ALPHA && pixtype == GL_UNSIGNED_BYTE)
			{
				// GL_LUMINANCE_ALPHA is deprecated, convert to RGBA
				if (pixels)
				{
					scratch.reserve(pixels_count);
					for (U32 i = 0; i < pixels_count; ++i)
					{
						U8 lum = ((U8*)pixels)[2 * i];
						U8 alpha = ((U8*)pixels)[2 * i + 1];
						U8* pix = (U8*)&scratch[i];
						pix[0] = pix[1] = pix[2] = lum;
						pix[3] = alpha;
					}
					pixels = (void*)scratch.data();
				}
				pixformat = GL_RGBA;
				intformat = GL_RGBA8;
			}

			if (pixformat == GL_LUMINANCE && pixtype == GL_UNSIGNED_BYTE)
			{
				// GL_LUMINANCE is deprecated, convert to RGBA
				if (pixels)
				{
					scratch.reserve(pixels_count);
					for (U32 i = 0; i < pixels_count; ++i)
					{
						U8 lum = ((U8*)pixels)[i];
						U8* pix = (U8*)&scratch[i];
						pix[0] = pix[1] = pix[2] = lum;
						pix[3] = 255;
					}
					pixels = (void*)scratch.data();
				}
				pixformat = GL_RGBA;
				intformat = GL_RGB8;
			}
		}
	}

	bool compress = allow_compression && sCompressTextures &&
					pixels_count > sCompressThreshold;
	if (compress)
	{
		switch (intformat)
		{
			case GL_RED:
			case GL_R8:
				intformat = GL_COMPRESSED_RED;
				break;

			case GL_RG:
			case GL_RG8:
				intformat = GL_COMPRESSED_RG;
				break;

			case GL_RGB:
			case GL_RGB8:
				intformat = GL_COMPRESSED_RGB;
				break;

			case GL_SRGB:
			case GL_SRGB8:
				intformat = GL_COMPRESSED_SRGB;
				break;

			case GL_RGBA:
			case GL_RGBA8:
				intformat = GL_COMPRESSED_RGBA;
				break;

			case GL_SRGB_ALPHA:
			case GL_SRGB8_ALPHA8:
				intformat = GL_COMPRESSED_SRGB_ALPHA;
				break;

			case GL_LUMINANCE:
			case GL_LUMINANCE8:
				intformat = GL_COMPRESSED_LUMINANCE;
				break;

			case GL_LUMINANCE_ALPHA:
			case GL_LUMINANCE8_ALPHA8:
				intformat = GL_COMPRESSED_LUMINANCE_ALPHA;
				break;

			case GL_ALPHA:
			case GL_ALPHA8:
				intformat = GL_COMPRESSED_ALPHA;
				break;

			default:
				compress = false;
		}
	}

	// Validate formats to avoid a crash in glTexImage2D(). HB
	bool valid;
	dataFormatBits(pixformat, valid);
	if (!valid)
	{
		llwarns << "Unknown pixel format: 0x" << std::hex << pixformat
				<< std::dec << ". Aborted." << llendl;
		llassert(false);
		return false;
	}
	U32 type_width;
	if (!get_pixtype_width(pixtype, type_width))
	{
		llwarns << "Unknown pixel type: 0x" << std::hex << pixtype << std::dec
				<< ". Aborted." << llendl;
		llassert(false);
		return false;
	}

	if (!compress && sSetSubImagePerLine && !LLGLManager::sIsIntel &&
		is_main_thread())
	{
		glTexImage2D(target, miplevel, intformat, width, height, 0, pixformat,
					 pixtype, NULL);
		if (pixels)
		{
			sub_image_lines(target, miplevel, 0, 0, width, height, pixformat,
							pixtype, type_width, (U8*)pixels, width);
		}
	}
	else
	{
		glTexImage2D(target, miplevel, intformat, width, height, 0, pixformat,
					 pixtype, pixels);
	}
	image_bound(width, height, pixformat);
	stop_glerror();

	return true;
}

// Create an empty GL texture: just create a texture name. The texture is
// associated with some image by calling glTexImage outside LLImageGL.
bool LLImageGL::createGLTexture()
{
	LL_TRACY_TIMER(TRC_CREATE_GL_TEXTURE1);
	if (LLGLManager::sIsDisabled)
	{
		llwarns << "Trying to create a texture while GL is disabled !"
				<< llendl;
		return false;
	}

	// Do not save this texture when GL is destroyed.
	mGLTextureCreated = false;

	llassert(LLGLManager::sInited);
	stop_glerror();

	syncTexName();

	if (mTexName)
	{
		deleteTextures(1, &mTexName);
		mTexName = 0;
	}

	generateTextures(1, &mTexName);
	stop_glerror();
	if (!mTexName)
	{
		llwarns << "Failed to make an empty texture" << llendl;
		return false;
	}

	return true;
}

bool LLImageGL::createGLTexture(S32 discard_level, const LLImageRaw* imagerawp,
								S32 usename, bool to_create, bool defer_copy,
								U32* tex_name)
{
	LL_TRACY_TIMER(TRC_CREATE_GL_TEXTURE2);
	if (LLGLManager::sIsDisabled || !LLGLManager::sInited)
	{
		llwarns << "Trying to create a texture while GL is disabled or not initialized !"
				<< llendl;
		return false;
	}

	if (!imagerawp || !imagerawp->getData())
	{
		llwarns_sparse << "Trying to create a texture from invalid image data"
					   << llendl;
		mGLTextureCreated = false;
		return false;
	}

	if (sPreserveDiscard)
	{
		discard_level = llclamp(discard_level, 0, mMaxDiscardLevel);
	}
	else if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
		if (discard_level < 0)
		{
			llwarns << "Negative discard level for " << getImageId()
					<< ". Aborted." << llendl;
			mGLTextureCreated = false;
			return false;
		}
	}
	if (discard_level > MAX_DISCARD_LEVEL)
	{
		llwarns << "Invalid discard level (" << discard_level << ") for"
				<< getImageId() << ". Aborted." << llendl;
		mGLTextureCreated = false;
		return false;
	}

	// Actual image width/height = raw image width/height * 2^discard_level
	U32 raw_w = imagerawp->getWidth();
	U32 raw_h = imagerawp->getHeight();
	U32 w = raw_w << discard_level;
	U32 h = raw_h << discard_level;

	if (!setSize(w, h, imagerawp->getComponents(), discard_level))
	{
		// Invalid (non power of two) size.
		mGLTextureCreated = false;
		return false;
	}

	if (mHasExplicitFormat &&
		((mFormatPrimary == GL_RGBA && mComponents < 4) ||
		 (mFormatPrimary == GL_RGB && mComponents < 3)))
	{
		llwarns << "Incorrect format: " << std::hex << mFormatPrimary
				<< std::dec << " - Number of components: " << (U32)mComponents
				<< " - For " << getImageId() << ". Explicit format ignored."
				<< llendl;
		mHasExplicitFormat = false;
	}

	if (!mHasExplicitFormat)
	{
		switch (mComponents)
		{
			case 1:
				// Use luminance alpha (for fonts)
				mFormatInternal = GL_LUMINANCE8;
				mFormatPrimary = GL_LUMINANCE;
				mFormatType = GL_UNSIGNED_BYTE;
				break;

			case 2:
				// Use luminance alpha (for fonts)
				mFormatInternal = GL_LUMINANCE8_ALPHA8;
				mFormatPrimary = GL_LUMINANCE_ALPHA;
				mFormatType = GL_UNSIGNED_BYTE;
				break;

			case 3:
				mFormatInternal = GL_RGB8;
				mFormatPrimary = GL_RGB;
				mFormatType = GL_UNSIGNED_BYTE;
				break;

			case 4:
				mFormatInternal = GL_RGBA8;
				mFormatPrimary = GL_RGBA;
				mFormatType = GL_UNSIGNED_BYTE;
				break;

			default:
				llwarns << "Bad number of components ( "
						<< (U32)getComponents() << ") for " << getImageId()
						<< ". Aborted." << llendl;
				llassert(false);
				mGLTextureCreated = false;
				return false;
		}

		calcAlphaChannelOffsetAndStride();
	}

	if (!to_create) // Do not create a GL texture
	{
		destroyGLTexture();
		mCurrentDiscardLevel = discard_level;
		mLastBindTime = sLastFrameTime;
		mGLTextureCreated = false;
		return true;
	}

 	const U8* rawdatap = imagerawp->getData();
	return createGLTexture(discard_level, rawdatap, false, usename, defer_copy,
						   tex_name);
}

bool LLImageGL::createGLTexture(S32 discard_level, const U8* data_in,
								bool data_hasmips, S32 usename,
								bool defer_copy, U32* tex_name)
{
	LL_TRACY_TIMER(TRC_CREATE_GL_TEXTURE3);

	bool main_thread = is_main_thread();

	if (defer_copy)
	{
		data_in = NULL;
	}
	llassert(defer_copy || data_in);
	stop_glerror();

	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	discard_level = llclamp(discard_level, 0,
							llmin(mMaxDiscardLevel, MAX_DISCARD_LEVEL));

	// Note: always force creation of new texname when not on main thread or
	// when defer copy is set.
	if (main_thread)
	{
		syncTexName();
		if (!defer_copy && mTexName && discard_level == mCurrentDiscardLevel)
		{
			// This will only be true if the size has not changed
			if (tex_name)
			{
				*tex_name = mTexName;
			}
			return setImage(data_in, data_hasmips);
		}
	}

	LLTexUnit* unit0 = gGL.getTexUnit(0);

	U32 old_name = mTexName;

	U32 new_name = 0;
	if (usename)
	{
		llassert(main_thread);
		new_name = usename;
	}
	else
	{
		generateTextures(1, &new_name);
		unit0->bind(this, false, new_name);
		U32 type = LLTexUnit::getInternalType(mBindTarget);
		glTexParameteri(type, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(type, GL_TEXTURE_MAX_LEVEL,
						mMaxDiscardLevel - discard_level);
	}

	if (tex_name)
	{
		*tex_name = new_name;
	}

	if (mUseMipMaps)
	{
		mAutoGenMips = true;
	}

	mCurrentDiscardLevel = discard_level;

	if (!setImage(data_in, data_hasmips, new_name))
	{
		return false;
	}

	// Set texture options to our defaults.
	unit0->setHasMipMaps(mHasMipMaps);
	unit0->setTextureAddressMode(mAddressMode);
	unit0->setTextureFilteringOption(mFilterOption);

	// Things will break if we do not unbind after creation
	unit0->unbind(mBindTarget);

	if (old_name)
	{
		sGlobalTexMemBytes -= mTextureMemory;
	}
	if (!defer_copy)
	{
		if (main_thread) // Not on background thread, immediately set mTexName
		{
			if (old_name != 0 && old_name != new_name)
			{
				deleteTextures(1, &old_name);
			}
			mTexName = new_name;
		}
		else
		{
			// If we are on the image loading thread, be sure to delete the
			// old texname and update mTexName on the main thread.
			syncToMainThread(new_name);
		}
	}

	mTextureMemory = getMipBytes();
	sGlobalTexMemBytes += mTextureMemory;

	// Mark this as bound at this point, so we do not throw it out immediately
	mLastBindTime = sLastFrameTime;
	return true;
}

void LLImageGL::syncToMainThread(U32 new_tex_name)
{
	LL_TRACY_TIMER(TRC_IMAGEGL_SYNC);

	llassert(!is_main_thread());

	// Verify that syncToMainThread() has not been called again before the main
	// thread had a chance to call syncTexName() for this texture (yes, this
	// happens a lot): if mNewTexName is non-zero, the name will get registered
	// as an unused texture and then freed at next freeDeletedTextures() call.
	// HB
	image_unused(mNewTexName);
	mNewTexName = 0;	// Make sure the main thread will not use it now.

	// We must now make sure all the GL commands have been flushed down the
	// thread's GL pipeline; without this, you would see flickering/black
	// images, or sudden random texture corruptions (e.g. for UI ones).
	if (LLGLManager::sHasSync)
	{
		if (LLGLManager::sIsNVIDIA && sSyncInThread)
		{
			mHasSync = false;	// Should already be the case. HB
			mTexNameSync = 0;	// Should already be the case. HB
			GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			glFlush();
			glClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
			glDeleteSync(sync);
		}
		else
		{
			sSyncMutex.lock();
			mHasSync = false;
			if (mTexNameSync)
			{
				glDeleteSync(mTexNameSync);
			}
			glFlush();
			mTexNameSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			glFlush();
			mHasSync = true;
			sSyncMutex.unlock();
		}
	}
	else
	{
		glFinish();	// Ouch, costly !... HB
	}

	// Note: instead of posting to the main thread work queue to swap the tex
	// name and delete the old one, we delay these operations (now both
	// performed in syncTexName(), which is called everytime before we use
	// mTexName) until we actually need to use the texture: this avoids to
	// needlessly stall the GL pipeline with a syncing when we could naturally
	// get past the fence due to the delay between the image creation and its
	// use (this was Kathrine Jansma's idea, that I reimplemented so that the
	// modification of mTexName is done only in the main thread, which also
	// allowed to remove the queueing of the deleteTextures() call into the
	// work queue). We therefore only register here the new tex name and in an
	// atomic variable indicating that mTexName needs to be updated. HB
	mNewTexName = new_tex_name;
}

void LLImageGL::setTexName(U32 name)
{
	syncTexName();
	if (mTexName != name)	// We need to update the GL image stats then. HB
	{
		image_unbound(mTexName);
		// The format may not yet have been set... In this case, the image will
		// be created later, and the bound memory will then be updated. HB
		if (mFormatPrimary > 0)
		{
			image_bound(mWidth, mHeight, mFormatPrimary, 1, name);
		}
	}
	mTexName = name;
}

void LLImageGL::syncTexName()
{
	if (mNewTexName && is_main_thread())
	{
		if (mHasSync)
		{
			sSyncMutex.lock();
			mHasSync = false;
			if (mTexNameSync)
			{
				glClientWaitSync(mTexNameSync, 0, GL_TIMEOUT_IGNORED);
				glDeleteSync(mTexNameSync);
				mTexNameSync = 0;
			}
			sSyncMutex.unlock();
		}

		if (mTexName && mTexName != mNewTexName)
		{
			deleteTextures(1, &mTexName);
		}
		mTexName = mNewTexName;
		mNewTexName = 0;
	}
}

void LLImageGL::syncTexName(U32 texname)
{
	if (texname)
	{
		syncTexName();
		if (mTexName && mTexName != texname)
		{
			deleteTextures(1, &mTexName);
		}
		mTexName = texname;
	}
}

bool LLImageGL::readBackRaw(S32 discard_level, LLImageRaw* imagerawp,
							bool compressed_ok) const
{
	const_cast<LLImageGL*>(this)->syncTexName();

	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;	// Validity checked below.
	}
	if (mTexName == 0 || mCurrentDiscardLevel < 0 ||
		discard_level < mCurrentDiscardLevel ||
		discard_level > mMaxDiscardLevel)
	{
		return false;
	}

	GLint gl_discard = discard_level - mCurrentDiscardLevel;

	// Explicitly unbind texture
	LLTexUnit* unit0 = gGL.getTexUnit(0);
	unit0->unbind(mBindTarget);
#if 1
	if (!unit0->bindManual(mBindTarget, mTexName))
	{
		llwarns << "Failed to bind." << llendl;
		return false;
	}
#else
	unit0->bindManual(mBindTarget, mTexName);
#endif

	// This is necessary to prevent previous, unrelated errors causing GL 
	// textures creation failures, due to the fact we are testing here for GL
	// errors and aborting when finding one. HB
	clear_glerror();

	GLint glwidth = 0;
	glGetTexLevelParameteriv(mTarget, gl_discard, GL_TEXTURE_WIDTH, &glwidth);
	if (glwidth == 0)
	{
		// No mip data smaller than current discard level
		return false;
	}

	U32 width = getWidth(discard_level);
	U32 height = getHeight(discard_level);
	U8 ncomponents = getComponents();
	if (ncomponents == 0)
	{
		return false;
	}
	if ((GLint)width < glwidth)
	{
		llwarns << "Texture size is smaller than it should be: width: "
				<< width << " - glwidth: " << glwidth << " - mWidth: "
				<< mWidth << " - mCurrentDiscardLevel: "
				<< (S32)mCurrentDiscardLevel << " - discard_level: "
				<< (S32)discard_level << llendl;
		return false;
	}

	if (width <= 0 || width > 2048 || height <= 0 || height > 2048 ||
		ncomponents < 1 || ncomponents > 4)
	{
		llwarns << "Bogus size/components: " << width << "x" << height << "x"
				<< ncomponents << llendl;
		return false;
	}

	GLint is_compressed = 0;
	if (compressed_ok)
	{
		glGetTexLevelParameteriv(mTarget, is_compressed, GL_TEXTURE_COMPRESSED,
								 &is_compressed);
	}

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		llwarns << "GL Error happens before reading back texture. Error code: "
				<< error << llendl;
		stop_glerror();
		// If there has been an error, we must abort (missing in LL's code). HB
		return false;
	}

	if (is_compressed)
	{
		GLint glbytes;
		glGetTexLevelParameteriv(mTarget, gl_discard,
								 GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &glbytes);
		if (!imagerawp->allocateDataSize(width, height, ncomponents, glbytes))
		{
			llwarns << "Memory allocation failed for reading back texture. Size is: "
					<< glbytes << " - width: " << width << " - height: "
					<< height << " - components: " << ncomponents << llendl;
			return false;
		}

		glGetCompressedTexImage(mTarget, gl_discard,
								(GLvoid*)imagerawp->getData());
	}
	else
	{
		if (!imagerawp->allocateDataSize(width, height, ncomponents))
		{
			llwarns << "Memory allocation failed for reading back texture: width: "
					<< width << " - height: " << height << " - components: "
					<< ncomponents << llendl;
			return false;
		}

		glGetTexImage(GL_TEXTURE_2D, gl_discard, mFormatPrimary, mFormatType,
					  (GLvoid*)(imagerawp->getData()));
	}

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		llwarns << "GL Error happens after reading back texture. Error code: "
				<< error << llendl;
		stop_glerror();
		imagerawp->deleteData();
		return false;
	}

	return true;
}

void LLImageGL::destroyGLTexture()
{
	syncTexName();

	if (mTexName)
	{
		sGlobalTexMemBytes -= mTextureMemory;
		mTextureMemory = 0;

		deleteTextures(1, &mTexName);
		mCurrentDiscardLevel = -1; // Invalidate mCurrentDiscardLevel.
		mTexName = 0;
		mGLTextureCreated = false;
	}
}

// Force to invalidate the gl texture, most likely a sculpty texture
void LLImageGL::forceToInvalidateGLTexture()
{
	syncTexName();

	if (mTexName)
	{
		destroyGLTexture();
	}
	else
	{
		mCurrentDiscardLevel = -1 ; // Invalidate mCurrentDiscardLevel.
	}
}

void LLImageGL::setAddressMode(LLTexUnit::eTextureAddressMode mode)
{
	syncTexName();

	if (mAddressMode != mode)
	{
		mTexOptionsDirty = true;
		mAddressMode = mode;
	}

	LLTexUnit* unit = gGL.getTexUnit(gGL.getCurrentTexUnitIndex());
	if (unit->getCurrTexture() == mTexName)
	{
		unit->setTextureAddressMode(mode);
		mTexOptionsDirty = false;
	}
}

void LLImageGL::setFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
	syncTexName();

	if (mFilterOption != option)
	{
		mTexOptionsDirty = true;
		mFilterOption = option;
	}

	if (mTexName == 0)
	{
		return;
	}

	LLTexUnit* unit = gGL.getTexUnit(gGL.getCurrentTexUnitIndex());
	if (unit->getCurrTexture() == mTexName)
	{
		unit->setTextureFilteringOption(option);
		mTexOptionsDirty = false;
		stop_glerror();
	}
}

U32 LLImageGL::getHeight(S32 discard_level) const
{
	if (discard_level < 0)
	{
		// NOTE: mCurrentDiscardLevel == -1 *does* happen a lot here, and we do
		// not want to right-shift by -1, which is an undefined behaviour (but
		// usually the shift result is 0 on current CPUs) !  HB
		if (mCurrentDiscardLevel < 0)
		{
			return 1;
		}
		discard_level = mCurrentDiscardLevel;
	}
	if (discard_level > 0)
	{
		return llmax(mHeight >> discard_level, 1);
	}
	return llmax(mHeight, 1);
}

U32 LLImageGL::getWidth(S32 discard_level) const
{
	if (discard_level < 0)
	{
		// NOTE: mCurrentDiscardLevel == -1 *does* happen a lot here, and we do
		// not want to right-shift by -1, which is an undefined behaviour (but
		// usually the shift result is 0 on current CPUs) !  HB
		if (mCurrentDiscardLevel < 0)
		{
			return 1;
		}
		discard_level = mCurrentDiscardLevel;
	}
	if (discard_level > 0)
	{
		return llmax(mWidth >> discard_level, 1);
	}
	return llmax(mWidth, 1);
}

S64 LLImageGL::getBytes(S32 discard_level) const
{
	U32 w = mWidth;
	U32 h = mHeight;
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
		if (discard_level < 0)
		{
			// The result of a right shift by -1 is usually 0 on current CPUs
			w = h = 0;
		}
	}
	// NOTE: we do not want to risk and shift by -1, which is an undefined
	// behaviour, and shifting by 0 is a no-operation.  HB
	if (discard_level > 0)
	{
		w >>= discard_level;
		h >>= discard_level;
	}
	return dataFormatBytes(mFormatPrimary, llmax(w, 1), llmax(h, 1));
}

S64 LLImageGL::getMipBytes(S32 discard_level) const
{
	U32 w = mWidth;
	U32 h = mHeight;
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
		if (discard_level < 0)
		{
			// The result of a right shift by -1 is usually 0 on current CPUs
			w = h = 0;
		}
	}
	// NOTE: we do not want to risk and shift by -1, which is an undefined
	// behaviour, and shifting by 0 is a no-operation.  HB
	if (discard_level > 0)
	{
		w >>= discard_level;
		h >>= discard_level;
	}
	S64 res = dataFormatBytes(mFormatPrimary, w, h);
	if (mUseMipMaps)
	{
		while (w > 1 && h > 1)
		{
			w >>= 1;
			if (w == 0)
			{
				w = 1;
			}
			h >>= 1;
			if (h == 0)
			{
				h = 1;
			}
			res += dataFormatBytes(mFormatPrimary, w, h);
		}
	}
	return res;
}

void LLImageGL::setTarget(U32 target, LLTexUnit::eTextureType bind_target)
{
	mTarget = target;
	mBindTarget = bind_target;
}

void LLImageGL::setNeedsAlphaAndPickMask(bool need_mask)
{
	if (mNeedsAlphaAndPickMask != need_mask)
	{
		mNeedsAlphaAndPickMask = need_mask;

		if (mNeedsAlphaAndPickMask)
		{
			mAlphaOffset = 0;
		}
		else // Do not need alpha mask
		{
			mAlphaOffset = INVALID_OFFSET;
			mIsMask = false;
		}
	}
}

// Helper function
LL_INLINE static bool is_little_endian()
{
	S32 a = 0x12345678;
    U8* c = (U8*)(&a);
	return *c == 0x78;
}

void LLImageGL::calcAlphaChannelOffsetAndStride()
{
	if (mAlphaOffset == INVALID_OFFSET) // Do not need alpha mask
	{
		return;
	}

	mAlphaStride = -1;
	switch (mFormatPrimary)
	{
		case GL_LUMINANCE:
		case GL_ALPHA:
			mAlphaStride = 1;
			break;

		case GL_LUMINANCE_ALPHA:
			mAlphaStride = 2;
			break;

		case GL_RED:
		case GL_RGB:
		case GL_SRGB:
			mNeedsAlphaAndPickMask = mIsMask = false;
#if FIX_MASKS
			mAlphaOffset = INVALID_OFFSET;
#endif
			return; // No alpha channel.

		case GL_RGBA:
		case GL_SRGB_ALPHA:
			mAlphaStride = 4;
			break;

		case GL_BGRA_EXT:
			mAlphaStride = 4;
			break;

		default:
			break;
	}

	mAlphaOffset = -1;
	if (mFormatType == GL_UNSIGNED_BYTE)
	{
		mAlphaOffset = mAlphaStride - 1;
	}
	else if (is_little_endian())
	{
		if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
		{
			mAlphaOffset = 0;
		}
		else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
		{
			mAlphaOffset = 3;
		}
	}
	else // Big endian
	{
		if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
		{
			mAlphaOffset = 3;
		}
		else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
		{
			mAlphaOffset = 0;
		}
	}

	if (mAlphaStride < 1 ||					// Unsupported format
		mAlphaOffset < 0 ||					// Unsupported type
		(mFormatPrimary == GL_BGRA_EXT &&
		 mFormatType != GL_UNSIGNED_BYTE))	// Unknown situation
	{
		llwarns << "Cannot analyze alpha for image with format type "
				<< std::hex << mFormatType << std::dec << llendl;

		mNeedsAlphaAndPickMask = mIsMask = false;
#if FIX_MASKS
		mAlphaOffset = INVALID_OFFSET;
#endif
	}
}

void LLImageGL::analyzeAlpha(const void* data_in, U32 w, U32 h)
{
	if (!mNeedsAlphaAndPickMask || !data_in)
	{
		return;
	}

	U32 length = w * h;
	U32 alphatotal = 0;

	U32 sample[16];
	memset(sample, 0, sizeof(U32) * 16);

	// Generate histogram of quantized alpha. Also add-in the histogram of a
	// 2x2 box-sampled version. The idea is this will mid-skew the data (and
	// thus increase the chances of not being used as a mask) from high-
	// frequency alpha maps which suffer the worst from aliasing when used as
	// alpha masks.
	if (w >= 4 && h >= 4)
	{
		llassert(w % 4 == 0);
		llassert(h % 4 == 0);
		const GLubyte* rowstart = (const GLubyte*)data_in + mAlphaOffset;
		for (U32 y = 0; y < h; y += 4)
		{
			const GLubyte* current = rowstart;
			for (U32 x = 0; x < w; x += 4)
			{
				const U32 s1 = current[0];
				alphatotal += s1;
				const U32 s2 = current[w * mAlphaStride];
				alphatotal += s2;
				current += mAlphaStride;
				const U32 s3 = current[0];
				alphatotal += s3;
				const U32 s4 = current[w * mAlphaStride];
				alphatotal += s4;
				current += mAlphaStride;

				++sample[s1 / 16];
				++sample[s2 / 16];
				++sample[s3 / 16];
				++sample[s4 / 16];

				const U32 asum = (s1 + s2 + s3 + s4);
				alphatotal += asum;
				sample[asum / 64] += 4;
			}


			rowstart += 4 * w * mAlphaStride;
		}
		length *= 2; // we sampled everything twice, essentially
	}
	else
	{
		const GLubyte* current = ((const GLubyte*)data_in) + mAlphaOffset;
		for (U32 i = 0; i < length; ++i)
		{
			const U32 s1 = *current;
			alphatotal += s1;
			++sample[s1 / 16];
			current += mAlphaStride;
		}
	}

	// If more than 1/16th of alpha samples are mid-range, this shouldn't be
	// treated as a 1-bit mask. Also, if all of the alpha samples are clumped
	// on one half of the range (but not at an absolute extreme), then consider
	// this to be an intentional effect and don't treat as a mask.

	U32 midrangetotal = 0;
	for (U32 i = 2; i < 13; ++i)
	{
		midrangetotal += sample[i];
	}
	U32 lowerhalftotal = 0;
	for (U32 i = 0; i < 8; ++i)
	{
		lowerhalftotal += sample[i];
	}
	U32 upperhalftotal = 0;
	for (U32 i = 8; i < 16; ++i)
	{
		upperhalftotal += sample[i];
	}

	if (midrangetotal > length / 48 ||	// Lots of midrange, or
		// All close to transparent but not all totally transparent, or
	    (lowerhalftotal == length && alphatotal != 0) ||
		// All close to opaque but not all totally opaque
	    (upperhalftotal == length && alphatotal != 255 * length))
	{
		mIsMask = false; // Not suitable for masking
	}
	else
	{
		mIsMask = true;
	}
}

U32 LLImageGL::createPickMask(U32 width, U32 height)
{
	freePickMask();

	U32 pick_width = width / 2 + 1;
	U32 pick_height = height / 2 + 1;

	U32 size = pick_width * pick_height;
	size = (size + 7) / 8; // pixelcount-to-bits
	mPickMask = new (std::nothrow) U8[size];
	if (mPickMask)
	{
		mPickMaskWidth = pick_width - 1;
		mPickMaskHeight = pick_height - 1;
		memset(mPickMask, 0, sizeof(U8) * size);
	}
	else
	{
		mPickMaskWidth = 0;
		mPickMaskHeight = 0;
		size = 0;
	}

	return size;
}

void LLImageGL::freePickMask()
{
	if (mPickMask)
	{
		delete[] mPickMask;
		mPickMask = NULL;
	}
	mPickMaskWidth = mPickMaskHeight = 0;
}

void LLImageGL::updatePickMask(U32 width, U32 height, const U8* data_in)
{
	if (!mNeedsAlphaAndPickMask)
	{
		return;
	}

	if (mFormatType != GL_UNSIGNED_BYTE ||
		(mFormatPrimary != GL_RGBA && mFormatPrimary != GL_SRGB_ALPHA))
	{
		// Cannot generate a pick mask for this texture
		freePickMask();
		return;
	}

#if LL_HAS_ASSERT
	const U32 size =
#endif
	createPickMask(width, height);

	U32 pick_bit = 0;
	for (U32 y = 0; y < height; y += 2)
	{
		for (U32 x = 0; x < width; x += 2)
		{
			U8 alpha = data_in[(y * width + x) * 4 + 3];

			if (alpha > 32)
			{
				U32 pick_idx = pick_bit / 8;
				U32 pick_offset = pick_bit % 8;
				llassert(pick_idx < size);

				mPickMask[pick_idx] |= 1 << pick_offset;
			}

			++pick_bit;
		}
	}
}

bool LLImageGL::getMask(const LLVector2& tc)
{
	bool res = true;

	if (mPickMask)
	{
		F32 u, v;
		if (LL_LIKELY(tc.isFinite()))
		{
			u = tc.mV[0] - floorf(tc.mV[0]);
			v = tc.mV[1] - floorf(tc.mV[1]);
		}
		else
		{
			llwarns_sparse << "Non-finite u/v in mask pick !" << llendl;
			u = v = 0.f;
			// removing assert per EXT-4388
			//llassert(false);
		}

		if (LL_UNLIKELY(u < 0.f || u > 1.f || v < 0.f || v > 1.f))
		{
			llwarns_sparse << "u/v out of range in image mask pick !"
						   << llendl;
			u = v = 0.f;
			// removing assert per EXT-4388
			//llassert(false);
		}

		U32 x = llfloor(u * mPickMaskWidth);
		U32 y = llfloor(v * mPickMaskHeight);

		if (LL_UNLIKELY(x > mPickMaskWidth))
		{
			llwarns_sparse << "Width overrun on pick mask read !" << llendl;
			x = llmax(mPickMaskWidth, 0);
		}
		if (LL_UNLIKELY(y > mPickMaskHeight))
		{
			llwarns_sparse << "Height overrun on pick mask read !" << llendl;
			y = llmax(mPickMaskHeight, 0);
		}

		U32 idx = y * mPickMaskWidth + x;
		U32 offset = idx % 8;

		res = (mPickMask[idx / 8] & (1 << offset)) != 0;
	}

	return res;
}

//static
void LLImageGL::initThread(LLWindow* windowp, S32 threads)
{
	if (!threads || sThread)
	{
		return;
	}
	if (threads < 0)	// Automatic setting requested
	{
		// Limit our pool to 32 threads (and thus 32 GL contexts); this matches
		// the limit for the threaded image decoder (see LLImageDecodeThread's
		// construtor code). HB
		threads = llmin(LLCPUInfo::getInstance()->getMaxThreadConcurrency(),
						32);
	}
	sThread = new LLImageGLThread(windowp, threads);
	gImageQueuep = LLWorkQueue::getNamedInstance("LLImageGL");
}

//static
void LLImageGL::stopThread()
{
	LLImageGLThread::sEnabled = false;
	if (sThread)
	{
		gImageQueuep.reset();
		sThread->close();
		// Note: thread instances are kept in memory and cleared at viewer
		// exit, which prevents crashes... HB
		sThread = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLImageGLThread class
///////////////////////////////////////////////////////////////////////////////

// Global variable
LLWorkQueue::weak_t gImageQueuep;

// Used to track the LLImageGLThread instances and destroy them on viewer
// shutdown (i.e. once their child threads have been stopped). HB
typedef std::set<std::unique_ptr<LLImageGLThread> > instances_set_t;
static instances_set_t sThreadInstances;

// Static members
bool LLImageGLThread::sEnabled = false;
// -1 = free VRAM is unknown.
LLAtomicS32	LLImageGLThread::sFreeVRAMMegabytes(-1);

// Must be called from main thread
LLImageGLThread::LLImageGLThread(LLWindow* windowp, U32 threads)
:	LLThreadPool("LLImageGL", threads),
	mWindow(windowp),
	mTheadCounter(0)
{
	assert_main_thread();
	llassert_always(threads > 0);	// At least one thread, please !

	sThreadInstances.emplace(this);

	llinfos << "Initializing with " << threads << " worker threads." << llendl;

	// We must create one GL context per thread, while still in the main thread
	for (U32 i = 0; i < threads; ++i)
	{
		mContexts.push_back(mWindow->createSharedContext());
	}
	LLThreadPool::start(true);	// true = wait until all threads are started.

	// Restore the main thread context
	mWindow->makeContextCurrent(NULL);

	// We can now use the threaded image creation
	sEnabled = true;
}

// Called from child threads in LLThreadPool.
//virtual
void LLImageGLThread::run()
{
	// We must perform setup on this thread before actually servicing our
	// LLWorkQueue, likewise cleanup afterwards.

	// Do not let threads race for GL contexts on mWindow, and protect
	// mTheadCounter from thread concurrency.
	mThreadsMutex.lock();

	if ((size_t)mTheadCounter >= mContexts.size())	// Paranoia
	{
		llerrs << "More threads created than available contexts ("
			   << mContexts.size() << ")" << llendl;
	}
	// Keep these values on stack for use on thread exit (see below)
	void* context = mContexts[mTheadCounter++];
	std::string name = getThreadName(LLThread::thisThreadIdHash());
	if (!context)
	{
		llwarns << "No available GL context for thread " << name
				<< ". Aborting this thread !" << llendl;
		mThreadsMutex.unlock();
		doIncStartedThreads();	// Account as started nonetheless.
		return;
	}
	llinfos << "Initializing GL for thread " << name << " with context: "
			<< std::hex << (intptr_t)context << std::dec << llendl;
	// Set the context on the viewer window and start GL for our thread (gGL is
	// thread_local).
	mWindow->makeContextCurrent(context);
	gGL.init();

	mThreadsMutex.unlock();

	// It is now safe to consider that this thread has indeed fully started.
	doIncStartedThreads();

	// Run the queue servicing, until the queue is closed.
	LLThreadPool::run();

	llinfos << "Shutting down GL for thread " << name << " with GL context: "
			<< std::hex << (intptr_t)context << std::dec << llendl;
	gGL.shutdown();
	// Do not do this under Windows: it takes forever to complete, when it does
	// not right out dead-locks !  HB
#if !LL_WINDOWS
	mWindow->destroySharedContext(context);
#endif
}

// Must be called from main thread
//static
void LLImageGLThread::cleanup()
{
	assert_main_thread();
	size_t count = sThreadInstances.size();
	// Since this is a set of std::unique_ptr's, clearing it will cause all
	// stored instances to get automatically destroyed.
	sThreadInstances.clear();
	llinfos << "Number of destroyed instances: " << count << llendl;
}

// Called from main thread
//static
void LLImageGLThread::updateFreeVRAM()
{
	LL_TRACY_TIMER(TRC_IMAGEGLTHREAD_UPDATE_FREE_VRAM);
	// Post update to background thread if available, otherwise execute
	// immediately.
	if (sEnabled)
	{
		auto queue = gImageQueuep.lock();
		if (queue)
		{
			queue->post([]() { readFreeVRAM(); });
			return;
		}
	}
	readFreeVRAM();
}

// Called from main or child threads
//static
void LLImageGLThread::readFreeVRAM()
{
	LL_TRACY_TIMER(TRC_IMAGEGLTHREAD_READ_VRAM);

	if (LLGLManager::sHasATIMemInfo)
	{
		// Reference:
		// https://registry.khronos.org/OpenGL/extensions/ATI/ATI_meminfo.txt
		GLint meminfo[4] = { -1, 0, 0, 0 };
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, meminfo);
		if (meminfo[0] >= 0)	// If call succeeded. HB
		{
			sFreeVRAMMegabytes = meminfo[0] / 1024; // Convert to MB !  HB
			LL_DEBUGS("ImageGLThread") << "Free VRAM: " << sFreeVRAMMegabytes
									   << "MB" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS_ONCE("ImageGLThread") << "GL_TEXTURE_FREE_MEMORY_ATI failed."
											<< LL_ENDL;
		}
		stop_glerror();			// Clear any error resulting from this call. HB
	}
	else if (LLGLManager::sHasNVXMemInfo)
	{
		GLint free_memory_kb = -1;
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX,
					  &free_memory_kb);
		if (free_memory_kb >= 0)	// If call succeeded. HB
		{
			sFreeVRAMMegabytes = free_memory_kb / 1024; // Convert to MB
			LL_DEBUGS("ImageGLThread") << "Free VRAM: " << sFreeVRAMMegabytes
									   << "MB" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS_ONCE("ImageGLThread") << "GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX failed."
											<< LL_ENDL;
		}
		stop_glerror();			// Clear any error resulting from this call. HB
	}
}
