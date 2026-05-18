/**
 * @file llavatarjointmesh.h
 * @brief Declaration of LLAvatarJointMesh class
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

#pragma once

#include "llavatarjoint.h"
#include "llgltexture.h"
#include "llpolymesh.h"
#include "llpreprocessor.h"
#include "llcolor4.h"

class LLCharacter;
class LLDrawable;
class LLFace;
class LLTexLayerSet;

typedef enum e_avatar_render_pass
{
	AVATAR_RENDER_PASS_SINGLE,
	AVATAR_RENDER_PASS_CLOTHING_INNER,
	AVATAR_RENDER_PASS_CLOTHING_OUTER
} EAvatarRenderPass;

//-----------------------------------------------------------------------------
// class LLSkinJoint
//-----------------------------------------------------------------------------

class LLSkinJoint
{
protected:
	LOG_CLASS(LLSkinJoint);

public:
	LLSkinJoint();
	~LLSkinJoint();
	bool setupSkinJoint(LLAvatarJoint* joint);

	static LLAvatarJoint* getBaseSkeletonAncestor(LLAvatarJoint* joint);

private:
	static LLVector3 totalSkinOffset(LLAvatarJoint* joint);

public:
	LLAvatarJoint*	mJoint;
	LLVector3		mRootToJointSkinOffset;
	LLVector3		mRootToParentJointSkinOffset;
};

//-----------------------------------------------------------------------------
// class LLAvatarJointMesh
//-----------------------------------------------------------------------------

class LLAvatarJointMesh : public virtual LLAvatarJoint
{
public:
	LLAvatarJointMesh();
	virtual ~LLAvatarJointMesh();

	// Gets the shape color
	void getColor(F32* red, F32* green, F32* blue, F32* alpha);

	// Sets the shape color
	void setColor(F32 red, F32 green, F32 blue, F32 alpha);
	void setColor(const LLColor4& color);

	// Sets the shininess
	LL_INLINE void setSpecular(const LLColor4& color, F32 shiny)
	{
#if 0
		mSpecular = color;
#endif
		mShiny = shiny;
	}

	// Sets the shape texture
	void setTexture(LLGLTexture* texture);

	bool hasGLTexture() const;

	LL_INLINE void setTestTexture(U32 name)				{ mTestImageName = name; }

	// Sets layer set responsible for a dynamic shape texture (takes precedence
	// over normal texture)
	void setLayerSet(LLTexLayerSet* layer_set);

	bool hasComposite() const;

	// Gets the poly mesh
	LL_INLINE LLPolyMesh* getMesh()						{ return mMesh; }

	// Sets the poly mesh
	void setMesh(LLPolyMesh* mesh);

	LL_INLINE LLFace* getFace()							{ return mFace; }

	// Sets up joint matrix data for rendering
	void setupJoint(LLAvatarJoint* current_joint);

	// Render time method to upload batches of joint matrices
	void uploadJointMatrices();

	// Sets ID for picking
	LL_INLINE void setMeshID(S32 id)					{ mMeshID = id; }

	// Gets ID for picking
	LL_INLINE S32 getMeshID()							{ return mMeshID; }

	LL_INLINE void setIsTransparent(bool b)				{ mIsTransparent = b; }

private:
	// Allocate skin data
	bool allocateSkinData(U32 numSkinJoints);

	// Free skin data
	void freeSkinData();

protected:
	LLPointer<LLGLTexture>		mTexture;		// ptr to a global texture
	LLTexLayerSet*				mLayerSet;		// ptr to a layer set owned by the avatar
	LLFace*						mFace;			// ptr to a face w/ AGP copy of mesh
	LLSkinJoint*				mSkinJoints;
	LLPolyMesh*					mMesh;			// ptr to a global polymesh
	LLColor4					mColor;			// color value
	F32							mShiny;			// shiny value
	S32							mMeshID;
	U32 						mTestImageName;	// handle to a temporary texture for previewing uploads
	U32							mFaceIndexCount;
	U32							mNumSkinJoints;

#if 0	// Not used
 	LLColor4					mSpecular;		// specular color (always white for now)
	bool						mCullBackFaces;	// true by default
#endif

public:
	// RN: this is here for testing purposes
	static U32					sClothingMaskImageName;
	static LLColor4				sClothingInnerColor;
};
