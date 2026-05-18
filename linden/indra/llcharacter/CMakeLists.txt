# -*- cmake -*-

project(llcharacter)

include(00-Common)
include(LLCommon)
include(LLFilesystem)
include(LLMath)
include(LLPrimitive)
include(LLMessage)
include(LLXML)

set(llcharacter_SOURCE_FILES
    llanimationstates.cpp
    llbvhloader.cpp
    llcharacter.cpp
    lleditingmotion.cpp
    llemote.cpp
    llgesture.cpp
    llhandmotion.cpp
    llheadrotmotion.cpp
    lljoint.cpp
    lljointsolverrp3.cpp
    llkeyframefallmotion.cpp
    llkeyframemotion.cpp
    llkeyframestandmotion.cpp
    llkeyframewalkmotion.cpp
    llmotion.cpp
    llmotioncontroller.cpp
    llmultigesture.cpp
    llpose.cpp
    lltargetingmotion.cpp
    llvisualparam.cpp
    )

set(llcharacter_HEADER_FILES
    CMakeLists.txt

    llanimationstates.h
    llbvhloader.h
    llbvhconsts.h
    llcharacter.h
    lleditingmotion.h
    llemote.h
    llgesture.h
    llhandmotion.h
    llheadrotmotion.h
    lljoint.h
    lljointsolverrp3.h
    lljointstate.h
    llkeyframefallmotion.h
    llkeyframemotion.h
    llkeyframestandmotion.h
    llkeyframewalkmotion.h
    llmotion.h
    llmotioncontroller.h
    llmultigesture.h
    llpose.h
    lltargetingmotion.h
    llvisualparam.h
    )

# LLIK is (at least for now) only used by the Puppetry code and this project
# has been withdrawn by LL. LLIK is therefore only built when puppetry is
# itself enabled at build time, which is not the case any more by default... HB
if (ENABLE_PUPPETRY)
  list(APPEND llcharacter_SOURCE_FILES llik.cpp)
  list(APPEND llcharacter_HEADER_FILES llik.h)
endif (ENABLE_PUPPETRY)

set_source_files_properties(${llcharacter_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llcharacter_SOURCE_FILES ${llcharacter_HEADER_FILES})

add_library (llcharacter ${llcharacter_SOURCE_FILES})
add_dependencies(llcharacter prepare)

target_precompile_headers(llcharacter PRIVATE <linden_common.h> <llfasttimer.h> <hbfastmap.h>)
if (USEUNITYBUILD)
  set_target_properties(llcharacter PROPERTIES UNITY_BUILD ON)
endif (USEUNITYBUILD)

get_directory_property(ALLDEFINES COMPILE_DEFINITIONS)
message("llcharacter COMPILE_DEFINITIONS = ${ALLDEFINES}")
