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

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

cmake_minimum_required(VERSION 3.1)
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW) # since 3.12
endif()

include(cmake/ospray_version.cmake)

project(OSPRay VERSION ${OSPRAY_VERSION} LANGUAGES C CXX)

set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# configure directories for components we are going to include
set(OSPRAY_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPONENTS_DIR ${OSPRAY_DIR}/components)
set(OSPCOMMON_DIR ${COMPONENTS_DIR}/ospcommon)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPCOMMON_DIR}/cmake)

## Target install macros for OSPRay libraries ##
include(GNUInstallDirs)

##############################################################
# CMake modules and macro files
##############################################################

include(macros)
include(ospray_macros)
include(ospray_options)
include(ispc)

## ospcommon
add_subdirectory(${OSPCOMMON_DIR})

## Components
include(components/configure.cmake)

## Main OSPRay library
add_subdirectory(ospray)

## OSPRay sample apps
option(OSPRAY_ENABLE_APPS "Enable the 'apps' subtree in the build." ON)
if (OSPRAY_ENABLE_APPS)
  add_subdirectory(apps)
endif()

## Modules
add_subdirectory(modules)

## Testing
if (OSPRAY_ENABLE_TESTING)
  add_subdirectory(tests)
endif()

## Clang-format target
option(OSPRAY_ENABLE_TARGET_CLANGFORMAT
       "Enable 'format' target, requires clang-format too" OFF)
include(clang-format)

# create a configure file that both ospray and ispc can read the cmake config
# from needs to be at the end, after all cache variables have been set
configure_file(ospray/common/OSPConfig.h.in OSPConfig.h)
install(FILES ${CMAKE_BINARY_DIR}/OSPConfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
)

# has to be last
include(CPack)
