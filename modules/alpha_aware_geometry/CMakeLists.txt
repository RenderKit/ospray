# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_MODULE_ALPHA_AWARE_GEOMETRY ON CACHE BOOL "Build 'alpha aware geometry' module")

IF (OSPRAY_MODULE_ALPHA_AWARE_GEOMETRY)
  
  CONFIGURE_OSPRAY()
  CONFIGURE_MPI()
  
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)
  
  OSPRAY_ADD_LIBRARY(ospray_module_alpha_aware${OSPRAY_LIB_SUFFIX} SHARED
    alpha_triangle_mesh.cpp
    alpha_triangle_mesh.ispc
    )
  TARGET_LINK_LIBRARIES(
    ospray_module_alpha_aware${OSPRAY_LIB_SUFFIX} 
    ospray${OSPRAY_LIB_SUFFIX}
    )
  # ------------------------------------------------------------
  SET_TARGET_PROPERTIES(ospray_module_alpha_aware${OSPRAY_LIB_SUFFIX}
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION  ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_module_alpha_aware${OSPRAY_LIB_SUFFIX}
    DESTINATION lib)
ENDIF()
