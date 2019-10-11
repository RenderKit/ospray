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

include(GenerateExportHeader)

add_library(ospray_testing SHARED
  ospray_testing.cpp

  builders/Boxes.cpp
  builders/Builder.cpp
  builders/CornellBox.cpp
  builders/Curves.cpp
  builders/Cylinders.cpp
  builders/GravitySpheresVolume.cpp
  builders/RandomSpheres.cpp
  builders/Streamlines.cpp
  builders/SubdivisionCube.cpp
  builders/UnstructuredVolume.cpp

  ../utility/rawToAMR.cpp
)

generate_export_header(ospray_testing)

target_link_libraries(ospray_testing PUBLIC ospray ospcommon::ospcommon)

ospray_install_library(ospray_testing COMPONENT apps)

set_target_properties(ospray_testing
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})

target_include_directories(ospray_testing
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
