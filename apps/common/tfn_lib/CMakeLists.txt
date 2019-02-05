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

ospray_create_library(ospray_tfn tfn_lib.cpp LINK ospray_common)

target_include_directories(ospray_tfn
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    #$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/tfn_lib>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

ospray_create_library(ospray_json jsoncpp.cpp)
target_link_libraries(ospray_json PUBLIC ospray_common)
set_target_properties(ospray_json PROPERTIES DEFINE_SYMBOL JSON_DLL_BUILD)

option(OSPRAY_APPS_PARAVIEW_TFN_CVT
  "Build ParaView to OSPRay viewer transfer function converter"
  ON
)

if (OSPRAY_APPS_PARAVIEW_TFN_CVT)
  ospray_create_application(ospCvtParaViewTfcn convertParaViewTfcn.cpp)

  target_link_libraries(ospCvtParaViewTfcn
  PRIVATE
    ospray
    ospray_common
    ospray_app
    ospray_sg
    ospray_json
    ospray_tfn
  )
endif()
