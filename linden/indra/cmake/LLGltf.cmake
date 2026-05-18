# -*- cmake -*-
if (LLGLTF_CMAKE_INCLUDED)
  return()
endif (LLGLTF_CMAKE_INCLUDED)
set (LLGLTF_CMAKE_INCLUDED TRUE)

include(00-BuildOptions)

include_directories(${CMAKE_SOURCE_DIR}/llgltf)
set(LLGLTF_LIBRARIES llgltf)
