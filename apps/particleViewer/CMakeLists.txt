# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_APPS_PARTICLEVIEWER OFF CACHE BOOL "Build ospParticleViewer viewer")

IF (OSPRAY_APPS_PARTICLEVIEWER)

  CONFIGURE_OSPRAY()

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/util)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

  IF (NOT THIS_IS_MIC)
    ADD_EXECUTABLE(ospParticleViewer
      viewer.cpp
      model.cpp
      )
    TARGET_LINK_LIBRARIES(ospParticleViewer ospray ospray_glut3d)
  ENDIF()
ENDIF()
