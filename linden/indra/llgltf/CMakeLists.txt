# -*- cmake -*-

project(llgltf)

include(00-Common)
include(GLM)
include(LLCommon)
include(LLAppearance)
include(LLCharacter)
include(LLFilesystem)
include(LLInventory)
include(LLMath)
include(LLPrimitive)
include(LLRender)
include(LLXML)
include(Json)
# We need GL_* constants for the glTF mesh loader, so let's add the include
# path for Epoxy headers... HB
include(Epoxy)

set(llgltf_SOURCE_FILES
	llgltfaccessor.cpp
	llgltfanimation.cpp
	llgltfasset.cpp
	llgltfbufferutil.cpp
	llgltfjointdata.cpp
	llgltfloader.cpp
	llgltfprimitive.cpp
    )

set(llgltf_HEADER_FILES
    CMakeLists.txt
	llgltfaccessor.h
	llgltfanimation.h
	llgltfasset.h
	llgltfbufferutil.h
	llgltfglm.h
	llgltfjointdata.h
	llgltfloader.h
 	llgltfprimitive.h
   )

set_source_files_properties(${llgltf_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llgltf_SOURCE_FILES ${llgltf_HEADER_FILES})

add_library (llgltf ${llgltf_SOURCE_FILES})
add_dependencies(llgltf prepare)

target_link_libraries(llgltf
    ${LLCOMMON_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLPRIMITIVE_LIBRARIES}
    ${LLAPPEARANCE_LIBRARIES}
    ${LLCHARACTER_LIBRARIES}
    ${LLRENDER_LIBRARIES}
    )

target_precompile_headers(llgltf PRIVATE <linden_common.h> <json.hpp> llgltfglm.h)
if (USEUNITYBUILD)
  set_target_properties(llgltf PROPERTIES UNITY_BUILD ON)
endif (USEUNITYBUILD)

get_directory_property(ALLDEFINES COMPILE_DEFINITIONS)
message("llgltf COMPILE_DEFINITIONS = ${ALLDEFINES}")
