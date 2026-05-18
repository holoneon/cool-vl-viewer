/**
 * @file llviewertexlayer.h
 * @brief Viewer texture layer classes. Used for avatars.
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

#include "llavatarappearance.h"
#include "llextendedstatus.h"
#include "lltexlayer.h"
#include "lluuid.h"

#include "lldynamictexture.h"

class LLVOAvatarSelf;
class LLViewerTexLayerSetBuffer;
struct LLBakedUploadData;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLViewerTexLayerSet
//
// An ordered set of texture layers that gets composited into a single texture.
// Only exists for llavatarappearanceself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLViewerTexLayerSet final : public LLTexLayerSet
{
public:
	LLViewerTexLayerSet(LLAvatarAppearance* const appearance);

	LL_INLINE LLViewerTexLayerSet* asViewerTexLayerSet() override
	{
		return this;
	}

	void requestUpdate() override;

	void requestUpload();
	void cancelUpload();

	bool isLocalTextureDataAvailable();
	bool isLocalTextureDataFinal();

	void updateComposite();
	void createComposite() override;

	LL_INLINE void setUpdatesEnabled(bool b) 			{ mUpdatesEnabled = b; }
	LL_INLINE bool getUpdatesEnabled() const 			{ return mUpdatesEnabled; }

	LLVOAvatarSelf* getAvatar();
	const LLVOAvatarSelf* getAvatar() const;

	LLViewerTexLayerSetBuffer* getViewerComposite();

private:
	bool mUpdatesEnabled;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLViewerTexLayerSetBuffer
//
// The composite image that a LLViewerTexLayerSetBuffer writes to. Each
// LLViewerTexLayerSetBuffer has one.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLViewerTexLayerSetBuffer final : public LLTexLayerSetBuffer,
										public LLViewerDynamicTexture
{
protected:
	LOG_CLASS(LLViewerTexLayerSetBuffer);

public:
	LLViewerTexLayerSetBuffer(LLTexLayerSet* const ownerp,
							  S32 width, S32 height);
	~LLViewerTexLayerSetBuffer() override;

	LL_INLINE LLViewerTexLayerSetBuffer* asViewerTexLayerSetBuffer() override
	{
		return this;
	}

	S8 getType() const override;
	bool isInitialized() const;
	static void dumpTotalByteCount();

	LL_INLINE LLViewerTexLayerSet* getViewerTexLayerSet()
	{
		return mTexLayerSet ? mTexLayerSet->asViewerTexLayerSet() : NULL;
	}

	static void uploadBakedTextureCoro(const std::string& url, LLUUID vfile_id,
									   LLBakedUploadData* data);

	//--------------------------------------------------------------------
	// Dynamic Texture Interface
	//--------------------------------------------------------------------
	bool needsRender() override;

	//--------------------------------------------------------------------
	// Tex Layer Render
	//--------------------------------------------------------------------
private:
	void preRenderTexLayerSet() override;
	void midRenderTexLayerSet(bool success) override;
	void postRenderTexLayerSet(bool success) override;
	LL_INLINE S32 getCompositeOriginX() const override	{ return getOriginX(); }
	LL_INLINE S32 getCompositeOriginY() const override	{ return getOriginY(); }
	LL_INLINE S32 getCompositeWidth() const override	{ return getFullWidth(); }
	LL_INLINE S32 getCompositeHeight() const override	{ return getFullHeight(); }

protected:
	// Pass these along for tex layer rendering.
	LL_INLINE void preRender(bool) override				{ preRenderTexLayerSet(); }
	LL_INLINE void postRender(bool success) override	{ postRenderTexLayerSet(success); }
	LL_INLINE bool render() override					{ return renderTexLayerSet(); }

	//--------------------------------------------------------------------
	// Uploads
	//--------------------------------------------------------------------
public:
	void requestUpload();
	void cancelUpload();
	// We need to upload a new texture:
	LL_INLINE bool uploadNeeded() const					{ return mNeedsUpload; }
 	// We have started uploading a new texture and are awaiting the result
	LL_INLINE bool uploadInProgress() const				{ return mUploadID.notNull(); }
	// We are expecting a new texture to be uploaded at some point
	LL_INLINE bool uploadPending() const				{ return mUploadPending; }

	static void onTextureUploadComplete(const LLUUID& uuid, void* userdata,
										S32 result,
										LLExtStat s = LLExtStat::NONE);

protected:
	bool isReadyToUpload();
	void doUpload(); 				// Does a read back and upload.
	void conditionalRestartUploadTimer();

	//--------------------------------------------------------------------
	// Updates
	//--------------------------------------------------------------------
public:
	void requestUpdate();
	bool requestUpdateImmediate();

protected:
	bool isReadyToUpdate();
	void doUpdate();
	void restartUpdateTimer();

private:
	// The current upload process (null if none).
	LLUUID			mUploadID;

 	// Tracks time since upload was requested and performed.
	LLFrameTimer    mNeedsUploadTimer;

 	// Tracks time since last upload failure.
	LLFrameTimer	mUploadRetryTimer;

	// Tracks time since update was requested and performed.
	LLFrameTimer    mNeedsUpdateTimer;

	// Number of times we have locally updated with lowres version of our baked
	// textures
	U32				mNumLowresUpdates;

 	// Number of times we have sent a lowres version of our baked textures to
	// the server
	U32				mNumLowresUploads;

	// Number of consecutive upload failures
	S32				mUploadFailCount;

 	// Whether we have received back the new baked textures
	bool			mUploadPending;

	// Whether we need to send our baked textures to the server
	bool			mNeedsUpload;

	// Whether we need to locally update our baked textures
	bool			mNeedsUpdate;

	static S32		sGLByteCount;
};
