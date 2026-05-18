# -*- cmake -*-
if (PYTHON_CMAKE_INCLUDED)
  return()
endif (PYTHON_CMAKE_INCLUDED)
set (PYTHON_CMAKE_INCLUDED TRUE)

set(PYTHONINTERP_FOUND)

if (WINDOWS)
	# On Windows, explicitly avoid Cygwin Python.
	foreach(hive HKEY_CURRENT_USER HKEY_LOCAL_MACHINE)
		foreach(pyver 3.14 3.13 3.12 3.11 3.10 3.9 3.8 3.7 3.6 3.5 3.4 3.3)
			list(APPEND regpaths "[${hive}\\SOFTWARE\\Python\\PythonCore\\${pyver}\\InstallPath]")
		endforeach()
	endforeach()

	find_program(PYTHON_EXECUTABLE
		NAMES python3.exe python.exe
		NO_DEFAULT_PATH # added so that cmake does not find cygwin python
		PATHS
		${regpaths}
    )
elseif (LINUX)
	string(REPLACE ":" ";" PATH_LIST "$ENV{PATH}")
	find_program(PYTHON_EXECUTABLE python3.14 python3.13 python3.12 python3.11 python3.10 python3.9 python3.8 python3.7 python3.6 python3.5 python3.4 python3.3 python3 python
				 PATHS ${PATH_LIST})
endif ()

if (PYTHON_EXECUTABLE)
	set(PYTHONINTERP_FOUND ON)
endif (PYTHON_EXECUTABLE)

if (NOT PYTHONINTERP_FOUND)
  message(FATAL_ERROR "No compatible Python interpreter found !")
endif (NOT PYTHONINTERP_FOUND)

mark_as_advanced(PYTHON_EXECUTABLE)
message("-- Using Python executable: ${PYTHON_EXECUTABLE}")
