# -*- cmake -*-
if (COMMON_CMAKE_INCLUDED)
	return()
endif (COMMON_CMAKE_INCLUDED)
set (COMMON_CMAKE_INCLUDED TRUE)

# Include compilation options
include(00-BuildOptions)

# Early download of utilities used for building the viewer under Windows: this
# is currently for zstd.exe only, which is used to unpack LL's newest *.tar.zst
# pre-built library packages, without needing to install the pyzst Python
# module under Windows, since Python cannot natively deal with them.
# Linux builds currently do not make use of *.zst packages, and should they do
# at some point in the future, zstd would likely already be installed on the
# build system already. HB
include(BuildUtils)

# This is for cmake v3.11.1 and newer:
set(OpenGL_GL_PREFERENCE "LEGACY")

# Platform-specific compilation flags.

if (LINUX)
	# Supported build types:
	set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo;Debug" CACHE STRING
		"Supported build types." FORCE)

	set(CMAKE_SKIP_RPATH TRUE)

	if (${CLANG_VERSION} LESS 700 AND ${GCC_VERSION} LESS 810)
		message(FATAL_ERROR "Cannot compile any more with gcc below version 8.1 or clang below version 7.0, sorry...")
	endif ()

	# Compiler-version-specific warnings disabling:
	set(IGNORED_C_WARNINGS "")
	set(IGNORED_CXX_WARNINGS "")
	# LTO got largely broken with gcc 12... It now spews totally bogus warnings
	# at link time !
	if (USELTO AND ${GCC_VERSION} GREATER_EQUAL 1200)
		set(IGNORED_CXX_WARNINGS "${IGNORED_CXX_WARNINGS} -Wno-stringop-overflow -Wno-alloc-size-larger-than")
	endif ()
	if (USE_TRACY AND ${GCC_VERSION} GREATER 0)
		# Avoid warnings seen in Tracy headers with gcc...
		set(IGNORED_CXX_WARNINGS "${IGNORED_CXX_WARNINGS} -Wno-maybe-uninitialized")
	endif ()

	# Debugging information generation options
	if (${CLANG_VERSION} GREATER 0)
		set(DEBUG_OPTIONS "-g")
	else ()
		set(DEBUG_OPTIONS "-g -gdwarf-4 -fno-var-tracking-assignments")
	endif ()

	# Specify the C standard in use.
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17")

	# Specify the C++ standard in use: C++17 is required, C++20 is for now
	# optional. gcc 10 and clang 10 should both have decent C++20 support, so
	# we use C++20 with them and newer compilers.
	if (${GCC_VERSION} GREATER_EQUAL 1000 OR ${CLANG_VERSION} GREATER_EQUAL 1000)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
	else ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
	endif ()

	if (PROTECTSTACK)
		# Also enable "safe" glibc functions variants usage (_FORTIFY_SOURCE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector -D_FORTIFY_SOURCE=2")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector -D_FORTIFY_SOURCE=2")
	else (PROTECTSTACK)
		# We do not care for protecting the stack with canaries. This option
		# provides some (small) speed and caches usage gains. Also forbid the
		# usage of slower but "safe" glibc functions variants.
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector -fcf-protection=none -U_FORTIFY_SOURCE")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector -fcf-protection=none -U_FORTIFY_SOURCE")
	endif (PROTECTSTACK)

	if (GPROF)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
	endif (GPROF)

	if (ASAN)
		if (${CLANG_VERSION} GREATER 0)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address --param asan-stack=0 --param asan-globals=0")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address --param asan-stack=0 --param asan-globals=0")
		else (${CLANG_VERSION} GREATER 0)
			message(WARNING "GGC's ASAN would not work properly, and would immeditaley stop the viewer on CEF memory leaks... ASAN *not* compiled in. Please use llvm/clang instead for ASAN builds.")
		endif (${CLANG_VERSION} GREATER 0)
	endif (ASAN)

	# Prevent executable stack (used by gcc's implementation of lambdas) when
	# possible.
	if (NOEXECSTACK AND ${GCC_VERSION} GREATER 0)
		# With gcc (v7 and newer) we may use -fno-trampolines, but then the
		# generated code (which uses function descriptors for lambdas) is
		# slightly slower.
		# See: https://gcc.gnu.org/onlinedocs/gccint/Trampolines.html
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-trampolines")
	endif (NOEXECSTACK AND ${GCC_VERSION} GREATER 0)

	# Never add thread-locking code to local static variables (we never use
	# non-thread_local static local variables in thread-sensitive code):
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")

	# Use -fPIC under Linux for 64 bits (especially for our libraries),
	# else linking errors may happen.
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

	# Remember whether the user asked for native optimization option or not.
	set(TUNED_BUILD OFF)
	if ("${CMAKE_CXX_FLAGS}" MATCHES ".*-march=native.*")
		set(TUNED_BUILD ON)
	endif ()

	# Vectorized maths selection
	if (ARCH STREQUAL "arm64")
		# We need a conversion header to get the SSE2 code translated into its
		# NEON counterpart (thanks to sse2neon.h).
		add_definitions(-DSSE2NEON=1)
		# If not already manualy specified, add the necessary -march tunes for
		# sse2neon.h to take full effect. See the "Usage" section at:
		# https://github.com/DLTcollab/sse2neon/blob/master/README.md
		if (NOT "${CMAKE_C_FLAGS}" MATCHES ".*-march.*")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+fp+simd")
		endif ()
		if (NOT "${CMAKE_CXX_FLAGS}" MATCHES ".*-march=.*")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+fp+simd")
		endif ()
	else (ARCH STREQUAL "arm64")
		# We only support 64 bits builds for x86 targets
		set(ARCH_FLAG "-m64")
		# SSE2 is always enabled by default for 64 bits targets
		set(COMPILER_MATHS "-mfpmath=sse")
		if (USEAVX2)
			set(COMPILER_MATHS "-mavx2 -mfma -mfpmath=sse")
		elseif (USEAVX)
			set(COMPILER_MATHS "-mavx -mfpmath=sse")
		endif ()
	endif (ARCH STREQUAL "arm64")

	# Compilation flags common to all build types and compilers.
	# NOTE: do not use -ffast-math: it makes us loose the Moon (!) and probably
	# causes other issues as well...
	# Please, note that -fno-strict-aliasing is *required*, because the code is
	# not strict aliasing safe and omitting this option would cause warnings or
	# possibly bogus optimizations.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe ${DEBUG_OPTIONS} -fexceptions -fno-strict-aliasing -fvisibility=hidden -fsigned-char ${ARCH_FLAG} ${COMPILER_MATHS} -fno-math-errno -fno-trapping-math -pthread")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe ${DEBUG_OPTIONS} -fexceptions -fno-strict-aliasing -fvisibility=hidden -fsigned-char ${ARCH_FLAG} ${COMPILER_MATHS} -fno-math-errno -fno-trapping-math -pthread")

	# OpenMP support, if requested
	if (OPENMP)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
	endif (OPENMP)
	# OpenMP support, if requested
	if (OPENMP)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
	endif (OPENMP)

	# Use OPT_FLAGS to set optimizations (used for the release builds) that would
	# break debugging
	if (${CLANG_VERSION} GREATER 0)
		set(OPT_FLAGS "-O3")
		if (${CLANG_VERSION} GREATER 799)
			set(OPT_FLAGS "${OPT_FLAGS} -fno-delete-null-pointer-checks -mno-retpoline -mno-retpoline-external-thunk")
		endif ()
		if (${CLANG_VERSION} GREATER 1099)
			set(OPT_FLAGS "${OPT_FLAGS} -mno-lvi-cfi -mno-lvi-hardening")
		endif ()
	else ()
		# Do not use our aggressive registers tuning options if the user
		# asked for -march=native optimization (i.e. let gcc set the adequate
		# optimizations in that case).
		if (TUNED_BUILD)
			set(OPT_FLAGS "-O3 -fno-delete-null-pointer-checks -fno-ipa-cp-clone -fno-align-labels -fno-align-loops")
		else (TUNED_BUILD)
			set(OPT_FLAGS "-O3 -fno-delete-null-pointer-checks -fno-ipa-cp-clone -fno-align-labels -fno-align-loops -fsched-pressure -frename-registers -fweb -fira-hoist-pressure")
		endif (TUNED_BUILD)
	endif ()

	# Using ld.bfd (and now ld.mold) with clang when LTO is enabled "now" (i.e.
	# with llgltf added: GLM issue ?) triggers the following assert in the gold
	# plugin of llvm/clang (seen at least with llvm/clang v18/19 and ld.bfd
	# v2.40 or ld.mold 2.33+):
	# ld: llvm-18.1.6/llvm/tools/gold/gold-plugin.cpp:1048:
	#    std::vector<std::pair<llvm::SmallString<128>, bool> > runLTO():
	#    Assertion `ObjFilename.second' failed.
	# This fault does not happen with gcc/LTO and ld.bfd, neither with
	# clang/LTO and ld.lld or ld.gold... So try and use one of the latter
	# linkers when using clang/LTO, and warn if none of them is present on the
	# build system.
	if (USELTO AND ${CLANG_VERSION} GREATER 0)
		# Check to see what linker type is in use...
		execute_process(
			COMMAND sh -c "ld --version"
			OUTPUT_VARIABLE LD_VER
		)
		# If the system default linker is indeed ld.bfd or ld.mold, then try
		# and change for another, clang-LTO-compatible linker...
		if (LD_VER MATCHES "^GNU ld.*" OR LD_VER MATCHES "^mold.*")
			# Give the priority to lld, since it should provide full clang
			# compatibility...
			find_program(LLD ld.lld)
			if (LLD)
				set(SELECTED_LINKER "lld")
			else (LLD)
				# Then try ld.gold, which is most often present on Linux
				# systems and a well known, LTO-capable linker.
				find_program(GOLD ld.gold)
				if (GOLD)
					set(SELECTED_LINKER "gold")
				else (GOLD)
					message(WARNING "Could not find either of ld.lld or ld.gold linkers: linking may fail when using ld.bfd or ld.mold together with clang and LTO enabled...")
				endif (GOLD)
			endif (LLD)
		endif ()
		if (SELECTED_LINKER)
			# clang is *stupidly* warning about "unused command line argument"
			# when not in the linking phase and the "-fuse-ld=" option was
			# passed... So we need to add -Wno-unused-command-line-argument. HB
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-command-line-argument -fuse-ld=${SELECTED_LINKER}")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -fuse-ld=${SELECTED_LINKER}")
		endif (SELECTED_LINKER)
	endif (USELTO AND ${CLANG_VERSION} GREATER 0)

	add_definitions(-DLL_LINUX=1)

	set(CMAKE_C_FLAGS_DEBUG "-fno-inline -D_DEBUG -DLL_DEBUG=1")
	set(CMAKE_CXX_FLAGS_DEBUG "-fno-inline -D_DEBUG -DLL_DEBUG=1")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "-fno-inline -DLL_NO_FORCE_INLINE=1 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-fno-inline -DLL_NO_FORCE_INLINE=1 -DNDEBUG")
	set(CMAKE_C_FLAGS_RELEASE "${OPT_FLAGS} -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${OPT_FLAGS} -DNDEBUG")

	set(IGNORED_C_WARNINGS "-Wall ${IGNORED_C_WARNINGS}")
	set(IGNORED_CXX_WARNINGS "-Wall -Wno-reorder ${IGNORED_CXX_WARNINGS}")

	if (NOT NO_FATAL_WARNINGS)
		set(IGNORED_C_WARNINGS "${IGNORED_C_WARNINGS} -Werror")
		set(IGNORED_CXX_WARNINGS "${IGNORED_CXX_WARNINGS} -Werror")
	endif (NOT NO_FATAL_WARNINGS)

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IGNORED_C_WARNINGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORED_CXX_WARNINGS}")
endif (LINUX)

