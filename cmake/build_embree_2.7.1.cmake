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

SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})

SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
FIND_PACKAGE(Threads REQUIRED)

CONFIGURE_OSPRAY()

ADD_DEFINITIONS(-DTASKING_TBB_INTERNAL)

ADD_SUBDIRECTORY(${OSPRAY_EMBREE_SOURCE_DIR}/common embree_common)

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

SET(EMBREE_PATH ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/)

SET(RTCORE_INTERSECTION_FILTER ON)
SET(RTCORE_BUFFER_STRIDE ON)
SET(RTCORE_IGNORE_INVALID_RAYS ON)
SET(RTCORE_RAY_PACKETS ON)


CONFIGURE_FILE(
  "${OSPRAY_EMBREE_SOURCE_DIR}/kernels/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)
CONFIGURE_FILE(
  "${OSPRAY_EMBREE_SOURCE_DIR}/kernels/version.h.in"
  "${PROJECT_BINARY_DIR}/version.h"
)



SET(EMBREE_LIBRARY_FILES
 
#   embree.rc

  ${EMBREE_PATH}../../common/tasking/taskscheduler_tbb.cpp
  ${EMBREE_PATH}../../common/tasking/tasksys.cpp

  ${EMBREE_PATH}../algorithms/parallel_for.cpp
  ${EMBREE_PATH}../algorithms/parallel_reduce.cpp
  ${EMBREE_PATH}../algorithms/parallel_prefix_sum.cpp
  ${EMBREE_PATH}../algorithms/parallel_for_for.cpp
  ${EMBREE_PATH}../algorithms/parallel_for_for_prefix_sum.cpp
  ${EMBREE_PATH}../algorithms/sort.cpp
  ${EMBREE_PATH}../algorithms/pset.cpp
  ${EMBREE_PATH}../algorithms/pmap.cpp
  ${EMBREE_PATH}../algorithms/prefix.cpp

  ${EMBREE_PATH}../common/device.cpp
  ${EMBREE_PATH}../common/stat.cpp
  ${EMBREE_PATH}../common/globals.cpp
  ${EMBREE_PATH}../common/acceln.cpp
  ${EMBREE_PATH}../common/accelset.cpp
  ${EMBREE_PATH}../common/state.cpp
  ${EMBREE_PATH}../common/rtcore.cpp
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
  ${EMBREE_PATH}../common/subdiv/tessellation_cache.cpp
  ${EMBREE_PATH}../common/subdiv/subdivpatch1base.cpp
  ${EMBREE_PATH}../common/subdiv/catmullclark_coefficients.cpp

  ${EMBREE_PATH}geometry/primitive.cpp
  ${EMBREE_PATH}geometry/instance_intersector1.cpp
  ${EMBREE_PATH}geometry/instance_intersector.cpp
  ${EMBREE_PATH}geometry/grid_soa.cpp	
  ${EMBREE_PATH}../common/subdiv/subdivpatch1base_eval.cpp	
  ${EMBREE_PATH}builders/primrefgen.cpp

  ${EMBREE_PATH}bvh/bvh.cpp
  ${EMBREE_PATH}bvh/bvh_statistics.cpp
  ${EMBREE_PATH}bvh/bvh4_factory.cpp
  ${EMBREE_PATH}bvh/bvh8_factory.cpp

  ${EMBREE_PATH}bvh4/bvh4_rotate.cpp
  ${EMBREE_PATH}bvh4/bvh4_refit.cpp
  ${EMBREE_PATH}bvh/bvh_builder.cpp
  ${EMBREE_PATH}bvh4/bvh4_builder_hair.cpp
  ${EMBREE_PATH}bvh4/bvh4_builder_morton.cpp
  ${EMBREE_PATH}bvh/bvh_builder_sah.cpp
  ${EMBREE_PATH}bvh4/bvh4_builder_twolevel.cpp
  ${EMBREE_PATH}bvh4/bvh4_builder_instancing.cpp
  ${EMBREE_PATH}bvh4/bvh4_builder_subdiv.cpp

  ${EMBREE_PATH}bvh/bvh_intersector1.cpp)

IF (RTCORE_RAY_PACKETS)
  SET(EMBREE_LIBRARY_FILES ${EMBREE_LIBRARY_FILES}

  ${EMBREE_PATH}../common/rtcore_ispc.cpp
  ${EMBREE_PATH}../common/rtcore_ispc.ispc

  ${EMBREE_PATH}bvh/bvh_intersector_single.cpp
  ${EMBREE_PATH}bvh/bvh_intersector_hybrid.cpp)
ENDIF()

SET(EMBREE_LIBRARY_FILES_SSE42
    ${EMBREE_PATH}geometry/grid_soa.cpp
    ${EMBREE_PATH}../common/subdiv/subdivpatch1base_eval.cpp
    ${EMBREE_PATH}bvh/bvh_intersector1.cpp)

IF (RTCORE_RAY_PACKETS)
    SET(EMBREE_LIBRARY_FILES_SSE42 ${EMBREE_LIBRARY_FILES_SSE42}
    ${EMBREE_PATH}bvh/bvh_intersector_single.cpp
    ${EMBREE_PATH}bvh/bvh_intersector_hybrid.cpp)
ENDIF()

IF (TARGET_SSE42 AND EMBREE_LIBRARY_FILES_SSE42)
 ADD_DEFINITIONS(-D__TARGET_SSE42__)
ENDIF()

SET(EMBREE_LIBRARY_FILES_AVX
    
    ${EMBREE_PATH}../common/scene_subdiv_mesh_avx.cpp

    ${EMBREE_PATH}geometry/primitive.cpp
    ${EMBREE_PATH}geometry/instance_intersector1.cpp
    
    ${EMBREE_PATH}geometry/grid_soa.cpp
    ${EMBREE_PATH}../common/subdiv/subdivpatch1base_eval.cpp
    ${EMBREE_PATH}builders/primrefgen.avx.cpp

    ${EMBREE_PATH}bvh4/bvh4_rotate.cpp
    ${EMBREE_PATH}bvh4/bvh4_refit.avx.cpp
    ${EMBREE_PATH}bvh/bvh_builder.avx.cpp
    ${EMBREE_PATH}bvh4/bvh4_builder_hair.avx.cpp
    ${EMBREE_PATH}bvh4/bvh4_builder_morton.avx.cpp
    ${EMBREE_PATH}bvh/bvh_builder_sah.avx.cpp
    ${EMBREE_PATH}bvh4/bvh4_builder_twolevel.avx.cpp
    ${EMBREE_PATH}bvh4/bvh4_builder_instancing.avx.cpp
    ${EMBREE_PATH}bvh4/bvh4_builder_subdiv.avx.cpp
    ${EMBREE_PATH}bvh/bvh_intersector1.cpp
    
    ${EMBREE_PATH}bvh/bvh.cpp
    ${EMBREE_PATH}bvh/bvh_statistics.cpp
    )

IF (RTCORE_RAY_PACKETS)
  SET(EMBREE_LIBRARY_FILES_AVX ${EMBREE_LIBRARY_FILES_AVX}

    ${EMBREE_PATH}geometry/instance_intersector.cpp

    ${EMBREE_PATH}bvh/bvh_intersector_single.cpp
    ${EMBREE_PATH}bvh/bvh_intersector_hybrid.cpp)
ENDIF()

IF (TARGET_AVX AND EMBREE_LIBRARY_FILES_AVX)
 ADD_DEFINITIONS(-D__TARGET_AVX__)
ENDIF()

SET(EMBREE_LIBRARY_FILES_AVX2

    ${EMBREE_PATH}geometry/instance_intersector1.cpp
    ${EMBREE_PATH}geometry/grid_soa.cpp
    ${EMBREE_PATH}../common/subdiv/subdivpatch1base_eval.cpp

    ${EMBREE_PATH}bvh/bvh_intersector1.cpp)

IF (RTCORE_RAY_PACKETS)
  SET(EMBREE_LIBRARY_FILES_AVX2 ${EMBREE_LIBRARY_FILES_AVX2}
    ${EMBREE_PATH}geometry/instance_intersector.cpp

    ${EMBREE_PATH}bvh/bvh_intersector_single.cpp
    ${EMBREE_PATH}bvh/bvh_intersector_hybrid.cpp)
ENDIF()

IF (TARGET_AVX2 AND EMBREE_LIBRARY_FILES_AVX2)
 ADD_DEFINITIONS(-D__TARGET_AVX2__)
ENDIF()

SET(EMBREE_LIBRARY_FILES_AVX512

    ${EMBREE_PATH}geometry/instance_intersector1.cpp
    ${EMBREE_PATH}geometry/grid_soa.cpp
    ${EMBREE_PATH}../common/subdiv/subdivpatch1base_eval.cpp

    builders/primrefgen.avx512.cpp
    ${EMBREE_PATH}bvh/bvh_builder.avx512.cpp
    ${EMBREE_PATH}bvh/bvh_builder_sah.avx512.cpp

    ${EMBREE_PATH}bvh4/bvh4_refit.cpp
    ${EMBREE_PATH}bvh4/bvh4_builder_subdiv.avx512.cpp
    ${EMBREE_PATH}bvh/bvh_intersector1.cpp)

IF (RTCORE_RAY_PACKETS)
  SET(EMBREE_LIBRARY_FILES_AVX512 ${EMBREE_LIBRARY_FILES_AVX512}

    ${EMBREE_PATH}geometry/instance_intersector.cpp

    ${EMBREE_PATH}bvh/bvh_intersector_single.cpp
    ${EMBREE_PATH}bvh/bvh_intersector_hybrid.cpp)
ENDIF()

IF (TARGET_AVX512 AND EMBREE_LIBRARY_FILES_AVX512)
 ADD_DEFINITIONS(-D__TARGET_AVX512__)
ENDIF()

OSPRAY_ADD_LIBRARY(ospray_embree ${EMBREE_LIB_TYPE} ${EMBREE_LIBRARY_FILES})
SET_PROPERTY(TARGET ospray_embree PROPERTY FOLDER kernels)

IF (TARGET_SSE42 AND EMBREE_LIBRARY_FILES_SSE42)
  ADD_LIBRARY(ospray_embree_sse42 STATIC ${EMBREE_LIBRARY_FILES_SSE42})
  SET_TARGET_PROPERTIES(ospray_embree_sse42 PROPERTIES COMPILE_FLAGS "${FLAGS_SSE42}")
  SET_PROPERTY(TARGET ospray_embree_sse42 PROPERTY FOLDER kernels)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARIES} ospray_embree_sse42)
