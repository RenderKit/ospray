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

set(APP_NAME ospGlutViewer)

find_package(Readline)

if(READLINE_FOUND)
  add_definitions(-DUSE_SYSTEM_READLINE)
  set(Readline_INC ${Readline_INCLUDE_DIR})
  set(Readline_LIB ${Readline_LIBRARY})
else()
  message(STATUS "Readline NOT found, command history feature NOT enabled.")
endif()

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/script
  ${Readline_INC}
)

# --------------------------------------------
# main executable
# --------------------------------------------
add_executable(${APP_NAME}
  GlutViewerScriptHandler.cpp
  GlutViewerScriptHandler.h
  OSPGlutViewer.cpp
  OSPGlutViewer.h
  ScriptedOSPGlutViewer.cpp
  ScriptedOSPGlutViewer.h
  glutViewer.cpp
)

target_link_libraries(${APP_NAME}
  ospray
  ospray_commandline
  ospray_common
  ospray_glut3d
  ospray_minisg
  ospray_script
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARIES}
  ${Readline_LIB}
)
# ------------------------------------------------------------

install(TARGETS ${APP_NAME}
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
