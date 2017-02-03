## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

SET(OSPRAY_DEFAULT_COMPONENT apps)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/ospray/include
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
  common
)

# common utilities
# TODO build only when needed
ADD_SUBDIRECTORY(common/commandline)
ADD_SUBDIRECTORY(common/importer)
ADD_SUBDIRECTORY(common/tfn_lib)
ADD_SUBDIRECTORY(common/miniSG)
ADD_SUBDIRECTORY(common/xml)
ADD_SUBDIRECTORY(common/sg)

# benchmark application which has no windowing framework dependencies
OPTION(OSPRAY_APPS_BENCHMARK "Build ospBenchmark application." ON)

IF(OSPRAY_APPS_BENCHMARK)
  ADD_SUBDIRECTORY(bench)
ENDIF()

OPTION(OSPRAY_APPS_ENABLE_SCRIPTING
       "Enable scripting features in viewer apps, may not compile with ICC"
       OFF)
MARK_AS_ADVANCED(OSPRAY_APPS_ENABLE_SCRIPTING)

IF(OSPRAY_APPS_ENABLE_SCRIPTING)
  ADD_DEFINITIONS(-DOSPRAY_APPS_ENABLE_SCRIPTING)
  INCLUDE_DIRECTORIES(common/script/chaiscript)
  ADD_SUBDIRECTORY(common/script)
ENDIF()

# mini-scene graph viewer implemented with GLUT
OPTION(OSPRAY_APPS_GLUTVIEWER "Build ospGlutViewer application." ON)

IF(OSPRAY_APPS_GLUTVIEWER)
  INCLUDE(glut)
  # common utility classes for GLUT-based 3D viewer widget
  ADD_SUBDIRECTORY(common/widgets)
  ADD_SUBDIRECTORY(glutViewer)
ENDIF()

# NOTE(jda) - Disable Qt based viewers when on OS X using ICC due to
#             unresolved issues
# qt-based viewer for geometry (and soon volumes)
IF(NOT (APPLE AND OSPRAY_COMPILER_ICC) AND NOT WIN32)
  OPTION(OSPRAY_APPS_QTVIEWER "Build ospQtViewer (Qt-based model viewer)" ON)
  IF (OSPRAY_APPS_QTVIEWER)
    ADD_SUBDIRECTORY(qtViewer)
  ENDIF()
ENDIF()

# volume viewer application
IF(NOT (APPLE AND OSPRAY_COMPILER_ICC))
  OPTION(OSPRAY_APPS_VOLUMEVIEWER "Build ospVolumeViewer application." ON)
  IF(OSPRAY_APPS_VOLUMEVIEWER)
    ADD_SUBDIRECTORY(volumeViewer)
  ENDIF()
ENDIF()

# -------------------------------------------------------
# redistribute QT on Windows
# -------------------------------------------------------
IF (OSPRAY_INSTALL_DEPENDENCIES AND WIN32 AND
    (OSPRAY_APPS_QTVIEWER OR OSPRAY_APPS_VOLUMEVIEWER))
  FOREACH(QT_COMPONENT QTCORE QTGUI QTOPENGL)
    STRING(REGEX REPLACE lib$ dll QT_DLL ${QT_${QT_COMPONENT}_LIBRARY_RELEASE})
    INSTALL(PROGRAMS ${QT_DLL}
            DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
  ENDFOREACH()
ENDIF()
