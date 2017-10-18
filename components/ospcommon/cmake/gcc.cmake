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

SET(OSPRAY_CXX_FLAGS "-std=c++11 -fPIC -fno-strict-aliasing -Wno-narrowing -Wno-unknown-pragmas")

IF(OSPRAY_STRICT_BUILD)
  # Should try to fix and remove...
  SET(OSPRAY_CXX_FLAGS "-Wno-aligned-new ${OSPRAY_CXX_FLAGS}")

  # Options selected for GCC 7.1+
  SET(OSPRAY_CXX_FLAGS "-Wall ${OSPRAY_CXX_FLAGS}")
ENDIF()

IF(OSPRAY_WARN_AS_ERRORS)
  SET(OSPRAY_CXX_FLAGS "-Werror ${OSPRAY_CXX_FLAGS}")
ENDIF()

SET(CMAKE_CXX_FLAGS "${OSPRAY_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g     -Wstrict-aliasing=1")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -Wstrict-aliasing=1 -ffast-math ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -Wstrict-aliasing=1 -ffast-math ")
SET(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")

IF (APPLE)
  SET(CMAKE_CXX_FLAGS "-mmacosx-version-min=10.8 ${CMAKE_CXX_FLAGS}") # we only use MacOSX 10.8 features
  SET(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}") # link against C++11 stdlib
ENDIF()

# check whether GCC version is new enough for C++11
SET(GCC_VERSION_REQUIRED "4.8.0")

IF(NOT GCC_VERSION)
  EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE)
  SET(GCC_VERSION ${GCC_OUTPUT} CACHE STRING "GCC Version")
  MARK_AS_ADVANCED(GCC_VERSION)
ENDIF()

IF (GCC_VERSION VERSION_LESS GCC_VERSION_REQUIRED)
  MESSAGE(FATAL_ERROR "GCC version 4.8.0 or greater is required to build OSPRay.")
ENDIF()

