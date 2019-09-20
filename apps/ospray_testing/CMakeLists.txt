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

add_library(ospray_testing SHARED
  ospray_testing.cpp

  geometry/Boxes.cpp
  geometry/CornellBox.cpp
  geometry/Curves.cpp
  geometry/Cylinders.cpp
  geometry/RandomSpheres.cpp
  geometry/Streamlines.cpp
  geometry/SubdivisionCube.cpp

  lights/AmbientOnly.cpp
  lights/AmbientAndDirectional.cpp

  transferFunction/Grayscale.cpp
  transferFunction/Jet.cpp
  transferFunction/RGB.cpp
  transferFunction/TransferFunction.cpp

  volume/GravitySpheresVolume.cpp
  volume/SimpleStructuredVolume.cpp
  volume/SimpleUnstructuredVolume.cpp

  ../utility/rawToAMR.cpp
)

target_link_libraries(ospray_testing PUBLIC ospray PRIVATE ospcommon::ospcommon)

ospray_install_library(ospray_testing COMPONENT apps)

set_target_properties(ospray_testing
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})

target_include_directories(ospray_testing
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/ospray_testing>
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
  FILES_MATCHING
  PATTERN *.h
)
