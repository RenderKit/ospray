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

SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_FLAGS "-fPIC -fno-strict-aliasing -Wno-narrowing")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g -O0 -Wstrict-aliasing=1")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -Wstrict-aliasing=1")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -Wstrict-aliasing=1")
SET(CMAKE_EXE_LINKER_FLAGS "")

IF (APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7")
ENDIF (APPLE)

# these flags apply ONLY to how embree is built; the rest of the ospray C++ code is ISA-agnostic
SET(OSPRAY_ARCH_SSE3    "-msse3")
SET(OSPRAY_ARCH_SSSE3   "-mssse3")
SET(OSPRAY_ARCH_SSE41   "-msse4.1")
SET(OSPRAY_ARCH_SSE42   "-msse4.2")
SET(OSPRAY_ARCH_AVX     "-mavx")
SET(OSPRAY_ARCH_AVX2   "-mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2")
SET(OSPRAY_ARCH_AVX512 "-mavx512")

SET(OSPRAY_COMPILER_SUPPORTS_AVX  TRUE)
SET(OSPRAY_COMPILER_SUPPORTS_AVX2 TRUE)
