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

# NOTE(jda) - Add CMake commands which will be invoked when find_package(ospray)
#             is called by client applications. For example, it may be useful
#             to automatically have an add_definitions(...) call for clients
#             which need certain preprocessor definitions based on how ospray
#             was built.

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPRAY_CMAKE_ROOT})

include(macros)
include(ispc)

message(STATUS "Using found OSPRay ISA targets: ${OSPRAY_ISPC_TARGET_LIST}")

include_directories_ispc(${OSPRAY_INCLUDE_DIRS})
