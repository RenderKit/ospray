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

SET(OSPRAY_DEFAULT_COMPONENT apps)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/ospray/include
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
  common
  common/boost.any
)

# common utilities
# TODO build only when needed
ADD_SUBDIRECTORY(common/commandline)
ADD_SUBDIRECTORY(common/importer)
ADD_SUBDIRECTORY(common/tfn_lib)
ADD_SUBDIRECTORY(common/miniSG)
ADD_SUBDIRECTORY(common/xml)
ADD_SUBDIRECTORY(common/sg)

# benchmark application which has no windowing framework dependencies
OPTION(OSPRAY_APPS_BENCHMARK "Build ospBenchmark application." ON)

IF(OSPRAY_APPS_BENCHMARK)
  ADD_SUBDIRECTORY(bench)
ENDIF()

OPTION(OSPRAY_APPS_ENABLE_SCRIPTING
       "Enable scripting features in viewer apps, may not compile with ICC"
       OFF)
MARK_AS_ADVANCED(OSPRAY_APPS_ENABLE_SCRIPTING)

IF(OSPRAY_APPS_ENABLE_SCRIPTING)
  ADD_DEFINITIONS(-DOSPRAY_APPS_ENABLE_SCRIPTING)
  INCLUDE_DIRECTORIES(common/script/chaiscript)
  ADD_SUBDIRECTORY(common/script)
ENDIF()

OPTION(OSPRAY_APPS_EXAMPLEVIEWER "Build example viewer apps" ON)
IF (OSPRAY_APPS_EXAMPLEVIEWER)
  ADD_SUBDIRECTORY(exampleViewer)
ENDIF()
