# -*- cmake -*-
if (WEBRTC_CMAKE_INCLUDED)
  return()
endif (WEBRTC_CMAKE_INCLUDED)
set (WEBRTC_CMAKE_INCLUDED TRUE)

include(Prebuilt)

use_prebuilt_binary(llwebrtc)

if (LINUX)
	set(WEBRTC_LIBRARY llwebrtc.so)
elseif (WINDOWS)
	set(WEBRTC_LIBRARY llwebrtc.lib)
endif ()
