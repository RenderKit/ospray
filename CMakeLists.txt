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

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0003 NEW) # since 2.6
if(POLICY CMP0015)
  cmake_policy(SET CMP0015 NEW) # since 2.8.1
endif()
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # since 2.8.11
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW) # since 3.0
endif()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 OLD) # since 3.0
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD) # since 3.1
endif()
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW) # since 3.12
endif()

project(OSPRay)

# configure directories for components we are going to include
set(OSPRAY_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPONENTS_DIR ${OSPRAY_DIR}/components)
set(OSPCOMMON_DIR ${COMPONENTS_DIR}/ospcommon)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPCOMMON_DIR}/cmake)

##############################################################
# CMake modules and macro files
##############################################################

include(macros)
include(ospray_macros)
include(ospray_options)
include(ispc)

##############################################################
# the OSPRay 'common' library
##############################################################
add_subdirectory(${OSPCOMMON_DIR})

##############################################################
# OSPRay components
#
# 'components' work similar to 'packages' in linux distributions: each
# component is its own little piece that builds some applications,
# libraries, etc, and may have dependencies to other components.
##############################################################
include(components/configure.cmake)

##############################################################
# the OSPRay library
##############################################################

add_subdirectory(ospray)

##############################################################
# OSPRay tutorials
##############################################################
if (OSPRAY_ENABLE_TUTORIALS)
  add_subdirectory(tutorials)
endif()

##############################################################
# OSPRay sample apps; at the end: they may need modules
##############################################################
if (OSPRAY_ENABLE_APPS)
  add_subdirectory(apps)
endif()

##############################################################
# OSPRay modules
# modules are OPTTIONAL and EXTERNAL packages that
# users can link into ospray. they can use any and all pieces of
# ospray but obviously no ospray parts may ever depend on a
# module (which by definition is OPTIONAL) - so those go
# AT THE BACK, only AFTER ALL OTHER PARTS OF OSPRAY ARE BUILT
##############################################################
add_subdirectory(modules)

##############################################################
# Google Test
##############################################################
if (OSPRAY_ENABLE_TESTING)
  add_subdirectory(tests)
endif()

##############################################################
# add target for running clang-format
##############################################################
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
