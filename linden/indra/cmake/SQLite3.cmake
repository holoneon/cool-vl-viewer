# -*- cmake -*-
if (SQLite3_CMAKE_INCLUDED)
  return()
endif (SQLite3_CMAKE_INCLUDED)
set (SQLite3_CMAKE_INCLUDED TRUE)

include(Prebuilt)

set(SQLite3_FIND_QUIETLY ON)
set(SQLite3_FIND_REQUIRED OFF)

if (USESYSTEMLIBS AND NOT USEPREBUILTSQLITE)
  include(FindSQLite3)
endif (USESYSTEMLIBS AND NOT USEPREBUILTSQLITE)

if (NOT SQLite3_FOUND)
  use_prebuilt_binary(libsqlite3)
  set(SQLite3_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  if (LINUX)
    set(SQLite3_LIBRARIES ${LIBS_PREBUILT_DIR}/lib/release/libsqlite3.a)
  elseif (WINDOWS)
    set(SQLite3_LIBRARIES ${LIBS_PREBUILT_DIR}/lib/release/sqlite3.lib)
  endif (LINUX)
  set(SQLite3_FOUND "YES")
endif (NOT SQLite3_FOUND)

include_directories(SYSTEM SQLite3_INCLUDE_DIRS)
