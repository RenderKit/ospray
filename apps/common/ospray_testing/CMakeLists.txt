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
  builders/Empty.cpp
  builders/GravitySpheresVolume.cpp
  builders/PerlinNoiseVolumes.cpp
  builders/RandomSpheres.cpp
  builders/Streamlines.cpp
  builders/SubdivisionCube.cpp
  builders/UnstructuredVolume.cpp

  # regression test scenes
  builders/test_pt_glass.cpp
  builders/test_pt_luminous.cpp
  builders/test_pt_metal_roughness.cpp
  builders/test_pt_metallic_flakes.cpp
  builders/test_pt_obj.cpp
)

generate_export_header(ospray_testing)

target_link_libraries(ospray_testing
PUBLIC
  ospray
  ospcommon::ospcommon
PRIVATE
  raw_to_amr
)

set_target_properties(ospray_testing
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})

target_include_directories(ospray_testing
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/ospray_testing>
)

## Install library/headers ##

ospray_install_library(ospray_testing COMPONENT apps)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
  FILES_MATCHING
  PATTERN *.h
  PATTERN *.inl
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ospray_testing_export.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/ospray_testing
  COMPONENT devel
)
