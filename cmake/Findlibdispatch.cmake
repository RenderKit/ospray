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

IF (NOT LIBDISPATCH_ROOT)
  SET(LIBDISPATCH_ROOT $ENV{LIBDISPATCH_ROOT})
ENDIF()
IF (NOT LIBDISPATCH_ROOT)
  SET(LIBDISPATCH_ROOT $ENV{LIBDISPATCHROOT})
ENDIF()

# detect changed LIBDISPATCH_ROOT
IF (NOT LIBDISPATCH_ROOT STREQUAL LIBDISPATCH_ROOT_LAST)
  UNSET(LIBDISPATCH_INCLUDE_DIR CACHE)
  UNSET(LIBDISPATCH_LIBRARY CACHE)
ENDIF()

FIND_PATH(LIBDISPATCH_ROOT include/dispatch/dispatch.h
  DOC "Root of 'libdispatch' installation"
  HINTS ${LIBDISPATCH_ROOT}
  PATHS
    /usr
    /usr/local
    /opt
    /opt/libdispatch
)

FIND_PATH(LIBDISPATCH_INCLUDE_DIR dispatch/dispatch.h PATHS ${LIBDISPATCH_ROOT}/include NO_DEFAULT_PATH)
FIND_LIBRARY(LIBDISPATCH_LIBRARY dispatch PATHS ${LIBDISPATCH_ROOT}/lib NO_DEFAULT_PATH)

SET(LIBDISPATCH_ROOT_LAST ${LIBDISPATCH_ROOT} CACHE INTERNAL "Last value of LIBDISPATCH_ROOT to detect changes")

SET(LIBDISPATCH_ERROR_MESSAGE
  "Apple's 'libdispatch' not found. Please set LIBDISPATCH_ROOT to the installation location of libdispatch or please choose a different tasking system backend (OSPRAY_TASKING_SYSTEM).")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBDISPATCH
  ${LIBDISPATCH_ERROR_MESSAGE}
  LIBDISPATCH_INCLUDE_DIR LIBDISPATCH_LIBRARY
)

IF (LIBDISPATCH_FOUND)
  SET(LIBDISPATCH_INCLUDE_DIRS ${LIBDISPATCH_INCLUDE_DIR})
  SET(LIBDISPATCH_LIBRARIES ${LIBDISPATCH_LIBRARY})
ENDIF()

MARK_AS_ADVANCED(LIBDISPATCH_INCLUDE_DIR)
MARK_AS_ADVANCED(LIBDISPATCH_LIBRARY)

