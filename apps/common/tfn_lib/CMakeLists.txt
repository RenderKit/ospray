## ======================================================================== ##
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

OSPRAY_CREATE_LIBRARY(ospray_tfn tfn_lib.cpp LINK ospray_common)

OPTION(OSPRAY_APPS_PARAVIEW_TFN_CVT
  "Build ParaView to OSPRay viewer transfer function converter"
  ON
)

IF (OSPRAY_APPS_PARAVIEW_TFN_CVT)
  OSPRAY_CREATE_APPLICATION(ospCvtParaViewTfcn
    convertParaViewTfcn.cpp
    jsoncpp.cpp
  LINK
    ospray_tfn
  )
ENDIF()
