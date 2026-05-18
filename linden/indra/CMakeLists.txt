# -*- cmake -*-

# cmake_minimum_required should appear before any other commands to guarantee
# full compatibility with the version specified.
cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)
cmake_policy(VERSION 3.16.0)

set(ROOT_PROJECT_NAME "CoolVLViewer" CACHE STRING
    "The root project/makefile/solution name. Defaults to CoolVLViewer.")
project(${ROOT_PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Variables)

# Uncomment this to enable pre-built packages download from the Cool VL Viewer
# mirror site (should the main site be down). Mainly relevant to Windows
# builds (for Linux, simply use the corresponding option in the linux-build.sh
# script, which will pass the appropriate boolean defines to cmake).
#set(USE_MIRROR ON)

# Load versions now. Install locations need them.
include(BuildVersion)
build_version(viewer)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Create a 'prepare' target in which to perform setup actions. This must be the
# first target created so other targets can depend on it.
set(prepare_depends ${CMAKE_BINARY_DIR}/prepare/prebuilt)

add_custom_target(prepare DEPENDS ${prepare_depends})

add_subdirectory(cmake)
add_subdirectory(libopenjpeg)
add_subdirectory(llappearance)
add_subdirectory(llaudio)
add_subdirectory(llcharacter)
add_subdirectory(llcommon)
add_subdirectory(llfilesystem)
add_subdirectory(llgltf)
add_subdirectory(llimage)
add_subdirectory(llinventory)
add_subdirectory(llmath)
add_subdirectory(llmessage)
add_subdirectory(llplugin)
add_subdirectory(llprimitive)
add_subdirectory(llrender)
add_subdirectory(llui)
add_subdirectory(llwindow)
add_subdirectory(llxml)

add_custom_target(viewer)

# Viewer media plugins
add_subdirectory(plugins)

add_subdirectory(newview)
add_dependencies(viewer CoolVLViewer)

# Configure prebuilt binary download. This must be done last so that all
# subdirectories have a chance to list the packages they need.

# The use_prebuilt_binary macro in cmake/Prebuilt.cmake records packages in
# the PREBUILT property of the 'prepare' target.
get_property(PREBUILT_PACKAGES TARGET prepare PROPERTY PREBUILT)

# Create a script to download the needed binaries.
configure_file(${CMAKE_SOURCE_DIR}/cmake/DownloadPrebuilt.cmake.in
			   ${CMAKE_BINARY_DIR}/DownloadPrebuilt.cmake @ONLY)

# Drive the download script at build time. Depend on 'install.xml' to acquire
# new binaries when needed.
add_custom_command(
	COMMENT "Obtaining prebuilt binaries..."
	OUTPUT ${CMAKE_BINARY_DIR}/prepare/prebuilt
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/DownloadPrebuilt.cmake
	DEPENDS ${CMAKE_SOURCE_DIR}/../install.xml
			${CMAKE_BINARY_DIR}/DownloadPrebuilt.cmake
)
