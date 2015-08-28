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

OPTION(OSPRAY_MODULE_OPENGL_UTIL "Build OpenGL utility functionality" ON)

CONFIGURE_OSPRAY()

IF (NOT THIS_IS_MIC)
  IF (OSPRAY_MODULE_OPENGL_UTIL)

    FIND_PACKAGE(OpenGL REQUIRED)

    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

    LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/modules/opengl)

    ADD_LIBRARY(ospray_module_opengl_util${OSPRAY_LIB_SUFFIX} SHARED
      util.cpp
    )

    TARGET_LINK_LIBRARIES(ospray_module_opengl_util${OSPRAY_LIB_SUFFIX} ospray${OSPRAY_LIB_SUFFIX} ${OPENGL_LIBRARIES})

    SET_TARGET_PROPERTIES(ospray_module_opengl_util${OSPRAY_LIB_SUFFIX}
      PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
    INSTALL(TARGETS ospray_module_opengl_util${OSPRAY_LIB_SUFFIX} DESTINATION lib)

  ENDIF (OSPRAY_MODULE_OPENGL_UTIL)
ENDIF (NOT THIS_IS_MIC)

