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

SET(COMMON_CXX_FLAGS "/EHsc /MP /GR ")

SET(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}        ${COMMON_CXX_FLAGS} /Ox /fp:fast /Oi /Gy ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${COMMON_CXX_FLAGS} /Ox /fp:fast /Oi /Gy ")

# optionally use static runtime library
OPTION(USE_STATIC_RUNTIME "Use the static version of the C/C++ runtime library." ON)
IF (USE_STATIC_RUNTIME)
  STRING(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  STRING(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  STRING(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
ENDIF()

# these flags apply ONLY to how embree is built; the rest of the ospray C++ code is ISA-agnostic
# the /QxXXX flags are meant for the Intel Compiler, MSVC ignores them
SET(OSPRAY_ARCH_SSE3  "/arch:SSE3 /QxSSE3")
SET(OSPRAY_ARCH_SSSE3 "/QxSSSE3")
SET(OSPRAY_ARCH_SSE41 "/QxSSE4.1 /DCONFIG_SSE41")
SET(OSPRAY_ARCH_SSE42 "/QxSSE4.2 /DCONFIG_SSE42")
SET(OSPRAY_ARCH_SSE   "/arch:SSE")
SET(OSPRAY_ARCH_AVX   "/arch:AVX /DCONFIG_AVX")
SET(OSPRAY_ARCH_AVX2  "/arch:AVX2 /QxCORE-AVX2 /DCONFIG_AVX2")

SET(OSPRAY_COMPILER_SUPPORTS_AVX  TRUE)
IF (MSVC_VERSION LESS 1700) # need at least MSVC 11.0 2012 for AVX2
  SET(OSPRAY_COMPILER_SUPPORTS_AVX2 FALSE)
ELSE()
  SET(OSPRAY_COMPILER_SUPPORTS_AVX2 TRUE)
ENDIF()
