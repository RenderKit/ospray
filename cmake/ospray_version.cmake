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

set(OSPRAY_VERSION_MAJOR 1)
set(OSPRAY_VERSION_MINOR 8)
set(OSPRAY_VERSION_PATCH 2)
set(OSPRAY_SOVERSION 0)
set(OSPRAY_VERSION_GITHASH 0)
set(OSPRAY_VERSION_NOTE "")
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  find_package(Git)
  if (GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "OSPRAY_VERSION_GITHASH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(SUBSTRING ${OSPRAY_VERSION_GITHASH} 0 8 OSPRAY_VERSION_GITHASH_SHORT)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "OSPRAY_VERSION_GITBRANCH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT OSPRAY_VERSION_GITBRANCH MATCHES "^master$|^release-")
      if (NOT OSPRAY_VERSION_GITBRANCH STREQUAL "HEAD")
        set(OSPRAY_VERSION_NOTE "-${OSPRAY_VERSION_GITBRANCH}")
      endif()
      set(OSPRAY_VERSION_NOTE "${OSPRAY_VERSION_NOTE} (${OSPRAY_VERSION_GITHASH_SHORT})")
    endif()
  endif()
endif()

set(OSPRAY_VERSION
  ${OSPRAY_VERSION_MAJOR}.${OSPRAY_VERSION_MINOR}.${OSPRAY_VERSION_PATCH}
)
