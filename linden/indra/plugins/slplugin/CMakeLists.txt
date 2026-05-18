project(SLPlugin)

include(00-Common)
include(jemalloc)
include(LLCommon)
include(LLPlugin)
include(Linking)
include(LLMessage)

### SLPlugin

set(SLPlugin_SOURCE_FILES
	slplugin.cpp
	)

if (WINDOWS)
	add_executable(SLPlugin
		WIN32
		${SLPlugin_SOURCE_FILES}
	)
	set_target_properties(SLPlugin
		PROPERTIES
		LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
		LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMTD"
	)
elseif (LINUX)
	add_executable(SLPlugin
		${SLPlugin_SOURCE_FILES}
	)
endif ()

target_link_libraries(SLPlugin
	# Make sure MIMALLOC_* appear first in the list of target link libraries
	${MIMALLOC_LIBRARY}
	${MIMALLOC_OBJECT}
	${LLPLUGIN_LIBRARIES}
	${LLMESSAGE_LIBRARIES}
	${LLCOMMON_LIBRARIES}
	${PLUGIN_API_LIBRARIES}
	${JEMALLOC_LIBRARY}
	${LEGACY_STDIO_LIBS}
)

get_directory_property(ALLDEFINES COMPILE_DEFINITIONS)
message("slplugin COMPILE_DEFINITIONS = ${ALLDEFINES}")
