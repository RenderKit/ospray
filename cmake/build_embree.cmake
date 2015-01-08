## ======================================================================== ##
## Copyright 2009-2014 Intel Corporation                                    ##
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

CONFIGURE_OSPRAY()

# =======================================================
# source for everything embree keeps in its 'sys' library
# =======================================================
IF (THIS_IS_MIC)
  SET(EMBREE_COMMON_SYS_SOURCES
  platform.cpp
  sysinfo.cpp
  filename.cpp
  library.cpp
  thread.cpp
  network.cpp
  tasklogger.cpp
  taskscheduler.cpp
  taskscheduler_sys.cpp
  taskscheduler_mic.cpp
  sync/mutex.cpp
  sync/condition.cpp
  sync/barrier.cpp
  stl/string.cpp
    )
ELSE()
  SET(EMBREE_COMMON_SYS_SOURCES
  platform.cpp
  sysinfo.cpp
  filename.cpp
  library.cpp
  thread.cpp
  network.cpp
  tasklogger.cpp
  taskscheduler.cpp
  taskscheduler_sys.cpp
  sync/mutex.cpp
  sync/condition.cpp
  sync/barrier.cpp
  stl/string.cpp
    )
ENDIF()

# =======================================================
# source for everything embree keeps in its 'simd' library
# =======================================================
IF(THIS_IS_MIC)
  SET(EMBREE_COMMON_SIMD_SOURCES 
    mic_i.cpp mic_f.cpp mic_m.cpp
    )
ELSE()
  SET(EMBREE_COMMON_SIMD_SOURCES 
    sse.cpp
    )
ENDIF()

# =======================================================
# embree kernel components common to both xeon and xeon phi
# =======================================================
SET(EMBREE_KERNELS_COMMON_SOURCES
  )

IF (THIS_IS_MIC)
  # =======================================================
  # embree kernel components ONLY for xeon phi
  # =======================================================
  SET(EMBREE_KERNELS_XEONPHI_SOURCES
  ../common/stat.cpp
  ../common/globals.cpp
  ../common/alloc.cpp
  ../common/tasksys.cpp
  ../common/acceln.cpp
  ../common/rtcore.cpp
  ../common/rtcore_ispc.cpp
  ../common/rtcore_ispc.ispc
  ../common/buffer.cpp
  ../common/scene.cpp
  ../common/geometry.cpp
  ../common/scene_user_geometry.cpp
  ../common/scene_triangle_mesh.cpp
  ../common/scene_bezier_curves.cpp
  ../common/scene_subdiv_mesh.cpp
  ../common/raystream_log.cpp
  ../common/subdiv/tessellation_cache.cpp
  ../common/subdiv/subdivpatch1base.cpp

  ../algorithms/parallel_for.cpp
  ../algorithms/parallel_reduce.cpp
  ../algorithms/parallel_prefix_sum.cpp
  ../algorithms/parallel_for_for.cpp
  ../algorithms/parallel_for_for_prefix_sum.cpp
  ../algorithms/sort.cpp
  ../algorithms/pset.cpp
  ../algorithms/pmap.cpp
  ../algorithms/prefix.cpp

  builders/bezierrefgen.cpp
  builders/primrefgen.cpp
  builders/heuristic_object_partition_unaligned.cpp
  builders/heuristic_object_partition.cpp
  builders/heuristic_spatial_split.cpp
  builders/heuristic_strand_partition.cpp
  builders/heuristic_fallback.cpp

  geometry/primitive.cpp
  geometry/bezier1v.cpp
  geometry/bezier1i.cpp
  geometry/triangle1.cpp
  geometry/triangle4.cpp
  geometry/triangle1v.cpp
  geometry/triangle4v.cpp
  geometry/triangle4v_mb.cpp
  geometry/triangle4i.cpp
  geometry/subdivpatch1.cpp
  geometry/virtual_accel.cpp
  geometry/instance_intersector1.cpp
  geometry/instance_intersector4.cpp
  geometry/subdivpatch1_intersector1.cpp
  geometry/subdivpatch1cached_intersector1.cpp		
  geometry/subdivpatch1cached.cpp

  bvh4/bvh4.cpp
  bvh4/bvh4_rotate.cpp
  bvh4/bvh4_refit.cpp
  bvh4/bvh4_builder.cpp
  bvh4/bvh4_builder_mb.cpp
  bvh4/bvh4_builder_hair.cpp
  bvh4/bvh4_builder_hair_mb.cpp
  bvh4/bvh4_builder_fast.cpp
  bvh4/bvh4_builder_morton.cpp
  bvh4/bvh4_builder_toplevel.cpp
  bvh4/bvh4_intersector1.cpp
  bvh4/bvh4_intersector4_single.cpp
  bvh4/bvh4_intersector4_chunk.cpp
  bvh4/bvh4_statistics.cpp
    )

  SET (OSPRAY_EMBREE_SOURCES "")
  FOREACH (src ${EMBREE_COMMON_SYS_SOURCES})
    SET (OSPRAY_EMBREE_SOURCES ${OSPRAY_EMBREE_SOURCES} ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/${src})
  ENDFOREACH()
  
  FOREACH (src ${EMBREE_COMMON_SIMD_SOURCES})
    SET (OSPRAY_EMBREE_SOURCES ${OSPRAY_EMBREE_SOURCES} ${OSPRAY_EMBREE_SOURCE_DIR}/common/simd/${src})
  ENDFOREACH()

  FOREACH (src ${EMBREE_KERNELS_XEONPHI_SOURCES})
    SET (OSPRAY_EMBREE_SOURCES ${OSPRAY_EMBREE_SOURCES} ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/${src})
  ENDFOREACH()
  
  OSPRAY_ADD_LIBRARY(ospray_embree${OSPRAY_LIB_SUFFIX} SHARED ${OSPRAY_EMBREE_SOURCES})
  TARGET_LINK_LIBRARIES(ospray_embree${OSPRAY_LIB_SUFFIX} pthread dl)
