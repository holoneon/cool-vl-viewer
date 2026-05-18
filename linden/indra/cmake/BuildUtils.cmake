# -*- cmake -*-
if (BUILDUTILS_CMAKE_INCLUDED)
  return()
endif (BUILDUTILS_CMAKE_INCLUDED)
set (BUILDUTILS_CMAKE_INCLUDED TRUE)

if (LINUX)
	return()
endif (LINUX)

include(Prebuilt)

# We need zstd to uncompress LL's pre-built libraries without requiring the
# installation of the pyzstd Python module on the build system... HB
use_prebuilt_binary(zstd)
