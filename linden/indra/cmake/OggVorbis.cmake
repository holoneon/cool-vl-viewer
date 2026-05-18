# -*- cmake -*-
if (OGGVORBIS_CMAKE_INCLUDED)
  return()
endif (OGGVORBIS_CMAKE_INCLUDED)
set (OGGVORBIS_CMAKE_INCLUDED TRUE)

include(Prebuilt)

if (USESYSTEMLIBS AND NOT USEPREBUILTOGG)
  include(FindPkgConfig)
  pkg_check_modules(OGG ogg)
  pkg_check_modules(VORBIS vorbis)
  pkg_check_modules(VORBISENC vorbisenc)
  pkg_check_modules(VORBISFILE vorbisfile)
endif (USESYSTEMLIBS AND NOT USEPREBUILTOGG)

if (NOT OGG_FOUND OR NOT VORBIS_FOUND OR NOT VORBISENC_FOUND OR NOT VORBISFILE_FOUND)
  use_prebuilt_binary(ogg-vorbis)
  set(VORBIS_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)

  if (WINDOWS)
    set(OGG_LIBRARIES libogg.lib)
    set(VORBIS_LIBRARIES libvorbis.lib)
    set(VORBISENC_LIBRARIES libvorbisenc.lib)
    set(VORBISFILE_LIBRARIES libvorbisfile.lib)
  else (WINDOWS)
    set(OGG_LIBRARIES ogg)
    set(VORBIS_LIBRARIES vorbis)
    set(VORBISENC_LIBRARIES vorbisenc)
    set(VORBISFILE_LIBRARIES vorbisfile)
  endif (WINDOWS)
endif (NOT OGG_FOUND OR NOT VORBIS_FOUND OR NOT VORBISENC_FOUND OR NOT VORBISFILE_FOUND)

if (NOT WINDOWS)
  add_definitions(-DOV_EXCLUDE_STATIC_CALLBACKS)
endif (NOT WINDOWS)

link_directories(
    ${VORBIS_LIBRARY_DIRS}
    ${VORBISENC_LIBRARY_DIRS}
    ${VORBISFILE_LIBRARY_DIRS}
    ${OGG_LIBRARY_DIRS}
    )

include_directories(SYSTEM
    ${VORBIS_INCLUDE_DIRS}
    ${VORBISENC_INCLUDE_DIRS}
    ${VORBISFILE_INCLUDE_DIRS}
    )
