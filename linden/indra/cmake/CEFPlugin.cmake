# -*- cmake -*-
if (CEFPLUGIN_CMAKE_INCLUDED)
  return()
endif (CEFPLUGIN_CMAKE_INCLUDED)
set (CEFPLUGIN_CMAKE_INCLUDED TRUE)

include(00-BuildOptions)
include(Prebuilt)

use_prebuilt_binary(dullahan)

if (LINUX)
  set(CEF_PLUGIN_LIBRARIES
      -Wl,-whole-archive
      ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.a
      ${ARCH_PREBUILT_DIRS_RELEASE}/libdullahan.a
      -Wl,-no-whole-archive
      ${ARCH_PREBUILT_DIRS_RELEASE}/libcef.so
  )
elseif (WINDOWS)
  set(CEF_PLUGIN_LIBRARIES
      libcef.lib
      libcef_dll_wrapper.lib
      dullahan.lib
  )
endif ()

include_directories(${LIBS_PREBUILT_DIR}/include/cef)
