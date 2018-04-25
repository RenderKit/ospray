## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

SET(OSPRAY_DEFAULT_COMPONENT modules)

INCLUDE_DIRECTORIES(
  ${PROJECT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ospray/include
  ${CMAKE_SOURCE_DIR}/ospray
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_SOURCE_DIR}/apps
  ${CMAKE_SOURCE_DIR}/apps/common
  ${CMAKE_SOURCE_DIR}
  ${EMBREE_INCLUDE_DIRS}
)

INCLUDE_DIRECTORIES_ISPC(
  ${PROJECT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ospray/include
  ${CMAKE_SOURCE_DIR}/ospray
  ${CMAKE_SOURCE_DIR}
  ${EMBREE_INCLUDE_DIRS}
)

FILE(GLOB plugins RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
FOREACH(plugin ${plugins})
  IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
    ADD_SUBDIRECTORY(${plugin})
  ENDIF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
ENDFOREACH(plugin ${plugins})
