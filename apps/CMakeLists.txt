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

  # common utility classes for reading and writing files
  ADD_SUBDIRECTORY(common/fileio)

  # common utility classes for 3D viewer widget
  ADD_SUBDIRECTORY(common/widgets)

  # common xml utility classes
  ADD_SUBDIRECTORY(common/xml)

  # mini-scene graph viewer for _geometry_ (intentionally simple)
  OPTION(OSPRAY_APPS_MODELVIEWER "Build ospModelViewer application." ON)
  IF (OSPRAY_APPS_MODELVIEWER)
    ADD_SUBDIRECTORY(modelViewer)
  ENDIF()

  # stream line viewer for NASA Stream Line Demo
  OPTION(OSPRAY_APPS_STREAMLINEVIEWER "Build ospStreamLineViewer application." ON)
  IF(OSPRAY_APPS_STREAMLINEVIEWER)
    ADD_SUBDIRECTORY(streamLineViewer)
  ENDIF()

  # xyz atom model viewer (one sphere per atom)
  OPTION(OSPRAY_APPS_PARTICLEVIEWER "Build ospParticleViewer application." OFF)
  IF(OSPRAY_APPS_PARTICLEVIEWER)
    ADD_SUBDIRECTORY(particleViewer)
  ENDIF()

  # volume viewer application
  OPTION(OSPRAY_APPS_VOLUMEVIEWER "Build ospVolumeViewer application." OFF)
  IF(OSPRAY_APPS_VOLUMEVIEWER)
    ADD_SUBDIRECTORY(volumeViewer)
  ENDIF()

ENDIF()
