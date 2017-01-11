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

FIND_PACKAGE(Readline)

SET(LIBS ospray_common ospray_tfn ospray ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/common)

IF(READLINE_FOUND)
  ADD_DEFINITIONS(-DUSE_SYSTEM_READLINE)
  INCLUDE_DIRECTORIES(${Readline_INCLUDE_DIR})
  SET(LIBS ${LIBS} ${Readline_LIBRARY})
ELSE()
  MESSAGE(STATUS "Readline NOT found, command history feature NOT enabled.")
ENDIF()

OSPRAY_CREATE_LIBRARY(ospray_script
  OSPRayScriptHandler.cpp
  OSPRayScriptHandler.h
LINK
  ${LIBS}
)

