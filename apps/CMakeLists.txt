## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

CONFIGURE_OSPRAY()

IF(NOT THIS_IS_MIC)

  # common xml utility classes
  ADD_SUBDIRECTORY(common/xml)

  # mini-scene graph viewer for _geometry_ (intentionally simple)
  OPTION(OSPRAY_APPS_MODELVIEWER "Build ospModelViewer application." ON)

  # include glut and widgets only when needed
  # needs to be before the first application, such that the GLUT include path is known
  # XXX if the defaults change, include could be missing on the first run of CMake
  IF(OSPRAY_APPS_MODELVIEWER      OR
     OSPRAY_APPS_STREAMLINEVIEWER OR
     OSPRAY_APPS_PARTICLEVIEWER)
    # GLUT is used by apps as well
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)
    # common utility classes for GLUT-based 3D viewer widget
    ADD_SUBDIRECTORY(common/widgets)
  ENDIF()

  IF (OSPRAY_APPS_MODELVIEWER)
    ADD_SUBDIRECTORY(modelViewer)
  ENDIF()

 IF(NOT WIN32)
  # NOTE(jda) - Disable Qt based viewers when on OS X using ICC due to
  #             unresolved issues
  SET(OSX_AND_ICC (APPLE AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel"))

  # qt-based viewer for geometry (and soon volumes)
  IF(NOT OSX_AND_ICC)
    OPTION(OSPRAY_APPS_QTVIEWER "Build ospQtViewer (Qt-based model viewer)" ON)
    IF (OSPRAY_APPS_QTVIEWER)
      ADD_SUBDIRECTORY(qtViewer)
    ENDIF()
  ENDIF()

  # stream line viewer for NASA Stream Line Demo
  OPTION(OSPRAY_APPS_STREAMLINEVIEWER "Build ospStreamLineViewer application." OFF)
  MARK_AS_ADVANCED(OSPRAY_APPS_STREAMLINEVIEWER)
  IF(OSPRAY_APPS_STREAMLINEVIEWER)
    ADD_SUBDIRECTORY(streamLineViewer)
  ENDIF()

  # xyz atom model viewer (one sphere per atom)
  OPTION(OSPRAY_APPS_PARTICLEVIEWER "Build ospParticleViewer application." OFF)
  IF(OSPRAY_APPS_PARTICLEVIEWER)
    ADD_SUBDIRECTORY(particleViewer)
  ENDIF()

  # volume viewer application
  IF(NOT OSX_AND_ICC)
    OPTION(OSPRAY_APPS_VOLUMEVIEWER "Build ospVolumeViewer application." ON)
    IF(OSPRAY_APPS_VOLUMEVIEWER)
      ADD_SUBDIRECTORY(volumeViewer)
    ENDIF()
  ENDIF()
 ENDIF(NOT WIN32)
ENDIF()
