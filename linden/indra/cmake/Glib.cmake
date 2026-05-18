# -*- cmake -*-
if (GLIB_CMAKE_INCLUDED)
  return()
endif (GLIB_CMAKE_INCLUDED)
set (GLIB_CMAKE_INCLUDED TRUE)

include(Prebuilt)

set(GLIB_LIBRARIES "")
if (LINUX AND USESYSTEMLIBS AND NOT USEPREBUILTGLIB)
  include(FindPkgConfig)

  set(PKGCONFIG_PACKAGES
      gio-2.0
      gobject-2.0
      gmodule-2.0
      glib-2.0
      gthread-2.0
    )
  foreach(pkg ${PKGCONFIG_PACKAGES})
    pkg_check_modules(${pkg} REQUIRED ${pkg})
    include_directories(SYSTEM ${${pkg}_INCLUDE_DIRS})
    link_directories(${${pkg}_LIBRARY_DIRS})
    list(APPEND GLIB_LIBRARIES ${${pkg}_LIBRARIES})
    add_definitions(${${pkg}_CFLAGS_OTHERS})
  endforeach(pkg)
else (LINUX AND USESYSTEMLIBS AND NOT USEPREBUILTGLIB)
  if (LINUX)
    use_prebuilt_binary(glib)
    list(APPEND GLIB_LIBRARIES
        gio-2.0
        gobject-2.0
        gmodule-2.0
        glib-2.0
        gthread-2.0
    )
	include_directories(SYSTEM ${LIBS_PREBUILT_DIR}/include/glib-2.0)
  endif (LINUX)
endif (LINUX AND USESYSTEMLIBS AND NOT USEPREBUILTGLIB)
