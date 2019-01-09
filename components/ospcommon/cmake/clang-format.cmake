## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
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

# additional target to run clang-format on all source files

set (EXCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/apps/bench/pico_bench
  ${CMAKE_SOURCE_DIR}/apps/common/sg/3rdParty
  ${CMAKE_SOURCE_DIR}/apps/exampleViewer/common/gl3w
  ${CMAKE_SOURCE_DIR}/apps/exampleViewer/common/glfw
  ${CMAKE_SOURCE_DIR}/apps/exampleViewer/common/imgui
  ${CMAKE_SOURCE_DIR}/build
  ${CMAKE_SOURCE_DIR}/components/testing
  ${CMAKE_SOURCE_DIR}/ospray-doc
)

# get all project files
file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h *.ih *.ispc)
foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
  foreach (EXCLUDE_DIR ${EXCLUDE_DIRS})
    string(FIND ${SOURCE_FILE} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
      list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
    endif()
  endforeach()
endforeach()

add_custom_target(
  format
  COMMAND clang-format
  -style=file
  -i
  ${ALL_SOURCE_FILES}
)
