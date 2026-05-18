# -*- cmake -*-

project(media_plugin_cef)

include(00-Common)
include(LLCommon)
include(LLFilesystem)
include(LLPlugin)
include(LLMath)
include(LLRender)
include(LLWindow)
include(Linking)
include(MediaPluginBase)
include(FindOpenGL)
include(SQLite3)

include(CEFPlugin)

### media_plugin_cef

if (NOT WINDOWS)
  add_definitions(-fPIC)
endif (NOT WINDOWS)

set(media_plugin_cef_SOURCE_FILES
    media_plugin_cef.cpp
    hbcookiesmerger.cpp
)
set(media_plugin_cef_HEADER_FILES
    hbcookiesmerger.h)
set_source_files_properties(${media_plugin_cef_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(media_plugin_cef
    SHARED
    ${media_plugin_cef_SOURCE_FILES}
)

if (LINUX)
  # Do not prepend 'lib' to the executable name
  set_target_properties(media_plugin_cef
    PROPERTIES
    PREFIX ""
  )
endif (LINUX)

if (WINDOWS)
  set_target_properties(media_plugin_cef
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO /SAFESEH:NO /NODEFAULTLIB:LIBCMT"
    LINK_FLAGS_DEBUG "/MANIFEST:NO /SAFESEH:NO /NODEFAULTLIB:LIBCMTD"
  )
endif (WINDOWS)

target_link_libraries(media_plugin_cef
  ${SQLite3_LIBRARIES}
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${CEF_PLUGIN_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${LLFILESYSTEM_LIBRARIES}
  ${PLUGIN_API_LIBRARIES}
)

get_directory_property(ALLDEFINES COMPILE_DEFINITIONS)
message("plugins/cef COMPILE_DEFINITIONS = ${ALLDEFINES}")
