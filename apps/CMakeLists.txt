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

set(OSPRAY_DEFAULT_COMPONENT apps)

# benchmark application which has no windowing framework dependencies
option(OSPRAY_APPS_BENCHMARK "Build ospBenchmark application." ON)
option(OSPRAY_APPS_EXAMPLEVIEWER "Build example viewer apps" ON)
option(OSPRAY_APPS_UTILITIES "Build utility apps" ON)

add_subdirectory(common/ospray_testing)

# common utilities
if(OSPRAY_APPS_BENCHMARK OR OSPRAY_APPS_EXAMPLEVIEWER)
  add_subdirectory(common/ospapp)
  add_subdirectory(common/sg)
  add_subdirectory(common/tfn_lib)
endif()

if(OSPRAY_APPS_BENCHMARK)
  add_subdirectory(bench)
endif()

if(OSPRAY_APPS_EXAMPLEVIEWER)
  add_subdirectory(exampleViewer)
endif()

if(OSPRAY_APPS_UTILITIES)
  add_subdirectory(utility)
endif()