if (WINDOWS)
	set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
		"Supported build types." FORCE)

	# Remove default /Zm1000 flag that cmake inserts
	string (REPLACE "/Zm1000" " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

	# Do not build DLLs.
	set(BUILD_SHARED_LIBS OFF)

	# SSE2 is always enabled by default for 64 bits targets
	set(COMPILER_MATHS "/fp:fast")
	if (USEAVX2)
		set(COMPILER_MATHS "/arch:AVX2 /fp:fast")
	elseif (USEAVX)
		set(COMPILER_MATHS "/arch:AVX /fp:fast")
	endif ()

	# Compilation flags common to all build types
	if (USING_CLANG)
		set(IGNORED_WARNINGS "-Wno-deprecated-declarations -Wno-reorder-ctor -Wno-inconsistent-dllimport -Wno-dll-attribute-on-redeclaration -Wno-ignored-pragma-optimize -Wno-writable-strings -Wno-unused-function -Wno-unused-local-typedef -Wno-unknown-warning-option")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHs ${COMPILER_MATHS} -m64 /MP /W2 /c")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /EHs ${COMPILER_MATHS} -m64 -fno-strict-aliasing -fno-delete-null-pointer-checks /MP /TP /W2 /c ${IGNORED_WARNINGS}")
	else (USING_CLANG)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHs ${COMPILER_MATHS} /MP /W2 /c /nologo")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /EHs ${COMPILER_MATHS} /MP /TP /W2 /c /nologo")
	endif (USING_CLANG)

	# Fix for a bug in cmake v3.30 which omits /MD...
	if (${CMAKE_VERSION} VERSION_GREATER 3.29.99)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MD")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD")
	endif (${CMAKE_VERSION} VERSION_GREATER 3.29.99)

	# Not strictly stack protection, but buffer overrun detection/protection
	if (PROTECTSTACK)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GS")
	else (PROTECTSTACK)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS- /guard:cf-")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GS- /guard:cf-")
	endif (PROTECTSTACK)

	# Never add thread-locking code to local static variables (we never use
	# non-thread_local static local variables in thread-sensitive code):
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:threadSafeInit-")

	# OpenMP support, if requested
	if (OPENMP)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp")
	endif (OPENMP)

	# Build type and compiler dependent compilation flags
	if (USING_CLANG)
		set(CMAKE_CXX_FLAGS_RELEASE "/clang:-O3 /DNDEBUG /D_SECURE_SCL=0 /D_HAS_ITERATOR_DEBUGGING=0"
			CACHE STRING "C++ compiler release options" FORCE)
	else (USING_CLANG)
		set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob3 /DNDEBUG /D_SECURE_SCL=0 /D_HAS_ITERATOR_DEBUGGING=0"
			CACHE STRING "C++ compiler release options" FORCE)
	endif (USING_CLANG)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

	# zlib has assembly-language object files incompatible with SAFESEH
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

	set(CMAKE_CXX_STANDARD_LIBRARIES "")
	set(CMAKE_C_STANDARD_LIBRARIES "")

	add_definitions(/DLL_WINDOWS=1 /DUNICODE /D_UNICODE)

	# See: http://msdn.microsoft.com/en-us/library/aa383745%28v=VS.85%29.aspx
	# Configure the win32 API for Windows 10 compatibility
	set(WINVER "0x0A00" CACHE STRING "Win32 API target version")
	add_definitions("/DWINVER=${WINVER}" "/D_WIN32_WINNT=${WINVER}")

	if (NOT NO_FATAL_WARNINGS)
		add_definitions(/WX)
	endif ()
