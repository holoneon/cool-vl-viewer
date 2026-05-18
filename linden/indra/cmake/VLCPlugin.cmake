# -*- cmake -*-
if (VLC_CMAKE_INCLUDED)
  return()
endif (VLC_CMAKE_INCLUDED)
set (VLC_CMAKE_INCLUDED TRUE)

include(Linking)
include(Prebuilt)

use_prebuilt_binary(libvlc)

if (LINUX)
    # Specify a full path to make sure we get a static link
    set(VLC_LIBRARIES
        ${LIBS_PREBUILT_DIR}/lib/libvlc.a
        ${LIBS_PREBUILT_DIR}/lib/libvlccore.a
    )
elseif (WINDOWS)
    set(VLC_LIBRARIES libvlc.lib libvlccore.lib)
endif ()
