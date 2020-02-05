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

find_package(MPI REQUIRED)
find_package(ospray 2.0 REQUIRED)
find_package(openvkl 0.8 REQUIRED)

include(${OSPRAY_CMAKE_ROOT}/ispc.cmake)

add_subdirectory(external/snappy EXCLUDE_FROM_ALL)
set_target_properties(snappy PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

add_subdirectory(common)
add_subdirectory(external/pico_bench)
add_subdirectory(ospray)

##############################################################
# Tutorials
##############################################################
if (OSPRAY_MODULE_MPI_ENABLE_APPS)
    add_subdirectory(tutorials)
endif()

