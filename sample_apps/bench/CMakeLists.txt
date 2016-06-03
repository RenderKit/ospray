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

set(APP_NAME ospBenchmark)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
)

# --------------------------------------------
# main executable
# --------------------------------------------
add_executable(${APP_NAME}
  bench.cpp
  OSPRayFixture.cpp
  OSPRayFixture.h
  simple_outputter.hpp
)

target_link_libraries(${APP_NAME}
  ospray
  ospray_commandline
  ospray_common
  ospray_importer
  ospray_minisg
)

# ------------------------------------------------------------

install(TARGETS ${APP_NAME}
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
