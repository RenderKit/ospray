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

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fno-strict-aliasing  -std=c++11 -Wno-narrowing")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g     -Wstrict-aliasing=1")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -Wstrict-aliasing=1 -ffast-math ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -Wstrict-aliasing=1 -ffast-math ")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

IF (APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7") # we only use MacOSX 10.7 features
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++") # link against C++11 stdlib
ENDIF()

# these flags apply ONLY to how embree is built; the rest of the ospray C++ code is ISA-agnostic
SET(OSPRAY_ARCH_SSE3    "-msse3")
SET(OSPRAY_ARCH_SSSE3   "-mssse3")
SET(OSPRAY_ARCH_SSE41   "-msse4.1")
SET(OSPRAY_ARCH_SSE42   "-msse4.2")
SET(OSPRAY_ARCH_AVX     "-mavx")
SET(OSPRAY_ARCH_AVX2   "-mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2")
SET(OSPRAY_ARCH_AVX512 "-mavx512f -mavx512pf -mavx512er -mavx512cd")


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

# GCC 4.8.0 supports AVX and AVX2...
SET(OSPRAY_COMPILER_SUPPORTS_AVX TRUE)
SET(OSPRAY_COMPILER_SUPPORTS_AVX2 TRUE)

SET(GCC_VERSION_REQUIRED_AVX512 "4.9.0")

IF (GCC_VERSION VERSION_LESS GCC_VERSION_REQUIRED_AVX512)
  SET(OSPRAY_COMPILER_SUPPORTS_AVX512 FALSE)
ELSE()
  SET(OSPRAY_COMPILER_SUPPORTS_AVX512 TRUE)
ENDIF()