ELSE()
  # =======================================================
  # embree kernel components ONLY for xeon
  # =======================================================
  SET(EMBREE_KERNELS_XEON_SOURCES
  ../common/stat.cpp
  ../common/globals.cpp
  ../common/alloc.cpp
  ../common/tasksys.cpp
  ../common/acceln.cpp
  ../common/rtcore.cpp
  ../common/rtcore_ispc.cpp
  ../common/rtcore_ispc.ispc
  ../common/buffer.cpp
  ../common/scene.cpp
  ../common/geometry.cpp
  ../common/scene_user_geometry.cpp
  ../common/scene_triangle_mesh.cpp
  ../common/scene_bezier_curves.cpp
  ../common/scene_subdiv_mesh.cpp
  ../common/raystream_log.cpp
  ../common/subdiv/tessellation_cache.cpp
  ../common/subdiv/subdivpatch1base.cpp

  ../algorithms/parallel_for.cpp
  ../algorithms/parallel_reduce.cpp
  ../algorithms/parallel_prefix_sum.cpp
  ../algorithms/parallel_for_for.cpp
  ../algorithms/parallel_for_for_prefix_sum.cpp
  ../algorithms/sort.cpp
  ../algorithms/pset.cpp
  ../algorithms/pmap.cpp
  ../algorithms/prefix.cpp

  builders/bezierrefgen.cpp
  builders/primrefgen.cpp
  builders/heuristic_object_partition_unaligned.cpp
  builders/heuristic_object_partition.cpp
  builders/heuristic_spatial_split.cpp
  builders/heuristic_strand_partition.cpp
  builders/heuristic_fallback.cpp

  geometry/primitive.cpp
  geometry/bezier1v.cpp
  geometry/bezier1i.cpp
  geometry/triangle1.cpp
  geometry/triangle4.cpp
  geometry/triangle1v.cpp
  geometry/triangle4v.cpp
  geometry/triangle4v_mb.cpp
  geometry/triangle4i.cpp
  geometry/subdivpatch1.cpp
  geometry/virtual_accel.cpp
  geometry/instance_intersector1.cpp
  geometry/instance_intersector4.cpp
  geometry/subdivpatch1_intersector1.cpp
  geometry/subdivpatch1cached_intersector1.cpp		
  geometry/subdivpatch1cached.cpp

  bvh4/bvh4.cpp
  bvh4/bvh4_rotate.cpp
  bvh4/bvh4_refit.cpp
  bvh4/bvh4_builder.cpp
  bvh4/bvh4_builder_mb.cpp
  bvh4/bvh4_builder_hair.cpp
  bvh4/bvh4_builder_hair_mb.cpp
  bvh4/bvh4_builder_fast.cpp
  bvh4/bvh4_builder_morton.cpp
  bvh4/bvh4_builder_toplevel.cpp
  bvh4/bvh4_intersector1.cpp
  bvh4/bvh4_intersector4_single.cpp
  bvh4/bvh4_intersector4_chunk.cpp
  bvh4/bvh4_statistics.cpp
    )

  # ------------------------------------------------------------------
  # create the *BASE* embree library that's available on all ISAs  
  # ------------------------------------------------------------------
  SET (OSPRAY_EMBREE_SOURCES "")
  FOREACH (src ${EMBREE_COMMON_SYS_SOURCES})
    SET (OSPRAY_EMBREE_SOURCES ${OSPRAY_EMBREE_SOURCES} ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/${src})
  ENDFOREACH()
  
  FOREACH (src ${EMBREE_COMMON_SIMD_SOURCES})
    SET (OSPRAY_EMBREE_SOURCES ${OSPRAY_EMBREE_SOURCES} ${OSPRAY_EMBREE_SOURCE_DIR}/common/simd/${src})
  ENDFOREACH()

  FOREACH (src ${EMBREE_KERNELS_XEON_SOURCES})
    SET (OSPRAY_EMBREE_SOURCES ${OSPRAY_EMBREE_SOURCES} ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/${src})
  ENDFOREACH()
  
  OSPRAY_ADD_LIBRARY(ospray_embree${OSPRAY_LIB_SUFFIX} SHARED ${OSPRAY_EMBREE_SOURCES})
  TARGET_LINK_LIBRARIES(ospray_embree${OSPRAY_LIB_SUFFIX} pthread dl)
  
  # ------------------------------------------------------------------
  # now, build and link in SSE41 support (for anything more than SSE)
  # note: "SSE" is a shortcut for SSE42
  # ------------------------------------------------------------------
  IF ((${OSPRAY_XEON_TARGET} STREQUAL "AVX2") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "AVX") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "SSE42") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "SSE41") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "SSE"))
    OSPRAY_ADD_LIBRARY(ospray_embree_sse41 STATIC
          ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector1.cpp
          ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_single.cpp
          ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_chunk.cpp

          ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/subdivpatch1_intersector1.cpp
          ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/subdivpatch1cached_intersector1.cpp

#      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_toplevel.cpp
 #     ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector1.cpp   
  #    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_chunk.cpp
      # remove for embree 2.3.3:
      #      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_binner.cpp
      )
    SET_TARGET_PROPERTIES(ospray_embree_sse41 PROPERTIES COMPILE_FLAGS "${OSPRAY_ARCH_SSE41}")
    TARGET_LINK_LIBRARIES(ospray_embree ospray_embree_sse41)
  ENDIF()

  # ------------------------------------------------------------------
  # now, build and link in SSE42 support (for anything more than SSE42)
  # note: "SSE" is a shortcut for SSE42
  # ------------------------------------------------------------------
  IF ((${OSPRAY_XEON_TARGET} STREQUAL "AVX2") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "AVX") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "SSE42") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "SSE"))
    OSPRAY_ADD_LIBRARY(ospray_embree_sse42 STATIC
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_hybrid.cpp
      )
    SET_TARGET_PROPERTIES(ospray_embree_sse42 PROPERTIES COMPILE_FLAGS "${OSPRAY_ARCH_SSE42}")
    TARGET_LINK_LIBRARIES(ospray_embree ospray_embree_sse42)
  ENDIF()

  # ------------------------------------------------------------------
  # now, build and link in AVX1 support (for AVX1 and AVX2)
  # ------------------------------------------------------------------
  IF ((${OSPRAY_XEON_TARGET} STREQUAL "AVX2") OR
      (${OSPRAY_XEON_TARGET} STREQUAL "AVX"))
    SET(EMBREE_KERNELS_AVX_SOURCES
    builders/bezierrefgen.cpp
    builders/primrefgen.cpp
    builders/heuristic_object_partition_unaligned.cpp
    builders/heuristic_object_partition.cpp
    builders/heuristic_spatial_split.cpp
    builders/heuristic_strand_partition.cpp
    builders/heuristic_fallback.cpp

    geometry/triangle8.cpp

    geometry/instance_intersector1.cpp
    geometry/instance_intersector4.cpp
    geometry/instance_intersector8.cpp
    geometry/subdivpatch1_intersector1.cpp
    geometry/subdivpatch1cached_intersector1.cpp

    bvh4/bvh4_rotate.cpp
    bvh4/bvh4_builder_toplevel.cpp
    bvh4/bvh4_builder_morton.cpp
    bvh4/bvh4_builder_fast.cpp
    bvh4/bvh4_builder.cpp
    bvh4/bvh4_builder_mb.cpp
    bvh4/bvh4_builder_hair.cpp
    bvh4/bvh4_builder_hair_mb.cpp
    bvh4/bvh4_refit.cpp

    bvh4/bvh4_intersector1.cpp
    bvh4/bvh4_intersector4_single.cpp
    bvh4/bvh4_intersector4_chunk.cpp
    bvh4/bvh4_intersector4_hybrid.cpp
    bvh4/bvh4_intersector8_single.cpp
    bvh4/bvh4_intersector8_chunk.cpp
    bvh4/bvh4_intersector8_hybrid.cpp

    bvh8/bvh8.cpp
    bvh8/bvh8_builder.cpp
    bvh8/bvh8_statistics.cpp
    bvh8/bvh8_intersector1.cpp
    bvh8/bvh8_intersector4_hybrid.cpp
    bvh8/bvh8_intersector8_chunk.cpp
    bvh8/bvh8_intersector8_hybrid.cpp
      )
    SET(OSPRAY_EMBREE_AVX_SOURCES "")
    FOREACH(src ${EMBREE_KERNELS_AVX_SOURCES})
      SET(OSPRAY_EMBREE_AVX_SOURCES ${OSPRAY_EMBREE_AVX_SOURCES} 		
	${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/${src})
    ENDFOREACH()
    OSPRAY_ADD_LIBRARY(ospray_embree_avx STATIC
      ${OSPRAY_EMBREE_AVX_SOURCES})
    SET_TARGET_PROPERTIES(ospray_embree_avx PROPERTIES COMPILE_FLAGS "${OSPRAY_ARCH_AVX}")
    TARGET_LINK_LIBRARIES(ospray_embree ospray_embree_avx)
  ENDIF()

  # ------------------------------------------------------------------
  # now, build and link in AVX2 support (for AVX2 only)
  # ------------------------------------------------------------------
  SET(EMBREE_KERNELS_AVX2_SOURCES
    geometry/instance_intersector1.cpp
    geometry/instance_intersector4.cpp
    geometry/instance_intersector8.cpp
    geometry/subdivpatch1_intersector1.cpp
    geometry/subdivpatch1cached_intersector1.cpp

    bvh4/bvh4_intersector1.cpp
    bvh4/bvh4_intersector4_single.cpp
    bvh4/bvh4_intersector4_chunk.cpp
    bvh4/bvh4_intersector4_hybrid.cpp
    bvh4/bvh4_intersector8_single.cpp
    bvh4/bvh4_intersector8_chunk.cpp
    bvh4/bvh4_intersector8_hybrid.cpp

    bvh8/bvh8_intersector1.cpp
    bvh8/bvh8_intersector4_hybrid.cpp
    bvh8/bvh8_intersector8_chunk.cpp
    bvh8/bvh8_intersector8_hybrid.cpp
    )
  IF ((${OSPRAY_XEON_TARGET} STREQUAL "AVX2"))
    SET(OSPRAY_EMBREE_AVX2_SOURCES "")
    FOREACH(src ${EMBREE_KERNELS_AVX2_SOURCES})
      SET(OSPRAY_EMBREE_AVX2_SOURCES ${OSPRAY_EMBREE_AVX2_SOURCES} 		
	${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/${src})
    ENDFOREACH()
    OSPRAY_ADD_LIBRARY(ospray_embree_avx2 STATIC
      ${OSPRAY_EMBREE_AVX2_SOURCES})
    SET_TARGET_PROPERTIES(ospray_embree_avx2 PROPERTIES COMPILE_FLAGS "${OSPRAY_ARCH_AVX2}")
    TARGET_LINK_LIBRARIES(ospray_embree ospray_embree_avx2)
  ENDIF()
ENDIF()
