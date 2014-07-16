# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_MODULE_MHTK OFF CACHE BOOL "Build 'MHTK' module")

IF (OSPRAY_MODULE_MHTK)
  CONFIGURE_OSPRAY()
  IF ((${OSPRAY_TARGET} STREQUAL "AVX") OR (${OSPRAY_TARGET} STREQUAL "AVX2"))
    #	if (NOT THIS_IS_MIC)
    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

    INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels/xeon/)
    INCLUDE_DIRECTORIES(${EMBREE_DIR}/tutorials/common)

    INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
    INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

    OSPRAY_ADD_LIBRARY(ospray_module_mhtk SHARED
      multihit_kernel.cpp
      multihit_kernel.ispc

      xray_renderer.cpp
      xray_renderer.ispc
      )
    TARGET_LINK_LIBRARIES(ospray_module_mhtk ospray)
  ELSEIF(THIS_IS_MIC)
    # no warning for MIC - not built on MIC riht now
  ELSE()
    MESSAGE("MHTK module right now only works under AVX or AVX2 targets")
  ENDIF()
ENDIF()
