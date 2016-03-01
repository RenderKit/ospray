## ======================================================================== ##
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

SET(OSPRAY_MODULE_SEISMIC OFF CACHE BOOL "Build seismic volume file support module")

CONFIGURE_OSPRAY()

IF(NOT THIS_IS_MIC AND OSPRAY_MODULE_SEISMIC)
  IF (NOT OSPRAY_MODULE_LOADERS)
    message(FATAL_ERROR
        "The 'loaders' module must be enabled to build the 'seismic' module.")
  ENDIF (NOT OSPRAY_MODULE_LOADERS)

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

  LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/modules/seismic)
  FIND_PACKAGE(FreeDDS REQUIRED)
  INCLUDE_DIRECTORIES(${FreeDDS_INCLUDE_DIRS})

  ADD_LIBRARY(ospray_module_seismic SHARED
    SeismicHorizonFile.cpp
    SeismicVolumeFile.cpp
    SymbolRegistry.cpp
  )

  TARGET_LINK_LIBRARIES(ospray_module_seismic ${FreeDDS_LIBRARIES})
  OSPRAY_SET_LIBRARY_VERSION(ospray_module_seismic)
  OSPRAY_INSTALL_LIBRARY(ospray_module_seismic)
ENDIF()
