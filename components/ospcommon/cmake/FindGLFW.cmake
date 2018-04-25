## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

IF (NOT GLFW_ROOT)
  SET(GLFW_ROOT $ENV{GLFW_ROOT})
ENDIF()
IF (NOT GLFW_ROOT)
  SET(GLFW_ROOT $ENV{GLFWROOT})
ENDIF()

# detect changed GLFW_ROOT
IF (NOT GLFW_ROOT STREQUAL GLFW_ROOT_LAST)
  UNSET(GLFW_INCLUDE_DIR CACHE)
  UNSET(GLFW_LIBRARY CACHE)
ENDIF()

set(GLFW_LIB_SUFFIX "")
IF (WIN32)
  IF (MSVC14)
    SET(GLFW_LIB_SUFFIX "vc2015")
  ELSEIF (MSVC12)
    SET(GLFW_LIB_SUFFIX "vc2013")
  ELSEIF (MSVC11)
    SET(GLFW_LIB_SUFFIX "vc2012")
  ELSEIF (MINGW)
    # Who's ever going to build for 32bit??
    SET(GLFW_LIB_SUFFIX "mingw-w64")
  ENDIF()
ENDIF ()

FIND_PATH(GLFW_ROOT include/GLFW/glfw3.h
  DOC "Root of GLFW installation"
  HINTS ${GLFW_ROOT}
  PATHS
    ${PROJECT_SOURCE_DIR}/glfw
    /usr/local
    /usr
    /
)

FIND_PATH(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS ${GLFW_ROOT}/include NO_DEFAULT_PATH)
FIND_LIBRARY(GLFW_LIBRARY
  NAMES glfw glfw3
  HINTS ${GLFW_ROOT}
  PATHS /usr /
  PATH_SUFFIXES
    /lib
    /lib64
    /lib-${GLFW_LIB_SUFFIX}
)

SET(GLFW_ROOT_LAST ${GLFW_ROOT} CACHE INTERNAL "Last value of GLFW_ROOT to detect changes")

SET(GLFW_ERROR_MESSAGE "GLFW not found in your environment. You can 1) install
                        via your OS package manager, or 2) install it
                        somewhere on your machine and point GLFW_ROOT to it.")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLFW
  ${GLFW_ERROR_MESSAGE}
  GLFW_INCLUDE_DIR GLFW_LIBRARY
)

IF (GLFW_FOUND)
  SET(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIR})
  SET(GLFW_LIBRARIES ${GLFW_LIBRARY})
ENDIF()

MARK_AS_ADVANCED(GLFW_INCLUDE_DIR)
MARK_AS_ADVANCED(GLFW_LIBRARY)
