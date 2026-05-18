# -*- cmake -*-
if (VIEWERMISCLIBS_CMAKE_INCLUDED)
  return()
endif (VIEWERMISCLIBS_CMAKE_INCLUDED)
set (VIEWERMISCLIBS_CMAKE_INCLUDED TRUE)

include(00-BuildOptions)
include(Prebuilt)

# SSE2 to Neon conversion header is needed for arm64 builds
if (ARCH STREQUAL "arm64")
	use_prebuilt_binary(sse2neon)
endif ()

# *TODO: check for libuuid devel files when USESYSTEMLIBS
if (LINUX AND (USEPREBUILTUUID OR NOT USESYSTEMLIBS))
	use_prebuilt_binary(libuuid)
endif ()
