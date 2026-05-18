# -*- cmake -*-
if (LLPRIMITIVE_CMAKE_INCLUDED)
  return()
endif (LLPRIMITIVE_CMAKE_INCLUDED)
set (LLPRIMITIVE_CMAKE_INCLUDED TRUE)

# These should be moved to their own cmake file
use_prebuilt_binary(colladadom)
use_prebuilt_binary(meshoptimizer)
use_prebuilt_binary(mikktspace)
use_prebuilt_binary(tinygltf)

include_directories(
    ${CMAKE_SOURCE_DIR}/llprimitive
    ${LIBS_PREBUILT_DIR}/include/collada
    ${LIBS_PREBUILT_DIR}/include/collada/1.4
    )

if (WINDOWS)
	set(COLLADADOM_LIBRARY libcollada14dom23-s)
else (WINDOWS)
	set(COLLADADOM_LIBRARY collada14dom)
endif (WINDOWS)

set(LLPRIMITIVE_LIBRARIES llprimitive)
