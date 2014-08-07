# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_MODULE_DVR ON CACHE BOOL
  "Build 'DVR' (Direct Volume Rendering ray caster) module")

IF (OSPRAY_MODULE_DVR)
  
  CONFIGURE_OSPRAY()
  IF (OSPRAY_MPI)
    CONFIGURE_MPI()
  ENDIF()

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)
  #  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)

  OSPRAY_ADD_LIBRARY(ospray_module_dvr${OSPRAY_LIB_SUFFIX} SHARED
    dvr.cpp
    dvr.ispc
    )
  TARGET_LINK_LIBRARIES(ospray_module_dvr${OSPRAY_LIB_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})
  # ------------------------------------------------------------
  SET_TARGET_PROPERTIES(ospray_module_dvr${OSPRAY_LIB_SUFFIX}
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_module_dvr${OSPRAY_LIB_SUFFIX} DESTINATION lib)
  # ------------------------------------------------------------


  IF (NOT THIS_IS_MIC)
    ADD_EXECUTABLE(ospSimpleVolumeViewer dvrView)
    TARGET_LINK_LIBRARIES(ospSimpleVolumeViewer ospray ospray_glut3d)
    # ------------------------------------------------------------
    INSTALL(TARGETS ospSimpleVolumeViewer DESTINATION bin)
  # ------------------------------------------------------------
  ENDIF()
ENDIF()
