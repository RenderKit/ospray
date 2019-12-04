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

cmake_minimum_required(VERSION 3.1)

##############################################################
# Language setup
##############################################################

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

##############################################################
# Establish project
##############################################################

include(cmake/ospray_version.cmake)

project(OSPRay VERSION ${OSPRAY_VERSION} LANGUAGES C CXX)

##############################################################
# CMake modules and macro files
##############################################################

list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
  ${PROJECT_SOURCE_DIR}/cmake/compiler
)

include(ospray_macros)
include(ospray_options)
include(package)
include(ispc)

if (OSPRAY_INSTALL_DEPENDENCIES)
  include(ospray_redistribute_deps)
endif()

##############################################################
# Add library and executable targets
##############################################################

## Main OSPRay library ##
add_subdirectory(ospray)

## OSPRay sample apps ##
if (OSPRAY_ENABLE_APPS)
  add_subdirectory(apps)
endif()

## Modules ##
if (OSPRAY_ENABLE_MODULES)
  add_subdirectory(modules)
endif()

## Testing ##
if (OSPRAY_APPS_TESTING)
  add_subdirectory(test_image_data)
endif()

## Clang-format target ##
if (OSPRAY_ENABLE_TARGET_CLANGFORMAT)
  include(clang-format)
endif()

# Must be last
include(CPack)
