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

FIND_PACKAGE(Readline)

IF(READLINE_FOUND)
  ADD_DEFINITIONS(-DUSE_SYSTEM_READLINE)
  SET(Readline_INC ${Readline_INCLUDE_DIR})
  SET(Readline_LIB ${Readline_LIBRARY})
ELSE()
  MESSAGE(STATUS "Readline NOT found, command history feature NOT enabled.")
ENDIF()

IF (OSPRAY_MODULE_DISPLAY_WALD)
  INCLUDE_DIRECTORIES(${DISPLAY_WALD_DIR})
  ADD_DEFINITIONS(-DOSPRAY_DISPLAY_WALD=1)
  CONFIGURE_MPI()
ENDIF()

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/script
  ${Readline_INC}
)

# --------------------------------------------
# main executable
# --------------------------------------------

SET(APP_SRCS
  glutViewer.cpp
)

IF(OSPRAY_APPS_ENABLE_SCRIPTING)
  LIST(APPEND APP_SRCS
    ScriptedOSPGlutViewer.cpp
    ScriptedOSPGlutViewer.h
    GlutViewerScriptHandler.cpp
    GlutViewerScriptHandler.h
  )
ENDIF()

SET(LIBS
  ospray
  ospray_commandline
  ospray_common
  ospray_glut3d
  ospray_minisg
  ospray_importer
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARIES}
  ${Readline_LIB}
)

IF (OSPRAY_APPS_ENABLE_SCRIPTING)
  LIST(APPEND LIBS ospray_script)
ENDIF()

IF (OSPRAY_MODULE_DISPLAY_WALD)
  LIST(APPEND LIBS ospray_displayWald_client)
ENDIF()

OSPRAY_CREATE_APPLICATION(GlutViewer ${APP_SRCS} LINK ${LIBS})
