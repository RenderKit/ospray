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

SET(OSPRAY_DEFAULT_COMPONENT apps)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/ospray/include
  ${CMAKE_SOURCE_DIR}/ospray
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
  common
)

# benchmark application which has no windowing framework dependencies
OPTION(OSPRAY_APPS_BENCHMARK "Build ospBenchmark application." ON)
OPTION(OSPRAY_APPS_EXAMPLEVIEWER "Build example viewer apps" ON)
OPTION(OSPRAY_APPS_UTILITIES "Build utility apps" ON)

# common utilities
IF(OSPRAY_APPS_BENCHMARK OR OSPRAY_APPS_EXAMPLEVIEWER)
  ADD_SUBDIRECTORY(common/ospapp)
  ADD_SUBDIRECTORY(common/sg)
  ADD_SUBDIRECTORY(common/tfn_lib)
ENDIF()

IF(OSPRAY_APPS_BENCHMARK)
  ADD_SUBDIRECTORY(bench)
ENDIF()

IF (OSPRAY_APPS_EXAMPLEVIEWER)
  ADD_SUBDIRECTORY(exampleViewer)
ENDIF()

IF (OSPRAY_APPS_UTILITIES)
  ADD_SUBDIRECTORY(utility)
ENDIF()
