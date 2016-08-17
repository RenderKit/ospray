# ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

add_definitions(-DPICO_BENCH_NO_DECLVAL)
set(LIBS ospray ospray_commandline ospray_common ospray_minisg
  ospray_importer ospray_tfn)

if (OSPRAY_APPS_ENABLE_SCRIPTING)
  add_definitions(-DOSPRAY_APPS_ENABLE_SCRIPTING)
  include_directories(${PROJECT_SOURCE_DIR}/apps/common/script)
  set(LIBS ${LIBS} ospray_script)
endif()

include_directories(${PROJECT_SOURCE_DIR}/apps/common)
include_directories(${CMAKE_CURRENT_LIST_DIR})

ospray_create_application(Benchmark
  bench.cpp
  OSPRayFixture.cpp
  BenchScriptHandler.cpp
LINK
  ${LIBS}
)
