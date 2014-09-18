# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

ADD_SUBDIRECTORY(miniSG)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(ospModelViewer
  # CPP sources
  modelViewer.cpp
  )
TARGET_LINK_LIBRARIES(ospModelViewer
  ospray_minisg
  ospray_glut3d
  ospray_sg
  ${OPENGL_LIBRARY} 
  ${GLUT_LIBRARY}
  )
# ------------------------------------------------------------
INSTALL(TARGETS ospModelViewer DESTINATION bin)
