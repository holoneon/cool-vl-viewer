/**
 * @file lllocaltextureobject.h
 * @brief LLLocalTextureObject class header file
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llgltexture.h"
#include "lluuid.h"

class LLTexLayer;
class LLTexLayerTemplate;
class LLWearable;

// Stores all relevant information for a single texture assumed to have
// ownership of all objects referred to - will delete objects when being
// replaced or if object is destroyed.
class LLLocalTextureObject
{
public:
	LLLocalTextureObject();
	LLLocalTextureObject(LLGLTexture* texp, const LLUUID& id);
	LLLocalTextureObject(const LLLocalTextureObject& lto);
	~LLLocalTextureObject();

	LL_INLINE const LLUUID& getID() const		{ return mID; }
	LL_INLINE void setID(const LLUUID& id)		{ mID = id; }

	LL_INLINE LLGLTexture* getImage() const		{ return mImage.get(); }
	void setImage(LLGLTexture* texp);

	LL_INLINE S32 getDiscard() const			{ return mDiscard; }
	LL_INLINE void setDiscard(S32 discard)		{ mDiscard = discard; }

	LL_INLINE bool getBakedReady() const		{ return mIsBakedReady; }
	LL_INLINE void setBakedReady(bool ready)	{ mIsBakedReady = ready; }

	LLTexLayer* getTexLayerByIdx(U32 index) const;
	LLTexLayer* getTexLayer(const std::string& name) const;
	bool setTexLayer(LLTexLayer* layerp, U32 index);
	bool addTexLayer(LLTexLayer* layerp, LLWearable* wearablep);

	bool addTexLayer(LLTexLayerTemplate* layerp, LLWearable* wearablep);
	bool removeTexLayer(U32 index);

	LL_INLINE U32 getNumTexLayers() const		{ return mTexLayers.size(); }

private:
	LLPointer<LLGLTexture>	mImage;
	LLUUID					mID;

	// NOTE: LLLocalTextureObject should be the exclusive owner of mTexEntry
	// and mTexLayer; using shared pointers here only for smart assignment &
	// cleanup. Do NOT create new shared pointers to these objects, or keep
	// pointers to them around
	typedef std::vector<LLTexLayer*> tex_layer_vec_t;
	tex_layer_vec_t			mTexLayers;

	S32						mDiscard;
	bool					mIsBakedReady;

public:
	// *HACK: in OpenSim, we need to make sure textures used for viewer-side
	// baking do not get deleted before the bake happens. It means we cannot
	// remove those textures when not rendered. This flag is set to true when
	// logged on an OpenSim grid for this purpose... HB
	static bool				sMarkNoDelete;
};
