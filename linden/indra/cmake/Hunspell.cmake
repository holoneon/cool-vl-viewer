# -*- cmake -*-
if (HUNSPELL_CMAKE_INCLUDED)
  return()
endif (HUNSPELL_CMAKE_INCLUDED)
set (HUNSPELL_CMAKE_INCLUDED TRUE)

include(Prebuilt)

if (USESYSTEMLIBS AND NOT USEPREBUILTHUNSPELL)
  set(HUNSPELL_FIND_QUIETLY OFF)
  set(HUNSPELL_FIND_REQUIRED OFF)
  include(FindHunSpell)
endif (USESYSTEMLIBS AND NOT USEPREBUILTHUNSPELL)

if (NOT HUNSPELL_FOUND)
  use_prebuilt_binary(hunspell)

  set(HUNSPELL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/hunspell)

  if (LINUX)
    set(HUNSPELL_LIBRARY hunspell-1.7)
  elseif (WINDOWS)
    set(HUNSPELL_LIBRARY libhunspell)
    add_definitions(-DHUNSPELL_STATIC=1)
  endif ()
endif (NOT HUNSPELL_FOUND)

include_directories(SYSTEM ${HUNSPELL_INCLUDE_DIR})
