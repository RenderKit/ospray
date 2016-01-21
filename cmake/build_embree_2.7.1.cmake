## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

option(OSPRAY_INTERNAL_EMBREE_CONFIG ON)
mark_as_advanced(OSPRAY_INTERNAL_EMBREE_CONFIG)

if (OSPRAY_INTERNAL_EMBREE_CONFIG)
  if (THIS_IS_MIC)
    include(${CMAKE_CURRENT_LIST_DIR}/build_embree_xeonphi.cmake)
  else ()
    include(${CMAKE_CURRENT_LIST_DIR}/build_embree_xeon.cmake)
  endif()
else(OSPRAY_INTERNAL_EMBREE_CONFIG)
  # Options for building embree
  #
  # NOTE(jda) - All of these variables are pubic to the embree project, we hide
  #             them intentionally from OSPRay's CMake configuration and force
  #             values into the CMake cache so the underlying cached variables
  #             in the embree CMakeLists.txt are picked up based on OSPRay's
  #             configuration.
  set(ENABLE_INSTALLER    OFF CACHE INTERNAL "" FORCE)
  set(ENABLE_ISPC_SUPPORT ON  CACHE INTERNAL "" FORCE)
  set(ENABLE_STATIC_LIB   OFF CACHE INTERNAL "" FORCE)
  set(ENABLE_TUTORIALS    OFF CACHE INTERNAL "" FORCE)

  set(RTCORE_RAY_MASK            ON  CACHE INTERNAL "" FORCE)
  set(RTCORE_STAT_COUNTERS       OFF CACHE INTERNAL "" FORCE)
  set(RTCORE_BACKFACE_CULLING    OFF CACHE INTERNAL "" FORCE)
  set(RTCORE_INTERSECTION_FILTER ON  CACHE INTERNAL "" FORCE)
  set(RTCORE_BUFFER_STRIDE       ON  CACHE INTERNAL "" FORCE)
  set(RTCORE_TRIANGLE_PAIRS      OFF CACHE INTERNAL "" FORCE)

  set(RTCORE_EXPORT_ALL_SYMBOLS      OFF CACHE INTERNAL "" FORCE)
  set(RTCORE_ENABLE_RAYSTREAM_LOGGER OFF CACHE INTERNAL "" FORCE)
  set(RTCORE_IGNORE_INVALID_RAYS     OFF CACHE INTERNAL "" FORCE)
  set(RTCORE_RAY_PACKETS             ON  CACHE INTERNAL "" FORCE)

  if (OSPRAY_USE_TBB)
    set(RTCORE_TASKING_SYSTEM TBB CACHE INTERNAL "" FORCE)
  else ()
    set(RTCORE_TASKING_SYSTEM Internal CACHE INTERNAL "" FORCE)
  endif ()

  if (THIS_IS_MIC)
    set (ENABLE_XEON_PHI_SUPPORT ON)
    message (FATAL_ERROR "Support for mic currently disabled (broken)")
  else()
    set (ENABLE_XEON_PHI_SUPPORT OFF)
  endif ()
  set (ENABLE_XEON_PHI_SUPPORT ${ENABLE_XEON_PHI_SUPPORT} CACHE INTERNAL "" FORCE)

  if (OSPRAY_EMBREE_ENABLE_AVX512)
    set (XEON_ISA "AVX512")
  elseif (OSPRAY_EMBREE_ENABLE_AVX2)
    set (XEON_ISA "AVX2")
  elseif (OSPRAY_EMBREE_ENABLE_AVX)
    set (XEON_ISA "AVX")
  else ()
    set (XEON_ISA "SSE4.2")
  endif()
  set(XEON_ISA ${XEON_ISA} CACHE INTERNAL "" FORCE)

  # Build embree
  add_subdirectory(${CMAKE_SOURCE_DIR}/ospray/embree-v2.7.1)
endif(OSPRAY_INTERNAL_EMBREE_CONFIG)
