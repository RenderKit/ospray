# ======================================================================== ##
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

ADD_DEFINITIONS(-DPICO_BENCH_NO_DECLVAL)
SET(LIBS ospray ospray_commandline ospray_common ospray_minisg
  ospray_importer ospray_tfn)

IF (OSPRAY_APPS_ENABLE_SCRIPTING)
  ADD_DEFINITIONS(-DOSPRAY_APPS_ENABLE_SCRIPTING)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/common/script)
  LIST(APPEND LIBS ospray_script)
ENDIF()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/common)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR})

OSPRAY_CREATE_APPLICATION(ospBenchmark
  bench.cpp
  OSPRayFixture.cpp
  BenchScriptHandler.cpp
LINK
  ${LIBS}
)
