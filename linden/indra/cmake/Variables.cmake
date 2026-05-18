# -*- cmake -*-
if (VARIABLES_CMAKE_INCLUDED)
	return()
endif (VARIABLES_CMAKE_INCLUDED)
set (VARIABLES_CMAKE_INCLUDED TRUE)

# Definitions of variables used throughout the Second Life build process.
#
# Platform variables:
#
#	LINUX	- Linux
#	WINDOWS	- Windows

# Relative and absolute paths to subtrees.

set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/../scripts)

set(LIBS_PREBUILT_DIR ${CMAKE_SOURCE_DIR}/.. CACHE PATH "Location of prebuilt libraries.")

# Only 64 bits builds are now supported, for all platforms.
# *TODO: support for cross-compilation on a different build architecture ?
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
	set(ARCH x86_64)
elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64*" OR ${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm64*")
	set(ARCH arm64)
else ()
	message(FATAL_ERROR "Unsupported architecture ${CMAKE_HOST_SYSTEM_PROCESSOR}, sorry !")
endif ()
message(STATUS "Architecture to build for: ${ARCH}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	message(FATAL_ERROR "This viewer does not support macOS any more, sorry...")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(LINUX ON BOOl FORCE)

	set (GCC_VERSION 0)
	set (CLANG_VERSION 0)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		execute_process(
			COMMAND sh -c "${CMAKE_CXX_COMPILER} -dumpversion"
			OUTPUT_VARIABLE CLANG_VERSION
		)
		# Let's actually get a numerical version of Clang's version
		STRING(REGEX REPLACE "([0-9]+)\\.([0-9])\\.([0-9]).*" "\\1\\2\\3" CLANG_VERSION ${CLANG_VERSION})
		message(STATUS "Clang version (dots removed): ${CLANG_VERSION}")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		execute_process(
			COMMAND sh -c "${CMAKE_CXX_COMPILER} -dumpversion"
			OUTPUT_VARIABLE GCC_VERSION
		)
		# Let's actually get a numerical version of GCC's version
		STRING(REGEX REPLACE "([0-9]+)\\.([0-9])\\.([0-9]).*" "\\1\\2\\3" GCC_VERSION ${GCC_VERSION})

		# When compiled with --with-gcc-major-version-only newer gcc versions
		# only report the major number with -dumpversion, and -dumpfullversion
		# (which is not understood by older gcc versions) must be used instead
		# to get the true version !!!  The guy who coded this (instead of
		# simply adding a -dumpmajorversion) should face death penalty for
		# utter (and lethal, natural-selection wise) stupidity !
		if (${GCC_VERSION} LESS 100)
			execute_process(
				COMMAND sh -c "${CMAKE_CXX_COMPILER} -dumpfullversion"
				OUTPUT_VARIABLE GCC_VERSION
			)
			STRING(REGEX REPLACE "([0-9]+)\\.([0-9])\\.([0-9]).*" "\\1\\2\\3" GCC_VERSION ${GCC_VERSION})	
		endif (${GCC_VERSION} LESS 100)
		message(STATUS "GCC version (dots removed): ${GCC_VERSION}")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
		# ICC might actually work for compiling the viewer, but would require
		# to make approriate changes/additions to 00-Common.cmake. If you feel
		# like it, be my guest and provide me with the necessary patch... HB
		message(FATAL_ERROR "Unsupported compiler on this platform, sorry !")
	else ()
		message(FATAL_ERROR "Unknown compiler !")
	endif ()
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(USING_CLANG OFF)
	if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
		set(USING_CLANG ON)
	elseif (MSVC_VERSION EQUAL 1937)
		message(FATAL_ERROR "The MSVC1937 compiler is utterly broken (e.g. the builds crash when ran under Wine), please update to MSVC1938 or newer !")
	elseif (MSVC_VERSION LESS 1930 OR MSVC_VERSION GREATER 1949)
		message(FATAL_ERROR "You need VS2022 to build this viewer !")
	else ()
		message(STATUS "MSVC version: ${MSVC_VERSION}")
	endif ()

	set(WINDOWS ON BOOL FORCE)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if (LINUX)
	if (ARCH STREQUAL "arm64")
		set(PREBUILT_TYPE linux64-arm)
	else ()
		set(PREBUILT_TYPE linux64)
	endif ()
elseif (WINDOWS)
	set(PREBUILT_TYPE windows64)
endif ()

set(VIEWER_BRANDING_ID "cool_vl_viewer" CACHE STRING "Viewer branding id (currently cool_vl_viewer)")
set(VIEWER_BRANDING_NAME "Cool VL Viewer")
set(VIEWER_BRANDING_NAME_CAMELCASE "CoolVLViewer")

set(USE_MIRROR OFF CACHE BOOL "Use the mirror web site of the Cool VL Viewer to download its pre-built packages.")

# The following flags are for use with USESYSTEMLIBS=ON and allow to build
# against a sub-set of system libraries while the use of the pre-built
# libraries for incompatible system libraries. HB
set(USESYSTEMLIBS OFF CACHE BOOL "Use system libraries instead of prebuilt libraries whenever possible.")
set(USEPREBUILTEPOXY OFF CACHE BOOL "Force the use of the pre-built epoxy library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTEXPAT OFF CACHE BOOL "Force the use of the pre-built expat library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTGLIB OFF CACHE BOOL "Force the use of the pre-built glib libraries instead of build system ones, when USESYSTEMLIBS is ON.")
set(USEPREBUILTGSTREAMER OFF CACHE BOOL "Force the use of the pre-built gstreamer libraries instead of build system ones, when USESYSTEMLIBS is ON.")
set(USEPREBUILTHUNSPELL OFF CACHE BOOL "Force the use of the pre-built hunspell library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTJPEG OFF CACHE BOOL "Force the use of the pre-built jpeg library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTLUA OFF CACHE BOOL "Force the use of the pre-built lua library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTNDOF OFF CACHE BOOL "Force the use of the pre-built ndof library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTOGG OFF CACHE BOOL "Force the use of the pre-built ogg and vorbis libraries instead of build system ones, when USESYSTEMLIBS is ON.")
set(USEPREBUILTOPENAL OFF CACHE BOOL "Force the use of the pre-built openal and alut libraries instead of build system ones, when USESYSTEMLIBS is ON.")
set(USEPREBUILTPNG OFF CACHE BOOL "Force the use of the pre-built png library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTSDL OFF CACHE BOOL "Force the use of the pre-built sdl2 library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTSQLITE OFF CACHE BOOL "Force the use of the pre-built sqlite3 library instead of build system one, when USESYSTEMLIBS is ON.")
set(USEPREBUILTUUID OFF CACHE BOOL "Force the use of the pre-built uuid library instead of build system one, when USESYSTEMLIBS is ON.")

# NOTE: USEAVX and USEAVX2 are mainly geared towards the MSVC compiler. For gcc
# or clang, you would rather pass -mavx[2] or -march=<cpu-type or native> in
# CMAKE_C_FLAGS and CMAKE_CXX_FLAGS[_RELEASE]. In particular, -march=native
# would automatically enable the adequate SSE2/AVX/AVX2 optimizations for
# compiler-generated maths on the build system.
set(USEAVX OFF CACHE BOOL "Use AVX instead of SSE2 for compiler-generated maths.")
set(USEAVX2 OFF CACHE BOOL "Use AVX2 instead of SSE2 for compiler-generated maths.")

# NOTE: LTO may actually prove detrimental (or simply neutral) to frame rates,
# because the compiler will try too hard to common up code that would otherwise
# get inlined... It appears, however that newer compilers (gcc 11/clang 12) do
# provide better results (frame rates) with LTO, especially llvm/clang v12.
set(USELTO OFF CACHE BOOL "Use link time optimization (Linux only and not supported by all compiler versions).")

# Use unity builds where possible (only available with cmake v3.16.0 or newer;
# this option is simply ignored with older versions). EXPERIMENTAL and largely
# untested: may result in weird bugs in the final binaries... See the comment
# at the end of indra/llplugin/CMakeLists.txt.
set(USEUNITYBUILD OFF CACHE BOOL "Use cmake v3.16.0+ UNITY_BUILD feature for faster builds.")

# Use -fstack-protector option to protect the stack with canaries (causes a
# small loss in speed due to added code and caches usage for each function
# call). Not really needed for an application such as the SL viewer; the risk
# of seeing some injected rogue data triggering an overflow bug and allowing
# arbitrary code execution (and without crashing the viewer, i.e. without the
# user noticing something is wrong) is totally negligible and hardly feasible
# at all for someone not controlling the grid servers.
set(PROTECTSTACK OFF CACHE BOOL "Protect the stack against overflows (for the paranoids).")

# Prevent the use of an executable stack by gcc, if requested. Note: this is
# EXPERIMENTAL and may result in slightly slower code or, at worst, a crashing
# viewer (see the corresponding comment in 00-Common.cmake).
set(NOEXECSTACK OFF CACHE BOOL "Prevent the use of an executable stack by gcc (for the paranoids).")

# OpenMP support, if requested
set(OPENMP OFF CACHE BOOL "Enable OpenMP optimizations.")

# Enable the Tracy profiler support, if requested.
set(TRACY OFF CACHE BOOL "Enable Tracy profiler support.")

# Profiling with gprof, if requested (Linux only).
set(GPROF OFF CACHE BOOL "Enable gprof profiling.")

# Sanitizing with ASAN, if requested (Linux only).
set(ASAN OFF CACHE BOOL "Enable ASAN sanitizer.")

source_group("CMake Rules" FILES CMakeLists.txt)
