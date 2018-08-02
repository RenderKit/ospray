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

if (NOT OPENIMAGEDENOISE_ROOT)
  set(OPENIMAGEDENOISE_ROOT $ENV{OPENIMAGEDENOISE_ROOT})
endif()

# detect changed OPENIMAGEDENOISE_ROOT
if (NOT OPENIMAGEDENOISE_ROOT STREQUAL OPENIMAGEDENOISE_ROOT_LAST)
  unset(OPENIMAGEDENOISE_INCLUDE_DIR CACHE)
  unset(OPENIMAGEDENOISE_LIBRARY CACHE)
endif()
set(OPENIMAGEDENOISE_ROOT_LAST ${OPENIMAGEDENOISE_ROOT} CACHE INTERNAL
 "Last value of OPENIMAGEDENOISE_ROOT to detect changes")

find_path(OPENIMAGEDENOISE_ROOT include/OpenImageDenoise/oidn.h
  DOC "Root of OpenImageDenoise"
  HINTS ${OPENIMAGEDENOISE_ROOT}
  PATHS
    ${CMAKE_SOURCE_DIR}/../oidn
    /usr/local
)

find_path(OPENIMAGEDENOISE_INCLUDE_DIR OpenImageDenoise/oidn.h
 PATHS ${OPENIMAGEDENOISE_ROOT}/include NO_DEFAULT_PATH)
mark_as_advanced(OPENIMAGEDENOISE_INCLUDE_DIR)

find_library(OPENIMAGEDENOISE_LIBRARY OpenImageDenoise
 PATHS ${OPENIMAGEDENOISE_ROOT}/build)
mark_as_advanced(OPENIMAGEDENOISE_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenImageDenoise
  "OpenImageDenoise not found. Use OPENIMAGEDENOISE_ROOT to hint."
  OPENIMAGEDENOISE_INCLUDE_DIR OPENIMAGEDENOISE_LIBRARY
)

if (OPENIMAGEDENOISE_FOUND)
  set(OPENIMAGEDENOISE_INCLUDE_DIRS ${OPENIMAGEDENOISE_INCLUDE_DIR})
  set(OPENIMAGEDENOISE_LIBRARIES ${OPENIMAGEDENOISE_LIBRARY})
endif()
