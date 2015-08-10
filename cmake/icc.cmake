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


SET(CMAKE_CXX_COMPILER "icpc")
SET(CMAKE_C_COMPILER "icc")
SET(CMAKE_CXX_FLAGS "-Wall -fPIC -no-ansi-alias -static-intel -openmp")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -no-ansi-alias -restrict -fp-model fast -fimf-precision=low -no-prec-div -no-prec-sqrt -fma -no-inline-max-total-size -inline-factor=200 ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -no-ansi-alias -restrict -fp-model fast -fimf-precision=low -no-prec-div -no-prec-sqrt  -fma  -no-inline-max-total-size -inline-factor=200")
SET(CMAKE_EXE_LINKER_FLAGS "") 

IF (APPLE)
  SET (CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_INIT} -dynamiclib)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7")
ENDIF (APPLE)

# these flags apply ONLY to how embree is built; the rest of the ospray C++ code is ISA-agnostic
SET(OSPRAY_ARCH_SSE3    "-xsse3")
SET(OSPRAY_ARCH_SSSE3   "-xssse3")
SET(OSPRAY_ARCH_SSE41   "-xsse4.1")
SET(OSPRAY_ARCH_SSE42   "-xsse4.2")
SET(OSPRAY_ARCH_SSE     "-xsse4.2")
SET(OSPRAY_ARCH_AVX     "-xAVX")
SET(OSPRAY_ARCH_AVX2    "-xCORE-AVX2")
SET(OSPRAY_ARCH_AVX512  "-xMIC-AVX512")

SET(OSPRAY_COMPILER_SUPPORTS_AVX  TRUE)
SET(OSPRAY_COMPILER_SUPPORTS_AVX2 TRUE)
