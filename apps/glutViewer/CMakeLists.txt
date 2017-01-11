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

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/script
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
)

IF (OSPRAY_APPS_ENABLE_SCRIPTING)
  LIST(APPEND LIBS ospray_script)
ENDIF()

OSPRAY_CREATE_APPLICATION(ospGlutViewer ${APP_SRCS} LINK ${LIBS})
