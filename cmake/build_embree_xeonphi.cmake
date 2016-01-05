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

IF(USE_TBB)#NOTE(jda) - defined in ospray/CMakeLists.txt
  ADD_DEFINITIONS(-DTASKING_TBB)
ELSE(USE_TBB)
  ADD_DEFINITIONS(-DTASKING_TBB_INTERNAL)
ENDIF(USE_TBB)

SET(FLAGS_SSE3 ${OSPRAY_ARCH_SSE3})
SET(FLAGS_SSSE3 ${OSPRAY_ARCH_SSSE3})
SET(FLAGS_SSE41 ${OSPRAY_ARCH_SSE41})
SET(FLAGS_SSE42 ${OSPRAY_ARCH_SSE42})
SET(FLAGS_AVX ${OSPRAY_ARCH_AVX})
SET(FLAGS_AVX2 ${OSPRAY_ARCH_AVX2})
SET(FLAGS_AVX512 ${OSPRAY_ARCH_AVX512})

IF (OSPRAY_EMBREE_ENABLE_SSE)
  ADD_DEFINITIONS(-D__TARGET_SIMD4__=1)
  SET(TARGET_SSE42 ON)
ENDIF()

IF (OSPRAY_EMBREE_ENABLE_AVX)
  ADD_DEFINITIONS(-D__TARGET_SIMD8__=1)
  SET(TARGET_AVX ON)
ENDIF()

IF (OSPRAY_EMBREE_ENABLE_AVX2)
  ADD_DEFINITIONS(-D__TARGET_SIMD8__=1)
  SET(TARGET_AVX2 ON)
ENDIF()

IF (OSPRAY_EMBREE_ENABLE_AVX512)
  ADD_DEFINITIONS(-D__TARGET_SIMD16__=1)
  SET(TARGET_AVX512 ON)
ENDIF()

SET(EMBREE_PATH ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/)

SET(RTCORE_INTERSECTION_FILTER ON)
SET(RTCORE_BUFFER_STRIDE ON)
SET(RTCORE_IGNORE_INVALID_RAYS ON)
SET(RTCORE_RAY_PACKETS ON)

SET(ENABLE_STATIC_LIB OFF)

SET(ENABLE_XEON_PHI_SUPPORT ON)

ADD_SUBDIRECTORY(${OSPRAY_EMBREE_SOURCE_DIR}/common embree_common_xeonphi)

