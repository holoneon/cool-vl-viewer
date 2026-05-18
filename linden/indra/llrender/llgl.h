/**
 * @file llgl.h
 * @brief LLGL definition
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

// This file contains various stuff for handling gl extensions and other GL
// related stuff.

#include <list>

#include "llcolor4.h"
#include "hbfastmap.h"
#include "llglheaders.h"
#include "llinstancetracker.h"
#include "llmatrix4a.h"
#include "llplane.h"
#include "llstring.h"

extern bool gDebugGL;
// Global flag for dual-renderer support (EE/WL and PBR). HB
extern bool gUsePBRShaders;

class LLSD;
class LLMatrix4;

// Manages GL extensions. Purely static class. Note: it is essential that this
// class is *static* (not global, not a singleton), since some of its member
// variables are tested very late during the viewer shutdown (e.g. when the
// global gPipeline instance and its member variables are destroyed), and
// outside of the programmer's control scope. HB
class LLGLManager
{
	LLGLManager() = delete;
	~LLGLManager() = delete;

protected:
	LOG_CLASS(LLGLManager);

public:
	static bool initGL();
	static void shutdownGL();

#if LL_WINDOWS
	// Initializes stupid WGL extensions
	static void initWGL(HDC dc);
#endif

	// For sending to simulator
	static std::string getRawGLString();

	// Gets the best pixel format
	static void getPixelFormat();

	static void printGLInfoString();
	static void getGLInfo(LLSD& info);

	static void asLLSD(LLSD& info);

	// Use this instead of glDeleteBuffers() in order to pool deletions.
	static void deleteBuffers(U32 count, U32* bufferp);

private:
	static void initExtensions();
	static void initGLStates();
	static void initGLImages();

public:
	static std::string	sDriverVersionVendorString;
	static std::string	sGLVersionString;

	// In ALL CAPS
	static std::string	sGLVendor;
	static std::string	sGLVendorShort;
	static std::string	sGLRenderer;

	static S32			sDriverVersionMajor;
	static S32			sDriverVersionMinor;
	static S32			sDriverVersionRelease;
	static F32			sGLVersion;				// E.g = 3.2
	static S32			sGLSLVersionMajor;
	static S32			sGLSLVersionMinor;

	// Total VRAM in MB
	static S32			sVRAM;
	// VRAM reserved for textures in MB
	static S32			sTexVRAM;
	static S32			sGLMaxVertexRange;
	static S32			sGLMaxIndexRange;
	static S32			sGLMaxTextureSize;

	// Limits
	static S32			sMaxSamples;
	static S32			sNumTextureImageUnits;
	static S32			sMaxUniformBlockSize;
	static S32			sMaxVaryingVectors;
	static F32			sMaxAnisotropy;

	static bool			sInited;
	static bool			sIsDisabled;

	// GPU vendor flags
	static bool			sIsAMD;
	static bool			sIsNVIDIA;
	static bool			sIsIntel;

	// Whether this version of GL is good enough for SL to use
	static bool		sHasRequirements;

	// Extensions
	static bool			sHasNVXMemInfo;
	static bool			sHasATIMemInfo;
#if LL_WINDOWS
	static bool			sHasAMDAssociations;
#endif
	static bool			sHasSync;
	static bool			sHasVertexArrayObject;
	static bool			sHasOcclusionQuery2;
	static bool			sHasTimerQuery;
	static bool			sHasDepthClamp;
	static bool			sUseDepthClamp;
	static bool			sHasAnisotropic;
	static bool			sHasCubeMapArray;
	static bool			sHasDebugOutput;
	static bool			sHasTextureSwizzle;
	static bool			sHasVertexAttribIPointer;
	static bool			sHasGpuShader4;
	static bool			sHasGpuShader5;
};

LL_NO_INLINE void log_glerror(const char* file, U32 line, bool crash = false);

#if LL_DEBUG
# define stop_glerror() if (LL_UNLIKELY(gDebugGL)) log_glerror(__FILE__, __LINE__, true)
#else
# define stop_glerror() if (LL_UNLIKELY(gDebugGL)) log_glerror(__FILE__, __LINE__)
#endif

void clear_glerror();

// This is a class for GL state management
//
// GL STATE MANAGEMENT DESCRIPTION
//
// LLGLState and its two subclasses, LLGLEnable and LLGLDisable, manage the
// current enable/disable states of the GL to prevent redundant setting of
// state within a render path or the accidental corruption of what state the
// next path expects.
//
// Essentially, wherever you would call glEnable set a state and then
// subsequently reset it by calling glDisable (or vice versa), make an
// instance of LLGLEnable with the state you want to set, and assume it will
// be restored to its original state when that instance of LLGLEnable is
// destroyed. It is good practice to exploit stack frame controls for optimal
// setting/unsetting and readability of code. There are a collection of helper
// classes below that define groups of enables/disables and that can cause
// multiple states to be set with the creation of one instance.
//
// Sample usage (disable lighting for rendering hud objects):
//
// INCORRECT USAGE:
//	LLGLEnable blend(GL_BLEND);
//	renderHUD();
//	LLGLDisable blend(GL_BLEND);
//
// CORRECT USAGE:
// 	{
//		LLGLEnable blend(GL_BLEND);
//		renderHUD();
//	}
//
// If a state is to be set on a conditional, the following mechanism is useful:
//	{
//		LLGLEnable blend(blend_hud ? GL_BLEND : 0);
//		renderHUD();
//	}
//
// A LLGLState initialized with a parameter of 0 does nothing.
//
// LLGLState works by maintaining a map of the current GL states, and ignoring
// redundant enables/disables. If a redundant call is attempted, it becomes a
// noop, otherwise, it is set in the constructor and reset in the destructor.
//
// For debugging GL state corruption, running with debug enabled will trigger
// asserts if the existing GL state does not match the expected GL state.

class LLGLState
{
protected:
	LOG_CLASS(LLGLState);

public:
	static void initClass();
	static void restoreGL();

	static void dumpStates();

	static void checkStates(const std::string& msg = LLStringUtil::null,
							S32 line = -1);

#if 0	// Not used, but kept in source, just in case... HB
	// Really should not be needed.
	static void resetTextureStates();
#endif

protected:
	typedef fast_hmap<U32, GLboolean> state_map_t;
	static state_map_t sStateMap;

public:
	enum { CURRENT_STATE = -2 };
	LLGLState(U32 state, S32 enabled = CURRENT_STATE);
	virtual ~LLGLState();

	void setEnabled(S32 enabled);
	LL_INLINE void enable()						{ setEnabled(GL_TRUE); }
	LL_INLINE void disable()					{ setEnabled(GL_FALSE); }

protected:
	U32			mState;
	S32			mWasEnabled;
	S32			mIsEnabled;
};

// Helper define, to log the filename and line number of the checkStates()
// call, whenever an error is detected. Unless you got a specific message to
// pass on error, use this macro instead of LLGLState::checkStates(). HB
#define LL_GL_CHECK_STATES	LLGLState::checkStates(__FILE__, __LINE__)

class LLGLEnable : public LLGLState
{
public:
	LL_INLINE LLGLEnable(U32 state)
	:	LLGLState(state, GL_TRUE)
	{
	}
};

class LLGLDisable : public LLGLState
{
public:
	LL_INLINE LLGLDisable(U32 state)
	:	LLGLState(state, GL_FALSE)
	{
	}
};


///////////////////////////////////////////////////////////////////////////////
// Stores and modifis projection matrix to create an oblique projection that
// clips to the specified plane. Oblique projections alter values in the depth
// buffer, so this class should not be used mid-renderpass.
// Restores projection matrix on destruction. Does not stack.
// GL_MODELVIEW_MATRIX is active whenever program execution leaves this class.

class alignas(16) LLGLUserClipPlane
{
public:
	LLGLUserClipPlane(const LLPlane& plane, const LLMatrix4a& modelview,
					  const LLMatrix4a& projection, bool apply = true);
	~LLGLUserClipPlane();

	void disable();
	void setPlane(F32 a, F32 b, F32 c, F32 d);

private:
	LLMatrix4a	mProjection;
	LLMatrix4a	mModelview;

	bool		mApply;
};

///////////////////////////////////////////////////////////////////////////////
// Modifies and loads projection matrix to push depth values to far clip plane.
// Restores projection matrix on destruction. Saves/restores matrix mode around
// projection manipulation. Does not stack.

class LLGLSquashToFarClip
{
public:
	LLGLSquashToFarClip(U32 layer = 0);
	~LLGLSquashToFarClip();
};

///////////////////////////////////////////////////////////////////////////////
// Interface for objects that need periodic GL updates applied to them.
// Used to synchronize GL updates with GL thread.

class LLGLUpdate
{
public:
	LLGLUpdate()
	:	mInQ(false)
	{
	}

	virtual ~LLGLUpdate()
	{
		if (mInQ)
		{
			std::list<LLGLUpdate*>::iterator end = sGLQ.end();
			std::list<LLGLUpdate*>::iterator iter = std::find(sGLQ.begin(),
															  end,
															  this);
			if (iter != end)
			{
				sGLQ.erase(iter);
			}
		}
	}

	virtual void updateGL() = 0;

public:
	static std::list<LLGLUpdate*> sGLQ;

	bool mInQ;
};

///////////////////////////////////////////////////////////////////////////////
// Formerly in llglstates.h, but since that header was #included by this one...
///////////////////////////////////////////////////////////////////////////////

class LLGLDepthTest
{
protected:
	LOG_CLASS(LLGLDepthTest);

public:
	// Enabled by default
	// Note: the new 'ignored' allows (when 'true' to make this class a no-
	// operation, so to simplify the dual renderer code. HB
	LLGLDepthTest(GLboolean depth_enabled, GLboolean write_enabled = GL_TRUE,
				  U32 depth_func = GL_LEQUAL, bool ignored = false);

	~LLGLDepthTest();

	void checkState();

public:
	U32					mPrevDepthFunc;
	GLboolean			mPrevDepthEnabled;
	GLboolean			mPrevWriteEnabled;

private:
	bool				mIgnored;

	static U32			sDepthFunc;		// defaults to GL_LESS
	static GLboolean	sDepthEnabled;	// defaults to GL_FALSE
	static GLboolean	sWriteEnabled;	// defaults to GL_TRUE
};

class LLGLSDefault
{
public:
	LLGLSDefault()
	:	// Disable
		mBlend(GL_BLEND),
		mCullFace(GL_CULL_FACE)
	{
	}

protected:
	LLGLDisable	mBlend;
	LLGLDisable	mCullFace;
};

class LLGLSObjectSelect
{
public:
	LLGLSObjectSelect()
	:	mBlend(GL_BLEND),
		mCullFace(GL_CULL_FACE)
	{
	}

protected:
	LLGLDisable	mBlend;
	LLGLEnable	mCullFace;
};

class LLGLSUIDefault
{
public:
	LLGLSUIDefault()
	:	mBlend(GL_BLEND),
		mCullFace(GL_CULL_FACE),
		mDepthTest(GL_FALSE, GL_TRUE, GL_LEQUAL),
		mMSAA(GL_MULTISAMPLE)
	{
	}

protected:
	LLGLEnable		mBlend;
	LLGLDisable		mCullFace;
	LLGLDepthTest	mDepthTest;
	LLGLDisable		mMSAA;
};

class LLGLSPipeline
{
public:
	LLGLSPipeline()
	:	mCullFace(GL_CULL_FACE),
		mDepthTest(GL_TRUE, GL_TRUE, GL_LEQUAL)
	{
	}

protected:
	LLGLEnable		mCullFace;
	LLGLDepthTest	mDepthTest;
};

class LLGLSPipelineAlpha // : public LLGLSPipeline
{
public:
	LLGLSPipelineAlpha()
	:	mBlend(GL_BLEND)
	{
	}

protected:
	LLGLEnable mBlend;
};

class LLGLSPipelineSelection
{
public:
	LLGLSPipelineSelection()
	:	mCullFace(GL_CULL_FACE)
	{
	}

protected:
	LLGLDisable mCullFace;
};

class LLGLSPipelineSkyBox
{
public:
	LLGLSPipelineSkyBox();
	virtual ~LLGLSPipelineSkyBox() = default;

protected:
	LLGLDisable			mCullFace;
	LLGLSquashToFarClip	mSquashClip;
};

class LLGLSPipelineDepthTestSkyBox : public LLGLSPipelineSkyBox
{
public:
	LLGLSPipelineDepthTestSkyBox(GLboolean depth_test, GLboolean depth_write);

public:
	LLGLDepthTest mDepth;
};

class LLGLSPipelineBlendSkyBox : public LLGLSPipelineDepthTestSkyBox
{
public:
	LLGLSPipelineBlendSkyBox(GLboolean depth_test, GLboolean depth_write);

public:
	LLGLEnable mBlend;
};

class LLGLSTracker
{
public:
	LLGLSTracker()
	:	mCullFace(GL_CULL_FACE),
		mBlend(GL_BLEND)
	{
	}

protected:
	LLGLEnable mCullFace;
	LLGLEnable mBlend;
};

class LLGLSSpecular
{
public:
	LLGLSSpecular(const LLColor4& color, F32 shininess)
	{
		mShininess = shininess;
		if (mShininess > 0.f)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color.mV);
			S32 shiny = (S32)(shininess * 128.f);
			shiny = llclamp(shiny, 0, 128);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shiny);
		}
	}

	~LLGLSSpecular()
	{
		if (mShininess > 0.f)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
						 LLColor4::transparent.mV);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 0);
		}
	}

public:
	F32 mShininess;
};

///////////////////////////////////////////////////////////////////////////////

const std::string getGLErrorString(U32 error);