ENDIF ()

IF (TARGET_AVX  AND EMBREE_LIBRARY_FILES_AVX)
  ADD_LIBRARY(ospray_embree_avx STATIC ${EMBREE_LIBRARY_FILES_AVX})
  SET_TARGET_PROPERTIES(ospray_embree_avx PROPERTIES COMPILE_FLAGS "${FLAGS_AVX}")
  SET_PROPERTY(TARGET ospray_embree_avx PROPERTY FOLDER kernels)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARIES} ospray_embree_avx)
 ENDIF()

IF (TARGET_AVX2 AND EMBREE_LIBRARY_FILES_AVX2)
  ADD_LIBRARY(ospray_embree_avx2 STATIC ${EMBREE_LIBRARY_FILES_AVX2})
  SET_TARGET_PROPERTIES(ospray_embree_avx2 PROPERTIES COMPILE_FLAGS "${FLAGS_AVX2}")
  SET_PROPERTY(TARGET ospray_embree_avx2 PROPERTY FOLDER kernels)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARIES} ospray_embree_avx2)
ENDIF()

IF (TARGET_AVX512 AND EMBREE_LIBRARY_FILES_AVX512)
  ADD_LIBRARY(ospray_embree_avx512 STATIC ${EMBREE_LIBRARY_FILES_AVX512})
  SET_TARGET_PROPERTIES(ospray_embree_avx512 PROPERTIES COMPILE_FLAGS "${FLAGS_AVX512}")
  SET_PROPERTY(TARGET ospray_embree_avx512 PROPERTY FOLDER kernels)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARIES} ospray_embree_avx512)
ENDIF()

TARGET_LINK_LIBRARIES(ospray_embree ${EMBREE_LIBRARIES} sys simd lexers)

#IF (ENABLE_INSTALLER)
#  SET_TARGET_PROPERTIES(ospray_embree PROPERTIES VERSION ${EMBREE_VERSION} SOVERSION ${EMBREE_VERSION_MAJOR})
#ELSE()
  SET_TARGET_PROPERTIES(ospray_embree PROPERTIES VERSION ${EMBREE_VERSION_MAJOR} SOVERSION ${EMBREE_VERSION_MAJOR})
#ENDIF()

