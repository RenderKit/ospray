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

ospray_create_library(ospray_testing
  ospray_testing.cpp

  geometry/RandomSpheres.cpp

  lights/AmbientOnly.cpp

  transferFunction/Grayscale.cpp
  transferFunction/Jet.cpp
  transferFunction/RGB.cpp
  transferFunction/TransferFunction.cpp

  volume/GravitySpheresVolume.cpp
  volume/SimpleStructuredVolume.cpp
  volume/SimpleUnstructuredVolume.cpp
LINK
  ospray
)

# TODO: export header includes

set(OSPTESTING_SDK_INSTALL_LOC ../ospray_testing)

ospray_install_sdk_headers(
  ospray_testing.h
  DESTINATION ${OSPTESTING_SDK_INSTALL_LOC}
)

ospray_install_sdk_headers(
  detail/objectFactory.h
  DESTINATION ${OSPTESTING_SDK_INSTALL_LOC}/detail
)

ospray_install_sdk_headers(
  geometry/Geometry.h
  DESTINATION ${OSPTESTING_SDK_INSTALL_LOC}/geometry
)

ospray_install_sdk_headers(
  lights/Lights.h
  DESTINATION ${OSPTESTING_SDK_INSTALL_LOC}/lights
)

ospray_install_sdk_headers(
  transferFunction/TransferFunction.h
  DESTINATION ${OSPTESTING_SDK_INSTALL_LOC}/transferFunction
)

ospray_install_sdk_headers(
  volume/Volume.h
  DESTINATION ${OSPTESTING_SDK_INSTALL_LOC}/volume
)
