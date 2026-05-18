# -*- cmake -*-
if (XML2_CMAKE_INCLUDED)
	return()
endif (XML2_CMAKE_INCLUDED)
set (XML2_CMAKE_INCLUDED TRUE)

# NOTE: we need the *static* libxml2 library for ColladaDOM, and this is the
# one and only purpose of using that library in the viewer. HB

include(Prebuilt)
use_prebuilt_binary(libxml2)

if (LINUX)
	# Make sure we will link against our static library
	set(XML2_LIBRARIES ${ARCH_PREBUILT_DIRS_RELEASE}/libxml2.a)
elseif (WINDOWS)
	set(XML2_LIBRARIES libxml2)
endif ()

# We do not actually use libxml2 headers in the viewer istelf. HB
#include_directories(SYSTEM ${LIBS_PREBUILT_DIR}/include/libxml2)
