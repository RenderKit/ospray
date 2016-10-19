## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

# Options for building embree
#
# NOTE(jda) - All of these variables are public to the embree project, we hide
#             them intentionally from OSPRay's CMake configuration and force
#             values into the CMake cache so the underlying cached variables
#             in the embree CMakeLists.txt are picked up based on OSPRay's
#             configuration.
set(EMBREE_GEOMETRY_HAIR       OFF CACHE INTERNAL "" FORCE)
set(EMBREE_GEOMETRY_LINES      OFF CACHE INTERNAL "" FORCE)
set(EMBREE_GEOMETRY_QUADS      OFF CACHE INTERNAL "" FORCE)
set(EMBREE_GEOMETRY_SUBDIV     OFF CACHE INTERNAL "" FORCE)
set(EMBREE_GEOMETRY_TRIANGLES  ON  CACHE INTERNAL "" FORCE)
set(EMBREE_GEOMETRY_USER       ON  CACHE INTERNAL "" FORCE)

set(EMBREE_BACKFACE_CULLING            OFF CACHE INTERNAL "" FORCE)
set(EMBREE_IGNORE_INVALID_RAYS         OFF CACHE INTERNAL "" FORCE)
set(EMBREE_INTERSECTION_FILTER         ON  CACHE INTERNAL "" FORCE)
set(EMBREE_INTERSECTION_FILTER_RESTORE ON  CACHE INTERNAL "" FORCE)
set(EMBREE_RAY_MASK                    ON  CACHE INTERNAL "" FORCE)
set(EMBREE_RAY_PACKETS                 ON  CACHE INTERNAL "" FORCE)
set(EMBREE_STATIC_LIB                  OFF CACHE INTERNAL "" FORCE)
set(EMBREE_STAT_COUNTERS               OFF CACHE INTERNAL "" FORCE)
set(EMBREE_ISPC_SUPPORT                ON  CACHE INTERNAL "" FORCE)

if (OSPRAY_TASKING_TBB)
  set(EMBREE_TASKING_SYSTEM TBB CACHE INTERNAL "" FORCE)
else ()
  set(EMBREE_TASKING_SYSTEM INTERNAL CACHE INTERNAL "" FORCE)
endif ()

if (OSPRAY_EMBREE_ENABLE_AVX512)
  set (EMBREE_MAX_ISA "AVX512KNL")
elseif (OSPRAY_EMBREE_ENABLE_AVX2)
  set (EMBREE_MAX_ISA "AVX2")
elseif (OSPRAY_EMBREE_ENABLE_AVX)
  set (EMBREE_MAX_ISA "AVX")
else ()
  set (EMBREE_MAX_ISA "SSE4.2")
endif()
set(EMBREE_MAX_ISA ${EMBREE_MAX_ISA} CACHE INTERNAL "" FORCE)

# Build embree
add_subdirectory(${CMAKE_SOURCE_DIR}/ospray/embree-v2.12.0)
