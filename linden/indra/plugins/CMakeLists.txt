# -*- cmake -*-

# Ensure Tracy support is off for plugins and SLPlugin, to avoid crashes in
# them; seen occurring at least under Windoze; Linux is fine.
# But we do not really care about SLPlugin profiling anyway... HB
include(00-BuildOptions)
set(USE_TRACY OFF)

# Plugins common to all OSes
add_subdirectory(base)
add_subdirectory(cef)
add_subdirectory(gstreamer)
add_subdirectory(webrtc)

# NOTE: for now, VLC is used only for Windows builds while Linux and macOS only
# need aand use Gstreamer. Sadly, starting with Windows 11 24H2 and/or newer
# VS2022 runtime libraries, Gstreamer stopped working properly (likely a runtime
# clash with Gstreamer's and/or something fishy with the shared memory used by
# plugins). HB
if (WINDOWS)
	add_subdirectory(vlc)
endif (WINDOWS)

# Plugin host program
add_subdirectory(slplugin)
