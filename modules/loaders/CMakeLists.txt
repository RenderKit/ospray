## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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

OPTION(OSPRAY_MODULE_LOADERS "Build loaders for common file types." ON)

CONFIGURE_OSPRAY()

IF (NOT THIS_IS_MIC)
  IF (OSPRAY_MODULE_LOADERS)

    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

    LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/modules/loaders)

    ADD_LIBRARY(ospray_module_loaders${OSPRAY_LIB_SUFFIX} SHARED
      ObjectFile.cpp
      OSPObjectFile.cpp
      PLYTriangleMeshFile.cpp
      RawVolumeFile.cpp
      SymbolRegistry.cpp
      TinyXML2.cpp
      TriangleMeshFile.cpp
      VolumeFile.cpp
    )

    TARGET_LINK_LIBRARIES(ospray_module_loaders${OSPRAY_LIB_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})

    SET_TARGET_PROPERTIES(ospray_module_loaders${OSPRAY_LIB_SUFFIX}
      PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
    INSTALL(TARGETS ospray_module_loaders${OSPRAY_LIB_SUFFIX} DESTINATION lib)

  ENDIF (OSPRAY_MODULE_LOADERS)
ENDIF (NOT THIS_IS_MIC)

