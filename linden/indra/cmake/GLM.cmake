# -*- cmake -*-

if (GLM_CMAKE_INCLUDED)
  return()
endif (GLM_CMAKE_INCLUDED)
set (GLM_CMAKE_INCLUDED TRUE)

include(Variables)

include(Prebuilt)

# NOTE: with VS2022, glm v1.0.2 fails to compile with the following error:
# include\glm\detail\type_vec4.inl(468,82): error C2039: 'call' is not a member
# of 'glm::detail::compute_vec_mul<4,float,glm::aligned_highp,true>'
# So we use the (patched) glm v1.0.1 for Windoze... HB
if (WINDOWS)
	use_prebuilt_binary(glm-old)
else (WINDOWS)
	use_prebuilt_binary(glm)
endif (WINDOWS)

add_definitions(-DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES=1
				-DGLM_ENABLE_EXPERIMENTAL=1)
# GLM does not have proper NEON support, so we use sse2neon.h translations and
# we force SSE2 intrinsics usage by GLM. HB
if (ARCH STREQUAL "arm64")
	add_definitions(-DGLM_FORCE_ARCH_UNKNOWN=1 -DGLM_FORCE_SSE2=1)
endif (ARCH STREQUAL "arm64")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_definitions(-DGLM_FORCE_INLINE=1)
endif (CMAKE_BUILD_TYPE STREQUAL "Release")
