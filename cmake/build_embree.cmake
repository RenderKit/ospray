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
    
    geometry/triangle1.cpp
    geometry/instance_intersector1.cpp
    geometry/instance_intersector16.cpp
    geometry/subdiv_intersector16.cpp

    builders/parallel_builder.cpp

    bvh4i/bvh4i.cpp
    bvh4i/bvh4i_builder.cpp
    bvh4i/bvh4i_builder_ext.cpp
    bvh4i/bvh4i_builder_morton.cpp
    bvh4i/bvh4i_builder_morton_64bit.cpp
    bvh4i/bvh4i_intersector16_chunk.cpp
    bvh4i/bvh4i_intersector16_single.cpp
    bvh4i/bvh4i_intersector16_hybrid.cpp
    bvh4i/bvh4i_intersector16_test.cpp
    bvh4i/bvh4i_intersector1.cpp
    bvh4i/bvh4i_statistics.cpp
    bvh4i/bvh4i_rotate.cpp


    bvh4mb/bvh4mb.cpp
    bvh4mb/bvh4mb_builder.cpp
    bvh4mb/bvh4mb_intersector16_chunk.cpp
    bvh4mb/bvh4mb_intersector16_single.cpp  
    bvh4mb/bvh4mb_intersector16_hybrid.cpp
    bvh4mb/bvh4mb_intersector1.cpp

    bvh4hair/bvh4hair_builder.cpp
    bvh4hair/bvh4hair.cpp	
    bvh4hair/bvh4hair_intersector16_single.cpp
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
  TARGET_LINK_LIBRARIES(ospray_embree${OSPRAY_LIB_SUFFIX} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

  SET_TARGET_PROPERTIES(ospray_embree${OSPRAY_LIB_SUFFIX} PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_embree${OSPRAY_LIB_SUFFIX} DESTINATION lib)
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

    builders/bezierrefgen.cpp 
    builders/primrefgen.cpp
    builders/heuristic_object_partition_unaligned.cpp
    builders/heuristic_object_partition.cpp
    builders/heuristic_spatial_split.cpp
    builders/heuristic_strand_partition.cpp
    builders/heuristic_fallback.cpp
    
    geometry/bezier1v.cpp
    geometry/bezier1i.cpp
    geometry/triangle1.cpp
    geometry/triangle4.cpp
    geometry/triangle1v.cpp
    geometry/triangle4v.cpp
    geometry/triangle4v_mb.cpp
    geometry/triangle4i.cpp
    geometry/subdivpatch1.cpp
    geometry/subdivpatchdispl1.cpp
    geometry/virtual_accel.cpp
    geometry/instance_intersector1.cpp
    geometry/instance_intersector4.cpp
    geometry/subdivpatch1_intersector1.cpp
    
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
  
SET(ISPC_DLLEXPORT "yes") # workaround for bug #1085 in ISPC 1.8.2: ospray_embree should export, but export in ospray cause link errors
  OSPRAY_ADD_LIBRARY(ospray_embree${OSPRAY_LIB_SUFFIX} SHARED ${OSPRAY_EMBREE_SOURCES})
UNSET(ISPC_DLLEXPORT)

  TARGET_LINK_LIBRARIES(ospray_embree${OSPRAY_LIB_SUFFIX} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

  IF (WIN32)
    TARGET_LINK_LIBRARIES(ospray_embree${OSPRAY_LIB_SUFFIX} ws2_32 setupapi) # TODO: should not be necessary
  ENDIF()
  
  # ------------------------------------------------------------------
  # now, build and link in SSE41 support (for anything more than SSE)
  # note: "SSE" is a shortcut for SSE42
  # ------------------------------------------------------------------
  IF (OSPRAY_EMBREE_ENABLE_SSE OR OSPRAY_EMBREE_ENABLE_AVX OR OSPRAY_EMBREE_ENABLE_AVX2)
    ADD_DEFINITIONS(-D__TARGET_SSE41__)
    OSPRAY_ADD_LIBRARY(ospray_embree_sse41 STATIC
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_toplevel.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector1.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_chunk.cpp
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
  IF (OSPRAY_EMBREE_ENABLE_SSE OR OSPRAY_EMBREE_ENABLE_AVX OR OSPRAY_EMBREE_ENABLE_AVX2)
    ADD_DEFINITIONS(-D__TARGET_SSE42__)
    OSPRAY_ADD_LIBRARY(ospray_embree_sse42 STATIC
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_hybrid.cpp
      )
    SET_TARGET_PROPERTIES(ospray_embree_sse42 PROPERTIES COMPILE_FLAGS "${OSPRAY_ARCH_SSE42}")
    TARGET_LINK_LIBRARIES(ospray_embree ospray_embree_sse42)
  ENDIF()

  # ------------------------------------------------------------------
  # now, build and link in AVX1 support (for AVX1 and AVX2)
  # ------------------------------------------------------------------
  IF (OSPRAY_EMBREE_ENABLE_AVX OR OSPRAY_EMBREE_ENABLE_AVX2)
    ADD_DEFINITIONS(-D__TARGET_AVX__)
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
  IF (OSPRAY_EMBREE_ENABLE_AVX2)
    SET(EMBREE_KERNELS_AVX2_SOURCES
      geometry/instance_intersector1.cpp
      geometry/instance_intersector4.cpp
      geometry/instance_intersector8.cpp
      
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
    ADD_DEFINITIONS(-D__TARGET_AVX2__)
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

  SET_TARGET_PROPERTIES(ospray_embree PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_embree DESTINATION lib)
ENDIF()
