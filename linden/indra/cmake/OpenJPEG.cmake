# -*- cmake -*-
if (OPENJPEG_CMAKE_INCLUDED)
  return()
endif (OPENJPEG_CMAKE_INCLUDED)
set (OPENJPEG_CMAKE_INCLUDED TRUE)


include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/libopenjpeg)
set(OPENJPEG_LIBRARIES openjpeg)
