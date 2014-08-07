# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})

CONFIGURE_OSPRAY()

# =======================================================
# source for everything embree keeps in its 'sys' library
# =======================================================
SET(EMBREE_COMMON_SYS_SOURCES
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/platform.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/sysinfo.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/filename.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/library.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/thread.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/network.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/tasklogger.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/taskscheduler.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/taskscheduler_sys.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/taskscheduler_mic.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/sync/mutex.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/sync/condition.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/sync/barrier.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/common/sys/stl/string.cpp
  )

# =======================================================
# source for everything embree keeps in its 'simd' library
# =======================================================
IF(THIS_IS_MIC)
  SET(EMBREE_COMMON_SIMD_SOURCES 
    ${OSPRAY_EMBREE_SOURCE_DIR}/common/simd/mic_i.cpp 
    ${OSPRAY_EMBREE_SOURCE_DIR}/common/simd/mic_f.cpp 
    ${OSPRAY_EMBREE_SOURCE_DIR}/common/simd/mic_m.cpp
    )
ELSE()
  SET(EMBREE_COMMON_SIMD_SOURCES 
    ${OSPRAY_EMBREE_SOURCE_DIR}/common/simd/sse.cpp
    )
ENDIF()

# =======================================================
# embree kernel components common to both xeon and xeon phi
# =======================================================
SET(EMBREE_KERNELS_COMMON_SOURCES
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/stat.cpp 
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/globals.cpp 
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/alloc.cpp 
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/tasksys.cpp 
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/acceln.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/rtcore.cpp 
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/rtcore_ispc.cpp 
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/rtcore_ispc.ispc 
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/buffer.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/scene.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/geometry.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/scene_user_geometry.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/scene_triangle_mesh.cpp
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/common/scene_bezier_curves.cpp
  )

IF (THIS_IS_MIC)
  # =======================================================
  # embree kernel components ONLY for xeon phi
  # =======================================================
  SET(EMBREE_KERNELS_XEONPHI_SOURCES
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/geometry/triangle1.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/geometry/ispc_wrapper_knc.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/geometry/instance_intersector1.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/geometry/instance_intersector16.cpp
    
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/builders/parallel_builder.cpp
    
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_builder.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_builder_ext.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_builder_morton.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_builder_morton_64bit.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_intersector16_chunk.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_intersector16_single.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_intersector16_hybrid.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_intersector1.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_statistics.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4i/bvh4i_rotate.cpp
    
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4mb/bvh4mb.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4mb/bvh4mb_builder.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4mb/bvh4mb_intersector16_chunk.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4mb/bvh4mb_intersector16_single.cpp  
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4mb/bvh4mb_intersector16_hybrid.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4mb/bvh4mb_intersector1.cpp
    
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4hair/bvh4hair_intersector16_single.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4hair/bvh4hair_builder.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi/bvh4hair/bvh4hair.cpp	
    )

    OSPRAY_ADD_LIBRARY(ospray_embree${OSPRAY_LIB_SUFFIX} SHARED
      ${EMBREE_COMMON_SYS_SOURCES}
      ${EMBREE_COMMON_SIMD_SOURCES}
      ${EMBREE_KERNELS_COMMON_SOURCES}
      ${EMBREE_KERNELS_XEONPHI_SOURCES}
      )
    TARGET_LINK_LIBRARIES(ospray_embree${OSPRAY_LIB_SUFFIX} pthread dl)

ELSE()
  # =======================================================
  # embree kernel components ONLY for xeon
  # =======================================================
  SET(EMBREE_KERNELS_XEON_SOURCES
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/bezierrefgen.cpp 
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/primrefgen.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_object_partition_unaligned.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_object_partition.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_spatial_split.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_strand_partition.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_fallback.cpp
    
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/bezier1.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/bezier1i.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/triangle1.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/triangle4.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/triangle1v.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/triangle4v.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/triangle4i.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/ispc_wrapper_sse.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector1.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector4.cpp
    
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_rotate.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_refit.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_fast.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_morton.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_binner.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_toplevel.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector1.cpp   
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_chunk.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_statistics.cpp

    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_builder.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector1.cpp   
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector4.cpp

    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_statistics.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_builder.cpp 
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector1.cpp
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector4.cpp
    )

  # ------------------------------------------------------------------
  # create the *BASE* embree library that's available on all ISAs  
  # ------------------------------------------------------------------
  OSPRAY_ADD_LIBRARY(ospray_embree SHARED
    ${EMBREE_COMMON_SYS_SOURCES}
    ${EMBREE_COMMON_SIMD_SOURCES}
    ${EMBREE_KERNELS_COMMON_SOURCES}
    ${EMBREE_KERNELS_XEON_SOURCES}
    )
  TARGET_LINK_LIBRARIES(ospray_embree pthread dl)

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
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_binner.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_toplevel.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector1.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_chunk.cpp
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
    OSPRAY_ADD_LIBRARY(ospray_embree_avx STATIC
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/bezierrefgen.cpp 
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/primrefgen.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_object_partition_unaligned.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_object_partition.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_spatial_split.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_strand_partition.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/builders/heuristic_fallback.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/triangle8.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/ispc_wrapper_avx.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector1.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector4.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector8.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_morton.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder_fast.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_builder.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_refit.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector1.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_chunk.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_hybrid.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector8_chunk.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector8_hybrid.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_builder.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector1.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector4.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector8.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_builder.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector1.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector4.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector8.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_builder.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_statistics.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector1.cpp  
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector4_hybrid.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector8_chunk.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector8_hybrid.cpp   
      )
    SET_TARGET_PROPERTIES(ospray_embree_avx PROPERTIES COMPILE_FLAGS "${OSPRAY_ARCH_AVX}")
    TARGET_LINK_LIBRARIES(ospray_embree ospray_embree_avx)
  ENDIF()

  # ------------------------------------------------------------------
  # now, build and link in AVX2 support (for AVX2 only)
  # ------------------------------------------------------------------
  IF ((${OSPRAY_XEON_TARGET} STREQUAL "AVX2"))
    OSPRAY_ADD_LIBRARY(ospray_embree_avx2 STATIC
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector1.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector4.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/instance_intersector8.cpp
      
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/geometry/ispc_wrapper_avx.cpp
      
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector1.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_chunk.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector4_hybrid.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector8_chunk.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4/bvh4_intersector8_hybrid.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector1.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector4.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4mb/bvh4mb_intersector8.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector1.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector4.cpp
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh4hair/bvh4hair_intersector8.cpp

      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector1.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector4_hybrid.cpp 
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector8_chunk.cpp   
      ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon/bvh8/bvh8_intersector8_hybrid.cpp   
      )
    SET_TARGET_PROPERTIES(ospray_embree_avx2 PROPERTIES COMPILE_FLAGS "${OSPRAY_ARCH_AVX2}")
    TARGET_LINK_LIBRARIES(ospray_embree ospray_embree_avx2)
  ENDIF()
ENDIF()
