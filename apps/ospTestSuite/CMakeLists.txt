## ======================================================================== ##
## Copyright 2017-2019 Intel Corporation                                    ##
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

ospray_disable_compiler_warnings()

add_executable(ospray_test_suite
  ospray_environment.cpp
  ospray_test_fixture.cpp
  ospray_test_geometry.cpp
  ospray_test_suite_main.cpp
  ospray_test_tools.cpp
  ospray_test_volumetric.cpp
)

target_link_libraries(ospray_test_suite
PRIVATE
  ospcommon::ospcommon
  gtest_main
  ospray
	stb_image
)

install(TARGETS ospray_test_suite
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