endif (WINDOWS)

# Add link-time optimization if requested, enabling it via cmake (which knows
# what compilers support it and what option(s) to use). Note that only cmake
# v3.9 and newer can deal with LTO for gcc and clang, and cmake v3.13 or newer
# for MSVC.
# From my experiments, LTO does not make the resulting binary any faster with
# gcc (and actually slower with gcc versions below 10)... I suspect that gcc's
# LTO causes some normally (and flagged to be) inlined functions to get wrongly
# factorized into normal functions... Clang 12 and newer, however, does take
# benefit from LTO to produce faster viewer binaries.
if (USELTO AND ${CMAKE_VERSION} VERSION_GREATER 3.8)
	if (NOT WINDOWS OR ${CMAKE_VERSION} VERSION_GREATER 3.12)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif ()
endif ()

# Add defines corresponding to compilation options
if (OPENMP)
	add_definitions(-DLL_OPENMP=1)
endif (OPENMP)

if (USE_JEMALLOC)
	add_definitions(-DLL_JEMALLOC=1)
elseif (USE_MIMALLOC)
	add_definitions(-DLL_MIMALLOC=1)
endif ()

if (NO_PHMAP)
  add_definitions(-DLL_NO_PHMAP=1)
endif (NO_PHMAP)

if (ENABLE_PUPPETRY)
  add_definitions(-DLL_PUPPETRY=1)
endif (ENABLE_PUPPETRY)

if (USE_TRACY)
  if (TRACY_WITH_FAST_TIMERS)
    if (TRACY_MEMORY)
      add_definitions(-DTRACY_ENABLE=4)
    else (TRACY_MEMORY)
      add_definitions(-DTRACY_ENABLE=3)
    endif (TRACY_MEMORY)
  else (TRACY_WITH_FAST_TIMERS)
    if (TRACY_MEMORY)
      add_definitions(-DTRACY_ENABLE=2)
    else (TRACY_MEMORY)
      add_definitions(-DTRACY_ENABLE=1)
    endif (TRACY_MEMORY)
  endif (TRACY_WITH_FAST_TIMERS)
endif (USE_TRACY)

if (WINDOWS AND USE_NETBIOS)
  add_definitions(-DLL_NETBIOS=1)
endif (WINDOWS AND USE_NETBIOS)