SET(EMBREE_LIBRARY_FILES

  ${EMBREE_PATH}../../common/tasking/taskscheduler_tbb.cpp
  ${EMBREE_PATH}../../common/tasking/taskscheduler_mic.cpp
  ${EMBREE_PATH}../../common/tasking/tasksys.cpp

  ${EMBREE_PATH}../common/device.cpp
  ${EMBREE_PATH}../common/stat.cpp
  ${EMBREE_PATH}../common/globals.cpp
  ${EMBREE_PATH}../common/acceln.cpp
  ${EMBREE_PATH}../common/accelset.cpp
  ${EMBREE_PATH}../common/state.cpp
  ${EMBREE_PATH}../common/rtcore.cpp
  ${EMBREE_PATH}../common/rtcore_ispc.cpp
  ${EMBREE_PATH}../common/rtcore_ispc.ispc
  ${EMBREE_PATH}../common/buffer.cpp
  ${EMBREE_PATH}../common/scene.cpp
  ${EMBREE_PATH}../common/geometry.cpp
  ${EMBREE_PATH}../common/scene_user_geometry.cpp
  ${EMBREE_PATH}../common/scene_instance.cpp
  ${EMBREE_PATH}../common/scene_geometry_instance.cpp
  ${EMBREE_PATH}../common/scene_triangle_mesh.cpp
  ${EMBREE_PATH}../common/scene_bezier_curves.cpp
  ${EMBREE_PATH}../common/scene_subdiv_mesh.cpp
  ${EMBREE_PATH}../common/raystream_log.cpp
  ${EMBREE_PATH}../common/subdiv/subdivpatch1base.cpp
  ${EMBREE_PATH}../common/subdiv/subdivpatch1base_eval.cpp
  ${EMBREE_PATH}../common/subdiv/tessellation_cache.cpp
  ${EMBREE_PATH}../common/subdiv/catmullclark_coefficients.cpp

  ${EMBREE_PATH}../algorithms/parallel_for.cpp
  ${EMBREE_PATH}../algorithms/parallel_reduce.cpp
  ${EMBREE_PATH}../algorithms/parallel_prefix_sum.cpp
  ${EMBREE_PATH}../algorithms/parallel_for_for.cpp
  ${EMBREE_PATH}../algorithms/parallel_for_for_prefix_sum.cpp
  ${EMBREE_PATH}../algorithms/sort.cpp
  ${EMBREE_PATH}../algorithms/pset.cpp
  ${EMBREE_PATH}../algorithms/pmap.cpp
  ${EMBREE_PATH}../algorithms/prefix.cpp

  ${EMBREE_PATH}geometry/triangle1.cpp
  ${EMBREE_PATH}geometry/instance_intersector1.cpp
  ${EMBREE_PATH}geometry/instance_intersector16.cpp

  ${EMBREE_PATH}builders/parallel_builder.cpp

  ${EMBREE_PATH}bvh4i/bvh4i.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_factory.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_builder.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_builder_ext.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_builder_morton.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_builder_morton_64bit.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_intersector16_chunk.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_intersector16_single.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_intersector16_hybrid.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_intersector16_subdiv.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_intersector1.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_statistics.cpp
  ${EMBREE_PATH}bvh4i/bvh4i_rotate.cpp


  ${EMBREE_PATH}bvh4mb/bvh4mb.cpp
  ${EMBREE_PATH}bvh4mb/bvh4mb_factory.cpp
  ${EMBREE_PATH}bvh4mb/bvh4mb_builder.cpp
  ${EMBREE_PATH}bvh4mb/bvh4mb_intersector16_chunk.cpp
  ${EMBREE_PATH}bvh4mb/bvh4mb_intersector16_single.cpp
  ${EMBREE_PATH}bvh4mb/bvh4mb_intersector16_hybrid.cpp
  ${EMBREE_PATH}bvh4mb/bvh4mb_intersector1.cpp

  ${EMBREE_PATH}bvh4hair/bvh4hair_builder.cpp
  ${EMBREE_PATH}bvh4hair/bvh4hair.cpp
  ${EMBREE_PATH}bvh4hair/bvh4hair_factory.cpp
  ${EMBREE_PATH}bvh4hair/bvh4hair_intersector16_single.cpp
)

SET(ISPC_DLLEXPORT "yes") # workaround for bug #1085 in ISPC 1.8.2: ospray_embree should export, but export in ospray cause link errors
OSPRAY_ADD_LIBRARY(ospray_embree${OSPRAY_LIB_SUFFIX} SHARED ${EMBREE_LIBRARY_FILES})
UNSET(ISPC_DLLEXPORT)
SET_PROPERTY(TARGET ospray_embree${OSPRAY_LIB_SUFFIX} PROPERTY FOLDER kernels)

TARGET_LINK_LIBRARIES(ospray_embree${OSPRAY_LIB_SUFFIX}
  sys_xeonphi
  simd_xeonphi
  lexers_xeonphi
  ${TBB_LIBRARIES}
)

INSTALL(TARGETS ospray_embree${OSPRAY_LIB_SUFFIX}
  EXPORT ospray_embree${OSPRAY_LIB_SUFFIX}Export
  DESTINATION lib
)

INSTALL(EXPORT ospray_embree${OSPRAY_LIB_SUFFIX}Export
  DESTINATION ${CMAKE_DIR} FILE libospray_embree${OSPRAY_LIB_SUFFIX}Targets.cmake
)

SET_TARGET_PROPERTIES(ospray_embree PROPERTIES VERSION ${EMBREE_VERSION_MAJOR} SOVERSION ${EMBREE_VERSION_MAJOR})

