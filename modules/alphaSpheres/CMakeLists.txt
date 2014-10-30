# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_MODULE_ALPHASPHERES OFF CACHE BOOL "Build 'alpha_spheres' geometry")

IF (OSPRAY_MODULE_ALPHASPHERES)

  CONFIGURE_OSPRAY()

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/util)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

  OSPRAY_ADD_LIBRARY(ospray_module_alpha_spheres${OSPRAY_LIB_SUFFIX} SHARED
    AlphaSpheres.ispc
    AlphaSpheres.cpp
    MinMaxBVH2.cpp
    MinMaxBVH2.ispc
    )

  TARGET_LINK_LIBRARIES(ospray_module_alpha_spheres${OSPRAY_LIB_SUFFIX} 
    ospray${OSPRAY_LIB_SUFFIX})
ENDIF()
