# -*- cmake -*-

project(media_plugin_vlc)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLPlugin)
include(LLMath)
include(LLRender)
include(LLWindow)
include(Linking)
include(MediaPluginBase)
include(FindOpenGL)
include(VLCPlugin)

### media_plugin_vlc

if (NOT WINDOWS)
  add_definitions(-fPIC)
endif (NOT WINDOWS)

set(media_plugin_vlc_SOURCE_FILES
    media_plugin_vlc.cpp
    )

add_library(media_plugin_vlc
    SHARED
    ${media_plugin_vlc_SOURCE_FILES}
)

if (LINUX)
  set_target_properties(media_plugin_vlc
      # Do not prepend 'lib' to the executable name
      PROPERTIES
      PREFIX ""
  )
endif (LINUX)

if (WINDOWS)
  set_target_properties(media_plugin_vlc
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO /SAFESEH:NO /NODEFAULTLIB:LIBCMT"
    )
endif (WINDOWS)

target_link_libraries(media_plugin_vlc
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${VLC_LIBRARIES}
  ${PLUGIN_API_LIBRARIES}
)

get_directory_property(ALLDEFINES COMPILE_DEFINITIONS)
message("plugins/vlc COMPILE_DEFINITIONS = ${ALLDEFINES}")

