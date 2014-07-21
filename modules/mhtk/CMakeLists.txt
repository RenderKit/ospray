# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_MODULE_MHTK OFF CACHE BOOL "Build 'MHTK' module")

IF (NOT OSPRAY_INTERSECTION_FILTER)
  MESSAGE( SEND_ERROR "OSPRAY_INTERSECTION_FILTER must be enabled to use the MHTK module.")
ENDIF()

IF (OSPRAY_MODULE_MHTK)
  CONFIGURE_OSPRAY()
    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

    INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels/xeon/)
    INCLUDE_DIRECTORIES(${EMBREE_DIR}/tutorials/common)

    INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
    INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

    OSPRAY_ADD_LIBRARY(ospray_module_mhtk${OSPRAY_LIB_SUFFIX} SHARED
#      multihit_kernel.cpp
#      multihit_kernel.ispc

      xray_renderer.cpp
      xray_renderer.ispc
      )
    TARGET_LINK_LIBRARIES(ospray_module_mhtk${OSPRAY_LIB_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})
ENDIF()
