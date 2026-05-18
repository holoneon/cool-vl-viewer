# -*- cmake -*-
if (HACD_CMAKE_INCLUDED)
  return()
endif (HACD_CMAKE_INCLUDED)
set (HACD_CMAKE_INCLUDED TRUE)

include(00-BuildOptions)
include(Prebuilt)

# We build both the HACD and VHACD decompositions, and the user can configure
# what they want to use... HB
use_prebuilt_binary(hacd)
use_prebuilt_binary(vhacd)

set(HACD_LIBRARY hacd)
include_directories(SYSTEM ${LIBS_PREBUILT_DIR}/include/hacd)
