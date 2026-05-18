/**
 * @file llgl.cpp
 * @brief LLGL implementation
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

// This file sets some global GL parameters, and implements some
// useful functions for GL operations.

#define GLH_EXT_SINGLE_FILE

#include "linden_common.h"

#include "boost/tokenizer.hpp"

#include "llgl.h"

#include "llglslshader.h"
#include "llimagegl.h"
#include "llmath.h"
#include "llquaternion.h"
#include "llrender.h"
#include "llsys.h"
#include "llmatrix4.h"

#if LL_WINDOWS
# include "lldxhardware.h"
#endif

bool gDebugGL = false;
// Global flag for dual-renderer support (EE/WL and PBR). HB
bool gUsePBRShaders = false;

std::list<LLGLUpdate*> LLGLUpdate::sGLQ;

// Utility functions

void log_glerror(const char* file, U32 line, bool crash)
{
	// Do not call glGetError() while GL is stopped or not yet initialized. HB
	if (!gGL.isValid())
	{
		return;
	}

	static std::string filename;
	GLenum error = glGetError();
	if (LL_UNLIKELY(error))
	{
		filename.assign(file);
		size_t i = filename.find("indra");
		if (i != std::string::npos)
		{
			filename = filename.substr(i);
		}
	}
	while (LL_UNLIKELY(error))
	{
		std::string gl_error_msg = getGLErrorString(error);
		if (crash)
		{
			llerrs << "GL Error: " << gl_error_msg << " (" << error
				   << ") - in file: " << filename << " - at line: "
				   << line << llendl;
		}
		else
		{
			llwarns << "GL Error: " << gl_error_msg << " (" << error
					<< ") - in file: " << filename << " - at line: "
					<< line << llendl;
		}
		error = glGetError();
	}
}

// There are 7 non-zero error flags, one of them being cleared on each call to
// glGetError(). Normally, all error flags should therefore get cleared after
// at most 7 calls to glGetError() and the 8th call should always return 0...
// See: http://www.opengl.org/sdk/docs/man/xhtml/glGetError.xml
#define MAX_LOOPS 8U
void clear_glerror()
{
	// Do not call glGetError() while GL is stopped or not yet initialized. HB
	if (!gGL.isValid())
	{
		return;
	}

	U32 counter = MAX_LOOPS;
	if (LL_UNLIKELY(gDebugGL))
	{
		GLenum error;
		while ((error = glGetError()))
		{
			if (--counter == 0)
			{
				llwarns << "glGetError() still returning errors ("
						<< getGLErrorString(error) <<") after "
						<< MAX_LOOPS << " consecutive calls." << llendl;
				break;
			}
			else
			{
				llwarns << "glGetError() returned error: "
						<< getGLErrorString(error) << llendl;
			}
		}
	}
	else
	{
		// Fast code, for when gDebugGL is false
		while (glGetError() && --counter != 0) ;
	}
}

const std::string getGLErrorString(U32 error)
{
	switch (error)
	{
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
			break;

		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
			break;

		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
			break;

		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
			break;

		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
			break;

		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
			break;

		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
			break;

		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
			break;

		default:
			return llformat("Unknown GL error #%d", error);
	}
}

static std::string parse_gl_version(S32& major, S32& minor, S32& release,
									std::string& vendor_specific)
{
	major = minor = release = 0;
	std::string version_string;

	// GL_VERSION returns a nul-terminated string with the format:
	// <major>.<minor>[.<release>] [<vendor specific>]
	const char* version = (const char*)glGetString(GL_VERSION);
	if (!version || !*version)
	{
		vendor_specific.clear();
		return version_string;
	}
	version_string.assign(version);

	std::string ver_copy(version);
	size_t len = strlen(version);
	size_t i = 0;
	size_t start;
	// Find the major version
	start = i;
	for ( ; i < len; ++i)
	{
		if (version[i] == '.')
		{
			break;
		}
	}
	std::string major_str = ver_copy.substr(start, i - start);
	LLStringUtil::convertToS32(major_str, major);

	if (version[i] == '.')
	{
		++i;
	}

	// Find the minor version
	start = i;
	for ( ; i < len; ++i)
	{
		if (version[i] == '.' || isspace(version[i]))
		{
			break;
		}
	}
	std::string minor_str = ver_copy.substr(start, i - start);
	LLStringUtil::convertToS32(minor_str, minor);

	// Find the release number (optional)
	if (version[i] == '.')
	{
		++i;

		start = i;
		for ( ; i < len; ++i)
		{
			if (isspace(version[i]))
			{
				break;
			}
		}

		std::string release_str = ver_copy.substr(start, i - start);
		LLStringUtil::convertToS32(release_str, release);
	}

	// Skip over any white space
	while (version[i] && isspace(version[i]))
	{
		++i;
	}

	// Copy the vendor-specific string (optional)
	if (version[i])
	{
		vendor_specific.assign(version + i);
	}

	return version_string;
}

static void parse_glsl_version(S32& major, S32& minor)
{
	major = minor = 0;

	// GL_SHADING_LANGUAGE_VERSION returns a nul-terminated string with the
	// format: <major>.<minor>[.<release>] [<vendor specific>]
	const char* version =
		(const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	if (!version)
	{
		return;
	}

	std::string ver_copy(version);
	size_t len = strlen(version);
	size_t i = 0;
	size_t start;
	// Find the major version
	start = i;
	for ( ; i < len; ++i)
	{
		if (version[i] == '.')
		{
			break;
		}
	}
	std::string major_str = ver_copy.substr(start, i - start);
	LLStringUtil::convertToS32(major_str, major);

	if (version[i] == '.')
	{
		i++;
	}

	// Find the minor version
	start = i;
	for ( ; i < len; ++i)
	{
		if (version[i] == '.' || isspace(version[i]))
		{
			break;
		}
	}
	std::string minor_str = ver_copy.substr(start, i - start);
	LLStringUtil::convertToS32(minor_str, minor);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLManager class
///////////////////////////////////////////////////////////////////////////////

// Static variable members
std::string LLGLManager::sDriverVersionVendorString;
std::string LLGLManager::sGLVersionString;
std::string LLGLManager::sGLVendor;
std::string LLGLManager::sGLVendorShort;
std::string LLGLManager::sGLRenderer;
S32 LLGLManager::sDriverVersionMajor = 1;
S32 LLGLManager::sDriverVersionMinor = 0;
S32 LLGLManager::sDriverVersionRelease = 0;
F32 LLGLManager::sGLVersion = 1.f;
S32 LLGLManager::sGLSLVersionMajor = 0;
S32 LLGLManager::sGLSLVersionMinor = 0;
S32 LLGLManager::sVRAM = 0;
S32 LLGLManager::sTexVRAM = 0;
S32 LLGLManager::sGLMaxVertexRange = 0;
S32 LLGLManager::sGLMaxIndexRange = 0;
S32 LLGLManager::sGLMaxTextureSize = 256;
S32 LLGLManager::sMaxSamples = 0;
S32 LLGLManager::sNumTextureImageUnits = 1;
S32 LLGLManager::sMaxUniformBlockSize = 16384;
S32 LLGLManager::sMaxVaryingVectors = 0;
F32 LLGLManager::sMaxAnisotropy = 1.f;
bool LLGLManager::sInited = false;
bool LLGLManager::sIsDisabled = false;
bool LLGLManager::sIsAMD = false;
bool LLGLManager::sIsNVIDIA = false;
bool LLGLManager::sIsIntel = false;
bool LLGLManager::sHasRequirements = true;
bool LLGLManager::sHasNVXMemInfo = false;
bool LLGLManager::sHasATIMemInfo = false;
#if LL_WINDOWS
bool LLGLManager::sHasAMDAssociations = false;
#endif
bool LLGLManager::sHasSync = false;
bool LLGLManager::sHasVertexArrayObject = false;
bool LLGLManager::sHasOcclusionQuery2 = false;
bool LLGLManager::sHasTimerQuery = false;
bool LLGLManager::sHasDepthClamp = false;
bool LLGLManager::sUseDepthClamp = false;
bool LLGLManager::sHasAnisotropic = false;
bool LLGLManager::sHasCubeMapArray = false;
bool LLGLManager::sHasDebugOutput = false;
bool LLGLManager::sHasTextureSwizzle = false;
bool LLGLManager::sHasVertexAttribIPointer = false;
bool LLGLManager::sHasGpuShader4 = false;
bool LLGLManager::sHasGpuShader5 = false;

//---------------------------------------------------------------------
// Global initialization for GL
//---------------------------------------------------------------------
#if LL_WINDOWS
//static
void LLGLManager::initWGL(HDC dc)
{
	if (!epoxy_has_wgl_extension(dc, "WGL_ARB_pixel_format"))
	{
		llwarns << "No ARB pixel format extensions" << llendl;
	}

	if (!epoxy_has_wgl_extension(dc, "WGL_ARB_create_context"))
	{
		llwarns << "No ARB create context extensions" << llendl;
	}

	sHasAMDAssociations =
		epoxy_has_wgl_extension(dc, "WGL_AMD_gpu_association");
}
#endif

// Return false if unable (or unwilling due to old drivers) to init GL
//static
bool LLGLManager::initGL()
{
	if (sInited)	// Should never happen.
	{
		llerrs << "GL manager already initialized !" << llendl;
	}

	// Extract video card strings and convert to upper case to work around
	// driver-to-driver variation in capitalization.
	sGLVendor = ll_safe_string((const char*)glGetString(GL_VENDOR));
	LLStringUtil::toUpper(sGLVendor);

	sGLRenderer = ll_safe_string((const char*)glGetString(GL_RENDERER));
	LLStringUtil::toUpper(sGLRenderer);

	sGLVersionString = parse_gl_version(sDriverVersionMajor,
										sDriverVersionMinor,
										sDriverVersionRelease,
										sDriverVersionVendorString);

	sGLVersion = sDriverVersionMajor + sDriverVersionMinor * 0.1f;
	llinfos << "Advertised OpenGL version: " << sDriverVersionMajor << "."
			<< sDriverVersionMinor << llendl;

	// We do not support OpenGL below v2.0 any more.
	if (sGLVersion < 2.f)
	{
		sHasRequirements = false;
		llwarns << "Graphics driver is too old: OpenGL v2.0 minimum is required"
				<< llendl;
		return false;
	}

	parse_glsl_version(sGLSLVersionMajor, sGLSLVersionMinor);
	llinfos << "Advertised GLSL version: " << sGLSLVersionMajor << "."
			<< sGLSLVersionMinor << llendl;

	// We do not use fixed GL functions any more so we need at the minimum
	// support for GLSL v1.10 so to load our basic shaders.
	if (sGLSLVersionMajor < 2 && sGLSLVersionMinor < 10)
	{
		sHasRequirements = false;
		llwarns << "Graphics driver is too old: GLSL v1.10 minimum is required"
				<< llendl;
		return false;
	}

	if (sGLVersion >= 2.1f && LLImageGL::sCompressTextures)
	{
		// Use texture compression
		glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
	}
	else
	{
		// Always disable texture compression
		LLImageGL::sCompressTextures = false;
	}

	if (sGLVendor.find("NVIDIA ") != std::string::npos)
	{
		sGLVendorShort = "NVIDIA";
		sIsNVIDIA = true;
	}
	else if (sGLVendor.find("INTEL") != std::string::npos
#if LL_LINUX
			 // The Mesa-based drivers put this in the Renderer string, not the
			 // Vendor string.
			 || sGLRenderer.find("INTEL") != std::string::npos
#endif
			 )
	{
		sGLVendorShort = "INTEL";
		sIsIntel = true;
#if LL_WINDOWS
		if (sGLVersion >= 4.f && sGLVersion < 4.6f)
		{
			llwarns << "Intel driver claims support for OpenGL " << sGLVersion
					<< " but is known to lie. Downgrading to OpenGL 3.33."
					<< llendl;
			// If we do not have OpenGL 4.6 on Intel, set it to OpenGL 3.3
			// since Intel's Windows drivers claim 4.3 or 4.4 support, but do
			// not seem to work properly. this is expected to be mainly pre-
			// Haswell Intel HD Graphics 4X00 and 5X00.
			sGLVersion = 3.33f;
			sGLSLVersionMajor = 3;
			sGLSLVersionMinor = 30;
		}
#endif
	}
	// AMD is tested last, since there is more risks than with other vendors to
	// see the three letters composing the name appearing in another vendor's
	// GL driver name... HB
	// Trailing space necessary to keep "nVidia Corpor_ati_on" cards from being
	// recognized as ATI/AMD.
	// Note: AMD has been pretty good about not breaking this check, do not
	// rename without a good reason.
	else if (sGLVendor.substr(0, 4) == "ATI "
#if LL_LINUX
			 // The Mesa-based drivers may report AMD instead of ATI.
			 || sGLRenderer.find("AMD") != std::string::npos
#endif
			 )
	{
		sGLVendorShort = "AMD";
		sIsAMD = true;
	}
	else
	{
		sGLVendorShort = "MISC";
	}

	// This is called here because it may depend on above settings.
	initExtensions();

	if (!sHasRequirements)
	{
		// We do not support cards that do not support the
		// GL_ARB_framebuffer_object extension
		llwarns << "GL driver does not support GL_ARB_framebuffer_object"
				<< llendl;
		return false;
	}

	if (sHasAnisotropic)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &sMaxAnisotropy);
		sMaxAnisotropy = llmax(1.f, sMaxAnisotropy);
		llinfos << "Max anisotropy: " << sMaxAnisotropy << llendl;
	}

	S32 old_vram = sVRAM;
	sVRAM = sTexVRAM = 0;

#if LL_WINDOWS
	if (sHasAMDAssociations)
	{
		GLuint gl_gpus_count = wglGetGPUIDsAMD(0, 0);
		if (gl_gpus_count > 0)
		{
			GLuint* ids = new GLuint[gl_gpus_count];
			wglGetGPUIDsAMD(gl_gpus_count, ids);

			GLuint mem_mb = 0;
			for (U32 i = 0; i < gl_gpus_count; ++i)
			{
				wglGetGPUInfoAMD(ids[i], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT,
								 sizeof(GLuint), &mem_mb);
				if (sVRAM < mem_mb)
				{
					// Basically pick the best AMD and trust driver/OS to know
					// to switch
					sVRAM = mem_mb;
				}
			}
		}
		if (sVRAM)
		{
			llinfos << "Detected VRAM via AMDAssociations: " << sVRAM
					<< llendl;
		}
	}
#endif

	if (sHasATIMemInfo)
	{
		// Ask GL how much VRAM is free for textures at startup
		GLint meminfo[4];
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, meminfo);
		sTexVRAM = meminfo[0] / 1024;
		llinfos << "Detected free VRAM for textures via ATIMemInfo: "
				<< sTexVRAM << " MB." << llendl;
	}
	else if (sHasNVXMemInfo)
	{
		GLint meminfo;
		glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &meminfo);
		sVRAM = meminfo / 1024;
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX,
					  &meminfo);
		sTexVRAM = meminfo / 1024;
		llinfos << "Detected VRAM via NVXMemInfo: Total = " << sVRAM
				<< " MB - Free for textures: " << sTexVRAM << " MB." << llendl;
	}

#if LL_WINDOWS
	if (sVRAM < 256)
	{
		// Something likely went wrong using the above extensions...
		// Try via DXGI which will check all GPUs it knows of and will pick up
		// the one with most memory (i.e. we assume the most powerful one),
		// which will *likely* be the one the OS will pick up to render SL.
		S32 mem = LLDXHardware::getMBVideoMemoryViaDXGI();
		if (mem > 0)
		{
			sVRAM = mem;
			llinfos << "Detected VRAM via DXGI: " << sVRAM << llendl;
		}
	}
#endif

	if (sVRAM < 256)
	{
		if (old_vram > sVRAM)
		{
			// Fall back to old method
			sVRAM = old_vram;
		}
		else if (sTexVRAM > 0)
		{
			sVRAM = 4 * sTexVRAM / 3;
			llinfos << "Estimating total VRAM based on reported free VRAM for textures (this is inaccurate): "
					<< sVRAM << " MB." << llendl;
		}
	}

	stop_glerror();

	GLint value;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &value);
	sNumTextureImageUnits = llmin(value, 32);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &value);
	// Clamp down to 64K (maximum observed for modern hardware/drivers), just
	// in case some implementation would report a crazy value.
	sMaxUniformBlockSize = llmin(value, 65536);
	// Note: min block size should be 16384...
	if (sMaxUniformBlockSize < 16384)
	{
		llwarns << "GL_MAX_UNIFORM_BLOCK_SIZE is very small on this GPU: "
				<< sMaxUniformBlockSize << llendl;
	}

	glGetIntegerv(GL_MAX_VARYING_VECTORS, &sMaxVaryingVectors);

	stop_glerror();

	glGetIntegerv(GL_MAX_SAMPLES, &sMaxSamples);
	stop_glerror();

	initGLStates();

	return true;
}

//static
void LLGLManager::initGLStates()
{
	LLGLState::initClass();
	stop_glerror();
}

//static
void LLGLManager::getGLInfo(LLSD& info)
{
	info["GLInfo"]["GLVendor"] =
		ll_safe_string((const char*)glGetString(GL_VENDOR));
	info["GLInfo"]["GLRenderer"] =
		ll_safe_string((const char*)glGetString(GL_RENDERER));
	info["GLInfo"]["GLVersion"] =
		ll_safe_string((const char*)glGetString(GL_VERSION));
	std::string all_exts =
		ll_safe_string((const char*)glGetString(GL_EXTENSIONS));
	boost::char_separator<char> sep(" ");
	boost::tokenizer<boost::char_separator<char> > tok(all_exts, sep);
	for (boost::tokenizer<boost::char_separator<char> >::iterator
			i = tok.begin(); i != tok.end(); ++i)
	{
		info["GLInfo"]["GLExtensions"].append(*i);
	}
}

//static
void LLGLManager::printGLInfoString()
{
	llinfos << "GL_VENDOR  : "
			<< ll_safe_string((const char*)glGetString(GL_VENDOR))
			<< llendl;
	llinfos << "GL_RENDERER: "
			<< ll_safe_string((const char*)glGetString(GL_RENDERER))
			<< llendl;
	llinfos << "GL_VERSION : "
			<< ll_safe_string((const char*)glGetString(GL_VERSION))
			<< llendl;
	std::string all_exts =
		ll_safe_string((const char*)glGetString(GL_EXTENSIONS));
	LLStringUtil::replaceChar(all_exts, ' ', '\n');
	LL_DEBUGS("RenderInit") << "GL_EXTENSIONS:\n" << all_exts << LL_ENDL;
}

//static
std::string LLGLManager::getRawGLString()
{
	return ll_safe_string((char*)glGetString(GL_VENDOR)) + " " +
		   ll_safe_string((char*)glGetString(GL_RENDERER));
}

//static
void LLGLManager::asLLSD(LLSD& info)
{
	// Currently these are duplicates of fields in LLViewerStats "system" info
	info["gpu_vendor"] = sGLVendorShort;
	info["gpu_version"] = sDriverVersionVendorString;
	info["opengl_version"] = sGLVersionString;
	info["gl_renderer"] = sGLRenderer;
	// Vendor
	info["is_ati"] = sIsAMD;
	info["is_intel"] = sIsIntel;
	info["is_nvidia"] = sIsNVIDIA;
	// Limits
	info["vram"] = sVRAM;
	info["num_texture_image_units"] =  sNumTextureImageUnits;
	info["max_samples"] = sMaxSamples;
	info["max_vertex_range"] = sGLMaxVertexRange;
	info["max_index_range"] = sGLMaxIndexRange;
	info["max_texture_size"] = sGLMaxTextureSize;
	// Extensions
	info["has_vertex_array_object"] = sHasVertexArrayObject;
	info["has_sync"] = sHasSync;
	info["has_timer_query"] = sHasTimerQuery;
	info["has_occlusion_query2"] = sHasOcclusionQuery2;
	info["has_depth_clamp"] = sHasDepthClamp;
	info["has_anisotropic"] = sHasAnisotropic;
	info["has_cubemap_array"] = sHasCubeMapArray;
	info["has_debug_output"] = sHasDebugOutput;
	info["has_nvx_mem_info"] = sHasNVXMemInfo;
	info["has_ati_mem_info"] = sHasATIMemInfo;
	// Got requirements for our renderer ?
	info["has_requirements"] = sHasRequirements;
}

//static
void LLGLManager::shutdownGL()
{
	if (sInited)
	{
		deleteBuffers(0, NULL);
		glFinish();
		stop_glerror();
		sInited = false;
	}
}

//static
void LLGLManager::initExtensions()
{
	sHasATIMemInfo = epoxy_has_gl_extension("GL_ATI_meminfo");
	sHasNVXMemInfo = epoxy_has_gl_extension("GL_NVX_gpu_memory_info");
	sHasAnisotropic = sGLVersion >= 4.6f ||
		epoxy_has_gl_extension("GL_EXT_texture_filter_anisotropic");
	sHasOcclusionQuery2 = sGLVersion >= 3.3f ||
						  epoxy_has_gl_extension("GL_ARB_occlusion_query2");
	sHasTimerQuery = sGLVersion >= 3.3f ||
					 epoxy_has_gl_extension("GL_ARB_timer_query");
	sHasVertexArrayObject = sGLVersion >= 3.f ||
		epoxy_has_gl_extension("GL_ARB_vertex_array_object");
	sHasSync = sGLVersion >= 3.2f || epoxy_has_gl_extension("GL_ARB_sync");
	sHasDepthClamp = sGLVersion >= 3.2f ||
					 epoxy_has_gl_extension("GL_ARB_depth_clamp") ||
					 epoxy_has_gl_extension("GL_NV_depth_clamp");
	if (!sHasDepthClamp)
	{
		sUseDepthClamp = false;
	}
	// Mask out FBO support when packed_depth_stencil is not there because we
	// need it for LLRenderTarget. Brad
#if GL_ARB_framebuffer_object
	sHasRequirements =
		sGLVersion >= 3.f ||
		epoxy_has_gl_extension("GL_ARB_framebuffer_object");
#else
	sHasRequirements =
		sGLVersion >= 3.f ||
		(epoxy_has_gl_extension("GL_EXT_framebuffer_object") &&
		 epoxy_has_gl_extension("GL_EXT_framebuffer_blit") &&
		 epoxy_has_gl_extension("GL_EXT_framebuffer_multisample") &&
		 epoxy_has_gl_extension("GL_EXT_packed_depth_stencil"));
#endif

	sHasCubeMapArray = sGLVersion >= 4.f;

	sHasDebugOutput = sGLVersion >= 4.3f ||
					  epoxy_has_gl_extension("GL_ARB_debug_output");

	sHasVertexAttribIPointer = sGLSLVersionMajor > 1 ||
							   sGLSLVersionMinor >= 30;

	sHasGpuShader4 = sGLVersion >= 3.f &&
					 epoxy_has_gl_extension("GL_ARB_gpu_shader4");
#if GL_ARB_gpu_shader5
	sHasGpuShader5 = epoxy_has_gl_extension("GL_ARB_gpu_shader5");
#endif

#if GL_ARB_texture_swizzle
	sHasTextureSwizzle = sGLVersion >= 3.3f ||
						 epoxy_has_gl_extension("GL_ARB_texture_swizzle");
#endif

	if (!sHasSync)
	{
		llinfos << "This GL implementation lacks GL_ARB_sync" << llendl;
	}
	if (!sHasAnisotropic)
	{
		llinfos << "Could not initialize anisotropic filtering" << llendl;
	}
	if (!sHasOcclusionQuery2)
	{
		llinfos << "Could not initialize GL_ARB_occlusion_query2" << llendl;
	}
	// Note: GL_ARB_vertex_array_object should exist in core GL profile v3.0+
	if (!sHasVertexArrayObject && LLRender::sGLCoreProfile)
	{
		llinfos << "Could not initialize GL_ARB_vertex_array_object" << llendl;
	}

	// Misc
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, (GLint*)&sGLMaxVertexRange);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, (GLint*)&sGLMaxIndexRange);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&sGLMaxTextureSize);

	sInited = true;

	clear_glerror();
}


//static
void LLGLManager::deleteBuffers(U32 count, U32* bufferp)
{
	// LL's comment: "Wait a few frames before actually deleting the buffers to
	// avoid synchronization issues with the GPU." My take: this is totally
	// moot since we do not share buffers between GL threads (which would then
	// require a proper GL sync instead of such a kludge) *and* the buffers
	// deletions and allocations are managed by the GL driver, meaning the
	// driver won't risk returning a handle for a new buffer that would
	// correspond to the handle of another buffer in the process of being
	// deleted... I reduced the delay to just 1 frame so that we can still pool
	// the deletions, thus minimizing the number of calls to glDeleteBuffers().
	// I also added a proper freeing of pending buffers on GL shutdown which is
	// missing from LL's code. HB
	constexpr U32 LIST_ENTRIES = 2;
	static std::vector<GLuint> sFreeList[LIST_ENTRIES];
	if (!sInited)	// Paranoia
	{
		return;
	}
	if (bufferp)
	{
		const U32 cur_frame = LLRender::sCurrentFrame;
		U32 idx = cur_frame % LIST_ENTRIES;
		for (U32 i = 0; i < count; ++i)
		{
			sFreeList[idx].push_back(bufferp[i]);
		}
		idx = (cur_frame + (LIST_ENTRIES - 1)) % LIST_ENTRIES;
		GLsizei sz = sFreeList[idx].size();
		if (sz)
		{
			glDeleteBuffers(sz, sFreeList[idx].data());
			sFreeList[idx].clear();
		}
	}
	else	// Used when shutting down GL. HB
	{
		// Delete all pending buffers now.
		for (U32 idx = 0; idx < LIST_ENTRIES; ++idx)
		{
			GLsizei sz = sFreeList[idx].size();
			if (sz)
			{
				glDeleteBuffers(sz, sFreeList[idx].data());
				sFreeList[idx].clear();
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// LLGLState class
///////////////////////////////////////////////////////////////////////////////

// Static members
LLGLState::state_map_t LLGLState::sStateMap;

GLboolean LLGLDepthTest::sDepthEnabled = GL_FALSE; // OpenGL default
U32 LLGLDepthTest::sDepthFunc = GL_LESS; // OpenGL default
GLboolean LLGLDepthTest::sWriteEnabled = GL_TRUE; // OpenGL default

//static
void LLGLState::initClass()
{
	sStateMap[GL_DITHER] = GL_TRUE;
	// Make sure multisample defaults to disabled
	sStateMap[GL_MULTISAMPLE] = GL_FALSE;
	glDisable(GL_MULTISAMPLE);
}

//static
void LLGLState::restoreGL()
{
	sStateMap.clear();
	initClass();
}

#if 0	// Not used, but kept in source, just in case... HB
//static
void LLGLState::resetTextureStates()
{
	gGL.flush();

	GLint max_tex_units;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_tex_units);

	for (S32 j = max_tex_units - 1; j >= 0; --j)
	{
		LLTexUnit* unitp = gGL.getTexUnit(j);
		unitp->activate();
		glClientActiveTexture(GL_TEXTURE0 + j);
		if (j == 0)
		{
			unitp->enable(LLTexUnit::TT_TEXTURE);
		}
		else
		{
			unitp->disable();
		}
	}
}
#endif

void LLGLState::dumpStates()
{
	llinfos << "GL States:";
	for (state_map_t::iterator iter = sStateMap.begin(), end = sStateMap.end();
		 iter != end; ++iter)
	{
		llcont << llformat("\n   0x%04x : %s", (S32)iter->first,
						   iter->second ? "true" : "false");
	}
	llcont << llendl;
}

//static
void LLGLState::checkStates(const std::string& msg, S32 line)
{
	if (!gDebugGL)
	{
		return;
	}
	stop_glerror();

	static std::string errors;

	if (glIsEnabled(GL_BLEND))
	{
		GLint src;
		glGetIntegerv(GL_BLEND_SRC, &src);
		GLint dst;
		glGetIntegerv(GL_BLEND_DST, &dst);
		if (src != GL_SRC_ALPHA || dst != GL_ONE_MINUS_SRC_ALPHA)
		{
			errors =
				llformat("Blend function corrupted: source: 0x%04x, destination: 0x%04x",
						 src, dst);
		}
	}

	bool has_state_error = false;
	for (state_map_t::iterator iter = sStateMap.begin(), end = sStateMap.end();
		 iter != end; ++iter)
	{
		U32 state = iter->first;
		GLboolean cur_state = iter->second;
		GLboolean gl_state = glIsEnabled(state);
		if (cur_state != gl_state)
		{
			has_state_error = true;
			if (!errors.empty())
			{
				errors.append(" - ");
			}
			errors += llformat("Incoherent state: 0x%04x", state);
		}
	}
	if (has_state_error)
	{
		dumpStates();
	}

	if (!errors.empty())
	{
		llwarns << errors;
		if (!msg.empty())
		{
			llcont << " - " << msg;
		}
		if (line > 0)
		{
			llcont << " - line " << line;
		}
		llcont << llendl;
		errors.clear();
	}
}

LLGLState::LLGLState(U32 state, S32 enabled)
:	mState(state),
	mWasEnabled(GL_FALSE),
	mIsEnabled(GL_FALSE)
{
	// Always ignore any state deprecated post GL 3.0
	switch (state)
	{
		case GL_STENCIL_TEST:
			if (gUsePBRShaders)
			{
				llerrs << "GL_STENCIL_TEST used in PBR rendering mode !"
					   << llendl;
			}
			break;

		case GL_ALPHA_TEST:
		case GL_NORMALIZE:
		case GL_TEXTURE_GEN_R:
		case GL_TEXTURE_GEN_S:
		case GL_TEXTURE_GEN_T:
		case GL_TEXTURE_GEN_Q:
		case GL_LIGHTING:
		case GL_COLOR_MATERIAL:
		case GL_FOG:
		case GL_LINE_STIPPLE:
		case GL_POLYGON_STIPPLE:
			mState = 0;
			llwarns_once << "Asked for a deprecated GL state: " << state
						 << llendl;
			llassert(false);
	}

	if (mState)
	{
		mWasEnabled = sStateMap[state];
		setEnabled(enabled);
		stop_glerror();
	}
}

void LLGLState::setEnabled(S32 enabled)
{
	stop_glerror();
	if (!mState)
	{
		return;
	}
	if (enabled == CURRENT_STATE)
	{
		enabled = sStateMap[mState] == GL_TRUE ? GL_TRUE : GL_FALSE;
	}
	else if (enabled == GL_TRUE && sStateMap[mState] != GL_TRUE)
	{
		gGL.flush();
		glEnable(mState);
		sStateMap[mState] = GL_TRUE;
		stop_glerror();
	}
	else if (enabled == GL_FALSE && sStateMap[mState] != GL_FALSE)
	{
		gGL.flush();
		glDisable(mState);
		sStateMap[mState] = GL_FALSE;
		stop_glerror();
	}
	mIsEnabled = enabled;
}

//virtual
LLGLState::~LLGLState()
{
	if (mState)
	{
		if (gDebugGL)
		{
			GLboolean state = glIsEnabled(mState);
			if (sStateMap[mState] != state)
			{
				llwarns_once << "Mismatch for state: " << std::hex << mState
							 << std::dec << " - Actual status: " << state
							 << " (should be " << sStateMap[mState] << ")."
							 << llendl;
			}
		}
		if (mIsEnabled != mWasEnabled)
		{
			gGL.flush();
			if (mWasEnabled)
			{
				glEnable(mState);
				sStateMap[mState] = GL_TRUE;
			}
			else
			{
				glDisable(mState);
				sStateMap[mState] = GL_FALSE;
			}
			stop_glerror();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLGLUserClipPlane class
///////////////////////////////////////////////////////////////////////////////

LLGLUserClipPlane::LLGLUserClipPlane(const LLPlane& p, const LLMatrix4a& mdlv,
									 const LLMatrix4a& proj, bool apply)
:	mApply(apply)
{
	if (apply)
	{
		mModelview = mdlv;
		mProjection = proj;
		// Flip incoming LLPlane to get consistent behavior compared to frustum
		// culling
		setPlane(-p[0], -p[1], -p[2], -p[3]);
	}
}

LLGLUserClipPlane::~LLGLUserClipPlane()
{
	disable();
}

void LLGLUserClipPlane::disable()
{
	if (mApply)
	{
		mApply = false;
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.popMatrix();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}

void LLGLUserClipPlane::setPlane(F32 a, F32 b, F32 c, F32 d)
{
	LLMatrix4a& p = mProjection;
	LLMatrix4a& m = mModelview;

	LLMatrix4a invtrans_mdlv;
	invtrans_mdlv.setMul(p, m);
	invtrans_mdlv.invert();
	invtrans_mdlv.transpose();

	LLVector4a oplane(a, b, c, d);
	LLVector4a cplane, cplane_splat, cplane_neg;

	invtrans_mdlv.rotate4(oplane, cplane);

	cplane_splat.splat<2>(cplane);
	cplane_splat.setAbs(cplane_splat);
	cplane.div(cplane_splat);
	cplane.sub(LLVector4a(0.f, 0.f, 0.f, 1.f));

	cplane_splat.splat<2>(cplane);
	cplane_neg = cplane;
	cplane_neg.negate();

	cplane.setSelectWithMask(cplane_splat.lessThan(_mm_setzero_ps()),
							 cplane_neg, cplane);

	LLMatrix4a suffix;
	suffix.setIdentity();
	suffix.setColumn<2>(cplane);
	LLMatrix4a new_proj;
	new_proj.setMul(suffix, p);

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadMatrix(new_proj);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLDepthTest class
///////////////////////////////////////////////////////////////////////////////

LLGLDepthTest::LLGLDepthTest(GLboolean depth_enabled, GLboolean write_enabled,
							 U32 depth_func, bool ignored)
:	mPrevDepthEnabled(sDepthEnabled),
	mPrevDepthFunc(sDepthFunc),
	mPrevWriteEnabled(sWriteEnabled),
	mIgnored(ignored)
{
	if (ignored)
	{
		// Do nothing.
		return;
	}

	stop_glerror();
	checkState();

	if (!depth_enabled)
	{
		// Always disable depth writes if depth testing is disabled. GL spec
		// defines this as a requirement, but some implementations allow depth
		// writes with testing disabled. The proper way to write to depth
		// buffer with testing disabled is to enable testing and use a
		// depth_func of GL_ALWAYS
		write_enabled = GL_FALSE;
	}

	if (depth_enabled != sDepthEnabled)
	{
		gGL.flush();
		if (depth_enabled)
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
		sDepthEnabled = depth_enabled;
	}
	if (depth_func != sDepthFunc)
	{
		gGL.flush();
		glDepthFunc(depth_func);
		sDepthFunc = depth_func;
	}
	if (write_enabled != sWriteEnabled)
	{
		gGL.flush();
		glDepthMask(write_enabled);
		sWriteEnabled = write_enabled;
	}
	stop_glerror();
}

LLGLDepthTest::~LLGLDepthTest()
{
	if (mIgnored)
	{
		// Nothing to do.
		return;
	}

	checkState();
	if (sDepthEnabled != mPrevDepthEnabled)
	{
		gGL.flush();
		if (mPrevDepthEnabled)
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
		sDepthEnabled = mPrevDepthEnabled;
	}
	if (sDepthFunc != mPrevDepthFunc)
	{
		gGL.flush();
		glDepthFunc(mPrevDepthFunc);
		sDepthFunc = mPrevDepthFunc;
	}
	if (sWriteEnabled != mPrevWriteEnabled)
	{
		gGL.flush();
		glDepthMask(mPrevWriteEnabled);
		sWriteEnabled = mPrevWriteEnabled;
	}
	stop_glerror();
}

void LLGLDepthTest::checkState()
{
	if (gDebugGL && !mIgnored)
	{
		GLint func = 0;
		GLboolean mask = GL_FALSE;

		glGetIntegerv(GL_DEPTH_FUNC, &func);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);

		if (glIsEnabled(GL_DEPTH_TEST) != sDepthEnabled ||
			sWriteEnabled != mask || (GLint)sDepthFunc != func)
		{
			llwarns << "Unexpected depth testing state." << llendl;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLGLSquashToFarClip class
///////////////////////////////////////////////////////////////////////////////

LLGLSquashToFarClip::LLGLSquashToFarClip(U32 layer)
{
	F32 depth = 0.99999f - 0.0001f * layer;
	LLMatrix4a proj = gGLProjection;
	LLVector4a col = proj.getColumn<3>();
	col.mul(depth);
	proj.setColumn<2>(col);

	U32 last_matrix_mode = gGL.getMatrixMode();
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadMatrix(proj);
	gGL.matrixMode(last_matrix_mode);
}

LLGLSquashToFarClip::~LLGLSquashToFarClip()
{
	U32 last_matrix_mode = gGL.getMatrixMode();
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(last_matrix_mode);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLSPipeline*SkyBox classes
///////////////////////////////////////////////////////////////////////////////

LLGLSPipelineSkyBox::LLGLSPipelineSkyBox()
:	mCullFace(GL_CULL_FACE),
	mSquashClip()
{
}

LLGLSPipelineDepthTestSkyBox::LLGLSPipelineDepthTestSkyBox(GLboolean depth_test,
														   GLboolean depth_write)
:	LLGLSPipelineSkyBox(),
	mDepth(depth_test, depth_write, GL_LEQUAL)
{
}

LLGLSPipelineBlendSkyBox::LLGLSPipelineBlendSkyBox(GLboolean depth_test,
												   GLboolean depth_write)
:	LLGLSPipelineDepthTestSkyBox(depth_test, depth_write),
	mBlend(GL_BLEND)
{
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

