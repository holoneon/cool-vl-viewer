# -*- cmake -*-

project(voice_plugin_webrtc)

include(00-Common)
include(LLCommon)
include(LLPlugin)
include(Linking)
include(MediaPluginBase)
include(WebRTC)

### voice_plugin_webrtc

if (NOT WINDOWS)
	add_definitions(-fPIC)
endif (NOT WINDOWS)

set(voice_plugin_webrtc_SOURCE_FILES
	voice_plugin_webrtc.cpp
)

add_library(voice_plugin_webrtc
 	SHARED
	${voice_plugin_webrtc_SOURCE_FILES}
)

if (LINUX)
  set_target_properties(voice_plugin_webrtc
      # Do not prepend 'lib' to the executable name
      PROPERTIES
      PREFIX ""
  )
endif (LINUX)

if (WINDOWS)
	set_target_properties(voice_plugin_webrtc
		PROPERTIES
		LINK_FLAGS "/MANIFEST:NO /SAFESEH:NO /NODEFAULTLIB:LIBCMT"
	)
endif (WINDOWS)

target_link_libraries(voice_plugin_webrtc
	${LLPLUGIN_LIBRARIES}
	${MEDIA_PLUGIN_BASE_LIBRARIES}
	${LLCOMMON_LIBRARIES}
	${WEBRTC_LIBRARY}
	${PLUGIN_API_LIBRARIES}
)

get_directory_property(ALLDEFINES COMPILE_DEFINITIONS)
message("plugins/webrtc COMPILE_DEFINITIONS = ${ALLDEFINES}")

