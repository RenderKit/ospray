# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################


SET(ENABLE_REMOTE_GLUT3D OFF CACHE BOOL "Enable remote glut3D support (experimental)")
MARK_AS_ADVANCED(ENABLE_REMOTE_GLUT3D)

IF(ENABLE_REMOTE_GLUT3D)
  ADD_DEFINITIONS(-DOSP_ENABLE_REMOTE_GLUT3D=1)
  ADD_LIBRARY(ospray_glut3d SHARED 
    glut3D.cpp 
    remoteGlut3D.cpp
    )
  ADD_EXECUTABLE(ospRemoteGlut3D
    remoteClient.cpp
    )
  TARGET_LINK_LIBRARIES(ospRemoteGlut3D ospray ${GLUT_LIBRARIES})
ELSE()
  ADD_LIBRARY(ospray_glut3d SHARED glut3D.cpp)
ENDIF()

TARGET_LINK_LIBRARIES(ospray_glut3d ospray ${GLUT_LIBRARIES})
# ------------------------------------------------------------
SET_TARGET_PROPERTIES(ospray_glut3d${OSPRAY_LIB_SUFFIX} 
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
INSTALL(TARGETS ospray_glut3d${OSPRAY_LIB_SUFFIX} DESTINATION lib)

