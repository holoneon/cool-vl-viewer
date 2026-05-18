# -*- cmake -*-

if (JSON_CMAKE_INCLUDED)
  return()
endif (JSON_CMAKE_INCLUDED)
set (JSON_CMAKE_INCLUDED TRUE)

include(Prebuilt)
use_prebuilt_binary(json)

include_directories(SYSTEM ${LIBS_PREBUILT_DIR}/include)
