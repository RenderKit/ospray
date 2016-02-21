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

  IF(USE_TBB)
    # NOTE(jda) - This is done automatically by the find_package config for apps
    #             outside the build environment of OSPRay.
    FIND_PACKAGE(TBB QUIET)
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${TBB_ROOT}/lib")
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${TBB_ROOT}/lib64")
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${TBB_ROOT}/lib/intel64/gcc4.4")
  ENDIF()

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
    # common utility classes for GLUT-based 3D viewer widget
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)
    ADD_SUBDIRECTORY(common/widgets)
  ENDIF()

  IF (OSPRAY_APPS_MODELVIEWER)
    ADD_SUBDIRECTORY(modelViewer)
  ENDIF()

 IF(NOT WIN32)
  # qt-based viewer for geometry (and soon volumes)
  OPTION(OSPRAY_APPS_QTVIEWER "Build ospQtViewer (Qt-based model viewer)" ON)
  IF (OSPRAY_APPS_QTVIEWER)
    ADD_SUBDIRECTORY(qtViewer)
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
  OPTION(OSPRAY_APPS_VOLUMEVIEWER "Build ospVolumeViewer application." ON)
  IF(OSPRAY_APPS_VOLUMEVIEWER)
    ADD_SUBDIRECTORY(volumeViewer)
  ENDIF()
 ENDIF(NOT WIN32)


#	ADD_EXECUTABLE(test1 test1.cpp)
#	TARGET_LINK_LIBRARIES(test1 ospray)

ENDIF()
