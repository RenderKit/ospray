# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)

CONFIGURE_OSPRAY()

IF(NOT THIS_IS_MIC)
  # common utility classes for 3D viewer widget, importers, etc
  ADD_SUBDIRECTORY(util)
  # mini-scene graph viewer for _geometry_ (intentionally simple)
  ADD_SUBDIRECTORY(modelViewer)

  # stream line viewer for NASA Stream Line Demo
  ADD_SUBDIRECTORY(streamLineViewer)
  # xyz atom model viewer (one sphere per atom)
  ADD_SUBDIRECTORY(particleViewer)

  # volume viewer application
  OPTION(OSPRAY_APPS_VOLUMEVIEWER "Build volume viewer application" OFF)
  IF(OSPRAY_APPS_VOLUMEVIEWER)
    ADD_SUBDIRECTORY(volumeViewer)
  ENDIF()
ENDIF()
